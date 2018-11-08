#include "argz.h"

#include "bufz.h"

#include <assert.h>
#include <string.h>

int argz_create (const char* const argv[], char** argz, size_t* argz_len)
{
    if (!argv || !argz || !argz_len)
        return -1;
    for (; *argv; argv++) {
        ptrdiff_t sz = *argz_len;
        bufz_cat (argz, &sz, *argv, strlen (*argv) + 1);
        *argz_len = sz;
    }
    return 0;
}

int argz_create_sep (const char* string, int sep, char** argz, size_t* argz_len)
{
    if (!string || !argz || !argz_len)
        return -1;
    *argz = strdup (string);
    *argz_len = strlen (string) + 1;
    char sepc = sep;
    ptrdiff_t azl = *argz_len;
    bufz_match_replace (argz, &azl, &sepc, 1, "\0", 1);
    *argz_len = azl;
    return 0;
}

size_t argz_count (const char* argz, size_t argz_len)
{
    size_t count = 0;
    if (!argz)
        return 0;
    for (size_t i = 0; i < argz_len; ++i) {
        if (argz[i] == '\0')
            count++;
    }
    return count;
}

void argz_extract (const char* argz, size_t argz_len, char** argv)
{
    if (!argz || !argv)
        return;
    size_t count = 0;
    const char* prev = argz;
    for (size_t i = 0; i < argz_len; ++i) {
        if (argz[i] == '\0') {
            argv[count] = (char*)prev;
            count++;
            prev = &argz[i + 1];
        }
    }
    argv[count] = NULL;
}

void argz_stringify (char* argz, size_t len, int sep)
{
    if (!argz || len <= 1)
        return;
    char sepc = sep;
    ptrdiff_t slen = len - 1;
    bufz_match_replace (&argz, &slen, "\0", 1, &sepc, 1);
    assert (slen == len - 1);
}

int argz_add (char** argz, size_t* argz_len, const char* str)
{
    ptrdiff_t sz = *argz_len;
    int e = bufz_cat (argz, &sz, str, strlen (str) + 1);
    *argz_len = sz;
    return e;
}

int argz_add_sep (char** argz, size_t* argz_len, const char* str, int delim)
{
    ptrdiff_t old_sz = *argz_len, sz = *argz_len;
    int e = bufz_cat (argz, &sz, str, strlen (str) + 1);
    *argz_len = sz;
    char delimc = delim;
    for (; old_sz < sz; ++old_sz) {
        if ((*argz)[old_sz] == delimc)
            argz[0][old_sz] = '\0';
    }
    return e;
}

int argz_append (char** argz, size_t* argz_len, const char* buf, size_t buf_len)
{
    if (!argz || !*argz || !argz_len)
        return -1;
    ptrdiff_t sz = *argz_len;
    int e = bufz_cat (argz, &sz, buf, buf_len);
    *argz_len = sz;
    return e;
}

void argz_delete (char** argz, size_t* argz_len, char* entry)
{
    if (!argz || !*argz || !argz_len || !entry)
        return;
    // past the end
    if (argz[0] + argz_len[0] <= entry)
        return;
    // before the start
    if (entry < argz[0])
        return;
    bzrange r = {};
    // not at the start
    if (entry != argz[0]) {
        // not the beginning of an entry
        if (entry[-1] != '\0')
            return;
        r.offset = entry - argz[0];
    }
    r.length = strlen (entry) + 1;
    ptrdiff_t sz = *argz_len;
    bufz_replace (argz, &sz, r, "", 0);
    *argz_len = sz;
}

int argz_insert (char** argz, size_t* argz_len, char* before, const char* entry)
{
    if (!argz || !*argz || !argz_len || !entry)
        return -1;
    ptrdiff_t sz = *argz_len;
    if (before == 0)
        before = *argz + *argz_len;
    int e = bufz_insert (argz, &sz, before - *argz, entry, strlen (entry) + 1);
    *argz_len = sz;
    return e;
}

char* argz_next (const char* argz, size_t argz_len, const char* entry)
{
    if (!entry)
        return (char*)argz;
    if (argz_len == 0)
        return 0;
    while ((entry < argz + argz_len - 1)) {
        if (entry[0] == '\0')
            return (char*)entry + 1;
        entry++;
    }
    return (char*)entry;
}

int argz_replace (char** argz,
                  size_t* argz_len,
                  const char* str,
                  const char* with,
                  unsigned* replace_count)
{
    if (!argz || !str || !with || (!argz[0] && argz_len[0]))
        return -1;
    ptrdiff_t sz = *argz_len;
    int e = bufz_match_replace_count (argz, &sz, str, strlen (str) + 1, with,
                                      strlen (with) + 1, replace_count);
    *argz_len = sz;
    return e;
}
