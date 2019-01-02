/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#include <flux/core.h>
#include "src/common/libutil/log.h"

int main (int argc, char **argv)
{
    flux_t *h;
    flux_msg_t *msg;
    const char *topic;

    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");
    if (flux_event_subscribe (h, "hb") < 0)
        log_err_exit ("flux_event_subscribe");
    if (!(msg = flux_recv (h, FLUX_MATCH_EVENT, 0)))
        log_err_exit ("flux_recv");
    if (flux_msg_get_topic (msg, &topic) < 0)
        log_err_exit ("flux_msg_get_topic");
    printf ("Event: %s\n", topic);
    if (flux_event_unsubscribe (h, "hb") < 0)
        log_err_exit ("flux_event_unsubscribe");
    flux_msg_destroy (msg);
    flux_close (h);
    return (0);
}
