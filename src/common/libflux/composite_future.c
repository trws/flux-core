/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */


#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <czmq.h>

#include "future.h"

/*  Type-specific data for a composite future:
 */
struct composite_future {
    unsigned int any:1;  /* true if this future is a "wait any" type */
    zhash_t *children;   /* hash of child futures by name            */
};

static void composite_future_destroy (struct composite_future *f)
{
    if (f) {
        if (f->children)
            zhash_destroy (&f->children);
        free (f);
    }
}

static struct composite_future * composite_future_create (void)
{
    struct composite_future *cf = calloc (1, sizeof (*cf));
    if (cf == NULL)
        return NULL;
    if (!(cf->children = zhash_new ())) {
        free (cf);
        return (NULL);
    }
    return (cf);
}

/*  Return the embedded composite_future data from future `f`
 */
static struct composite_future * composite_get (flux_future_t *f)
{
    return flux_future_aux_get (f, "flux::composite");
}

/*
 *  Return true if all futures in this composite are ready
 */
static bool wait_all_is_ready (struct composite_future *cf)
{
    flux_future_t *f = zhash_first (cf->children);
    while (f) {
        if (!flux_future_is_ready (f))
            return (false);
        f = zhash_next (cf->children);
    }
    return (true);
}

/*  Continuation for children of a composition future -- simply check
 *   to see if the parent composite is "ready" and fulfill it if so.
 */
static void child_cb (flux_future_t *f, void *arg)
{
    flux_future_t *parent = arg;
    struct composite_future *cf = composite_get (parent);
    if (!arg || !cf)
        return;
    /*
     *  Fulfill the composite future if "wait any" is set, or all child
     *   futures are fulfilled:
     */
    if (cf->any || wait_all_is_ready (cf))
        flux_future_fulfill (parent, NULL, NULL);
}


/*  Propagate the current reactor *and* flux_t handle context from
 *   future `f` to another future `next`.
 */
static void future_propagate_context (flux_future_t *f, flux_future_t *next)
{
    /*  Note: flux_future_set_flux(3) will *also* reset the reactor
     *   for the future `next` using `flux_get_reactor (handle)`.
     *   However, we still have to explicitly call set_reactor() here
     *   for the case where a flux_t handle is *not* currently set
     *   in the context of future `f` (e.g. if operating within a
     *   reactor only with no connection to flux)
     */
    flux_future_set_reactor (next, flux_future_get_reactor (f));
    flux_future_set_flux (next, flux_future_get_flux (f));
}

/*  Initialization callback for a composite future. Register then
 *   continuations for all child futures on active reactor.
 */
void composite_future_init (flux_future_t *f, void *arg)
{
    flux_future_t *child;
    struct composite_future *cf = arg;
    if (cf == NULL) {
        errno = EINVAL;
        goto error;
    }
    /*
     *  Propagate current context of this composite future to all children
     *   so that the composite future's 'then' *or* 'now' context becomes
     *   a 'then' context for all children.
     */
    child = zhash_first (cf->children);
    while (child) {
        future_propagate_context (f, child);
        if (flux_future_then (child, -1., child_cb, (void *) f) < 0)
            goto error;
        child = zhash_next (cf->children);
    }
    return;
error:
    flux_future_fulfill_error (f, errno, NULL);
}

/*
 *  Construct a composite future.
 *  If the wait_any flag is 1 then make this a "wait any" composite.
 */
static flux_future_t *future_create_composite (int wait_any)
{
    struct composite_future *cf = composite_future_create ();
    flux_future_t *f = flux_future_create (composite_future_init, (void *) cf);
    if (!f || !cf || flux_future_aux_set (f, "flux::composite", cf,
                         (flux_free_f) composite_future_destroy) < 0) {
        composite_future_destroy (cf);
        flux_future_destroy (f);
        return (NULL);
    }
    cf->any = wait_any;
    return (f);
}

/*  Constructor for "wait_all" composite future.
 */
flux_future_t *flux_future_wait_all_create (void)
{
    return future_create_composite (0);
}

/*  Constructor for "wait_any" composite future
 */
flux_future_t *flux_future_wait_any_create (void)
{
    return future_create_composite (1);
}

