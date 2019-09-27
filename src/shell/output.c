/************************************************************\
 * Copyright 2019 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* std output handling
 *
 * Intercept task stdout, stderr and dispose of it according to
 * selected I/O mode.
 *
 * The leader shell implements an "shell-<id>.output" service that all
 * ranks send task output to.  Output objects accumulate in a json
 * array on the leader.  If standalone is set, output is written
 * directly to stdout/stderr.  If not standalone, output objects are
 * written to the "output" key in the job's guest KVS namespace per
 * RFC24.
 *
 * Notes:
 * - leader takes a completion reference which it gives up once each
 *   task sends an EOF for both stdout and stderr.
 * - completion reference also taken for each KVS commit, to ensure
 *   commits complete before shell exits
 * - all shells (even the leader) send I/O to the service with RPC
 * - Any errors getting I/O to the leader are logged by RPC completion
 *   callbacks.
 * - Any outstanding RPCs at shell_output_destroy() are synchronously waited for
 *   there (checked for error, then destroyed).
 * - In standalone mode, the loop:// connector enables RPCs to work
 * - In standalone mode, output is written to the shell's stdout/stderr not KVS
 * - The number of in-flight write requests on each shell is limited to
 *   shell_output_hwm, to avoid matchtag exhaustion, etc. for chatty tasks.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <jansson.h>
#include <flux/core.h>

#include "src/common/libutil/log.h"
#include "src/common/libeventlog/eventlog.h"
#include "src/common/libioencode/ioencode.h"

#include "task.h"
#include "svc.h"
#include "internal.h"
#include "builtins.h"

struct shell_output {
    flux_shell_t *shell;
    int refcount;
    int eof_pending;
    zlist_t *pending_writes;
    json_t *output;
    bool stopped;
    bool output_ready_sent;
};

static const int shell_output_lwm = 100;
static const int shell_output_hwm = 1000;

/* Pause/resume output on 'stream' of 'task'.
 */
static void shell_output_control_task (struct shell_task *task,
                                       const char *stream,
                                       bool stop)
{
    if (stop) {
        if (flux_subprocess_stream_stop (task->proc, stream) < 0)
            log_err ("flux_subprocess_stream_stop %d:%s", task->rank, stream);
    }
    else {
        if (flux_subprocess_stream_start (task->proc, stream) < 0)
            log_err ("flux_subprocess_stream_start %d:%s", task->rank, stream);
    }
}

/* Pause/resume output for all tasks.
 */
static void shell_output_control (struct shell_output *out, bool stop)
{
    struct shell_task *task;

    if (out->stopped != stop) {
        task = zlist_first (out->shell->tasks);
        while (task) {
            shell_output_control_task (task, "stdout", stop);
            shell_output_control_task (task, "stderr", stop);
            task = zlist_next (out->shell->tasks);
        }
        out->stopped = stop;
    }
}

static int shell_output_flush (struct shell_output *out)
{
    json_t *entry;
    size_t index;
    FILE *f;

    json_array_foreach (out->output, index, entry) {
        json_t *context;
        const char *name;
        const char *stream = NULL;
        int rank;
        char *data = NULL;
        int len = 0;
        if (eventlog_entry_parse (entry, NULL, &name, &context) < 0) {
            log_err ("eventlog_entry_parse");
            return -1;
        }
        if (!strcmp (name, "header")) {
            // TODO: acquire per-stream encoding type
        }
        else if (!strcmp (name, "data")) {
            if (iodecode (context, &stream, &rank, &data, &len, NULL) < 0) {
                log_err ("iodecode");
                return -1;
            }
            f = !strcmp (stream, "stdout") ? stdout : stderr;
            if (len > 0) {
                fprintf (f, "%d: ", rank);
                fwrite (data, len, 1, f);
            }
            free (data);
        }
    }
    if (json_array_clear (out->output) < 0)
        log_msg ("json_array_clear failed");
    return 0;
}

static void shell_output_commit_completion (flux_future_t *f, void *arg)
{
    struct shell_output *out = arg;

    /* Error failing to commit is a fatal error.  Should be cleaner in
     * future. Issue #2378 */
    if (flux_future_get (f, NULL) < 0)
        log_err_exit ("shell_output_commit");
    flux_future_destroy (f);

    if (flux_shell_remove_completion_ref (out->shell, "output.commit") < 0)
        log_err ("flux_shell_remove_completion_ref");
}

/* log entry to exec.eventlog that we've created the output directory */
static int shell_output_ready (struct shell_output *out, flux_kvs_txn_t *txn)
{
    json_t *entry = NULL;
    char *entrystr = NULL;
    const char *key = "exec.eventlog";
    int saved_errno, rc = -1;

    if (!(entry = eventlog_entry_create (0., "output-ready", NULL))) {
        log_err ("eventlog_entry_create");
        goto error;
    }
    if (!(entrystr = eventlog_entry_encode (entry))) {
        log_err ("eventlog_entry_encode");
        goto error;
    }
    if (flux_kvs_txn_put (txn, FLUX_KVS_APPEND, key, entrystr) < 0) {
        log_err ("flux_kvs_txn_put");
        goto error;
    }
    rc = 0;
error:
    /* on error, future destroyed via shell_output destroy */
    saved_errno = errno;
    json_decref (entry);
    free (entrystr);
    errno = saved_errno;
    return rc;
}

