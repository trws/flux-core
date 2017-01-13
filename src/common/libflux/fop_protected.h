#ifndef __FLUX_CORE_FOP_PROT_H
#define __FLUX_CORE_FOP_PROT_H
#include "fop.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*fop_vv_f) (void);

struct object {
    int32_t magic;
    int32_t refcount;
    const fop_class_t *fclass;
};

typedef void (*self_only_v_f) (void *);
typedef int (*self_only_i_f) (void *);
typedef fop *(*self_only_p_f) (fop *);
typedef fop *(*new_f) (const fop_class_t *, va_list *);
typedef fop *(*init_f) (fop *, va_list *);
typedef fop *(*putter_f) (fop *, FILE *);

struct fclass;

struct fclass_inner {
    size_t tags_len;
    size_t tags_cap;
    struct fop_method_record *tags_by_selector;
    size_t interfaces_len;
    fop_class_t **interfaces;
    bool sorted;
};

typedef struct fn_info {
    const char *name;
    fop_vv_f selector;
} fni_t;

struct method_block {
    fop_vv_f fn;
    fni_t fn_info;
};

struct fclass {
    struct object _;  // A class is also an object
    const char *name;
    const fop_class_t *super;
    size_t size;
    struct fclass_inner inner;

    struct {
        new_f new;
        fni_t new_i;
    };
    struct {
        init_f initialize;
        fni_t initialize_i;
    };
    struct {
        self_only_v_f finalize;
        fni_t finalize_i;
    };
    struct {
        putter_f describe;
        fni_t describe_i;
    };
    struct {
        putter_f represent;
        fni_t represent_i;
    };
    struct {
        self_only_v_f retain;
        fni_t retain_i;
    };
    struct {
        self_only_v_f release;
        fni_t release_i;
    };
};

#endif /* __FLUX_CORE_FOP_PROT_H */
