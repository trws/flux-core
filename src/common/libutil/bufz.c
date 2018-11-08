#include "bufz.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int bufz_resize_nozero (char** bufz, ptrdiff_t* bufz_size, ptrdiff_t new_size)
{
    if (!bufz || !bufz_size)
        return -1;
    void* old_buf = *bufz;
    if (new_size == 0) {
        free (*bufz);
        *bufz = NULL;
    } else {
        *bufz = realloc (*bufz, new_size);
        if (!*bufz) {
            *bufz = old_buf;
            return -1;
        }
    }
    *bufz_size = new_size;
    return 0;
}

int bufz_resize (char** bufz, ptrdiff_t* bufz_size, ptrdiff_t new_size)
{
    ptrdiff_t old_size = bufz_size ? *bufz_size : 0;
    if (bufz_resize_nozero (bufz, bufz_size, new_size))
        return -1;
    if (old_size < new_size) {
        memset (*bufz + old_size, 0, new_size - old_size);
    }
    return 0;
}

int bufz_replace (char** bufz,
                  ptrdiff_t* bufz_size,
                  bzrange range,
                  const char* with,
                  ptrdiff_t with_size)
{
    if (!bufz || !bufz_size || range.offset > *bufz_size)
        return -1;
    ptrdiff_t before_size = range.offset;
    ptrdiff_t to_go_size = range.length;
    ptrdiff_t after_size = *bufz_size - before_size - range.length;
    ptrdiff_t total_size = before_size + with_size + after_size;

    if (total_size > *bufz_size) {
        if (bufz_resize_nozero (bufz, bufz_size, total_size) < 0)
            return -1;
    }

    char* before = *bufz;
    char* to_go = before + range.offset;
    char* after = to_go + to_go_size;

    if (after_size > 0)
        memmove (to_go + with_size, after, after_size);
    if (with_size)
        memmove (to_go, with, with_size);

    if (total_size < *bufz_size) {
        if (bufz_resize_nozero (bufz, bufz_size, total_size) < 0)
            return -1;
    }
    return 0;
}

int bufz_match_replace (char** bz,
                        ptrdiff_t* bz_size,
                        const char* match,
                        ptrdiff_t match_size,
                        const char* with,
                        ptrdiff_t with_size)
{
    return bufz_match_replace_count (bz, bz_size, match, match_size, with,
                                     with_size, NULL);
}

int bufz_match_replace_count (char** bz,
                              ptrdiff_t* bz_size,
                              const char* match,
                              ptrdiff_t match_size,
                              const char* with,
                              ptrdiff_t with_size,
                              unsigned* count)
{
    if (!bz || !*bz || !bz_size || !match || !with)
        return -1;
    if (bz_size == 0 || match_size == 0 || match_size > *bz_size)
        return 0;

    ptrdiff_t i = 0;
    while (i < *bz_size) {
        ptrdiff_t to_cmp =
            *bz_size - i < match_size ? *bz_size - i : match_size;
        if (!strncmp ((*bz) + i, match, to_cmp)) {
            bufz_replace (bz, bz_size, mkbzrange (i, match_size), with,
                          with_size);
            if (count)
                (*count)++;
            i += with_size ? with_size - 1 : 0;
        } else {
            i++;
        }
    }

    return 0;
}

int bufz_insert (char** bufz,
                 ptrdiff_t* bufz_size,
                 ptrdiff_t offset,
                 const char* with,
                 ptrdiff_t with_size)
{
    return bufz_replace (bufz, bufz_size, mkbzrange (offset, 0), with,
                         with_size);
}

int bufz_cat (char** bufz,
              ptrdiff_t* bufz_size,
              const char* with,
              ptrdiff_t with_size)
{
    return bufz_insert (bufz, bufz_size, *bufz_size, with, with_size);
}

int bufz_replace_cstr (char** bufz,
                       ptrdiff_t* bufz_size,
                       bzrange range,
                       const char* with)
{
    return bufz_replace (bufz, bufz_size, range, with, strlen (with));
}

int bufz_insert_cstr (char** bufz,
                      ptrdiff_t* bufz_size,
                      ptrdiff_t offset,
                      const char* with)
{
    return bufz_insert (bufz, bufz_size, offset, with, strlen (with));
}

int bufz_cat_cstr (char** bufz, ptrdiff_t* bufz_size, const char* with)
{
    return bufz_cat (bufz, bufz_size, with, strlen (with));
}
