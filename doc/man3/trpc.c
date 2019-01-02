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
    flux_future_t *f;
    const char *rankstr;

    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");

    if (!(f = flux_rpc_pack (h, "attr.get", FLUX_NODEID_ANY, 0,
		             "{s:s}", "name", "rank")))
        log_err_exit ("flux_rpc_pack");

    if (flux_rpc_get_unpack (f, "{s:s}", "value", &rankstr) < 0)
        log_err_exit ("flux_rpc_get_unpack");

    printf ("rank is %s\n", rankstr);
    flux_future_destroy (f);

    flux_close (h);
    return (0);
}