static int shell_output_commit (struct shell_output *out)
{
    flux_kvs_txn_t *txn = NULL;
    flux_future_t *f = NULL;
    char *chunk = NULL;
    int saved_errno;
    int rc = -1;

    if (!(chunk = eventlog_encode (out->output)))
        goto error;
    if (!(txn = flux_kvs_txn_create ()))
        goto error;
    if (flux_kvs_txn_put (txn, FLUX_KVS_APPEND, "output", chunk) < 0)
        goto error;
    /* if the output-ready eventlog entry has not been sent, send now.
     * This is usually sent when the output header is sent. */
    if (!out->output_ready_sent) {
        if (shell_output_ready (out, txn) < 0)
            goto error;
    }
    if (!(f = flux_kvs_commit (out->shell->h, NULL, 0, txn)))
        goto error;
    if (!out->output_ready_sent)
        out->output_ready_sent = true;
    if (flux_future_then (f, -1, shell_output_commit_completion, out) < 0)
        goto error;
    if (flux_shell_add_completion_ref (out->shell, "output.commit") < 0) {
        log_err ("flux_shell_remove_completion_ref");
        goto error;
    }
    /* f memory responsibility of shell_output_commit_completion()
     * callback */
    f = NULL;
    if (json_array_clear (out->output) < 0) {
        log_msg ("json_array_clear failed");
        goto error;
    }
    rc = 0;
error:
    saved_errno = errno;
    flux_kvs_txn_destroy (txn);
    free (chunk);
    flux_future_destroy (f);
    errno = saved_errno;
    return rc;
}

/* Convert 'iodecode' object to an valid RFC 24 data event.
 * N.B. the iodecode object is a valid "context" for the event.
 */
static void shell_output_write_cb (flux_t *h,
                                   flux_msg_handler_t *mh,
                                   const flux_msg_t *msg,
                                   void *arg)
{
    struct shell_output *out = arg;
    bool eof = false;
    json_t *o;
    json_t *entry;

    if (flux_request_unpack (msg, NULL, "o", &o) < 0)
        goto error;
    if (iodecode (o, NULL, NULL, NULL, NULL, &eof) < 0)
        goto error;
    if (shell_svc_allowed (out->shell->svc, msg) < 0)
        goto error;
    if (!(entry = eventlog_entry_pack (0., "data", "O", o))) // increfs 'o'
        goto error;
    if (json_array_append_new (out->output, entry) < 0) {
        json_decref (entry);
        errno = ENOMEM;
        goto error;
    }
    /* Error failing to commit is a fatal error.  Should be cleaner in
     * future. Issue #2378 */
    if (out->shell->standalone) {
        if (shell_output_flush (out) < 0)
            log_err_exit ("shell_output_flush");
    }
    else {
        if (shell_output_commit (out) < 0)
            log_err_exit ("shell_output_commit");
    }
    if (eof) {
        if (--out->eof_pending == 0) {
            flux_msg_handler_stop (mh);
            if (flux_shell_remove_completion_ref (out->shell, "output.write") < 0)
                log_err ("flux_shell_remove_completion_ref");
        }
    }
    if (flux_respond (out->shell->h, msg, NULL) < 0)
        log_err ("flux_respond");
    return;
error:
    if (flux_respond_error (out->shell->h, msg, errno, NULL) < 0)
        log_err ("flux_respond");
}

static void shell_output_write_completion (flux_future_t *f, void *arg)
{
    struct shell_output *out = arg;

    if (flux_future_get (f, NULL) < 0)
        log_err ("shell_output_write");
    zlist_remove (out->pending_writes, f);
    flux_future_destroy (f);

    if (zlist_size (out->pending_writes) <= shell_output_lwm)
        shell_output_control (out, false);
}

static int shell_output_write (struct shell_output *out,
                               int rank,
                               const char *stream,
                               const char *data,
                               int len,
                               bool eof)
{
    flux_future_t *f = NULL;
    json_t *o = NULL;

    if (!(o = ioencode (stream, rank, data, len, eof))) {
        log_err ("ioencode");
        return -1;
    }

    if (!(f = flux_shell_rpc_pack (out->shell, "write", 0, 0, "O", o)))
        goto error;
    if (flux_future_then (f, -1, shell_output_write_completion, out) < 0)
        goto error;
    if (zlist_append (out->pending_writes, f) < 0)
        log_msg ("zlist_append failed");
    json_decref (o);

    if (zlist_size (out->pending_writes) >= shell_output_hwm)
        shell_output_control (out, true);
    return 0;

error:
    flux_future_destroy (f);
    json_decref (o);
    return -1;
}

