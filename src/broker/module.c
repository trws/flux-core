/************************************************************\
 * Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <dlfcn.h>
#ifdef HAVE_ARGZ_ADD
#include <argz.h>
#else
#include "src/common/libmissing/argz.h"
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <uuid.h>
#ifndef UUID_STR_LEN
#define UUID_STR_LEN 37     // defined in later libuuid headers
#endif
#include <flux/core.h>
#include <jansson.h>
#if HAVE_CALIPER
#include <caliper/cali.h>
#include <sys/syscall.h>
#endif

#include "src/common/libutil/log.h"
#include "src/common/libutil/errprintf.h"
#include "src/common/libutil/errno_safe.h"
#include "src/common/librouter/subhash.h"
#include "ccan/str/str.h"

#include "module.h"
#include "modservice.h"

struct broker_module {
    flux_watcher_t *broker_w;

    double lastseen;

    flux_t *h_broker;       /* broker end of interthread channel */
    char uri[128];

    uuid_t uuid;            /* uuid for unique request sender identity */
    char uuid_str[UUID_STR_LEN];
    char *parent_uuid_str;
    int rank;
    json_t *attr_cache;     /* attrs to be cached in module flux_t */
    flux_conf_t *conf;
    pthread_t t;            /* module thread */
    mod_main_f *main;       /* dlopened mod_main() */
    char *name;
    char *path;             /* retain the full path as a key for lookup */
    void *dso;              /* reference on dlopened module */
    size_t argz_len;
    char *argz;
    int status;
    int errnum;
    bool muted;             /* module is under directive 42, no new messages */

    modpoller_cb_f poller_cb;
    void *poller_arg;
    module_status_cb_f status_cb;
    void *status_arg;

    struct disconnect *disconnect;

    struct flux_msglist *rmmod_requests;
    struct flux_msglist *insmod_requests;

    flux_t *h;               /* module's handle */
    struct subhash *sub;
};

static int setup_module_profiling (module_t *p)
{
#if HAVE_CALIPER
    cali_begin_string_byname ("flux.type", "module");
    cali_begin_int_byname ("flux.tid", syscall (SYS_gettid));
    cali_begin_int_byname ("flux.rank", p->rank);
    cali_begin_string_byname ("flux.name", p->name);
#endif
    return (0);
}

static int attr_cache_to_json (flux_t *h, json_t **cachep)
{
    json_t *cache;
    const char *name;

    if (!(cache = json_object ()))
        return -1;
    name = flux_attr_cache_first (h);
    while (name) {
        json_t *val = json_string (flux_attr_get (h, name));
        if (!val || json_object_set_new (cache, name, val) < 0) {
            json_decref (val);
            goto error;
        }
        name = flux_attr_cache_next (h);
    }
    *cachep = cache;
    return 0;
error:
    json_decref (cache);
    return -1;
}

static int attr_cache_from_json (flux_t *h, json_t *cache)
{
    const char *name;
    json_t *o;

    json_object_foreach (cache, name, o) {
        const char *val = json_string_value (o);
        if (flux_attr_set_cacheonly (h, name, val) < 0)
            return -1;
    }
    return 0;
}

/*  Synchronize the FINALIZING state with the broker, so the broker
 *   can stop messages to this module until we're fully shutdown.
 */
static int module_finalizing (module_t *p)
{
    flux_future_t *f;

    if (!(f = flux_rpc_pack (p->h,
                             "broker.module-status",
                             FLUX_NODEID_ANY,
                             0,
                             "{s:i}",
                             "status", FLUX_MODSTATE_FINALIZING))
        || flux_rpc_get (f, NULL)) {
        flux_log_error (p->h, "broker.module-status FINALIZING error");
        flux_future_destroy (f);
        return -1;
    }
    flux_future_destroy (f);
    return 0;
}

