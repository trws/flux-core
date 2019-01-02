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
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <flux/core.h>
#include <czmq.h>

#include "src/common/libutil/log.h"
#include "src/common/libutil/oom.h"
#include "src/common/libutil/xzmalloc.h"
#include "src/common/libutil/iterators.h"

const double barrier_reduction_timeout_sec = 0.001;

typedef struct {
    zhash_t *barriers;
    flux_t *h;
    bool timer_armed;
    flux_watcher_t *timer;
    uint32_t rank;
} barrier_ctx_t;

typedef struct _barrier_struct {
    char *name;
    int nprocs;
    int count;
    zhash_t *clients;
    barrier_ctx_t *ctx;
    int errnum;
    flux_watcher_t *debug_timer;
} barrier_t;

static int exit_event_send (flux_t *h, const char *name, int errnum);
static void timeout_cb (flux_reactor_t *r, flux_watcher_t *w,
                        int revents, void *arg);

static void freectx (void *arg)
{
    barrier_ctx_t *ctx = arg;
    if (ctx) {
        zhash_destroy (&ctx->barriers);
        if (ctx->timer)
            flux_watcher_destroy (ctx->timer);
        free (ctx);
    }
}

static barrier_ctx_t *getctx (flux_t *h)
{
    barrier_ctx_t *ctx = (barrier_ctx_t *)flux_aux_get (h, "flux::barrier");

    if (!ctx) {
        ctx = xzmalloc (sizeof (*ctx));
        if (!(ctx->barriers = zhash_new ())) {
            errno = ENOMEM;
            goto error;
        }
        if (flux_get_rank (h, &ctx->rank) < 0) {
            flux_log_error (h, "flux_get_rank");
            goto error;
        }
        if (!(ctx->timer = flux_timer_watcher_create (flux_get_reactor (h),
                       barrier_reduction_timeout_sec, 0., timeout_cb, ctx) )) {
            flux_log_error (h, "flux_timer_watacher_create");
            goto error;
        }
        ctx->h = h;
        if (flux_aux_set (h, "flux::barrier", ctx, freectx) < 0)
            goto error;
    }
    return ctx;
error:
    freectx (ctx);
    return NULL;
}

static void debug_timer_cb (flux_reactor_t *r, flux_watcher_t *w,
                            int revents, void *arg)
{
    barrier_t *b = arg;
    flux_log (b->ctx->h, LOG_DEBUG, "debug %s %d", b->name, b->nprocs);
}

static void barrier_destroy (void *arg)
{
    barrier_t *b = arg;

    if (b->debug_timer) {
        flux_log (b->ctx->h, LOG_DEBUG, "destroy %s %d", b->name, b->nprocs);
        flux_watcher_stop (b->debug_timer);
        flux_watcher_destroy (b->debug_timer);
    }
    zhash_destroy (&b->clients);
    free (b->name);
    free (b);
    return;
}

static barrier_t *barrier_create (barrier_ctx_t *ctx, const char *name, int nprocs)
{
    barrier_t *b;

    b = xzmalloc (sizeof (barrier_t));
    b->name = xstrdup (name);
    b->nprocs = nprocs;
    if (!(b->clients = zhash_new ()))
        oom ();
    b->ctx = ctx;
    zhash_insert (ctx->barriers, b->name, b);
    zhash_freefn (ctx->barriers, b->name, barrier_destroy);

    /* Start a timer for some debug
     */
    if (ctx->rank == 0) {
        flux_log (ctx->h, LOG_DEBUG, "create %s %d", name, nprocs);
        b->debug_timer = flux_timer_watcher_create (flux_get_reactor (ctx->h),
                                                    1.0, 1.0,
                                                    debug_timer_cb, b);
        if (!b->debug_timer) {
            flux_log_error (ctx->h, "flux_timer_watcher_create");
            goto done;
        }
        flux_watcher_start (b->debug_timer);

    }
done:
    return b;
}

static int barrier_add_client (barrier_t *b, char *sender, const flux_msg_t *msg)
{
    flux_msg_t *cpy = flux_msg_copy (msg, true);
    if (!cpy || zhash_insert (b->clients, sender, cpy) < 0)
        return -1;
    zhash_freefn (b->clients, sender, (zhash_free_fn *)flux_msg_destroy);
    return 0;
}

static void send_enter_request (barrier_ctx_t *ctx, barrier_t *b)
{
    flux_future_t *f;

    if (!(f = flux_rpc_pack (ctx->h, "barrier.enter", FLUX_NODEID_UPSTREAM,
                             FLUX_RPC_NORESPONSE, "{s:s s:i s:i s:b}",
                             "name", b->name,
                             "count", b->count,
                             "nprocs", b->nprocs,
                             "internal", true))) {
        flux_log_error (ctx->h, "sending barrier.enter request");
        goto done;
    }
done:
    flux_future_destroy (f);
}

/* Barrier entry happens in two ways:
 * - client calling flux_barrier ()
 * - downstream barrier plugin sending count upstream.
 * In the first case only, we track client uuid to handle disconnect and
 * notification upon barrier termination.
 */