void shell_output_destroy (struct shell_output *out)
{
    if (out) {
        int saved_errno = errno;
        if (out->pending_writes) {
            flux_future_t *f;

            while ((f = zlist_pop (out->pending_writes))) { // leader+follower
                if (flux_future_get (f, NULL) < 0)
                    log_err ("shell_output_write");
                flux_future_destroy (f);
            }
            zlist_destroy (&out->pending_writes);
        }
        if (out->output && json_array_size (out->output) > 0) { // leader only
            if (out->shell->standalone) {
                if (shell_output_flush (out) < 0)
                    log_err ("shell_output_flush");
            }
            else {
                if (shell_output_commit (out) < 0)
                    log_err ("shell_output_commit");
            }
        }
        json_decref (out->output);
        free (out);
        errno = saved_errno;
    }
}

/* Append RFC 24 header event to 'output' JSON array and write out to
 * KVS.  Assume:
 * - fixed base64 encoding for stdout, stderr
 * - no options
 * - no stdlog
 */
static int shell_output_header (struct shell_output *out)
{
    json_t *o;
    int rc = -1;

    o = eventlog_entry_pack (0, "header",
                             "{s:i s:{s:s s:s} s:{s:i s:i} s:{}}",
                             "version", 1,
                             "encoding",
                               "stdout", "base64",
                               "stderr", "base64",
                             "count",
                               "stdout", out->shell->info->jobspec->task_count,
                               "stderr", out->shell->info->jobspec->task_count,
                             "options");
    if (!o) {
        errno = ENOMEM;
        goto error;
    }
    if (json_array_append_new (out->output, o) < 0) {
        json_decref (o);
        errno = ENOMEM;
        goto error;
    }
    if (out->shell->standalone) {
        if (shell_output_flush (out) < 0)
            log_err ("shell_output_flush");
    }
    else {
        /* will also emit output-ready event */
        if (shell_output_commit (out) < 0)
            log_err ("shell_output_commit");
    }
    rc = 0;
error:
    return rc;
}

struct shell_output *shell_output_create (flux_shell_t *shell)
{
    struct shell_output *out;

    if (!(out = calloc (1, sizeof (*out))))
        return NULL;
    out->shell = shell;
    if (!(out->pending_writes = zlist_new ()))
        goto error;
    if (shell->info->shell_rank == 0) {
        if (flux_shell_service_register (shell,
                                         "write",
                                         shell_output_write_cb,
                                         out) < 0)
            goto error;
        out->eof_pending = 2 * shell->info->jobspec->task_count;
        if (flux_shell_add_completion_ref (shell, "output.write") < 0)
            goto error;
        if (!(out->output = json_array ())) {
            errno = ENOMEM;
            goto error;
        }
        if (shell_output_header (out) < 0)
            goto error;
    }
    return out;
error:
    shell_output_destroy (out);
    return NULL;
}

static void task_output_cb (struct shell_task *task,
                            const char *stream,
                            void *arg)
{
    struct shell_output *out = arg;
    const char *data;
    int len;

    data = flux_subprocess_getline (task->proc, stream, &len);
    if (len < 0) {
        log_err ("read %s task %d", stream, task->rank);
    }
    else if (len > 0) {
        if (shell_output_write (out, task->rank, stream, data, len, false) < 0)
            log_err ("write %s task %d", stream, task->rank);
    }
    else if (flux_subprocess_read_stream_closed (task->proc, stream)) {
        if (shell_output_write (out, task->rank, stream, NULL, 0, true) < 0)
            log_err ("write eof %s task %d", stream, task->rank);
    }
}

static int shell_output_task_init (flux_plugin_t *p,
                                   const char *topic,
                                   flux_plugin_arg_t *args,
                                   void *data)
{
    flux_shell_t *shell = flux_plugin_get_shell (p);
    struct shell_output *out = flux_plugin_aux_get (p, "builtin.output");
    flux_shell_task_t *task;

    if (!shell || !out || !(task = flux_shell_current_task (shell)))
        return -1;

    if (flux_shell_task_channel_subscribe (task, "stderr",
                                           task_output_cb, out) < 0
        || flux_shell_task_channel_subscribe (task, "stdout",
                                              task_output_cb, out) < 0)
        return -1;
    return 0;
}

static int shell_output_init (flux_plugin_t *p,
                              const char *topic,
                              flux_plugin_arg_t *args,
                              void *data)
{
    flux_shell_t *shell = flux_plugin_get_shell (p);
    struct shell_output *out = shell_output_create (shell);
    if (!out)
        return -1;
    if (flux_plugin_aux_set (p, "builtin.output", out,
                            (flux_free_f) shell_output_destroy) < 0) {
        shell_output_destroy (out);
        return -1;
    }
    return 0;
}

struct shell_builtin builtin_output = {
    .name = "output",
    .init = shell_output_init,
    .task_init = shell_output_task_init
};

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */