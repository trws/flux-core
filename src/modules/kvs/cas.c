/*****************************************************************************\
 *  Copyright (c) 2015 Lawrence Livermore National Security, LLC.  Produced at
 *  the Lawrence Livermore National Laboratory (cf, AUTHORS, DISCLAIMER.LLNS).
 *  LLNL-CODE-658032 All rights reserved.
 *
 *  This file is part of the Flux resource manager framework.
 *  For details, see https://github.com/flux-framework.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license, or (at your option)
 *  any later version.
 *
 *  Flux is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the terms and conditions of the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *  See also:  http://www.gnu.org/licenses/
\*****************************************************************************/

/* cas.c - content addressable storage */

/* Sophia put/commit is nearly as fast as hash insert.
 * Sophia get is O(20X) slower.
 * Sophia get with lz4 compression is O(4X) slower.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <czmq.h>
#include <flux/core.h>

#include "src/common/libutil/cleanup.h"
#include "src/common/libutil/shortjson.h"
#include "src/common/libsophia/sophia.h"
#include "src/common/libutil/xzmalloc.h"
#include "src/common/libutil/log.h"

typedef struct {
    char *dir;
    void *env;
    void *cas;
    flux_t h;
} ctx_t;

static void log_sophia_error (ctx_t *ctx, const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    char *s = xvasprintf (fmt, ap);
    va_end (ap);

    int error_size;
    char *error = NULL;
    if (ctx->env)
        error = sp_getstring (ctx->env, "sophia.error", &error_size);
    (void)flux_log (ctx->h, LOG_ERR, "%s: %s", s, error ? error : "failure");
    if (error)
        free (error);
    free (s);
}

static void freectx (void *arg)
{
    ctx_t *ctx = arg;
    if (ctx) {
        if (ctx->dir)
            free (ctx->dir); /* remove? */
        if (ctx->cas)
            sp_destroy (ctx->cas);
        if (ctx->env)
            sp_destroy (ctx->env);
        free (ctx);
    }
}

static ctx_t *getctx (flux_t h)
{
    ctx_t *ctx = (ctx_t *)flux_aux_get (h, "flux::cas");
    const char *dir;

    if (!ctx) {
        ctx = xzmalloc (sizeof (*ctx));
        ctx->h = h;
        if (!(dir = flux_attr_get (h, "scratch-directory", NULL))) {
            flux_log_error (h, "scratch-directory");
            freectx (ctx);
            return NULL;
        }
        ctx->dir = xasprintf ("%s/cas", dir);
        if (!(ctx->env = sp_env ())
                || sp_setstring (ctx->env, "sophia.path", ctx->dir, 0) < 0
                || sp_setstring (ctx->env, "db", "cas", 0) < 0
                || sp_setstring (ctx->env, "db.cas.index.key", "string", 0) < 0
                || sp_open (ctx->env) < 0
                || !(ctx->cas = sp_getobject (ctx->env, "db.cas"))) {
            log_sophia_error (ctx, "initialization");
            freectx (ctx);
            return NULL;
        }
        cleanup_push_string (cleanup_directory_recursive, ctx->dir);
        flux_aux_set (h, "flux::cas", ctx, freectx);
    }
    return ctx;
}

void load_cb (flux_t h, flux_msg_handler_t *w,
              const flux_msg_t *msg, void *arg)
{
    ctx_t *ctx = arg;
    void *key_data;
    int key_size;
    void *data = NULL;
    int size = 0;
    int rc = -1;
    void *o, *result = NULL;

    if (flux_request_decode_raw (msg, NULL, &key_data, &key_size) < 0) {
        flux_log_error (h, "load: request decode failed");
        goto done;
    }
    if (key_size != 20) {
        errno = EPROTO;
        flux_log_error (h, "load: incorrect key size");
        goto done;
    }
    o = sp_object (ctx->cas);
    if (sp_setstring (o, "key", key_data, key_size) < 0) {
        log_sophia_error (ctx, "load: sp_setstring key");
        errno = EINVAL;
        goto done;
    }
    if (!(result = sp_get (ctx->cas, o))) {
        log_sophia_error (ctx, "load: sp_get");
        errno = ENOENT; /* XXX */
        goto done;
    }
    if (!(data = sp_getstring (result, "value", &size))) {
        log_sophia_error (ctx, "load: sp_getstring value");
        errno = ENOENT;
        goto done;
    }
    rc = 0;
done:
    if (flux_respond_raw (h, msg, rc < 0 ? errno : 0,
                                  rc < 0 ? NULL : data, size) < 0)
        flux_log_error (h, "flux_respond");
    if (result)
        sp_destroy (result);
}

