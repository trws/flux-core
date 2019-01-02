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
#include <errno.h>
#include <stdarg.h>
#include "request.h"
#include "message.h"

static int request_decode (const flux_msg_t *msg, const char **topic)
{
    int type;
    const char *ts;
    int rc = -1;

    if (msg == NULL) {
        errno = EINVAL;
        goto done;
    }
    if (flux_msg_get_type (msg, &type) < 0)
        goto done;
    if (type != FLUX_MSGTYPE_REQUEST) {
        errno = EPROTO;
        goto done;
    }
    if (flux_msg_get_topic (msg, &ts) < 0)
        goto done;
    if (topic)
        *topic = ts;
    rc = 0;
done:
    return rc;
}

int flux_request_decode (const flux_msg_t *msg, const char **topicp,
                         const char **sp)
{
    const char *topic, *s;
    int rc = -1;

    if (request_decode (msg, &topic) < 0)
        goto done;
    if (flux_msg_get_string (msg, &s) < 0)
        goto done;
    if (topicp)
        *topicp = topic;
    if (sp)
        *sp = s;
    rc = 0;
done:
    return rc;
}

int flux_request_decode_raw (const flux_msg_t *msg, const char **topic,
                             const void **data, int *len)
{
    const char *ts;
    const void *d = NULL;
    int l = 0;
    int rc = -1;

    if (!data || !len) {
        errno = EINVAL;
        goto done;
    }
    if (request_decode (msg, &ts) < 0)
        goto done;
    if (flux_msg_get_payload (msg, &d, &l) < 0) {
        if (errno != EPROTO)
            goto done;
        errno = 0;
    }
    if (topic)
        *topic = ts;
    *data = d;
    *len = l;
    rc = 0;
done:
    return rc;
}

static int flux_request_vunpack (const flux_msg_t *msg, const char **topic,
                                 const char *fmt, va_list ap)
{
    const char *ts;
    int rc = -1;

    if (!fmt) {
        errno = EINVAL;
        goto done;
    }
    if (request_decode (msg, &ts) < 0)
        goto done;
    if (flux_msg_vunpack (msg, fmt, ap) < 0)
        goto done;
    if (topic)
        *topic = ts;
    rc = 0;
done:
    return rc;
}

int flux_request_unpack (const flux_msg_t *msg, const char **topic,
                         const char *fmt, ...)
{
    va_list ap;
    int rc;

    va_start (ap, fmt);
    rc = flux_request_vunpack (msg, topic, fmt, ap);
    va_end (ap);
    return rc;
}

static flux_msg_t *request_encode (const char *topic)
{
    flux_msg_t *msg = NULL;

    if (!topic) {
        errno = EINVAL;
        goto error;
    }
    if (!(msg = flux_msg_create (FLUX_MSGTYPE_REQUEST)))
        goto error;
    if (flux_msg_set_topic (msg, topic) < 0)
        goto error;
    if (flux_msg_enable_route (msg) < 0)
        goto error;
    return msg;
error:
    flux_msg_destroy (msg);
    return NULL;
}

flux_msg_t *flux_request_encode (const char *topic, const char *s)
{
    flux_msg_t *msg = request_encode (topic);

    if (!msg)
        goto error;
    if (s && flux_msg_set_string (msg, s) < 0)
        goto error;
    return msg;
error:
    flux_msg_destroy (msg);
    return NULL;
}

flux_msg_t *flux_request_encode_raw (const char *topic,
                                     const void *data, int len)
{
    flux_msg_t *msg = request_encode (topic);

    if (!msg)
        goto error;
    if (data && flux_msg_set_payload (msg, data, len) < 0)
        goto error;
    return msg;
error:
    flux_msg_destroy (msg);
    return NULL;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
