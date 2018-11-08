#ifndef __ARGZ_H
#define __ARGZ_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int argz_create(const char* const argv[], char** argz, size_t* argz_len);

int argz_create_sep(
    const char* string, int sep, char** argz, size_t* argz_len);

size_t argz_count(const char* argz, size_t argz_len);

void argz_extract(const char* argz, size_t argz_len, char** argv);

void argz_stringify(char* argz, size_t len, int sep);

int argz_add(char** argz, size_t* argz_len, const char* str);

int argz_add_sep(char** argz, size_t* argz_len, const char* str, int delim);

int argz_append(
    char** argz, size_t* argz_len, const char* buf, size_t buf_len);

void argz_delete(char** argz, size_t* argz_len, char* entry);

int argz_insert(
    char** argz, size_t* argz_len, char* before, const char* entry);

char* argz_next(const char* argz, size_t argz_len, const char* entry);

int argz_replace(char** argz, size_t* argz_len, const char* str,
    const char* with, unsigned* replace_count);

#ifdef __cplusplus
}
#endif
#endif /* ifndef __ARGZ_H */