void store_cb (flux_t h, flux_msg_handler_t *w,
               const flux_msg_t *msg, void *arg)
{
    ctx_t *ctx = arg;
    void *data;
    int size;
    zdigest_t *zd = NULL;
    void *o;
    int rc = -1;

    if (flux_request_decode_raw (msg, NULL, &data, &size) < 0) {
        flux_log_error (h, "store: request decode failed");
        goto done;
    }
    if (!(zd = zdigest_new ())) {
        flux_log_error (h, "store: zdigest_new");
        errno = ENOMEM;
        goto done;
    }
    zdigest_update (zd, data, size);
    if (!zdigest_data (zd)) {
        flux_log_error (h, "store: zdigest_data NULL");
        goto done;
    }
    /* Checking if object is already stored is very costly, so don't.
     */
    o = sp_object (ctx->cas);
    if (sp_setstring (o, "key",  zdigest_data (zd), zdigest_size (zd)) < 0) {
        log_sophia_error (ctx, "store: sp_setstring key");
        errno = EINVAL;
        goto done;
    }
    if (sp_setstring (o, "value", data, size) < 0) {
        log_sophia_error (ctx, "store: sp_setstring value");
        errno = EINVAL;
        goto done;
    }
    if (sp_set (ctx->cas, o) < 0) {
        log_sophia_error (ctx, "store: sp_set");
        errno = EINVAL; /* XXX */
        goto done;
    }
    rc = 0;
done:
    if (flux_respond_raw (h, msg, rc < 0 ? errno : 0,
                                  rc < 0 ? NULL : zdigest_data (zd),
                                  rc < 0 ? 0 : zdigest_size (zd)) < 0)
        flux_log_error (h, "flux_respond");
    zdigest_destroy (&zd);
}

static void stats_get_cb (flux_t h, flux_msg_handler_t *w,
                          const flux_msg_t *msg, void *arg)
{
    ctx_t *ctx = arg;
    flux_msgcounters_t mcs;
    JSON out = Jnew ();

    /* replicate stats returned by all modules by default */
    flux_get_msgcounters (h, &mcs);
    Jadd_int (out, "#request (tx)", mcs.request_tx);
    Jadd_int (out, "#request (rx)", mcs.request_rx);
    Jadd_int (out, "#response (tx)", mcs.response_tx);
    Jadd_int (out, "#response (rx)", mcs.response_rx);
    Jadd_int (out, "#event (tx)", mcs.event_tx);
    Jadd_int (out, "#event (rx)", mcs.event_rx);
    Jadd_int (out, "#keepalive (tx)", mcs.keepalive_tx);
    Jadd_int (out, "#keepalive (rx)", mcs.keepalive_rx);

    /* add sophia system objects and configuration values */
    void *cursor = sp_getobject (ctx->env, NULL);
    void *ptr = NULL;
    while ((ptr = sp_get (cursor, ptr))) {
        char *key = sp_getstring (ptr, "key", NULL);
        char *value = sp_getstring (ptr, "value", NULL);
        Jadd_str (out, key, value ? value : "");
    }
    sp_destroy (cursor);

    if (flux_respond (h, msg, 0, Jtostr (out)) < 0)
        FLUX_LOG_ERROR (h);
    Jput (out);
}

static struct flux_msg_handler_spec htab[] = {
    { FLUX_MSGTYPE_REQUEST,     "cas.load",         load_cb },
    { FLUX_MSGTYPE_REQUEST,     "cas.store",        store_cb },
    { FLUX_MSGTYPE_REQUEST,     "cas.stats.get",    stats_get_cb },
    FLUX_MSGHANDLER_TABLE_END,
};

int mod_main (flux_t h, int argc, char **argv)
{
    ctx_t *ctx = getctx (h);
    if (!ctx)
        return -1;
    if (flux_msg_handler_addvec (h, htab, ctx) < 0) {
        flux_log_error (h, "flux_msg_handler_addvec");
        return -1;
    }
    if (flux_reactor_run (flux_get_reactor (h), 0) < 0) {
        flux_log_error (h, "flux_reactor_run");
        return -1;
    }
    flux_msg_handler_delvec (htab);
    return 0;
}

MOD_NAME ("cas");

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
