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

/* content-sqlite.c - content addressable storage with sqlite back end */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sqlite3.h>
#include <czmq.h>
#include <flux/core.h>

#include "src/common/libutil/cleanup.h"
#include "src/common/libutil/shortjson.h"
#include "src/common/libutil/xzmalloc.h"
#include "src/common/libutil/log.h"

const char *sql_create_table = "CREATE TABLE objects("
                               "  hash CHAR(20) PRIMARY KEY,"
                               "  object BLOB"
                               ");";
const char *sql_load = "SELECT object FROM objects WHERE hash = ?1 LIMIT 1";
const char *sql_store = "INSERT INTO objects (hash,object) values (?1, ?2)";

typedef struct {
    char *dbdir;
    char *dbfile;
    sqlite3 *db;
    sqlite3_stmt *load_stmt;
    sqlite3_stmt *store_stmt;
    flux_t h;
} ctx_t;

static void log_sqlite_error (ctx_t *ctx, const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    char *s = xvasprintf (fmt, ap);
    va_end (ap);

    const char *error = sqlite3_errmsg (ctx->db);
    (void)flux_log (ctx->h, LOG_ERR, "%s: %s", s, error ? error : "failure");
    free (s);
}

static void freectx (void *arg)
{
    ctx_t *ctx = arg;
    if (ctx) {
        if (ctx->store_stmt)
            sqlite3_finalize (ctx->store_stmt);
        if (ctx->load_stmt)
            sqlite3_finalize (ctx->load_stmt);
        if (ctx->dbdir)
            free (ctx->dbdir);
        if (ctx->dbfile)
            free (ctx->dbfile);
        if (ctx->db)
            sqlite3_close (ctx->db);
        free (ctx);
    }
}

static ctx_t *getctx (flux_t h)
{
    ctx_t *ctx = (ctx_t *)flux_aux_get (h, "flux::content-sqlite");
    const char *scratchdir;

    if (!ctx) {
        ctx = xzmalloc (sizeof (*ctx));
        ctx->h = h;
        if (!(scratchdir = flux_attr_get (h, "scratch-directory", NULL))) {
            flux_log_error (h, "scratch-directory");
            goto error;
        }
        ctx->dbdir = xasprintf ("%s/content", scratchdir);
        if (mkdir (ctx->dbdir, 0755) < 0) {
            flux_log_error (h, "mkdir %s", ctx->dbdir);
            goto error;
        }
        cleanup_push_string (cleanup_directory_recursive, ctx->dbdir);
        ctx->dbfile = xasprintf ("%s/sqlite", ctx->dbdir);

        if (sqlite3_open (ctx->dbfile, &ctx->db) != SQLITE_OK) {
            flux_log_error (h, "sqlite3_open %s", ctx->dbfile);
            goto error;
        }
        if (sqlite3_exec (ctx->db, "PRAGMA journal_mode=OFF",
                                            NULL, NULL, NULL) != SQLITE_OK
                || sqlite3_exec (ctx->db, "PRAGMA synchronous=OFF",
                                            NULL, NULL, NULL) != SQLITE_OK
                || sqlite3_exec (ctx->db, "PRAGMA locking_mode=EXCLUSIVE",
                                            NULL, NULL, NULL) != SQLITE_OK) {
            log_sqlite_error (ctx, "setting sqlite pragmas");
            goto error;
        }
        if (sqlite3_exec (ctx->db, sql_create_table,
                                            NULL, NULL, NULL) != SQLITE_OK) {
            log_sqlite_error (ctx, "creating table");
            goto error;
        }
        if (sqlite3_prepare_v2 (ctx->db, sql_load, -1, &ctx->load_stmt,
                                            NULL) != SQLITE_OK) {
            log_sqlite_error (ctx, "preparing load stmt");
            goto error;
        }
        if (sqlite3_prepare_v2 (ctx->db, sql_store, -1, &ctx->store_stmt,
                                            NULL) != SQLITE_OK) {
            log_sqlite_error (ctx, "preparing store stmt");
            goto error;
        }
        flux_aux_set (h, "flux::content-sqlite", ctx, freectx);
    }
    return ctx;
error:
    freectx (ctx);
    return NULL;
}

void load_cb (flux_t h, flux_msg_handler_t *w,
              const flux_msg_t *msg, void *arg)
{
    ctx_t *ctx = arg;
    void *key_data;
    int key_size;
    const void *data = NULL;
    int size = 0;
    int rc = -1;

    if (flux_request_decode_raw (msg, NULL, &key_data, &key_size) < 0) {
        flux_log_error (h, "load: request decode failed");
        goto done;
    }
    if (key_size != 20) {
        errno = EPROTO;
        flux_log_error (h, "load: incorrect key size");
        goto done;
    }
    if (sqlite3_bind_text (ctx->load_stmt, 1, (char *)key_data,
                                    key_size, SQLITE_STATIC) != SQLITE_OK) {
        log_sqlite_error (ctx, "load: binding key");
        errno = EIO;
        goto done;
    }
    if (sqlite3_step (ctx->load_stmt) != SQLITE_ROW) {
        log_sqlite_error (ctx, "load: executing stmt");
        errno = ENOENT;
    }
    if (sqlite3_column_type (ctx->load_stmt, 0) != SQLITE_BLOB) {
        flux_log (h, LOG_ERR, "load: selected value is not a blob");
        errno = EINVAL;
        goto done;
    }
    data = sqlite3_column_blob (ctx->load_stmt, 0);
    size = sqlite3_column_bytes (ctx->load_stmt, 0);
    rc = 0;
done:
    if (flux_respond_raw (h, msg, rc < 0 ? errno : 0, data, size) < 0)
        flux_log_error (h, "load: flux_respond");
    (void )sqlite3_reset (ctx->load_stmt);
}

void store_cb (flux_t h, flux_msg_handler_t *w,
               const flux_msg_t *msg, void *arg)
{
    ctx_t *ctx = arg;
    void *data, *key_data = NULL;
    int size, key_size = 0;
    zdigest_t *zd = NULL;
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
    if (sqlite3_bind_text (ctx->store_stmt, 1, (char *)zdigest_data (zd),
                           zdigest_size (zd), SQLITE_STATIC) != SQLITE_OK) {
        log_sqlite_error (ctx, "store: binding key");
        errno = EINVAL;
        goto done;
    }
    if (sqlite3_bind_blob (ctx->store_stmt, 2,
                           data, size, SQLITE_STATIC) != SQLITE_OK) {
        log_sqlite_error (ctx, "store: binding data");
        errno = EINVAL;
        goto done;
    }
    if (sqlite3_step (ctx->store_stmt) != SQLITE_DONE
                    && sqlite3_errcode (ctx->db) != SQLITE_CONSTRAINT) {
        log_sqlite_error (ctx, "store: executing stmt");
        errno = EINVAL;
        goto done;
    }
    key_data = zdigest_data (zd);
    key_size = zdigest_size (zd);
    rc = 0;
done:
    if (flux_respond_raw (h, msg, rc < 0 ? errno : 0, key_data, key_size) < 0)
        flux_log_error (h, "store: flux_respond");
    zdigest_destroy (&zd);
    (void) sqlite3_reset (ctx->store_stmt);
}

static struct flux_msg_handler_spec htab[] = {
    { FLUX_MSGTYPE_REQUEST,     "content.load",         load_cb },
    { FLUX_MSGTYPE_REQUEST,     "content.store",        store_cb },
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

MOD_NAME ("content-sqlite");
MOD_SERVICE ("content");

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