static void enter_request_cb (flux_t *h, flux_msg_handler_t *mh,
                              const flux_msg_t *msg, void *arg)
{
    barrier_ctx_t *ctx = arg;
    barrier_t *b;
    char *sender = NULL;
    const char *name;
    int count, nprocs, internal;

    if (flux_request_unpack (msg, NULL, "{s:s s:i s:i s:b !}",
                             "name", &name,
                             "count", &count,
                             "nprocs", &nprocs,
                             "internal", &internal) < 0
                || flux_msg_get_route_first (msg, &sender) < 0) {
        flux_log_error (ctx->h, "%s: decoding request", __FUNCTION__);
        goto done;
    }

    if (!(b = zhash_lookup (ctx->barriers, name)))
        b = barrier_create (ctx, name, nprocs);

    /* Distinguish client (tracked) vs downstream barrier plugin (untracked).
     * A client, distinguished by internal == false, can only enter barrier once.
     */
    if (internal == false) {
        if (barrier_add_client (b, sender, msg) < 0) {
            flux_respond (ctx->h, msg, EEXIST, NULL);
            flux_log (ctx->h, LOG_ERR,
                        "abort %s due to double entry by client %s",
                        name, sender);
            if (exit_event_send (ctx->h, b->name, ECONNABORTED) < 0)
                flux_log_error (ctx->h, "exit_event_send");
            goto done;
        }
    }

    /* If the count has been reached, terminate the barrier;
     * o/w set timer to pass count upstream and zero it here.
     */
    b->count += count;
    if (b->count == b->nprocs) {
        if (exit_event_send (ctx->h, b->name, 0) < 0)
            flux_log_error (ctx->h, "exit_event_send");
    } else if (ctx->rank > 0 && !ctx->timer_armed) {
        flux_timer_watcher_reset (ctx->timer, barrier_reduction_timeout_sec, 0.);
        flux_watcher_start (ctx->timer);
        ctx->timer_armed = true;
    }
done:
    if (sender)
        free (sender);
}

/* Upon client disconnect, abort any pending barriers it was
 * participating in.
 */

static void disconnect_request_cb (flux_t *h, flux_msg_handler_t *mh,
                                   const flux_msg_t *msg, void *arg)
{
    barrier_ctx_t *ctx = arg;
    char *sender;
    const char *key;
    barrier_t *b;

    if (flux_msg_get_route_first (msg, &sender) < 0)
        return;
    FOREACH_ZHASH (ctx->barriers, key, b) {
        if (zhash_lookup (b->clients, sender)) {
            if (exit_event_send (h, b->name, ECONNABORTED) < 0)
                flux_log_error (h, "exit_event_send");
        }
    }
    free (sender);
}

static int exit_event_send (flux_t *h, const char *name, int errnum)
{
    flux_msg_t *msg = NULL;
    int rc = -1;

    if (!(msg = flux_event_pack ("barrier.exit", "{s:s s:i}",
                                 "name", name,
                                 "errnum", errnum)))
        goto done;
    if (flux_send (h, msg, 0) < 0)
        goto done;
    rc = 0;
done:
    flux_msg_destroy (msg);
    return rc;
}

static void exit_event_cb (flux_t *h, flux_msg_handler_t *mh,
                           const flux_msg_t *msg, void *arg)
{
    barrier_ctx_t *ctx = arg;
    barrier_t *b;
    const char *name;
    int errnum;
    const char *key;
    flux_msg_t *req;

    if (flux_event_unpack (msg, NULL, "{s:s s:i !}",
                           "name", &name,
                           "errnum", &errnum) < 0) {
        flux_log_error (h, "%s: decoding event", __FUNCTION__);
        return;
    }
    if ((b = zhash_lookup (ctx->barriers, name))) {
        b->errnum = errnum;
        FOREACH_ZHASH (b->clients, key, req) {
            if (flux_respond (h, req, b->errnum, NULL) < 0)
                flux_log_error (h, "%s: sending enter response", __FUNCTION__);
        }
        zhash_delete (ctx->barriers, name);
    }
}

static void timeout_cb (flux_reactor_t *r, flux_watcher_t *w,
                        int revents, void *arg)
{
    barrier_ctx_t *ctx = arg;
    const char *key;
    barrier_t *b;

    assert (ctx->rank != 0);
    ctx->timer_armed = false; /* one shot */

    FOREACH_ZHASH (ctx->barriers, key, b) {
        if (b->count > 0) {
            send_enter_request (ctx, b);
            b->count = 0;
        }
    }
}

static struct flux_msg_handler_spec htab[] = {
    { FLUX_MSGTYPE_REQUEST, "barrier.enter",       enter_request_cb, 0 },
    { FLUX_MSGTYPE_REQUEST, "barrier.disconnect",  disconnect_request_cb, 0 },
    { FLUX_MSGTYPE_EVENT,   "barrier.exit",        exit_event_cb, 0 },
    FLUX_MSGHANDLER_TABLE_END,
};

int mod_main (flux_t *h, int argc, char **argv)
{
    int rc = -1;
    barrier_ctx_t *ctx = getctx (h);
    flux_msg_handler_t **handlers = NULL;

    if (!ctx)
        goto done;
    if (flux_event_subscribe (h, "barrier.") < 0) {
        flux_log_error (h, "flux_event_subscribe");
        goto done;
    }
    if (flux_msg_handler_addvec (h, htab, ctx, &handlers) < 0) {
        flux_log_error (h, "flux_msghandler_add");
        goto done;
    }
    if (flux_reactor_run (flux_get_reactor (h), 0) < 0) {
        flux_log_error (h, "flux_reactor_run");
        goto done;
    }
    rc = 0;
done:
    flux_msg_handler_delvec (handlers);
    return rc;
}

MOD_NAME ("barrier");

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
