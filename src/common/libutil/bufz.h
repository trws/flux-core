/**
 * buffer management, where a buffer is defined as a char* and an ptrdiff_t
 * tuple, an empty tuple should be initialized as (NULL, 0), much like argz for
 * path-like or argv-like strings this will manage length-based rather than
 * zero-terminated strings and arbitrary buffers with similar characteristics.
 * If a buffer is of size <=0, the pointer should be NULL.  The buffer
 * component of a bufz should be freed after use.
 */
#ifndef __BUFZ_H
#define __BUFZ_H 1

#include <inttypes.h>
#include <stddef.h>

typedef struct {
    ptrdiff_t offset;
    ptrdiff_t length;
} bzrange;

static inline bzrange mkbzrange (ptrdiff_t offset, ptrdiff_t length)
{
    bzrange r;
    r.offset = offset;
    r.length = length;
    return r;
}

/**
 * @brief resize bufz to new_size filling right with zeros
 *
 * If an error occurs in reallocation, the original buffer is preserved and
 * negative is returned.
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param new_size
 *
 * @return 0 on success, <0 on failure
 */
int bufz_resize (char** bufz, ptrdiff_t* bufz_size, ptrdiff_t new_size);

/**
 * @brief resize bufz to new_size, extra space uninitialized
 *
 * If an error occurs in reallocation, the original buffer is preserved and
 * negative is returned.
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param new_size
 *
 * @return 0 on success, <0 on failure
 */
int bufz_resize_nozero (char** bufz, ptrdiff_t* bufz_size, ptrdiff_t new_size);

/**
 * @brief replace bufz[range] with the buffer in with
 *
 * Replace a range of bufz with an arbitrary buffer, the range can be of zero
 * length and the with can be of zero length to perform insertions, deletions
 * or any such operation implied by these arguments.  On reallocation error,
 * the original buffer is preserved and error is returned.
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param range offset/length pair, create with sc_range
 * @param with char * component of bufz with
 * @param with_size ptrdiff_t component of bufz with
 *
 * @return 0 on success, <0 on error
 */
int bufz_replace (char** bufz,
                  ptrdiff_t* bufz_size,
                  bzrange range,
                  const char* with,
                  ptrdiff_t with_size);

/**
 * @brief replace bufz[range] with a cstr
 *
 * See bufz_replace, this is a convenience wrapper for:
 *
 *   bufz_replace(bufz, bufz_size, range, with, strlen(with));
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param range offset/length pair, create with sc_range
 * @param with cstring to use as replacement
 *
 * @return 0 on success, <0 on error
 */
int bufz_replace_cstr (char** bufz,
                       ptrdiff_t* bufz_size,
                       bzrange range,
                       const char* with);

/**
 * @brief insert bufz with into bufz at offset
 *
 * This is a convenience routine for:
 *
 *   bufz_replace(bufz, bufz_size, sc_range(offset, 0), with, with_size);
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param offset offset at which to insert with
 * @param with char * component of bufz with
 * @param with_size ptrdiff_t component of bufz with
 *
 * @return 0 on success, <0 on error
 */
int bufz_insert (char** bufz,
                 ptrdiff_t* bufz_size,
                 ptrdiff_t offset,
                 const char* with,
                 ptrdiff_t with_size);

/**
 * @brief insert cstr with into bufz at offset
 *
 * This is a convenience routine for:
 *
 *   bufz_insert(bufz, bufz_size, offset, with, strlen(with));
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param offset offset at which to insert with
 * @param with cstr to insert
 *
 * @return 0 on success, <0 on error
 */
int bufz_insert_cstr (char** bufz,
                      ptrdiff_t* bufz_size,
                      ptrdiff_t offset,
                      const char* with);

/**
 * @brief concatenate bufz and with into bufz
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param with char * component of bufz with
 * @param with_size ptrdiff_t component of bufz with
 *
 * @return 0 on success, <0 on error
 */
int bufz_cat (char** bufz,
              ptrdiff_t* bufz_size,
              const char* with,
              ptrdiff_t with_size);

/**
 * @brief concatenate bufz and the cstr with into bufz
 *
 * @param bufz char * component of bufz
 * @param bufz_size ptrdiff_t component of bufz
 * @param with cstr to concatenate
 *
 * @return 0 on success, <0 on error
 */
int bufz_cat_cstr (char** bufz, ptrdiff_t* bufz_size, const char* with);

/**
 * @brief Replace all instances of the bufz match in bz with the bufz with
 *
 * Unlike most bufz routines, this routine cannot guarantee an umodified bufz
 * on reallocation failure unless there is <=1 replacements to be made.
 *
 * @param bz bufz to modify
 * @param bz_size size component of bufz to modify
 * @param match bufz to match
 * @param match_size size of bufz to match
 * @param with bufz to replace all instances of match with
 * @param with_size size of bufz to replace all instances of match with
 *
 * @return 0 on success <0 on failure
 */
int bufz_match_replace (char** bz,
                        ptrdiff_t* bz_size,
                        const char* match,
                        ptrdiff_t match_size,
                        const char* with,
                        ptrdiff_t with_size);

/**
 * @brief Replace all instances of the bufz match in bz with the bufz with
 *
 * See bufz_match_replace, this routine can additionally return the number of
 * replacements performed through the count pointer.
 *
 * @param bz bufz to modify
 * @param bz_size size component of bufz to modify
 * @param match bufz to match
 * @param match_size size of bufz to match
 * @param with bufz to replace all instances of match with
 * @param with_size size of bufz to replace all instances of match with
 * @param count if non-null is populated with the number of replacements made
 *
 * @return 0 on success <0 on failure
 */
int bufz_match_replace_count (char** bz,
                              ptrdiff_t* bz_size,
                              const char* match,
                              ptrdiff_t match_size,
                              const char* with,
                              ptrdiff_t with_size,
                              unsigned* count);

#endif /* ifndef __BUFZ_H */