static void *module_thread (void *arg)
{
    module_t *p = arg;
    sigset_t signal_set;
    int errnum;
    char uri[128];
    char **av = NULL;
    int ac;
    int mod_main_errno = 0;
    flux_msg_t *msg;
    flux_future_t *f;

    setup_module_profiling (p);

    /* Connect to broker socket, enable logging, register built-in services
     */
    if (!(p->h = flux_open (p->uri, 0))) {
        log_err ("flux_open %s", uri);
        goto done;
    }
    if (attr_cache_from_json (p->h, p->attr_cache) < 0) {
        log_err ("%s: error priming broker attribute cache", p->name);
        goto done;
    }
    flux_log_set_appname (p->h, p->name);
    if (flux_set_conf (p->h, p->conf) < 0) {
        log_err ("%s: error setting config object", p->name);
        goto done;
    }
    p->conf = NULL; // flux_set_conf() transfers ownership to p->h
    if (modservice_register (p->h, p) < 0) {
        log_err ("%s: modservice_register", p->name);
        goto done;
    }

    /* Block all signals
     */
    if (sigfillset (&signal_set) < 0) {
        log_err ("%s: sigfillset", p->name);
        goto done;
    }
    if ((errnum = pthread_sigmask (SIG_BLOCK, &signal_set, NULL)) != 0) {
        log_errn (errnum, "pthread_sigmask");
        goto done;
    }

    /* Run the module's main().
     */
    ac = argz_count (p->argz, p->argz_len);
    if (!(av = calloc (1, sizeof (av[0]) * (ac + 1)))) {
        log_err ("calloc");
        goto done;
    }
    argz_extract (p->argz, p->argz_len, av);
    if (p->main (p->h, ac, av) < 0) {
        mod_main_errno = errno;
        if (mod_main_errno == 0)
            mod_main_errno = ECONNRESET;
        flux_log (p->h, LOG_CRIT, "module exiting abnormally");
    }

    /* Before processing unhandled requests, ensure that this module
     * is "muted" in the broker. This ensures the broker won't try to
     * feed a message to this module after we've closed the handle,
     * which could cause the broker to block.
     */
    if (module_finalizing (p) < 0)
        flux_log_error (p->h, "failed to set module state to finalizing");

    /* If any unhandled requests were received during shutdown,
     * respond to them now with ENOSYS.
     */
    while ((msg = flux_recv (p->h, FLUX_MATCH_REQUEST, FLUX_O_NONBLOCK))) {
        const char *topic = "unknown";
        (void)flux_msg_get_topic (msg, &topic);
        flux_log (p->h, LOG_DEBUG, "responding to post-shutdown %s", topic);
        if (flux_respond_error (p->h, msg, ENOSYS, NULL) < 0)
            flux_log_error (p->h, "responding to post-shutdown %s", topic);
        flux_msg_destroy (msg);
    }
    if (!(f = flux_rpc_pack (p->h,
                             "broker.module-status",
                             FLUX_NODEID_ANY,
                             FLUX_RPC_NORESPONSE,
                             "{s:i s:i}",
                             "status", FLUX_MODSTATE_EXITED,
                             "errnum", mod_main_errno))) {
        flux_log_error (p->h, "broker.module-status EXITED error");
        goto done;
    }
    flux_future_destroy (f);
done:
    free (av);
    flux_close (p->h);
    p->h = NULL;
    return NULL;
}

static void module_cb (flux_reactor_t *r,
                       flux_watcher_t *w,
                       int revents,
                       void *arg)
{
    module_t *p = arg;
    p->lastseen = flux_reactor_now (r);
    if (p->poller_cb)
        p->poller_cb (p, p->poller_arg);
}

static char *module_name_from_path (const char *s)
{
    char *path, *name, *cpy;
    char *cp;

    if (!(path = strdup (s))
        || !(name = basename (path)))
        goto error;
    if ((cp = strstr (name, ".so")))
        *cp = '\0';
    if (!(cpy = strdup (name)))
        goto error;
    free (path);
    return cpy;
error:
    ERRNO_SAFE_WRAP (free, path);
    return NULL;
}

