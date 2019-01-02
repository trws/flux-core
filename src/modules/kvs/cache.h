/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#ifndef _FLUX_KVS_CACHE_H
#define _FLUX_KVS_CACHE_H

#include <jansson.h>

#include "src/common/libutil/tstat.h"
#include "waitqueue.h"

struct cache_entry;
struct cache;


/* Create/destroy cache entry.
 *
 * cache_entry_create() creates an empty cache entry.  Data can be set
 * in an entry via cache_entry_set_raw().
 */
struct cache_entry *cache_entry_create (const char *ref);
void cache_entry_destroy (void *arg);

/* Return true if cache entry contains valid data.  False would
 * indicate that a load RPC is in progress.
 */
bool cache_entry_get_valid (struct cache_entry *entry);

/* Get/set cache entry's dirty bit.
 * The dirty bit indicates that a store RPC is in progress.
 * A true->false transitions runs the entry's wait queue, if any.
 * cache_entry_set_dirty() returns -1 on error, 0 on success
 */
bool cache_entry_get_dirty (struct cache_entry *entry);
int cache_entry_set_dirty (struct cache_entry *entry, bool val);

/* cache_entry_clear_dirty() is similar to calling
 * cache_entry_set_dirty(entry,false), but it will not set the dirty bit
 * to false if there are waiters for notdirty
 * (i.e. cache_entry_wait_notdirty() has been called on this entry).
 * This is typically called in an error path where the caller wishes
 * to give up on a previously marked dirty cache entry but has not yet
 * done anything with it.  Returns 0 on success, -1 on error.  Caller
 * should call cache_entry_get_dirty() to see if dirty bit was cleared
 * or not.
 *
 * cache_entry_force_clear_dirty() is similar to
 * cache_entry_clear_dirty(), but will clear the dirty bit no matter
 * what and destroy the internal wait queue of dirty bit waiters.  It
 * should be only used in emergency error handling cases.
 */
int cache_entry_clear_dirty (struct cache_entry *entry);
int cache_entry_force_clear_dirty (struct cache_entry *entry);

/* Accessors for cache entry data.
 *
 * raw set accessor transfers ownership of 'data' to the cache entry
 * if it is non-NULL.  If 'data' is non-NULL, 'len' must be > 0.  If
 * 'data' is NULL, 'len' must be zero.
 *
 * treeobj get accessor is a convenience function that will return the
 * treeobj object equivalent of the raw data stored internally.  If the
 * internal raw data is not a valid treeobj object (i.e. improperly
 * formatted or zero length), an error will be result.
 *
 * An invalid->valid transition runs the entry's wait queue, if any in
 * both set accessors.
 *
 * Generally speaking, a cache entry can only be set once.  An attempt
 * to set new data in a cache entry will silently succeed.
 *
 * cache_entry_set_raw() & cache_entry_clear_data()
 * return -1 on error, 0 on success
 */
int cache_entry_get_raw (struct cache_entry *entry, const void **data,
                         int *len);
int cache_entry_set_raw (struct cache_entry *entry, const void *data, int len);

const json_t *cache_entry_get_treeobj (struct cache_entry *entry);

/* in the event of a load or store RPC error, inform the cache to set
 * an error on all waiters of a type on a cache entry.
 */
int cache_entry_set_errnum_on_valid (struct cache_entry *entry, int errnum);
int cache_entry_set_errnum_on_notdirty (struct cache_entry *entry, int errnum);

/* Arrange for message handler represented by 'wait' to be restarted
 * once cache entry becomes valid or not dirty at completion of a
 * load or store RPC.
 * Returns -1 on error, 0 on success
 */
int cache_entry_wait_notdirty (struct cache_entry *entry, wait_t *wait);
int cache_entry_wait_valid (struct cache_entry *entry, wait_t *wait);

/* Get the blobref of this entry */
const char *cache_entry_get_blobref (struct cache_entry *entry);

/* Create/destroy the cache container and its contents.
 */
struct cache *cache_create (void);
void cache_destroy (struct cache *cache);

/* Look up a cache entry.
 * Update the entry's "last used" time to 'current_epoch',
 * taking care not to not run backwards.
 */
struct cache_entry *cache_lookup (struct cache *cache,
                                  const char *ref, int current_epoch);

/* Insert entry in the cache.  Reference for entry created during
 * cache_entry_create() time.  Ownership of the cache entry is
 * transferred to the cache.
 */
int cache_insert (struct cache *cache, struct cache_entry *entry);

/* Remove a cache_entry from the cache.  Will not be removed if dirty
 * or there are any waiters of any sort.
 * Returns 1 on removed, 0 if not
 */
int cache_remove_entry (struct cache *cache, const char *ref);

/* Return the number of cache entries.
 */
int cache_count_entries (struct cache *cache);

/* Expire cache entries that are not dirty, not incomplete, and last
 * used more than 'thresh' epoch's ago.
 * Returns -1 on error, expired count on success.
 */
int cache_expire_entries (struct cache *cache, int current_epoch, int thresh);

/* Obtain statistics on the cache.
 * Returns -1 on error, 0 on success
 */
int cache_get_stats (struct cache *cache, tstat_t *ts, int *size,
                     int *incomplete, int *dirty);

/* Destroy wait_t's on the waitqueue_t of any cache entry
 * if they meet match criteria.
 */
int cache_wait_destroy_msg (struct cache *cache, wait_test_msg_f cb, void *arg);

#endif /* !_FLUX_KVS_CACHE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
