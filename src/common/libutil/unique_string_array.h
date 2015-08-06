#ifndef FLUX_UNIQUE_STRING_ARRAY_H
#define FLUX_UNIQUE_STRING_ARRAY_H

#include <stdbool.h>

#include "sds.h"
#include "vec.h"

typedef struct unique_string_array {
    sds sep;
    vec_str_t components;
    sds string_cache;
    bool clean;
} unique_string_array_t;

#define usa_length(a) ((a)->components.length)

#define usa_data(a) ((a)->components.data)

unique_string_array_t * usa_alloc ();
void usa_init (unique_string_array_t *item);
unique_string_array_t * usa_new ();
void usa_clear (unique_string_array_t *item);
void usa_destroy (unique_string_array_t *item);

void usa_set (unique_string_array_t *item, const char *str);
void usa_push (unique_string_array_t *item, const char *str);
void usa_push_back (unique_string_array_t *item, const char *str);

void usa_set_separator(unique_string_array_t *item, const char *separator);
void usa_split_and_push(unique_string_array_t *item, const char *value, bool before);
const char * usa_get_joined(unique_string_array_t *item);

#endif /* FLUX_UNIQUE_STRING_ARRAY_H */

