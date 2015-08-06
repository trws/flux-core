#include "unique_string_array.h"

#include "xzmalloc.h"

unique_string_array_t * usa_alloc ()
{
    return xzmalloc(sizeof(unique_string_array_t));
}

void usa_init (unique_string_array_t *item)
{
    item->clean = false;
    item->sep = sdsempty();
    item->string_cache = sdsempty();
    vec_init(&item->components);
}

unique_string_array_t * usa_new ()
{
    unique_string_array_t *item = usa_alloc();
    usa_init(item);
    return item;
}

void usa_clear (unique_string_array_t *item)
{
    int i;
    sds to_free;
    if (!item)
        return;

    sdsclear(item->sep);
    sdsclear(item->string_cache);
    vec_foreach(&item->components, to_free, i){
        sdsfree(to_free);
    }
    vec_clear(&item->components);
    item->clean = false;
}

void usa_destroy (unique_string_array_t *item)
{
    usa_clear(item);
    vec_deinit(&item->components);
    sdsfree(item->sep);
    sdsfree(item->string_cache);
    free(item);
}


void usa_set (unique_string_array_t *item, const char * value)
{
    item->clean = false;
    usa_split_and_push(item, value, false);
}

void usa_push (unique_string_array_t *item, const char *str)
{
    vec_insert(&item->components, 0, sdsnew(str));
}

void usa_push_back (unique_string_array_t *item, const char *str)
{
    vec_push(&item->components, sdsnew(str));
}

void usa_split_and_push (unique_string_array_t * item, const char *value, bool before)
{
    int value_count=0, i, j;
    bool insert_on_blank = true;

    if (!value || strlen(value) == 0)
        return;

    sds * new_parts = sdssplitlen(value, strlen(value), item->sep, strlen(item->sep), &value_count);

    for (i=0; i<value_count; i++){
        const char *old_path_part;
        bool skip = false;
        vec_foreach(&item->components, old_path_part, j){
            if ( !strcmp(new_parts[i], old_path_part) ) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            if (!sdslen(new_parts[i]) && insert_on_blank) {
                // This odd dance is for lua's default path item
                new_parts[i] = sdscpy(new_parts[i], item->sep);
                insert_on_blank=false;
            }
            if (before)
                vec_insert(&item->components, 0, new_parts[i]);
            else
                vec_push(&item->components, new_parts[i]);
            item->clean = false;
            new_parts[i] = NULL; // Prevent this sds from being freed by sdsfreesplitres
        }
    }

    sdsfreesplitres(new_parts, value_count);
}

void usa_set_separator(unique_string_array_t *item, const char *separator)
{
    if (item)
        item->sep = sdscpy(item->sep, separator);
}

const char * usa_get_joined(unique_string_array_t *item)
{
    if (! item)
        return NULL;

    if (usa_length(item) == 1) 
        return usa_data(item)[0];

    if (! item->clean) {
        sdsfree(item->string_cache);
        item->string_cache = sdsjoinsds(item->components.data, item->components.length, item->sep, sdslen(item->sep));
    }

    return item->string_cache;
}