module_t *module_create (flux_t *h,
                         const char *parent_uuid,
                         const char *name, // may be NULL
                         const char *path,
                         int rank,
                         json_t *args,
                         flux_error_t *error)
{
    flux_reactor_t *r = flux_get_reactor (h);
    module_t *p;
    void *dso;
    const char **mod_namep;
    mod_main_f *mod_main;

    dlerror ();
    if (!(dso = dlopen (path, RTLD_NOW | RTLD_GLOBAL | FLUX_DEEPBIND))) {
        errprintf (error, "%s", dlerror ());
        errno = ENOENT;
        return NULL;
    }
    if (!(mod_main = dlsym (dso, "mod_main"))) {
        errprintf (error, "module does not define mod_main()");
        dlclose (dso);
        errno = EINVAL;
        return NULL;
    }
    if (!(p = calloc (1, sizeof (*p))))
        goto nomem;
    p->main = mod_main;
    p->dso = dso;
    p->rank = rank;
    if (!(p->conf = flux_conf_copy (flux_get_conf (h))))
        goto cleanup;
    if (!(p->parent_uuid_str = strdup (parent_uuid)))
        goto nomem;
    strncpy (p->uuid_str, parent_uuid, sizeof (p->uuid_str) - 1);
    if (args) {
        size_t index;
        json_t *entry;

        json_array_foreach (args, index, entry) {
            const char *s = json_string_value (entry);
            if (s && (argz_add (&p->argz, &p->argz_len, s) != 0))
                goto nomem;
        }
    }
    if (!(p->path = strdup (path))
        || !(p->rmmod_requests = flux_msglist_create ())
        || !(p->insmod_requests = flux_msglist_create ()))
        goto nomem;
    if (name) {
        if (!(p->name = strdup (name)))
            goto nomem;
    }
    else {
        if (!(p->name = module_name_from_path (path)))
            goto nomem;
    }
    if (!(p->sub = subhash_create ())) {
        errprintf (error, "error creating subscription hash");
        goto cleanup;
    }
    /* Handle legacy 'mod_name' symbol - not recommended for new modules
     * but double check that it's sane if present.
     */
    if ((mod_namep = dlsym (p->dso, "mod_name")) && *mod_namep != NULL) {
        if (!streq (*mod_namep, p->name)) {
            errprintf (error, "mod_name %s != name %s", *mod_namep, name);
            errno = EINVAL;
            goto cleanup;
        }
    }
    uuid_generate (p->uuid);
    uuid_unparse (p->uuid, p->uuid_str);

    /* Broker end of interthread pair is opened here.
     */
    // copying 13 + 37 + 1 = 51 bytes into 128 byte buffer cannot fail
    (void)snprintf (p->uri, sizeof (p->uri), "interthread://%s", p->uuid_str);
    if (!(p->h_broker = flux_open (p->uri, FLUX_O_NOREQUEUE))
        || flux_opt_set (p->h_broker,
                         FLUX_OPT_ROUTER_NAME,
                         parent_uuid,
                         strlen (parent_uuid) + 1) < 0
        || flux_set_reactor (p->h_broker, r) < 0) {
        errprintf (error, "could not create %s interthread handle", p->name);
        goto cleanup;
    }
    if (!(p->broker_w = flux_handle_watcher_create (r,
                                                    p->h_broker,
                                                    FLUX_POLLIN,
                                                    module_cb,
                                                    p))) {
        errprintf (error, "could not create %s flux handle watcher", p->name);
        goto cleanup;
    }
    /* Optimization: create attribute cache to be primed in the module's
     * flux_t handle.  Priming the cache avoids a synchronous RPC from
     * flux_attr_get(3) for common attrs like rank, etc.
     */
    if (attr_cache_to_json (h, &p->attr_cache) < 0)
        goto nomem;
    return p;
nomem:
    errprintf (error, "out of memory");
    errno = ENOMEM;
cleanup:
    module_destroy (p);
    return NULL;
}

const char *module_get_path (module_t *p)
{
    return p && p->path ? p->path : "unknown";
}

const char *module_get_name (module_t *p)
{
    return p && p->name ? p->name : "unknown";
}

const char *module_get_uuid (module_t *p)
{
    return p ? p->uuid_str : "unknown";
}

double module_get_lastseen (module_t *p)
{
    return p ? p->lastseen : 0;
}

int module_get_status (module_t *p)
{
    return p ? p->status : 0;
}

flux_msg_t *module_recvmsg (module_t *p)
{
    return flux_recv (p->h_broker, FLUX_MATCH_ANY, FLUX_O_NONBLOCK);
}

int module_sendmsg (module_t *p, const flux_msg_t *msg)
{
    int type;
    const char *topic;

    if (!msg)
        return 0;
    if (flux_msg_get_type (msg, &type) < 0
        || flux_msg_get_topic (msg, &topic) < 0)
        return -1;
    /* Muted modules only accept response to broker.module-status
     */
    if (p->muted) {
        if (type != FLUX_MSGTYPE_RESPONSE
            || !streq (topic, "broker.module-status")) {
            errno = ENOSYS;
            return -1;
        }
    }
    return flux_send (p->h_broker, msg, 0);
}

int module_disconnect_arm (module_t *p,
                           const flux_msg_t *msg,
                           disconnect_send_f cb,
                           void *arg)
{
    if (!p->disconnect) {
        if (!(p->disconnect = disconnect_create (cb, arg)))
            return -1;
    }
    if (disconnect_arm (p->disconnect, msg) < 0)
        return -1;
    return 0;
}