int flux_future_push (flux_future_t *f, const char *name, flux_future_t *child)
{
    struct composite_future *cf = NULL;

    if (!f || !name || !child || !(cf = composite_get (f))) {
        errno = EINVAL;
        return (-1);
    }
    if (zhash_insert (cf->children, name, child) < 0)
        return (-1);
    zhash_freefn (cf->children, name, (flux_free_f) flux_future_destroy);
    if (flux_future_aux_set (child, "flux::parent", f, NULL) < 0) {
        zhash_delete (cf->children, name);
        return (-1);
    }
    return (0);
}

flux_future_t *flux_future_get_child (flux_future_t *f, const char *name)
{
    struct composite_future *cf = NULL;
    if (!f || !name || !(cf = composite_get (f))) {
        errno = EINVAL;
        return (NULL);
    }
    return zhash_lookup (cf->children, name);
}

const char *flux_future_first_child (flux_future_t *f)
{
    struct composite_future *cf = NULL;
    if (!f || !(cf = composite_get (f))) {
        errno = EINVAL;
        return (NULL);
    }
    if (!zhash_first (cf->children))
        return (NULL);
    return (zhash_cursor (cf->children));
}

const char *flux_future_next_child (flux_future_t *f)
{
    struct composite_future *cf = NULL;
    if (!f || !(cf = composite_get (f))) {
        errno = EINVAL;
        return (NULL);
    }
    if (!zhash_next (cf->children))
        return (NULL);
    return (zhash_cursor (cf->children));
}

/*  Chained futures support: */

struct continuation_info {
    flux_continuation_f cb;
    void *arg;
};

struct chained_future {
    flux_future_t *next;
    flux_future_t *prev;
    struct continuation_info and_then;
    struct continuation_info or_then;
};

/*
 *  Continuation for chained futures: fulfill future `next` immediately
 *   with the result from `f`. Since a result freed by destruction of
 *   `f` will now be placed into `next`, the `next` future must steal
 *   a reference to `f`, thus we place `f` in the aux hash.
 */
static void fulfill_next (flux_future_t *f, flux_future_t *next)
{
    void *result;
    /*  Tie destruction of future `f` to future `next` since we
     *   are fulfilling `next` via `f`
     */
    flux_future_aux_set (next, NULL, f, (flux_free_f) flux_future_destroy);

    if (flux_future_get (f, (const void **)&result) < 0)
        flux_future_fulfill_error (next, errno, NULL);
    else
        flux_future_fulfill (next, result, NULL);
}

/*  Callback for chained continuations. Obtains the result of the completed
 *   "previous" future, then calls the appropriate "and_then" or "or_then"
 *   callback, or fulfill the "next" future with an error automatically.
 */
static void chained_continuation (flux_future_t *prev, void *arg)
{
    struct chained_future *cf = arg;

    if (flux_future_get (prev, NULL) < 0) {
        /*  Handle "or" callback if set and return immediately */
        if (cf->or_then.cb)
            return (*cf->or_then.cb) (prev, cf->or_then.arg);
    }
    else if (cf->and_then.cb)
        return (*cf->and_then.cb) (prev, cf->and_then.arg);

    /* Neither and-then nor or-then callback was used, auto-fulfill cf->next
     *  using the result of prev:
     */
    fulfill_next (prev, cf->next);
}

/*  Initialization for a chained future. Get current reactor for this
 *   context and install it in "previous" future, _then_ set the "then"
 *   callback for that future to `chained_continuation()` which will
 *   call take the appropriate action for the result.
 */
static void chained_future_init (flux_future_t *f, void *arg)
{
    struct chained_future *cf = arg;
    if (cf == NULL || cf->prev == NULL || cf->next == NULL
        || !(flux_future_get_reactor (f))) {
        errno = EINVAL;
        goto error;
    }
    /*  Grab the reactor and flux_t handle (if any) for the current
     *   context of future 'f', and propagate it to the previous future
     *   in the chain. This ensures that the chain of "then" registrations
     *   are placed on the correct reactor (our main reactor if in 'then'
     *   context, or the temporary reactor in 'now' context), and that the
     *   cloned handle is used on these callbacks if we are in 'now'
     *   context.
     */
    future_propagate_context (f, cf->prev);

    /*  Now call flux_future_then(3) with our chained-future continuation
     *   function on the previous future in this chain. This allows
     *   a flux_future_get(3) on 'f' to block, while its antecedent
     *   futures are fulfilled asynchronously.
     */
    if (flux_future_then (cf->prev, -1., chained_continuation, cf) < 0)
        goto error;
    return;
error:
    /* Initialization failed. Fulfill f with error to indicate the failure,
     *  and pass the error up the chain to cf->next, since that is likely the
     *  future which has callbacks registered on it.
     */
    flux_future_fulfill_error (f, errno, NULL);
    fulfill_next (f, cf->next);
}


