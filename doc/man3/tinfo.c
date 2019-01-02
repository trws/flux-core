/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#include <math.h>
#include <flux/core.h>
#include <inttypes.h>
#include "src/common/libutil/log.h"

int main (int argc, char **argv)
{
    flux_t *h;
    uint32_t rank, n;

    if (!(h = flux_open (NULL, 0)))
        log_err_exit ("flux_open");
    if (flux_get_rank (h, &rank) < 0)
        log_err_exit ("flux_get_rank");
    if (flux_get_size (h, &n) < 0)
        log_err_exit ("flux_get_size");
    flux_close (h);
    return (0);
}
