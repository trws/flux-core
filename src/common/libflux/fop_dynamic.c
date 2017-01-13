#include "fop_dynamic.h"

#include <assert.h>
#include <search.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// method record comparators
/* static int cmp_frm (const void *l, const void *r) */
/* { */
/*     const struct fop_method_record *lhs = l, *rhs = r; */
/*     if (lhs && rhs && lhs->tag && rhs->tag) */
/*         return strcmp (lhs->tag, rhs->tag); */
/*     else */
/*         return -1; */
/* } */

/* static int cmp_frm_sel (const void *l, const void *r) */
/* { */
/*     const struct fop_method_record *lhs = l, *rhs = r; */
/*     if ((uintptr_t)lhs->selector < (uintptr_t)rhs->selector) */
/*         return -1; */
/*     if ((uintptr_t)lhs->selector == (uintptr_t)rhs->selector) */
/*         return 0; */
/*     return 1; */
/* } */

static int cmp_mb (const void *l, const void *r)
{
    const struct method_block *lhs = l, *rhs = r;
    if (lhs && rhs && lhs->fn_info.name && rhs->fn_info.name)
        return strcmp (lhs->fn_info.name, rhs->fn_info.name);
    else
        return -1;
}

static int cmp_mb_sel (const void *l, const void *r)
{
    const struct method_block *lhs = l, *rhs = r;
    if ((uintptr_t)lhs->fn_info.selector < (uintptr_t)rhs->fn_info.selector)
        return -1;
    if ((uintptr_t)lhs->fn_info.selector == (uintptr_t)rhs->fn_info.selector)
        return 0;
    return 1;
}

static int cmp_interface (const void *key, const void *member)
{
    const fop_class_t *k = key;
    return fop_cast (k, member) == 0;
}

const fop_class_t *fop_interface_c ()
{
    static fop_class_t *cls = NULL;
    if (!cls) {
        cls = fop_new (fop_class_c (), "Interface", fop_class_c (),
                       sizeof (fop_class_t));
        struct method_block *imethods = (void *)&cls->new;
        for (size_t i = 0; i < cls->inner.tags_len; ++i) {
            imethods[i].fn_info.selector = NULL;
        }
    }
    return cls;
}

const void *fop_get_interface (const fop *o, const fop_class_t *interface)
{
    // TODO: Make interfaces un-allocatable, not really classes, just
    // interfaces
    const fop_class_t *c = fop_get_class (o);
    fop_describe ((void *)c, stderr);
    fop_describe ((void *)interface, stderr);

    interface = fop_cast (fop_class_c (), interface);
    if (!c || !interface)
        return NULL;
    size_t nel = c->inner.interfaces_len;
    const fop_class_t *ret = bsearch (&interface, c->inner.interfaces, nel,
                                      sizeof (void *), cmp_interface);
    if (ret)
        return ret;

    ret = fop_new (interface, c->name, interface, 0);
    struct method_block *imethods = (void *)&interface->new;

    for (size_t i = 0; i < interface->inner.tags_len; ++i) {
        if (!imethods[i].fn_info.selector)
            continue;
        fop_vv_f fn = fop_get_method (c, imethods[i].fn_info.selector);
        assert (fn);

        imethods[i].fn = fn;
    }

    return ret;
}

// method registration and implementation functions

fop_vv_f fop_get_method_by_name (const fop_class_t *c, const char *name)
{
    struct method_block matcher = {0, {name, 0}};
    size_t nel = c->inner.tags_len;
    struct method_block *r = lfind (&matcher, &c->new, &nel, sizeof *r, cmp_mb);
    if (!r)
        return NULL;
    return r->fn;
}
fop_vv_f fop_get_method (const fop_class_t *c, fop_vv_f sel)
{
    struct method_block matcher = {0, {0, sel}};
    size_t nel = c->inner.tags_len;
    struct method_block *r =
        lfind (&matcher, &c->new, &nel, sizeof *r, cmp_mb_sel);
    if (!r)
        return NULL;
    return r->fn;
}
int fop_implement_method_by_name (fop_class_t *c,
                                  const char *tag,
                                  fop_vv_f method)
{
    struct method_block matcher = {0, {tag, 0}};
    size_t nel = c->inner.tags_len;
    struct method_block *r = lfind (&matcher, &c->new, &nel, sizeof *r, cmp_mb);
    assert (r);
    r->fn = method;

    return 0;
}
int fop_implement_method (fop_class_t *c, fop_vv_f sel, fop_vv_f method)
{
    struct method_block matcher = {0, {0, sel}};
    size_t nel = c->inner.tags_len;
    struct method_block *r =
        lfind (&matcher, &c->new, &nel, sizeof *r, cmp_mb_sel);
    assert (r);
    r->fn = method;

    return 0;
}