/*  Allocate a chained future structure */
static struct chained_future *chained_future_alloc (void)
{
    struct chained_future *cf = calloc (1, sizeof (*cf));
    if (cf == NULL)
        return NULL;
    if (!(cf->next = flux_future_create (chained_future_init, (void *)cf))) {
        free (cf);
        return (NULL);
    }
    return (cf);
}

static void chained_future_destroy (struct chained_future *cf)
{
    free (cf);
}

/*  Create a chained future on `f` by embedding a chained future
 *   structure as "flux::chained" in the aux hash.
 *
 *  The future `f` doesn't "own" the memory for cf->next,
 *   since this next future in the chain may be passed to the user
 *   or another continuation etc.
 */
static struct chained_future *chained_future_create (flux_future_t *f)
{
    struct chained_future *cf = flux_future_aux_get (f, "flux::chained");
    if (cf == NULL && (cf = chained_future_alloc ())) {
        if (flux_future_aux_set (f, "flux::chained",
                                 (void *) cf,
                                 (flux_free_f) chained_future_destroy) < 0) {
            chained_future_destroy (cf);
            return (NULL);
        }
    }
    cf->prev = f;
    /*
     * Ensure the empty "next" future we have just created inherits
     *  the same reactor (if any) and handle (if any) from the previous
     *  future in the chain `f`. If this is not done, then there may be
     *  no default reactor on which `flux_future_then(3)` can operate,
     *  and no default handle to clone in `flux_future_wait_for(3)`.
     */
    future_propagate_context (f, cf->next);
    return (cf);
}

static struct chained_future *chained_future_get (flux_future_t *f)
{
    return (flux_future_aux_get (f, "flux::chained"));
}

/* "Continue" the chained "next" future embedded in `prev` with the
 *  future `f` by setting the continuation of `f` to fulfill "next".
 *
 * Steals ownership of `f` so that its destruction can be tied to
 *  next. (`prev`, however, is free to be destroyed after this call)
 */
int flux_future_continue (flux_future_t *prev, flux_future_t *f)
{
    struct chained_future *cf = chained_future_get (prev);
    if (cf == NULL || !cf->next) {
        errno = EINVAL;
        return -1;
    }
    /*  Ensure that the reactor/handle for f matches the current reactor
     *   context for the previous future `prev`.
     */
    future_propagate_context (prev, f);

    /*  Set the "next" future in the chain (prev->next) to be fulfilled
     *   by the provided future `f` once it is fulfilled.
     */
    return flux_future_then (f, -1.,
                             (flux_continuation_f) fulfill_next,
                             cf->next);
}

/* "Continue" the chained "next" future embedded in `prev` with an error
 */
void flux_future_continue_error (flux_future_t *prev, int errnum)
{
    struct chained_future *cf = chained_future_get (prev);
    if (cf && cf->next)
        flux_future_fulfill_error (cf->next, errnum, NULL);
}

flux_future_t *flux_future_and_then (flux_future_t *prev,
                                     flux_continuation_f next_cb, void *arg)
{
    struct chained_future *cf = chained_future_create (prev);
    if (!cf)
        return NULL;
    cf->and_then.cb = next_cb;
    cf->and_then.arg = arg;
    return (cf->next);
}

flux_future_t *flux_future_or_then (flux_future_t *prev,
                                    flux_continuation_f or_cb, void *arg)
{
    struct chained_future *cf = chained_future_create (prev);
    if (!cf)
        return NULL;
    cf->or_then.cb = or_cb;
    cf->or_then.arg = arg;
    return (cf->next);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