void module_destroy (module_t *p)
{
    int e;
    void *res;
    int saved_errno = errno;

    if (!p)
        return;

    if (p->t) {
        if ((e = pthread_join (p->t, &res)) != 0)
            log_errn_exit (e, "pthread_cancel");
        if (p->status != FLUX_MODSTATE_EXITED) {
            /* Calls broker.c module_status_cb() => service_remove_byuuid()
             * and releases a reference on 'p'.  Without this, disconnect
             * requests sent when other modules are destroyed can still find
             * this service name and trigger a use-after-free segfault.
             * See also: flux-framework/flux-core#4564.
             */
            module_set_status (p, FLUX_MODSTATE_EXITED);
        }
    }

    /* Send disconnect messages to services used by this module.
     */
    disconnect_destroy (p->disconnect);

    flux_watcher_stop (p->broker_w);
    flux_watcher_destroy (p->broker_w);
    flux_close (p->h_broker);

#ifndef __SANITIZE_ADDRESS__
    dlclose (p->dso);
#endif
    free (p->argz);
    free (p->name);
    free (p->path);
    free (p->parent_uuid_str);
    flux_conf_decref (p->conf);
    json_decref (p->attr_cache);
    flux_msglist_destroy (p->rmmod_requests);
    flux_msglist_destroy (p->insmod_requests);
    subhash_destroy (p->sub);
    free (p);
    errno = saved_errno;
}

/* Send shutdown request, broker to module.
 */
int module_stop (module_t *p, flux_t *h)
{
    char *topic = NULL;
    flux_future_t *f = NULL;
    int rc = -1;

    if (asprintf (&topic, "%s.shutdown", p->name) < 0)
        goto done;
    if (!(f = flux_rpc (h,
                        topic,
                        NULL,
                        FLUX_NODEID_ANY,
                        FLUX_RPC_NORESPONSE)))
        goto done;
    rc = 0;
done:
    free (topic);
    flux_future_destroy (f);
    return rc;
}

void module_mute (module_t *p)
{
    p->muted = true;
}

int module_start (module_t *p)
{
    int errnum;
    int rc = -1;

    flux_watcher_start (p->broker_w);
    if ((errnum = pthread_create (&p->t, NULL, module_thread, p))) {
        errno = errnum;
        goto done;
    }
    rc = 0;
done:
    return rc;
}

int module_cancel (module_t *p, flux_error_t *error)
{
    if (p->t) {
        int e;
        if ((e = pthread_cancel (p->t)) != 0 && e != ESRCH) {
            errprintf (error, "pthread_cancel: %s", strerror (e));
            return -1;
        }
    }
    return 0;
}

void module_set_poller_cb (module_t *p, modpoller_cb_f cb, void *arg)
{
    p->poller_cb = cb;
    p->poller_arg = arg;
}

void module_set_status_cb (module_t *p, module_status_cb_f cb, void *arg)
{
    p->status_cb = cb;
    p->status_arg = arg;
}

void module_set_status (module_t *p, int new_status)
{
    assert (new_status != FLUX_MODSTATE_INIT);  /* illegal state transition */
    assert (p->status != FLUX_MODSTATE_EXITED); /* illegal state transition */
    int prev_status = p->status;
    p->status = new_status;
    if (p->status_cb)
        p->status_cb (p, prev_status, p->status_arg);
}

void module_set_errnum (module_t *p, int errnum)
{
    p->errnum = errnum;
}

int module_get_errnum (module_t *p)
{
    return p->errnum;
}

int module_push_rmmod (module_t *p, const flux_msg_t *msg)
{
    return flux_msglist_push (p->rmmod_requests, msg);
}

const flux_msg_t *module_pop_rmmod (module_t *p)
{
    return flux_msglist_pop (p->rmmod_requests);
}

/* There can be only one insmod request.
 */
int module_push_insmod (module_t *p, const flux_msg_t *msg)
{
    if (flux_msglist_count (p->insmod_requests) > 0) {
        errno = EEXIST;
        return -1;
    }
    return flux_msglist_push (p->insmod_requests, msg);
}

const flux_msg_t *module_pop_insmod (module_t *p)
{
    return flux_msglist_pop (p->insmod_requests);
}

int module_subscribe (module_t *p, const char *topic)
{
    return subhash_subscribe (p->sub, topic);
}

int module_unsubscribe (module_t *p, const char *topic)
{
    return subhash_unsubscribe (p->sub, topic);
}

int module_event_cast (module_t *p, const flux_msg_t *msg)
{
    const char *topic;

    if (flux_msg_get_topic (msg, &topic) < 0)
        return -1;
    if (subhash_topic_match (p->sub, topic)) {
        if (module_sendmsg (p, msg) < 0)
            return -1;
    }
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
