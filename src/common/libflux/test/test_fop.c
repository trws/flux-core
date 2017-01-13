#include "fop_dynamic.h"
#include "fop_protected.h"

#include "src/common/libtap/tap.h"

#include <assert.h>
#include <jansson.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Dynamic method resolution test
struct jsonableInterface {
    fop_class_t _;
    struct {
        json_t *(*to_json) (void *o);
        fni_t to_json_i;
    };
};
typedef struct jsonable {
    const struct jsonableInterface *i;
    void *data;
} jsonable_i;
const fop_class_t *jsonable_interface_c ();
json_t *jsonable_to_json (jsonable_i o)
{
    if (!o.i || !o.data)
        return NULL;
    return o.i->to_json (o.data);
}
jsonable_i jsonable_cast (fop *o)
{
    jsonable_i iface = {0};
    iface.i = fop_get_interface (o, jsonable_interface_c ());
    if (iface.i)
        iface.data = o;
    return iface;
}
const fop_class_t *jsonable_interface_c ()
{
    static struct jsonableInterface *cls = NULL;
    if (!cls) {
        cls = fop_new (fop_interface_c (), "jsonable_interface_c",
                       fop_interface_c (), sizeof (struct jsonableInterface));
        cls->to_json_i = (fni_t){"jsonable_to_json", (fop_vv_f)jsonable_to_json};
        /* fop_register_method (cls, (fop_vv_f)jsonable_to_json,
         * "jsonable_to_json", offsetof(struct jsonableInterface, to_json)); */
    }
    return (void*)cls;
}

// impl for geom
json_t *geom_to_json (void *o)
{
    fprintf (stderr, "in to_json\n");
    return o;
}
// END Dynamic method resolution test

struct geom;
// geom_class metaclass
struct geomClass {
    fop_class_t _;
    struct {
        double (*area) (struct geom *);
        fni_t area_i;
    };
    struct {
        double (*perim) (struct geom *);
        fni_t perim_i;
    };
    struct {
        json_t *(*to_json) (void *o);
        fni_t to_json_i;
    };
};
double geom_area (struct geom *self);
double geom_perim (struct geom *self);
const fop_class_t *geom_class_c ()
{
    static struct geomClass *cls = NULL;
    if (!cls) {
        cls = fop_new (fop_class_c (), "geom_class_c", fop_class_c (),
                       sizeof (struct geomClass));
    }
    return (void*)cls;
}

// abstract class geom, instance of metaclass geom_class
struct geom {
    fop_object_t _;
};
const fop_class_t *geom_c ();
void *geom_init (void *_self, va_list *app)
{
    struct rect *self = fop_initialize_super (geom_c (), _self, app);
    self = fop_cast (geom_c (), self);
    if (!self)
        return NULL;
    fprintf (stderr, "INITIALIZING GEOM!\n");
    return self;
}
void geom_fini (void *_c)
{
    fop_finalize_super (geom_c (), _c);
    fprintf (stderr, "FINALIZING GEOM!\n");
}
const fop_class_t *geom_c ()
{
    static struct geomClass *cls = NULL;
    if (!cls) {
        cls = fop_new (geom_class_c (), "geom_c", fop_object_c (),
                       sizeof (struct geom));
        cls->area_i = (fni_t){"area", (fop_vv_f)geom_area};
        cls->perim_i = (fni_t){"perim", (fop_vv_f)geom_perim};
        cls->to_json_i = (fni_t){"jsonable_to_json", (fop_vv_f)geom_to_json};
        cls->to_json = geom_to_json;
        cls->_.initialize = geom_init;
        cls->_.finalize = geom_fini;
        fop_describe (cls, stderr);
    }
    return (void *)cls;
}

// selectors
double geom_area (struct geom *o)
{
    const struct geomClass *c = (void *)fop_get_class_checked (o, geom_c ());
    assert (c->area);
    return c->area (o);
}
double geom_perim (struct geom *o)
{
    const struct geomClass *c = (void *)fop_get_class_checked (o, geom_c ());
    assert (c->perim);
    return c->perim (o);
}

// class rect, instance of metaclass geom_class, child of geom
struct rect {
    struct geom _;
    double w, h;
};
const fop_class_t *rect_c ();
void *rect_init (void *_self, va_list *app)
{
    struct rect *self = fop_initialize_super (rect_c (), _self, app);
    self = fop_cast (rect_c (), self);
    if (!self)
        return NULL;
    self->w = va_arg (*app, double);
    self->h = va_arg (*app, double);
    fprintf (stderr, "INITIALIZING RECT!\n");
    return self;
}
double rect_area (struct geom *_r)
{
    struct rect *r = fop_cast (rect_c (), _r);
    return r->w * r->h;
}

double rect_perim (struct geom *_r)
{
    struct rect *r = fop_cast (rect_c (), _r);
    return r->w * 2 + 2 * r->h;
}
const fop_class_t *rect_c ()
{
    static struct geomClass *cls = NULL;
    if (!cls) {
        cls = fop_new (geom_class_c (), "rect_c", geom_c (),
                       sizeof (struct rect));
        cls->_.initialize = rect_init;
        cls->area = rect_area;
        cls->perim = rect_perim;
    }
    return &cls->_;
}

// class rect, instance of metaclass geom_class, child of geom
struct circle {
    struct geom _;
    double r;
};
const fop_class_t *circle_c ();
double circle_area (struct geom *_c)
{
    struct circle *c = fop_cast (circle_c (), _c);
    return c->r * c->r * 3.14159;
}

double circle_perim (struct geom *_c)
{
    struct circle *c = fop_cast (circle_c (), _c);
    return 3.14159 * c->r * 2;
}
void circle_fini (void *_c)
{
    fop_finalize_super (circle_c (), _c);
    fprintf (stderr, "FINALIZING CIRCLE!\n");
}
void *circle_desc (void *_c, FILE *s)
{
    struct circle *c = fop_cast (circle_c (), _c);
    if (!c)
        return NULL;
    fprintf (s, "<Circle: radius=%lf>", c->r);
    return c;
}
const fop_class_t *circle_c ()
{
    static struct geomClass *cls = NULL;
    if (!cls) {
        cls = fop_new (geom_class_c (), "circle_c", geom_c (),
                       sizeof (struct circle));
        cls->_.finalize = circle_fini;
        cls->_.describe = circle_desc;
        cls->perim = circle_perim;
        cls->area = circle_area;
    }
    return &cls->_;
}

void measure (struct geom *g)
{
    printf ("a %lf\n", geom_area (g));
    printf ("p %lf\n", geom_perim (g));
}

int main (int argc, char *argv[])
{
    plan (2);
    struct rect *r = fop_new (rect_c (), 3.0, 4.0);
    ok (r != NULL);
    struct circle *c = fop_new (circle_c ());
    ok (c != NULL);
    c->r = 5;
    measure (&r->_);
    measure (&c->_);
    jsonable_i j = jsonable_cast (r);
    jsonable_to_json (j);
    fop_describe (r, stderr);
    fprintf (stderr, "\n");
    fop_describe (c, stderr);
    fprintf (stderr, "\n");
    fop_release (r);
    fop_release (c);
    return 0;
}
