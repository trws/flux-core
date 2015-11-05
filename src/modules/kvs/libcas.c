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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "src/common/libutil/xzmalloc.h"
#include "src/common/libutil/log.h"
#include "cas.h"

static const int cas_hash_length = 20;

flux_rpc_t *cas_store (flux_t h, const void *data, int len)
{
    flux_rpc_t *rpc;

    if ((rpc = flux_rpc_raw (h, "cas.store", data, len, 0, 0))) {
        flux_rpc_type_set (rpc, "flux::cas_store");
    }
    return rpc;
}

int cas_store_get_hash (flux_rpc_t *rpc, void *hash, int *len)
{
    int l;
    void *h;
    if (strcmp (flux_rpc_type_get (rpc), "flux::cas_store") != 0) {
        errno = EINVAL;
        return -1;
    }
    if (flux_rpc_get_raw (rpc, NULL, &h, &l) < 0)
        return -1;
    if (l != cas_hash_length) {
        errno = EPROTO;
        return -1;
    }
    if (len)
        *len = l;
    if (hash)
        *(void **)hash = h;
    return 0;
}

flux_rpc_t *cas_load (flux_t h, const void *hash, int len)
{
    flux_rpc_t *rpc;

    if (len != cas_hash_length) {
        errno = EINVAL;
        return NULL;
    }
    if ((rpc = flux_rpc_raw (h, "cas.load", hash, len, 0, 0))) {
        flux_rpc_type_set (rpc, "flux::cas_load");
        uint8_t *hash_cpy = xzmalloc (len);
        memcpy (hash_cpy, hash, len);
        flux_rpc_aux_set (rpc, hash_cpy, free);
    }
    return rpc;
}

int cas_load_get_data (flux_rpc_t *rpc, void *data, int *len)
{
    if (strcmp (flux_rpc_type_get (rpc), "flux::cas_load") != 0) {
        errno = EINVAL;
        return -1;
    }
    return flux_rpc_get_raw (rpc, NULL, data, len);
}

int cas_load_get_hash (flux_rpc_t *rpc, void *hash, int *len)
{
    if (strcmp (flux_rpc_type_get (rpc), "flux::cas_load") != 0) {
        errno = EINVAL;
        return -1;
    }
    if (hash)
        *(void **)hash = flux_rpc_aux_get (rpc);
    if (len)
        *len = cas_hash_length;
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

