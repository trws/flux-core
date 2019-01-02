/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#ifndef BROKER_BOOT_CONFIG_H
#define BROKER_BOOT_CONFIG_H

/* boot_config - bootstrap broker/overlay from config file */

/* Usage: flux broker -Sboot.method=config -Sboot.config_file=PATH
 *
 * Example config file (TOML):
 *
 *   session-id = "hype"
 *
 *   # tbon-endpoints array is ordered by rank (zeromq URI format)
 *   tbon-endpoints = [
 *       "tcp://192.168.1.100:8020",  # rank 0
 *       "tcp://192.168.1.101:8020",  # rank 1
 *       "tcp://192.168.1.102:8020",  # rank 2
 *   ]
 *
 *   # if commented out, rank is determined by scanning tbon-endpoints
 *   # for a local address
 *   rank = 2
 *
 *   # if commented out, instance size is the size of tbon-endpoints.
 *   size = 3
 */

#include "attr.h"
#include "overlay.h"

/* Broker attributes read/written directly by this method:
 *   boot.config_file (r)
 *   session-id (w)
 *   tbon.endpoint (w)
 */

int boot_config (overlay_t *overlay, attr_t *attrs, int tbon_k);

#endif /* BROKER_BOOT_CONFIG_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
