#ifndef _FLUX_CORE_CONTENT_H
#define _FLUX_CORE_CONTENT_H

#include <stdint.h>
#include <flux/core.h>

/* Store object consisting of 'data' of size 'len'.
 * Response is a 20-byte SHA1 hash.
 * Hash is valid until rpc is destroyed with flux_rpc_destroy ().
 */
flux_rpc_t *content_store (flux_t h, const void *data, int len);
int content_store_get_hash (flux_rpc_t *r, void *hash, int *len);

/* Retrieve object with 'hash'.
 * Response is 'data' of size 'len'.
 * For convenience, a copy of the original hash is atttached to the rpc
 * so it can be retrieved again in a continuation callback.
 * Data is valid until rpc is destroyed with flux_rpc_destroy ().
 */
flux_rpc_t *content_load (flux_t h, const void *hash, int len);
int content_load_get_data (flux_rpc_t *r, void *data, int *len);
int content_load_get_hash (flux_rpc_t *r, void *hash, int *len);

#endif /* !_FLUX_CORE_CONTENT_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

