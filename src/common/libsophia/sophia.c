
/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

/* amalgamation build
 *
 * version:     1.2
 * build:       
 * build date:  Mon Nov  2 11:39:40 PST 2015
 *
 * compilation:
 * cc -O2 -DNDEBUG -std=c99 -pedantic -Wall -Wextra -pthread -c sophia.c
*/

/* {{{ */

#define SOPHIA_BUILD ""

#line 1 "sophia/std/ss_stdc.h"
#ifndef SS_STDC_H_
#define SS_STDC_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
/* crc */
#if defined (__x86_64__) || defined (__i386__)
#include <cpuid.h>
#endif
/* zstd */
#ifdef __AVX2__
#include <immintrin.h>
#endif

#endif
#line 1 "sophia/std/ss_macro.h"
#ifndef SS_MACRO_H_
#define SS_MACRO_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
#  define sshot __attribute__((hot))
#else
#  define sshot
#endif

#define sspacked __attribute__((packed))
#define ssunused __attribute__((unused))
#define ssinline __attribute__((always_inline))

#define sscast(N, T, F) ((T*)((char*)(N) - __builtin_offsetof(T, F)))

#define sslikely(EXPR)   __builtin_expect(!! (EXPR), 1)
#define ssunlikely(EXPR) __builtin_expect(!! (EXPR), 0)

#define ss_templatecat(a, b) ss_##a##b
#define ss_template(a, b) ss_templatecat(a, b)

#define ss_cmp(a, b) ((a) == (b) ? 0 : (((a) > (b)) ? 1 : -1))

#endif
#line 1 "sophia/std/ss_time.h"
#ifndef SS_TIME_H_
#define SS_TIME_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

void     ss_sleep(uint64_t);
uint64_t ss_utime(void);

#endif
#line 1 "sophia/std/ss_spinlock.h"
#ifndef SS_LOCK_H_
#define SS_LOCK_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#if 0
typedef pthread_spinlock_t ssspinlock;

static inline void
ss_spinlockinit(ssspinlock *l) {
	pthread_spin_init(l, 0);
}

static inline void
ss_spinlockfree(ssspinlock *l) {
	pthread_spin_destroy(l);
}

static inline void
ss_spinlock(ssspinlock *l) {
	pthread_spin_lock(l);
}

static inline void
ss_spinunlock(ssspinlock *l) {
	pthread_spin_unlock(l);
}
#endif

typedef uint8_t ssspinlock;

#if defined(__x86_64__) || defined(__i386) || defined(_X86_)
# define CPU_PAUSE __asm__ ("pause")
#else
# define CPU_PAUSE do { } while(0)
#endif

static inline void
ss_spinlockinit(ssspinlock *l) {
	*l = 0;
}

static inline void
ss_spinlockfree(ssspinlock *l) {
	*l = 0;
}

static inline void
ss_spinlock(ssspinlock *l) {
	if (__sync_lock_test_and_set(l, 1) != 0) {
		unsigned int spin_count = 0U;
		for (;;) {
			CPU_PAUSE;
			if (*l == 0U && __sync_lock_test_and_set(l, 1) == 0)
				break;
			if (++spin_count > 100U)
				usleep(0);
		}
	}
}

static inline void
ss_spinunlock(ssspinlock *l) {
	__sync_lock_release(l);
}

#endif
#line 1 "sophia/std/ss_list.h"
#ifndef SS_LIST_H_
#define SS_LIST_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sslist sslist;

struct sslist {
	sslist *next, *prev;
};

static inline void
ss_listinit(sslist *h) {
	h->next = h->prev = h;
}

static inline void
ss_listappend(sslist *h, sslist *n) {
	n->next = h;
	n->prev = h->prev;
	n->prev->next = n;
	n->next->prev = n;
}

static inline void
ss_listunlink(sslist *n) {
	n->prev->next = n->next;
	n->next->prev = n->prev;
}

static inline void
ss_listpush(sslist *h, sslist *n) {
	n->next = h->next;
	n->prev = h;
	n->prev->next = n;
	n->next->prev = n;
}

static inline sslist*
ss_listpop(sslist *h) {
	register sslist *pop = h->next;
	ss_listunlink(pop);
	return pop;
}

static inline int
ss_listempty(sslist *l) {
	return l->next == l && l->prev == l;
}

static inline void
ss_listmerge(sslist *a, sslist *b) {
	if (ssunlikely(ss_listempty(b)))
		return;
	register sslist *first = b->next;
	register sslist *last = b->prev;
	first->prev = a->prev;
	a->prev->next = first;
	last->next = a;
	a->prev = last;
}

static inline void
ss_listreplace(sslist *o, sslist *n) {
	n->next = o->next;
	n->next->prev = n;
	n->prev = o->prev;
	n->prev->next = n;
}

#define ss_listlast(H, N) ((H) == (N))

#define ss_listforeach(H, I) \
	for (I = (H)->next; I != H; I = (I)->next)

#define ss_listforeach_continue(H, I) \
	for (; I != H; I = (I)->next)

#define ss_listforeach_safe(H, I, N) \
	for (I = (H)->next; I != H && (N = I->next); I = N)

#define ss_listforeach_reverse(H, I) \
	for (I = (H)->prev; I != H; I = (I)->prev)

#endif
#line 1 "sophia/std/ss_pager.h"
#ifndef SS_PAGER_H_
#define SS_PAGER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sspagepool sspagepool;
typedef struct sspage sspage;
typedef struct sspager sspager;

struct sspagepool {
	uint32_t used;
	sspagepool *next;
} sspacked;

struct sspage {
	sspagepool *pool;
	sspage *next;
} sspacked;

struct sspager {
	uint32_t page_size;
	uint32_t pool_count;
	uint32_t pool_size;
	uint32_t pools;
	sspagepool *pp;
	sspage *p;
};

void  ss_pagerinit(sspager*, uint32_t, uint32_t);
void  ss_pagerfree(sspager*);
int   ss_pageradd(sspager*);
void *ss_pagerpop(sspager*);
void  ss_pagerpush(sspager*, sspage*);

#endif
#line 1 "sophia/std/ss_a.h"
#ifndef SS_A_H_
#define SS_A_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssaif ssaif;
typedef struct ssa ssa;

struct ssaif {
	int   (*open)(ssa*, va_list);
	int   (*close)(ssa*);
	void *(*malloc)(ssa*, int);
	void *(*realloc)(ssa*, void*, int);
	void  (*free)(ssa*, void*);
};

struct ssa {
	ssaif *i;
	char priv[48];
};

static inline int
ss_aopen(ssa *a, ssaif *i, ...) {
	a->i = i;
	va_list args;
	va_start(args, i);
	int rc = i->open(a, args);
	va_end(args);
	return rc;
}

static inline int
ss_aclose(ssa *a) {
	return a->i->close(a);
}

static inline void*
ss_malloc(ssa *a, int size) {
	return a->i->malloc(a, size);
}

static inline void*
ss_realloc(ssa *a, void *ptr, int size) {
	return a->i->realloc(a, ptr, size);
}

static inline void
ss_free(ssa *a, void *ptr) {
	a->i->free(a, ptr);
}

static inline char*
ss_strdup(ssa *a, char *str) {
	int sz = strlen(str) + 1;
	char *s = ss_malloc(a, sz);
	if (ssunlikely(s == NULL))
		return NULL;
	memcpy(s, str, sz);
	return s;
}

static inline char*
ss_memdup(ssa *a, void *ptr, size_t size) {
	char *s = ss_malloc(a, size);
	if (ssunlikely(s == NULL))
		return NULL;
	memcpy(s, ptr, size);
	return s;
}

#endif
#line 1 "sophia/std/ss_stda.h"
#ifndef SS_STDA_H_
#define SS_STDA_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

extern ssaif ss_stda;

#endif
#line 1 "sophia/std/ss_slaba.h"
#ifndef SS_SLABA_H_
#define SS_SLABA_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

extern ssaif ss_slaba;

#endif
#line 1 "sophia/std/ss_trace.h"
#ifndef SS_TRACE_H_
#define SS_TRACE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sstrace sstrace;

struct sstrace {
	ssspinlock lock;
	const char *file;
	const char *function;
	int line;
	char message[100];
};

static inline void
ss_traceinit(sstrace *t) {
	ss_spinlockinit(&t->lock);
	t->message[0] = 0;
	t->line = 0;
	t->function = NULL;
	t->file = NULL;
}

static inline void
ss_tracefree(sstrace *t) {
	ss_spinlockfree(&t->lock);
}

static inline int
ss_tracecopy(sstrace *t, char *buf, int bufsize) {
	ss_spinlock(&t->lock);
	int len = snprintf(buf, bufsize, "%s", t->message);
	ss_spinunlock(&t->lock);
	return len;
}

static inline void
ss_vtrace(sstrace *t,
          const char *file,
          const char *function, int line,
          char *fmt, va_list args)
{
	ss_spinlock(&t->lock);
	t->file     = file;
	t->function = function;
	t->line     = line;
	vsnprintf(t->message, sizeof(t->message), fmt, args);
	ss_spinunlock(&t->lock);
}

static inline int
ss_traceset(sstrace *t,
            const char *file,
            const char *function, int line,
            char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ss_vtrace(t, file, function, line, fmt, args);
	va_end(args);
	return -1;
}

#define ss_trace(t, fmt, ...) \
	ss_traceset(t, __FILE__, __FUNCTION__, __LINE__, fmt, __VA_ARGS__)

#endif
#line 1 "sophia/std/ss_gc.h"
#ifndef SS_GC_H_
#define SS_GC_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssgc ssgc;

struct ssgc {
	ssspinlock lock;
	int mark;
	int sweep;
	int complete;
};

static inline void
ss_gcinit(ssgc *gc)
{
	ss_spinlockinit(&gc->lock);
	gc->mark     = 0;
	gc->sweep    = 0;
	gc->complete = 0;
}

static inline void
ss_gclock(ssgc *gc) {
	ss_spinlock(&gc->lock);
}

static inline void
ss_gcunlock(ssgc *gc) {
	ss_spinunlock(&gc->lock);
}

static inline void
ss_gcfree(ssgc *gc)
{
	ss_spinlockfree(&gc->lock);
}

static inline void
ss_gcmark(ssgc *gc, int n)
{
	ss_spinlock(&gc->lock);
	gc->mark += n;
	ss_spinunlock(&gc->lock);
}

static inline void
ss_gcsweep(ssgc *gc, int n)
{
	ss_spinlock(&gc->lock);
	gc->sweep += n;
	ss_spinunlock(&gc->lock);
}

static inline void
ss_gccomplete(ssgc *gc)
{
	ss_spinlock(&gc->lock);
	gc->complete = 1;
	ss_spinunlock(&gc->lock);
}

static inline int
ss_gcinprogress(ssgc *gc)
{
	ss_spinlock(&gc->lock);
	int v = gc->complete;
	ss_spinunlock(&gc->lock);
	return !v;
}

static inline int
ss_gcready(ssgc *gc, float factor)
{
	ss_spinlock(&gc->lock);
	int ready = gc->sweep >= (gc->mark * factor);
	int rc = ready && gc->complete;
	ss_spinunlock(&gc->lock);
	return rc;
}

static inline int
ss_gcrotateready(ssgc *gc, int wm)
{
	ss_spinlock(&gc->lock);
	int rc = gc->mark >= wm;
	ss_spinunlock(&gc->lock);
	return rc;
}

static inline int
ss_gcgarbage(ssgc *gc)
{
	ss_spinlock(&gc->lock);
	int ready = (gc->mark == gc->sweep);
	int rc = gc->complete && ready;
	ss_spinunlock(&gc->lock);
	return rc;
}

#endif
#line 1 "sophia/std/ss_order.h"
#ifndef SS_ORDER_H_
#define SS_ORDER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef enum {
	SS_LT,
	SS_LTE,
	SS_GT,
	SS_GTE,
	SS_EQ,
	SS_STOP
} ssorder;

static inline ssorder
ss_orderof(char *order, int size)
{
	ssorder cmp = SS_STOP;
	if (strncmp(order, ">", size) == 0) {
		cmp = SS_GT;
	} else
	if (strncmp(order, ">=", size) == 0) {
		cmp = SS_GTE;
	} else
	if (strncmp(order, "<", size) == 0) {
		cmp = SS_LT;
	} else
	if (strncmp(order, "<=", size) == 0) {
		cmp = SS_LTE;
	}
	return cmp;
}

static inline char*
ss_ordername(ssorder o)
{
	switch (o) {
	case SS_LT:  return "<";
	case SS_LTE: return "<=";
	case SS_GT:  return ">";
	case SS_GTE: return ">=";
	default: break;
	}
	return NULL;
}

#endif
#line 1 "sophia/std/ss_trigger.h"
#ifndef SS_TRIGGER_H_
#define SS_TRIGGER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef int (*sstriggerf)(void *arg);

typedef struct sstrigger sstrigger;

struct sstrigger {
	sstriggerf function;
	void *arg;
};

static inline void
ss_triggerinit(sstrigger *t)
{
	t->function = NULL;
	t->arg = NULL;
}

static inline void
ss_triggerset(sstrigger *t, void *pointer)
{
	t->function = (sstriggerf)(uintptr_t)pointer;
}

static inline void
ss_triggerset_arg(sstrigger *t, void *pointer)
{
	t->arg = pointer;
}

static inline void
ss_triggerrun(sstrigger *t)
{
	if (t->function == NULL)
		return;
	t->function(t->arg);
}

#endif
#line 1 "sophia/std/ss_buf.h"
#ifndef SS_BUF_H_
#define SS_BUF_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssbuf ssbuf;

struct ssbuf {
	char *reserve;
	char *s, *p, *e;
};

static inline void
ss_bufinit(ssbuf *b)
{
	b->reserve = NULL;
	b->s = NULL;
	b->p = NULL;
	b->e = NULL;
}

static inline void
ss_bufinit_reserve(ssbuf *b, void *buf, int size)
{
	b->reserve = buf;
	b->s = buf;
	b->p = b->s; 
	b->e = b->s + size;
}

static inline void
ss_buffree(ssbuf *b, ssa *a)
{
	if (ssunlikely(b->s == NULL))
		return;
	if (ssunlikely(b->s != b->reserve))
		ss_free(a, b->s);
	b->s = NULL;
	b->p = NULL;
	b->e = NULL;
}

static inline void
ss_bufreset(ssbuf *b) {
	b->p = b->s;
}

static inline int
ss_bufsize(ssbuf *b) {
	return b->e - b->s;
}

static inline int
ss_bufused(ssbuf *b) {
	return b->p - b->s;
}

static inline int
ss_bufunused(ssbuf *b) {
	return b->e - b->p;
}

static inline int
ss_bufensure(ssbuf *b, ssa *a, int size)
{
	if (sslikely(b->e - b->p >= size))
		return 0;
	int sz = ss_bufsize(b) * 2;
	int actual = ss_bufused(b) + size;
	if (ssunlikely(actual > sz))
		sz = actual;
	char *p;
	if (ssunlikely(b->s == b->reserve)) {
		p = ss_malloc(a, sz);
		if (ssunlikely(p == NULL))
			return -1;
		memcpy(p, b->s, ss_bufused(b));
	} else {
		p = ss_realloc(a, b->s, sz);
		if (ssunlikely(p == NULL))
			return -1;
	}
	b->p = p + (b->p - b->s);
	b->e = p + sz;
	b->s = p;
	assert((b->e - b->p) >= size);
	return 0;
}

static inline int
ss_buftruncate(ssbuf *b, ssa *a, int size)
{
	assert(size <= (b->p - b->s));
	char *p = b->reserve;
	if (b->s != b->reserve) {
		p = ss_realloc(a, b->s, size);
		if (ssunlikely(p == NULL))
			return -1;
	}
	b->p = p + (b->p - b->s);
	b->e = p + size;
	b->s = p;
	return 0;
}

static inline void
ss_bufadvance(ssbuf *b, int size)
{
	b->p += size;
}

static inline int
ss_bufadd(ssbuf *b, ssa *a, void *buf, int size)
{
	int rc = ss_bufensure(b, a, size);
	if (ssunlikely(rc == -1))
		return -1;
	memcpy(b->p, buf, size);
	ss_bufadvance(b, size);
	return 0;
}

static inline int
ss_bufin(ssbuf *b, void *v) {
	assert(b->s != NULL);
	return (char*)v >= b->s && (char*)v < b->p;
}

static inline void*
ss_bufat(ssbuf *b, int size, int i) {
	return b->s + size * i;
}

static inline void
ss_bufset(ssbuf *b, int size, int i, char *buf, int bufsize)
{
	assert(b->s + (size * i + bufsize) <= b->p);
	memcpy(b->s + size * i, buf, bufsize);
}

#endif
#line 1 "sophia/std/ss_injection.h"
#ifndef SS_INJECTION_H_
#define SS_INJECTION_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssinjection ssinjection;

#define SS_INJECTION_SD_BUILD_0      0
#define SS_INJECTION_SD_BUILD_1      1
#define SS_INJECTION_SI_BRANCH_0     2
#define SS_INJECTION_SI_COMPACTION_0 3
#define SS_INJECTION_SI_COMPACTION_1 4
#define SS_INJECTION_SI_COMPACTION_2 5
#define SS_INJECTION_SI_COMPACTION_3 6
#define SS_INJECTION_SI_COMPACTION_4 7
#define SS_INJECTION_SI_RECOVER_0    8

struct ssinjection {
	int e[9];
};

#ifdef SS_INJECTION_ENABLE
	#define SS_INJECTION(E, ID, X) \
	if ((E)->e[(ID)]) { \
		X; \
	} else {}
#else
	#define SS_INJECTION(E, ID, X)
#endif

#endif
#line 1 "sophia/std/ss_crc.h"
#ifndef SS_CRC_H_
#define SS_CRC_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef uint32_t (*sscrcf)(uint32_t, const void*, int);

sscrcf ss_crc32c_function(void);

#define ss_crcp(F, p, size, crc) \
	F(crc, p, size)

#define ss_crcs(F, p, size, crc) \
	F(crc, (char*)p + sizeof(uint32_t), size - sizeof(uint32_t))

#endif
#line 1 "sophia/std/ss_type.h"
#ifndef SS_TYPE_H_
#define SS_TYPE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef enum {
	SS_UNDEF,
	SS_STRING,
	SS_STRINGPTR,
	SS_U32,
	SS_U64,
	SS_I64,
	SS_OBJECT,
	SS_FUNCTION
} sstype;

static inline char*
ss_typeof(sstype type) {
	switch (type) {
	case SS_UNDEF:     return "undef";
	case SS_STRING:    return "string";
	case SS_STRINGPTR: return "stringptr";
	case SS_U32:       return "u32";
	case SS_U64:       return "u64";
	case SS_I64:       return "i64";
	case SS_OBJECT:    return "object";
	case SS_FUNCTION:  return "function";
	}
	return NULL;
}

#endif
#line 1 "sophia/std/ss_mutex.h"
#ifndef SS_MUTEX_H_
#define SS_MUTEX_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssmutex ssmutex;

struct ssmutex {
	pthread_mutex_t m;
};

static inline void
ss_mutexinit(ssmutex *m) {
	pthread_mutex_init(&m->m, NULL);
}

static inline void
ss_mutexfree(ssmutex *m) {
	pthread_mutex_destroy(&m->m);
}

static inline void
ss_mutexlock(ssmutex *m) {
	pthread_mutex_lock(&m->m);
}

static inline void
ss_mutexunlock(ssmutex *m) {
	pthread_mutex_unlock(&m->m);
}

#endif
#line 1 "sophia/std/ss_cond.h"
#ifndef SS_COND_H_
#define SS_COND_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sscond sscond;

struct sscond {
	pthread_cond_t c;
};

static inline void
ss_condinit(sscond *c) {
	pthread_cond_init(&c->c, NULL);
}

static inline void
ss_condfree(sscond *c) {
	pthread_cond_destroy(&c->c);
}

static inline void
ss_condsignal(sscond *c) {
	pthread_cond_signal(&c->c);
}

static inline void
ss_condwait(sscond *c, ssmutex *m) {
	pthread_cond_wait(&c->c, &m->m);
}

#endif
#line 1 "sophia/std/ss_thread.h"
#ifndef SS_THREAD_H_
#define SS_THREAD_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssthread ssthread;

typedef void *(*ssthreadf)(void*);

struct ssthread {
	pthread_t id;
	void *arg;
};

int ss_threadnew(ssthread*, ssthreadf, void*);
int ss_threadjoin(ssthread*);

#endif
#line 1 "sophia/std/ss_quota.h"
#ifndef SS_QUOTA_H_
#define SS_QUOTA_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssquota ssquota;

typedef enum ssquotaop {
	SS_QGROW,
	SS_QADD,
	SS_QREMOVE
} ssquotaop;

struct ssquota {
	int enable;
	int wait;
	uint64_t limit;
	uint64_t used;
	ssmutex lock;
	sscond cond;
};

int ss_quotainit(ssquota*);
int ss_quotaset(ssquota*, uint64_t);
int ss_quotaenable(ssquota*, int);
int ss_quotafree(ssquota*);
int ss_quota(ssquota*, ssquotaop, uint64_t);

static inline uint64_t
ss_quotaused(ssquota *q)
{
	ss_mutexlock(&q->lock);
	uint64_t used = q->used;
	ss_mutexunlock(&q->lock);
	return used;
}

static inline int
ss_quotaused_percent(ssquota *q)
{
	ss_mutexlock(&q->lock);
	int percent;
	if (q->limit == 0) {
		percent = 0;
	} else {
		percent = (q->used * 100) / q->limit;
	}
	ss_mutexunlock(&q->lock);
	return percent;
}

#endif
#line 1 "sophia/std/ss_rb.h"
#ifndef SS_RB_H_
#define SS_RB_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssrbnode ssrbnode;
typedef struct ssrb  ssrb;

struct ssrbnode {
	ssrbnode *p, *l, *r;
	uint8_t color;
} sspacked;

struct ssrb {
	ssrbnode *root;
} sspacked;

static inline void
ss_rbinit(ssrb *t) {
	t->root = NULL;
}

static inline void
ss_rbinitnode(ssrbnode *n) {
	n->color = 2;
	n->p = NULL;
	n->l = NULL;
	n->r = NULL;
}

#define ss_rbget(name, compare) \
\
static inline int \
name(ssrb *t, \
     void *scheme ssunused, \
     void *key ssunused, int keysize ssunused, \
     ssrbnode **match) \
{ \
	ssrbnode *n = t->root; \
	*match = NULL; \
	int rc = 0; \
	while (n) { \
		*match = n; \
		switch ((rc = (compare))) { \
		case  0: return 0; \
		case -1: n = n->r; \
			break; \
		case  1: n = n->l; \
			break; \
		} \
	} \
	return rc; \
}

#define ss_rbtruncate(name, executable) \
\
static inline void \
name(ssrbnode *n, void *arg) \
{ \
	if (n->l) \
		name(n->l, arg); \
	if (n->r) \
		name(n->r, arg); \
	executable; \
}

ssrbnode *ss_rbmin(ssrb*);
ssrbnode *ss_rbmax(ssrb*);
ssrbnode *ss_rbnext(ssrb*, ssrbnode*);
ssrbnode *ss_rbprev(ssrb*, ssrbnode*);

void ss_rbset(ssrb*, ssrbnode*, int, ssrbnode*);
void ss_rbreplace(ssrb*, ssrbnode*, ssrbnode*);
void ss_rbremove(ssrb*, ssrbnode*);

#endif
#line 1 "sophia/std/ss_leb128.h"
#ifndef SS_LEB128_H_
#define SS_LEB128_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

static inline int
ss_leb128size(uint64_t value)
{
	int size = 0;
	do {
		value >>= 7;
		size++;
	} while (value != 0);
	return size;
}

static inline int
ss_leb128write(char *dest, uint64_t value)
{
	int size = 0;
	do {
		uint8_t byte = value & 0x7F;
		value >>= 7;
		if (value != 0)
			byte |= 0x80;
		((unsigned char*)dest)[size++] = byte;
	} while (value != 0);
	return size;
}

static inline int
ss_leb128read(char *src, uint64_t *value)
{
	unsigned char *start = (unsigned char*)src;
	int lsh = 0;
	*value = 0;
	do {
		*value |= ((uint64_t)(*(unsigned char*)src & 0x7F)) << lsh;
		lsh += 7;
	} while (*((unsigned char*)src++) >= 128);

	return (unsigned char*)src - start;
}

static inline int
ss_leb128skip(char *src)
{
	unsigned char *start = (unsigned char*)src;
	while (*((unsigned char*)src++) >= 128);
	return (unsigned char*)src - start;
}

#endif
#line 1 "sophia/std/ss_hash.h"
#ifndef SS_HASH_H_
#define SS_HASH_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

static inline unsigned int
ss_fnv(char *key, int len)
{
	unsigned char *p = (unsigned char*)key;
	unsigned char *end = p + len;
	unsigned h = 2166136261;
	while (p < end) {
		h = (h * 16777619) ^ *p;
		p++;
	}
	return h;
}

#endif
#line 1 "sophia/std/ss_ht.h"
#ifndef SS_HT_H_
#define SS_HT_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sshtnode sshtnode;
typedef struct ssht ssht;

struct sshtnode {
	uint32_t hash;
};

struct ssht {
	sshtnode **i;
	int count;
	int size;
};

static inline int
ss_htinit(ssht *t, ssa *a, int size)
{
	t->count = 0;
	t->size = size;
	int sz = size * sizeof(sshtnode*);
	t->i = (sshtnode**)ss_malloc(a, sz);
	if (ssunlikely(t->i == NULL))
		return -1;
	memset(t->i, 0, sz);
	return 0;
}

static inline void
ss_htfree(ssht *t, ssa *a)
{
	if (ssunlikely(t->i == NULL))
		return;
	ss_free(a, t->i);
	t->i = NULL;
}

static inline void
ss_htreset(ssht *t)
{
	int sz = t->size * sizeof(sshtnode*);
	memset(t->i, 0, sz);
	t->count = 0;
}

static inline int
ss_htisfull(ssht *t)
{
	return t->count > (t->size / 2);
}

static inline int
ss_htplace(ssht *t, sshtnode *node)
{
	uint32_t pos = node->hash % t->size;
	for (;;) {
		if (t->i[pos] != NULL) {
			pos = (pos + 1) % t->size;
			continue;
		}
		return pos;
	}
	return -1;
}

static inline int
ss_htresize(ssht *t, ssa *a)
{
	ssht nt;
	int rc = ss_htinit(&nt, a, t->size * 2);
	if (ssunlikely(rc == -1))
		return -1;
	int i = 0;
	while (i < t->size) {
		if (t->i[i]) {
			int pos = ss_htplace(&nt, t->i[i]);
			nt.i[pos] = t->i[i];
		}
		i++;
	}
	nt.count = t->count;
	ss_htfree(t, a);
	*t = nt;
	return 0;
}

#define ss_htsearch(name, compare) \
static inline int \
name(ssht *t, uint32_t hash, \
     char *key ssunused, \
     uint32_t size ssunused, void *ptr ssunused) \
{ \
	uint32_t pos = hash % t->size; \
	for (;;) { \
		if (t->i[pos] != NULL) { \
			if ( (compare) ) \
				return pos; \
			pos = (pos + 1) % t->size; \
			continue; \
		} \
		return pos; \
	} \
	return -1; \
}

static inline void
ss_htset(ssht *t, int pos, sshtnode *node)
{
	if (t->i[pos] == NULL)
		t->count++;
	t->i[pos] = node;
}

#endif
#line 1 "sophia/std/ss_rq.h"
#ifndef SS_RQ_H_
#define SS_RQ_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

/* range queue */

typedef struct ssrqnode ssrqnode;
typedef struct ssrqq ssrqq;
typedef struct ssrq ssrq;

struct ssrqnode {
	uint32_t q, v;
	sslist link;
};

struct ssrqq {
	uint32_t count;
	uint32_t q;
	sslist list;
};

struct ssrq {
	uint32_t range_count;
	uint32_t range;
	uint32_t last;
	ssrqq *q;
};

static inline void
ss_rqinitnode(ssrqnode *n) {
	ss_listinit(&n->link);
	n->q = UINT32_MAX;
	n->v = 0;
}

static inline int
ss_rqinit(ssrq *q, ssa *a, uint32_t range, uint32_t count)
{
	q->range_count = count + 1 /* zero */;
	q->range = range;
	q->q = ss_malloc(a, sizeof(ssrqq) * q->range_count);
	if (ssunlikely(q->q == NULL))
		return -1;
	uint32_t i = 0;
	while (i < q->range_count) {
		ssrqq *p = &q->q[i];
		ss_listinit(&p->list);
		p->count = 0;
		p->q = i;
		i++;
	}
	q->last = 0;
	return 0;
}

static inline void
ss_rqfree(ssrq *q, ssa *a) {
	ss_free(a, q->q);
}

static inline void
ss_rqadd(ssrq *q, ssrqnode *n, uint32_t v)
{
	uint32_t pos;
	if (ssunlikely(v == 0)) {
		pos = 0;
	} else {
		pos = (v / q->range) + 1;
		if (ssunlikely(pos >= q->range_count))
			pos = q->range_count - 1;
	}
	ssrqq *p = &q->q[pos];
	ss_listinit(&n->link);
	n->v = v;
	n->q = pos;
	ss_listappend(&p->list, &n->link);
	if (ssunlikely(p->count == 0)) {
		if (pos > q->last)
			q->last = pos;
	}
	p->count++;
}

static inline void
ss_rqdelete(ssrq *q, ssrqnode *n)
{
	ssrqq *p = &q->q[n->q];
	p->count--;
	ss_listunlink(&n->link);
	if (ssunlikely(p->count == 0 && q->last == n->q))
	{
		int i = n->q - 1;
		while (i >= 0) {
			ssrqq *p = &q->q[i];
			if (p->count > 0) {
				q->last = i;
				return;
			}
			i--;
		}
	}
}

static inline void
ss_rqupdate(ssrq *q, ssrqnode *n, uint32_t v)
{
	if (sslikely(n->q != UINT32_MAX))
		ss_rqdelete(q, n);
	ss_rqadd(q, n, v);
}

static inline ssrqnode*
ss_rqprev(ssrq *q, ssrqnode *n)
{
	int pos;
	ssrqq *p;
	if (sslikely(n)) {
		pos = n->q;
		p = &q->q[pos];
		if (n->link.next != (&p->list)) {
			return sscast(n->link.next, ssrqnode, link);
		}
		pos--;
	} else {
		pos = q->last;
	}
	for (; pos >= 0; pos--) {
		p = &q->q[pos];
		if (ssunlikely(p->count == 0))
			continue;
		return sscast(p->list.next, ssrqnode, link);
	}
	return NULL;
}

#endif
#line 1 "sophia/std/ss_path.h"
#ifndef SS_PATH_H_
#define SS_PATH_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sspath sspath;

struct sspath {
	char path[PATH_MAX];
};

static inline void
ss_pathset(sspath *p, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(p->path, sizeof(p->path), fmt, args);
	va_end(args);
}

static inline void
ss_pathA(sspath *p, char *dir, uint32_t id, char *ext)
{
	ss_pathset(p, "%s/%010"PRIu32"%s", dir, id, ext);
}

static inline void
ss_pathAB(sspath *p, char *dir, uint32_t a, uint32_t b, char *ext)
{
	ss_pathset(p, "%s/%010"PRIu32".%010"PRIu32"%s", dir, a, b, ext);
}

#endif
#line 1 "sophia/std/ss_iov.h"
#ifndef SD_IOV_H_
#define SD_IOV_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssiov ssiov;

struct ssiov {
	struct iovec *v;
	int iovmax;
	int iovc;
};

static inline void
ss_iovinit(ssiov *v, struct iovec *vp, int max)
{
	v->v = vp;
	v->iovc = 0;
	v->iovmax = max;
}

static inline int
ss_iovensure(ssiov *v, int count) {
	return (v->iovc + count) < v->iovmax;
}

static inline int
ss_iovhas(ssiov *v) {
	return v->iovc > 0;
}

static inline void
ss_iovreset(ssiov *v) {
	v->iovc = 0;
}

static inline void
ss_iovadd(ssiov *v, void *ptr, size_t size)
{
	assert(v->iovc < v->iovmax);
	v->v[v->iovc].iov_base = ptr;
	v->v[v->iovc].iov_len = size;
	v->iovc++;
}

#endif
#line 1 "sophia/std/ss_file.h"
#ifndef SS_FILE_H_
#define SS_FILE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssfile ssfile;

struct ssfile {
	ssa *a;
	int creat;
	uint64_t size;
	char *file;
	int fd;
};

static inline void
ss_fileinit(ssfile *f, ssa *a)
{
	f->a     = a;
	f->creat = 0;
	f->size  = 0;
	f->file  = NULL;
	f->fd    = -1;
}

static inline uint64_t
ss_filesvp(ssfile *f) {
	return f->size;
}

ssize_t ss_filesize(char*);
int ss_fileunlink(char*);
int ss_filemove(char*, char*);
int ss_fileexists(char*);
int ss_filemkdir(char*);
int ss_fileopen(ssfile*, char*);
int ss_filenew(ssfile*, char*);
int ss_filerename(ssfile*, char*);
int ss_fileclose(ssfile*);
int ss_filesync(ssfile*);
int ss_fileresize(ssfile*, uint64_t);
int ss_filepread(ssfile*, uint64_t, void*, size_t);
int ss_filewrite(ssfile*, void*, size_t);
int ss_filewritev(ssfile*, ssiov*);
int ss_fileseek(ssfile*, uint64_t);
int ss_filelock(ssfile*);
int ss_fileunlock(ssfile*);
int ss_filerlb(ssfile*, uint64_t);

#endif
#line 1 "sophia/std/ss_mmap.h"
#ifndef SS_MMAP_H_
#define SS_MMAP_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssmmap ssmmap;

struct ssmmap {
	char *p;
	size_t size;
};

static inline void
ss_mmapinit(ssmmap *m) {
	m->p = NULL;
	m->size = 0;
}

int ss_mmap(ssmmap*, int, uint64_t, int);
int ss_munmap(ssmmap*);

#endif
#line 1 "sophia/std/ss_filter.h"
#ifndef SS_FILTER_H_
#define SS_FILTER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssfilterif ssfilterif;
typedef struct ssfilter ssfilter;

typedef enum {
	SS_FINPUT,
	SS_FOUTPUT
} ssfilterop;

struct ssfilterif {
	char *name;
	int (*init)(ssfilter*, va_list);
	int (*free)(ssfilter*);
	int (*reset)(ssfilter*);
	int (*start)(ssfilter*, ssbuf*);
	int (*next)(ssfilter*, ssbuf*, char*, int);
	int (*complete)(ssfilter*, ssbuf*);
};

struct ssfilter {
	ssfilterif *i;
	ssfilterop op;
	ssa *a;
	char priv[90];
};

static inline int
ss_filterinit(ssfilter *c, ssfilterif *ci, ssa *a, ssfilterop op, ...)
{
	c->op = op;
	c->a  = a;
	c->i  = ci;
	va_list args;
	va_start(args, op);
	int rc = c->i->init(c, args);
	va_end(args);
	return rc;
}

static inline int
ss_filterfree(ssfilter *c)
{
	return c->i->free(c);
}

static inline int
ss_filterreset(ssfilter *c)
{
	return c->i->reset(c);
}

static inline int
ss_filterstart(ssfilter *c, ssbuf *dest)
{
	return c->i->start(c, dest);
}

static inline int
ss_filternext(ssfilter *c, ssbuf *dest, char *buf, int size)
{
	return c->i->next(c, dest, buf, size);
}

static inline int
ss_filtercomplete(ssfilter *c, ssbuf *dest)
{
	return c->i->complete(c, dest);
}

#endif
#line 1 "sophia/std/ss_nonefilter.h"
#ifndef SS_NONEFILTER_H_
#define SS_NONEFILTER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

extern ssfilterif ss_nonefilter;

#endif
#line 1 "sophia/std/ss_lz4filter.h"
#ifndef SS_LZ4FILTER_H_
#define SS_LZ4FILTER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

extern ssfilterif ss_lz4filter;

#endif
#line 1 "sophia/std/ss_zstdfilter.h"
#ifndef SS_ZSTDFILTER_H_
#define SS_ZSTDFILTER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

extern ssfilterif ss_zstdfilter;

#endif
#line 1 "sophia/std/ss_iter.h"
#ifndef SS_ITER_H_
#define SS_ITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct ssiterif ssiterif;
typedef struct ssiter ssiter;

struct ssiterif {
	void  (*close)(ssiter*);
	int   (*has)(ssiter*);
	void *(*of)(ssiter*);
	void  (*next)(ssiter*);
};

struct ssiter {
	ssiterif *vif;
	char priv[120];
};

#define ss_iterinit(iterator_if, i) \
do { \
	(i)->vif = &iterator_if; \
} while (0)

#define ss_iteropen(iterator_if, i, ...) iterator_if##_open(i, __VA_ARGS__)
#define ss_iterclose(iterator_if, i) iterator_if##_close(i)
#define ss_iterhas(iterator_if, i) iterator_if##_has(i)
#define ss_iterof(iterator_if, i) iterator_if##_of(i)
#define ss_iternext(iterator_if, i) iterator_if##_next(i)

#define ss_iteratorclose(i) (i)->vif->close(i)
#define ss_iteratorhas(i) (i)->vif->has(i)
#define ss_iteratorof(i) (i)->vif->of(i)
#define ss_iteratornext(i) (i)->vif->next(i)

#endif
#line 1 "sophia/std/ss_bufiter.h"
#ifndef SS_BUFITER_H_
#define SS_BUFITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

extern ssiterif ss_bufiter;
extern ssiterif ss_bufiterref;

typedef struct ssbufiter ssbufiter;

struct ssbufiter {
	ssbuf *buf;
	int vsize;
	void *v;
} sspacked;

static inline int
ss_bufiter_open(ssiter *i, ssbuf *buf, int vsize)
{
	ssbufiter *bi = (ssbufiter*)i->priv;
	bi->buf = buf;
	bi->vsize = vsize;
	bi->v = bi->buf->s;
	if (ssunlikely(bi->v == NULL))
		return 0;
	if (ssunlikely(! ss_bufin(bi->buf, bi->v))) {
		bi->v = NULL;
		return 0;
	}
	return 1;
}

static inline void
ss_bufiter_close(ssiter *i ssunused)
{ }

static inline int
ss_bufiter_has(ssiter *i)
{
	ssbufiter *bi = (ssbufiter*)i->priv;
	return bi->v != NULL;
}

static inline void*
ss_bufiter_of(ssiter *i)
{
	ssbufiter *bi = (ssbufiter*)i->priv;
	return bi->v;
}

static inline void
ss_bufiter_next(ssiter *i)
{
	ssbufiter *bi = (ssbufiter*)i->priv;
	if (ssunlikely(bi->v == NULL))
		return;
	bi->v = (char*)bi->v + bi->vsize;
	if (ssunlikely(! ss_bufin(bi->buf, bi->v)))
		bi->v = NULL;
}

static inline int
ss_bufiterref_open(ssiter *i, ssbuf *buf, int vsize) {
	return ss_bufiter_open(i, buf, vsize);
}

static inline void
ss_bufiterref_close(ssiter *i ssunused)
{ }

static inline int
ss_bufiterref_has(ssiter *i) {
	return ss_bufiter_has(i);
}

static inline void*
ss_bufiterref_of(ssiter *i)
{
	ssbufiter *bi = (ssbufiter*)i->priv;
	if (ssunlikely(bi->v == NULL))
		return NULL;
	return *(void**)bi->v;
}

static inline void
ss_bufiterref_next(ssiter *i) {
	ss_bufiter_next(i);
}

#endif
#line 1 "sophia/std/ss_bufiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



ssiterif ss_bufiter =
{
	.close   = ss_bufiter_close,
	.has     = ss_bufiter_has,
	.of      = ss_bufiter_of,
	.next    = ss_bufiter_next
};

ssiterif ss_bufiterref =
{
	.close   = ss_bufiterref_close,
	.has     = ss_bufiterref_has,
	.of      = ss_bufiterref_of,
	.next    = ss_bufiterref_next
};
#line 1 "sophia/std/ss_crc.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

/*
 * Copyright (c) 2008-2010 Massachusetts Institute of Technology
 * Copyright (c) 2004-2006 Intel Corporation
 *
 * This software program is licensed subject to the BSD License, 
 * available at http://www.opensource.org/licenses/bsd-license.html
*/



static const uint32_t crc_tableil8_o32[256] =
{
	0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4, 0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
	0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B, 0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
	0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B, 0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
	0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54, 0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
	0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A, 0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
	0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5, 0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
	0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45, 0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
	0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A, 0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
	0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48, 0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
	0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687, 0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
	0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927, 0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
	0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8, 0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
	0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096, 0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
	0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859, 0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
	0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9, 0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
	0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36, 0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
	0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C, 0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
	0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043, 0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
	0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3, 0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
	0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C, 0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
	0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652, 0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
	0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D, 0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
	0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D, 0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
	0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2, 0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
	0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530, 0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
	0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF, 0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
	0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F, 0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
	0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90, 0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
	0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE, 0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
	0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321, 0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
	0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81, 0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
	0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E, 0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351
};

static const uint32_t crc_tableil8_o40[256] =
{
	0x00000000, 0x13A29877, 0x274530EE, 0x34E7A899, 0x4E8A61DC, 0x5D28F9AB, 0x69CF5132, 0x7A6DC945,
	0x9D14C3B8, 0x8EB65BCF, 0xBA51F356, 0xA9F36B21, 0xD39EA264, 0xC03C3A13, 0xF4DB928A, 0xE7790AFD,
	0x3FC5F181, 0x2C6769F6, 0x1880C16F, 0x0B225918, 0x714F905D, 0x62ED082A, 0x560AA0B3, 0x45A838C4,
	0xA2D13239, 0xB173AA4E, 0x859402D7, 0x96369AA0, 0xEC5B53E5, 0xFFF9CB92, 0xCB1E630B, 0xD8BCFB7C,
	0x7F8BE302, 0x6C297B75, 0x58CED3EC, 0x4B6C4B9B, 0x310182DE, 0x22A31AA9, 0x1644B230, 0x05E62A47,
	0xE29F20BA, 0xF13DB8CD, 0xC5DA1054, 0xD6788823, 0xAC154166, 0xBFB7D911, 0x8B507188, 0x98F2E9FF,
	0x404E1283, 0x53EC8AF4, 0x670B226D, 0x74A9BA1A, 0x0EC4735F, 0x1D66EB28, 0x298143B1, 0x3A23DBC6,
	0xDD5AD13B, 0xCEF8494C, 0xFA1FE1D5, 0xE9BD79A2, 0x93D0B0E7, 0x80722890, 0xB4958009, 0xA737187E,
	0xFF17C604, 0xECB55E73, 0xD852F6EA, 0xCBF06E9D, 0xB19DA7D8, 0xA23F3FAF, 0x96D89736, 0x857A0F41,
	0x620305BC, 0x71A19DCB, 0x45463552, 0x56E4AD25, 0x2C896460, 0x3F2BFC17, 0x0BCC548E, 0x186ECCF9,
	0xC0D23785, 0xD370AFF2, 0xE797076B, 0xF4359F1C, 0x8E585659, 0x9DFACE2E, 0xA91D66B7, 0xBABFFEC0,
	0x5DC6F43D, 0x4E646C4A, 0x7A83C4D3, 0x69215CA4, 0x134C95E1, 0x00EE0D96, 0x3409A50F, 0x27AB3D78,
	0x809C2506, 0x933EBD71, 0xA7D915E8, 0xB47B8D9F, 0xCE1644DA, 0xDDB4DCAD, 0xE9537434, 0xFAF1EC43,
	0x1D88E6BE, 0x0E2A7EC9, 0x3ACDD650, 0x296F4E27, 0x53028762, 0x40A01F15, 0x7447B78C, 0x67E52FFB,
	0xBF59D487, 0xACFB4CF0, 0x981CE469, 0x8BBE7C1E, 0xF1D3B55B, 0xE2712D2C, 0xD69685B5, 0xC5341DC2,
	0x224D173F, 0x31EF8F48, 0x050827D1, 0x16AABFA6, 0x6CC776E3, 0x7F65EE94, 0x4B82460D, 0x5820DE7A,
	0xFBC3FAF9, 0xE861628E, 0xDC86CA17, 0xCF245260, 0xB5499B25, 0xA6EB0352, 0x920CABCB, 0x81AE33BC,
	0x66D73941, 0x7575A136, 0x419209AF, 0x523091D8, 0x285D589D, 0x3BFFC0EA, 0x0F186873, 0x1CBAF004,
	0xC4060B78, 0xD7A4930F, 0xE3433B96, 0xF0E1A3E1, 0x8A8C6AA4, 0x992EF2D3, 0xADC95A4A, 0xBE6BC23D,
	0x5912C8C0, 0x4AB050B7, 0x7E57F82E, 0x6DF56059, 0x1798A91C, 0x043A316B, 0x30DD99F2, 0x237F0185,
	0x844819FB, 0x97EA818C, 0xA30D2915, 0xB0AFB162, 0xCAC27827, 0xD960E050, 0xED8748C9, 0xFE25D0BE,
	0x195CDA43, 0x0AFE4234, 0x3E19EAAD, 0x2DBB72DA, 0x57D6BB9F, 0x447423E8, 0x70938B71, 0x63311306,
	0xBB8DE87A, 0xA82F700D, 0x9CC8D894, 0x8F6A40E3, 0xF50789A6, 0xE6A511D1, 0xD242B948, 0xC1E0213F,
	0x26992BC2, 0x353BB3B5, 0x01DC1B2C, 0x127E835B, 0x68134A1E, 0x7BB1D269, 0x4F567AF0, 0x5CF4E287,
	0x04D43CFD, 0x1776A48A, 0x23910C13, 0x30339464, 0x4A5E5D21, 0x59FCC556, 0x6D1B6DCF, 0x7EB9F5B8,
	0x99C0FF45, 0x8A626732, 0xBE85CFAB, 0xAD2757DC, 0xD74A9E99, 0xC4E806EE, 0xF00FAE77, 0xE3AD3600,
	0x3B11CD7C, 0x28B3550B, 0x1C54FD92, 0x0FF665E5, 0x759BACA0, 0x663934D7, 0x52DE9C4E, 0x417C0439,
	0xA6050EC4, 0xB5A796B3, 0x81403E2A, 0x92E2A65D, 0xE88F6F18, 0xFB2DF76F, 0xCFCA5FF6, 0xDC68C781,
	0x7B5FDFFF, 0x68FD4788, 0x5C1AEF11, 0x4FB87766, 0x35D5BE23, 0x26772654, 0x12908ECD, 0x013216BA,
	0xE64B1C47, 0xF5E98430, 0xC10E2CA9, 0xD2ACB4DE, 0xA8C17D9B, 0xBB63E5EC, 0x8F844D75, 0x9C26D502,
	0x449A2E7E, 0x5738B609, 0x63DF1E90, 0x707D86E7, 0x0A104FA2, 0x19B2D7D5, 0x2D557F4C, 0x3EF7E73B,
	0xD98EEDC6, 0xCA2C75B1, 0xFECBDD28, 0xED69455F, 0x97048C1A, 0x84A6146D, 0xB041BCF4, 0xA3E32483
};

static const uint32_t crc_tableil8_o48[256] =
{
	0x00000000, 0xA541927E, 0x4F6F520D, 0xEA2EC073, 0x9EDEA41A, 0x3B9F3664, 0xD1B1F617, 0x74F06469,
	0x38513EC5, 0x9D10ACBB, 0x773E6CC8, 0xD27FFEB6, 0xA68F9ADF, 0x03CE08A1, 0xE9E0C8D2, 0x4CA15AAC,
	0x70A27D8A, 0xD5E3EFF4, 0x3FCD2F87, 0x9A8CBDF9, 0xEE7CD990, 0x4B3D4BEE, 0xA1138B9D, 0x045219E3,
	0x48F3434F, 0xEDB2D131, 0x079C1142, 0xA2DD833C, 0xD62DE755, 0x736C752B, 0x9942B558, 0x3C032726,
	0xE144FB14, 0x4405696A, 0xAE2BA919, 0x0B6A3B67, 0x7F9A5F0E, 0xDADBCD70, 0x30F50D03, 0x95B49F7D,
	0xD915C5D1, 0x7C5457AF, 0x967A97DC, 0x333B05A2, 0x47CB61CB, 0xE28AF3B5, 0x08A433C6, 0xADE5A1B8,
	0x91E6869E, 0x34A714E0, 0xDE89D493, 0x7BC846ED, 0x0F382284, 0xAA79B0FA, 0x40577089, 0xE516E2F7,
	0xA9B7B85B, 0x0CF62A25, 0xE6D8EA56, 0x43997828, 0x37691C41, 0x92288E3F, 0x78064E4C, 0xDD47DC32,
	0xC76580D9, 0x622412A7, 0x880AD2D4, 0x2D4B40AA, 0x59BB24C3, 0xFCFAB6BD, 0x16D476CE, 0xB395E4B0,
	0xFF34BE1C, 0x5A752C62, 0xB05BEC11, 0x151A7E6F, 0x61EA1A06, 0xC4AB8878, 0x2E85480B, 0x8BC4DA75,
	0xB7C7FD53, 0x12866F2D, 0xF8A8AF5E, 0x5DE93D20, 0x29195949, 0x8C58CB37, 0x66760B44, 0xC337993A,
	0x8F96C396, 0x2AD751E8, 0xC0F9919B, 0x65B803E5, 0x1148678C, 0xB409F5F2, 0x5E273581, 0xFB66A7FF,
	0x26217BCD, 0x8360E9B3, 0x694E29C0, 0xCC0FBBBE, 0xB8FFDFD7, 0x1DBE4DA9, 0xF7908DDA, 0x52D11FA4,
	0x1E704508, 0xBB31D776, 0x511F1705, 0xF45E857B, 0x80AEE112, 0x25EF736C, 0xCFC1B31F, 0x6A802161,
	0x56830647, 0xF3C29439, 0x19EC544A, 0xBCADC634, 0xC85DA25D, 0x6D1C3023, 0x8732F050, 0x2273622E,
	0x6ED23882, 0xCB93AAFC, 0x21BD6A8F, 0x84FCF8F1, 0xF00C9C98, 0x554D0EE6, 0xBF63CE95, 0x1A225CEB,
	0x8B277743, 0x2E66E53D, 0xC448254E, 0x6109B730, 0x15F9D359, 0xB0B84127, 0x5A968154, 0xFFD7132A,
	0xB3764986, 0x1637DBF8, 0xFC191B8B, 0x595889F5, 0x2DA8ED9C, 0x88E97FE2, 0x62C7BF91, 0xC7862DEF,
	0xFB850AC9, 0x5EC498B7, 0xB4EA58C4, 0x11ABCABA, 0x655BAED3, 0xC01A3CAD, 0x2A34FCDE, 0x8F756EA0,
	0xC3D4340C, 0x6695A672, 0x8CBB6601, 0x29FAF47F, 0x5D0A9016, 0xF84B0268, 0x1265C21B, 0xB7245065,
	0x6A638C57, 0xCF221E29, 0x250CDE5A, 0x804D4C24, 0xF4BD284D, 0x51FCBA33, 0xBBD27A40, 0x1E93E83E,
	0x5232B292, 0xF77320EC, 0x1D5DE09F, 0xB81C72E1, 0xCCEC1688, 0x69AD84F6, 0x83834485, 0x26C2D6FB,
	0x1AC1F1DD, 0xBF8063A3, 0x55AEA3D0, 0xF0EF31AE, 0x841F55C7, 0x215EC7B9, 0xCB7007CA, 0x6E3195B4,
	0x2290CF18, 0x87D15D66, 0x6DFF9D15, 0xC8BE0F6B, 0xBC4E6B02, 0x190FF97C, 0xF321390F, 0x5660AB71,
	0x4C42F79A, 0xE90365E4, 0x032DA597, 0xA66C37E9, 0xD29C5380, 0x77DDC1FE, 0x9DF3018D, 0x38B293F3,
	0x7413C95F, 0xD1525B21, 0x3B7C9B52, 0x9E3D092C, 0xEACD6D45, 0x4F8CFF3B, 0xA5A23F48, 0x00E3AD36,
	0x3CE08A10, 0x99A1186E, 0x738FD81D, 0xD6CE4A63, 0xA23E2E0A, 0x077FBC74, 0xED517C07, 0x4810EE79,
	0x04B1B4D5, 0xA1F026AB, 0x4BDEE6D8, 0xEE9F74A6, 0x9A6F10CF, 0x3F2E82B1, 0xD50042C2, 0x7041D0BC,
	0xAD060C8E, 0x08479EF0, 0xE2695E83, 0x4728CCFD, 0x33D8A894, 0x96993AEA, 0x7CB7FA99, 0xD9F668E7,
	0x9557324B, 0x3016A035, 0xDA386046, 0x7F79F238, 0x0B899651, 0xAEC8042F, 0x44E6C45C, 0xE1A75622,
	0xDDA47104, 0x78E5E37A, 0x92CB2309, 0x378AB177, 0x437AD51E, 0xE63B4760, 0x0C158713, 0xA954156D,
	0xE5F54FC1, 0x40B4DDBF, 0xAA9A1DCC, 0x0FDB8FB2, 0x7B2BEBDB, 0xDE6A79A5, 0x3444B9D6, 0x91052BA8
};

static const uint32_t crc_tableil8_o56[256] =
{
	0x00000000, 0xDD45AAB8, 0xBF672381, 0x62228939, 0x7B2231F3, 0xA6679B4B, 0xC4451272, 0x1900B8CA,
	0xF64463E6, 0x2B01C95E, 0x49234067, 0x9466EADF, 0x8D665215, 0x5023F8AD, 0x32017194, 0xEF44DB2C,
	0xE964B13D, 0x34211B85, 0x560392BC, 0x8B463804, 0x924680CE, 0x4F032A76, 0x2D21A34F, 0xF06409F7,
	0x1F20D2DB, 0xC2657863, 0xA047F15A, 0x7D025BE2, 0x6402E328, 0xB9474990, 0xDB65C0A9, 0x06206A11,
	0xD725148B, 0x0A60BE33, 0x6842370A, 0xB5079DB2, 0xAC072578, 0x71428FC0, 0x136006F9, 0xCE25AC41,
	0x2161776D, 0xFC24DDD5, 0x9E0654EC, 0x4343FE54, 0x5A43469E, 0x8706EC26, 0xE524651F, 0x3861CFA7,
	0x3E41A5B6, 0xE3040F0E, 0x81268637, 0x5C632C8F, 0x45639445, 0x98263EFD, 0xFA04B7C4, 0x27411D7C,
	0xC805C650, 0x15406CE8, 0x7762E5D1, 0xAA274F69, 0xB327F7A3, 0x6E625D1B, 0x0C40D422, 0xD1057E9A,
	0xABA65FE7, 0x76E3F55F, 0x14C17C66, 0xC984D6DE, 0xD0846E14, 0x0DC1C4AC, 0x6FE34D95, 0xB2A6E72D,
	0x5DE23C01, 0x80A796B9, 0xE2851F80, 0x3FC0B538, 0x26C00DF2, 0xFB85A74A, 0x99A72E73, 0x44E284CB,
	0x42C2EEDA, 0x9F874462, 0xFDA5CD5B, 0x20E067E3, 0x39E0DF29, 0xE4A57591, 0x8687FCA8, 0x5BC25610,
	0xB4868D3C, 0x69C32784, 0x0BE1AEBD, 0xD6A40405, 0xCFA4BCCF, 0x12E11677, 0x70C39F4E, 0xAD8635F6,
	0x7C834B6C, 0xA1C6E1D4, 0xC3E468ED, 0x1EA1C255, 0x07A17A9F, 0xDAE4D027, 0xB8C6591E, 0x6583F3A6,
	0x8AC7288A, 0x57828232, 0x35A00B0B, 0xE8E5A1B3, 0xF1E51979, 0x2CA0B3C1, 0x4E823AF8, 0x93C79040,
	0x95E7FA51, 0x48A250E9, 0x2A80D9D0, 0xF7C57368, 0xEEC5CBA2, 0x3380611A, 0x51A2E823, 0x8CE7429B,
	0x63A399B7, 0xBEE6330F, 0xDCC4BA36, 0x0181108E, 0x1881A844, 0xC5C402FC, 0xA7E68BC5, 0x7AA3217D,
	0x52A0C93F, 0x8FE56387, 0xEDC7EABE, 0x30824006, 0x2982F8CC, 0xF4C75274, 0x96E5DB4D, 0x4BA071F5,
	0xA4E4AAD9, 0x79A10061, 0x1B838958, 0xC6C623E0, 0xDFC69B2A, 0x02833192, 0x60A1B8AB, 0xBDE41213,
	0xBBC47802, 0x6681D2BA, 0x04A35B83, 0xD9E6F13B, 0xC0E649F1, 0x1DA3E349, 0x7F816A70, 0xA2C4C0C8,
	0x4D801BE4, 0x90C5B15C, 0xF2E73865, 0x2FA292DD, 0x36A22A17, 0xEBE780AF, 0x89C50996, 0x5480A32E,
	0x8585DDB4, 0x58C0770C, 0x3AE2FE35, 0xE7A7548D, 0xFEA7EC47, 0x23E246FF, 0x41C0CFC6, 0x9C85657E,
	0x73C1BE52, 0xAE8414EA, 0xCCA69DD3, 0x11E3376B, 0x08E38FA1, 0xD5A62519, 0xB784AC20, 0x6AC10698,
	0x6CE16C89, 0xB1A4C631, 0xD3864F08, 0x0EC3E5B0, 0x17C35D7A, 0xCA86F7C2, 0xA8A47EFB, 0x75E1D443,
	0x9AA50F6F, 0x47E0A5D7, 0x25C22CEE, 0xF8878656, 0xE1873E9C, 0x3CC29424, 0x5EE01D1D, 0x83A5B7A5,
	0xF90696D8, 0x24433C60, 0x4661B559, 0x9B241FE1, 0x8224A72B, 0x5F610D93, 0x3D4384AA, 0xE0062E12,
	0x0F42F53E, 0xD2075F86, 0xB025D6BF, 0x6D607C07, 0x7460C4CD, 0xA9256E75, 0xCB07E74C, 0x16424DF4,
	0x106227E5, 0xCD278D5D, 0xAF050464, 0x7240AEDC, 0x6B401616, 0xB605BCAE, 0xD4273597, 0x09629F2F,
	0xE6264403, 0x3B63EEBB, 0x59416782, 0x8404CD3A, 0x9D0475F0, 0x4041DF48, 0x22635671, 0xFF26FCC9,
	0x2E238253, 0xF36628EB, 0x9144A1D2, 0x4C010B6A, 0x5501B3A0, 0x88441918, 0xEA669021, 0x37233A99,
	0xD867E1B5, 0x05224B0D, 0x6700C234, 0xBA45688C, 0xA345D046, 0x7E007AFE, 0x1C22F3C7, 0xC167597F,
	0xC747336E, 0x1A0299D6, 0x782010EF, 0xA565BA57, 0xBC65029D, 0x6120A825, 0x0302211C, 0xDE478BA4,
	0x31035088, 0xEC46FA30, 0x8E647309, 0x5321D9B1, 0x4A21617B, 0x9764CBC3, 0xF54642FA, 0x2803E842
};

static const uint32_t crc_tableil8_o64[256] =
{
	0x00000000, 0x38116FAC, 0x7022DF58, 0x4833B0F4, 0xE045BEB0, 0xD854D11C, 0x906761E8, 0xA8760E44,
	0xC5670B91, 0xFD76643D, 0xB545D4C9, 0x8D54BB65, 0x2522B521, 0x1D33DA8D, 0x55006A79, 0x6D1105D5,
	0x8F2261D3, 0xB7330E7F, 0xFF00BE8B, 0xC711D127, 0x6F67DF63, 0x5776B0CF, 0x1F45003B, 0x27546F97,
	0x4A456A42, 0x725405EE, 0x3A67B51A, 0x0276DAB6, 0xAA00D4F2, 0x9211BB5E, 0xDA220BAA, 0xE2336406,
	0x1BA8B557, 0x23B9DAFB, 0x6B8A6A0F, 0x539B05A3, 0xFBED0BE7, 0xC3FC644B, 0x8BCFD4BF, 0xB3DEBB13,
	0xDECFBEC6, 0xE6DED16A, 0xAEED619E, 0x96FC0E32, 0x3E8A0076, 0x069B6FDA, 0x4EA8DF2E, 0x76B9B082,
	0x948AD484, 0xAC9BBB28, 0xE4A80BDC, 0xDCB96470, 0x74CF6A34, 0x4CDE0598, 0x04EDB56C, 0x3CFCDAC0,
	0x51EDDF15, 0x69FCB0B9, 0x21CF004D, 0x19DE6FE1, 0xB1A861A5, 0x89B90E09, 0xC18ABEFD, 0xF99BD151,
	0x37516AAE, 0x0F400502, 0x4773B5F6, 0x7F62DA5A, 0xD714D41E, 0xEF05BBB2, 0xA7360B46, 0x9F2764EA,
	0xF236613F, 0xCA270E93, 0x8214BE67, 0xBA05D1CB, 0x1273DF8F, 0x2A62B023, 0x625100D7, 0x5A406F7B,
	0xB8730B7D, 0x806264D1, 0xC851D425, 0xF040BB89, 0x5836B5CD, 0x6027DA61, 0x28146A95, 0x10050539,
	0x7D1400EC, 0x45056F40, 0x0D36DFB4, 0x3527B018, 0x9D51BE5C, 0xA540D1F0, 0xED736104, 0xD5620EA8,
	0x2CF9DFF9, 0x14E8B055, 0x5CDB00A1, 0x64CA6F0D, 0xCCBC6149, 0xF4AD0EE5, 0xBC9EBE11, 0x848FD1BD,
	0xE99ED468, 0xD18FBBC4, 0x99BC0B30, 0xA1AD649C, 0x09DB6AD8, 0x31CA0574, 0x79F9B580, 0x41E8DA2C,
	0xA3DBBE2A, 0x9BCAD186, 0xD3F96172, 0xEBE80EDE, 0x439E009A, 0x7B8F6F36, 0x33BCDFC2, 0x0BADB06E,
	0x66BCB5BB, 0x5EADDA17, 0x169E6AE3, 0x2E8F054F, 0x86F90B0B, 0xBEE864A7, 0xF6DBD453, 0xCECABBFF,
	0x6EA2D55C, 0x56B3BAF0, 0x1E800A04, 0x269165A8, 0x8EE76BEC, 0xB6F60440, 0xFEC5B4B4, 0xC6D4DB18,
	0xABC5DECD, 0x93D4B161, 0xDBE70195, 0xE3F66E39, 0x4B80607D, 0x73910FD1, 0x3BA2BF25, 0x03B3D089,
	0xE180B48F, 0xD991DB23, 0x91A26BD7, 0xA9B3047B, 0x01C50A3F, 0x39D46593, 0x71E7D567, 0x49F6BACB,
	0x24E7BF1E, 0x1CF6D0B2, 0x54C56046, 0x6CD40FEA, 0xC4A201AE, 0xFCB36E02, 0xB480DEF6, 0x8C91B15A,
	0x750A600B, 0x4D1B0FA7, 0x0528BF53, 0x3D39D0FF, 0x954FDEBB, 0xAD5EB117, 0xE56D01E3, 0xDD7C6E4F,
	0xB06D6B9A, 0x887C0436, 0xC04FB4C2, 0xF85EDB6E, 0x5028D52A, 0x6839BA86, 0x200A0A72, 0x181B65DE,
	0xFA2801D8, 0xC2396E74, 0x8A0ADE80, 0xB21BB12C, 0x1A6DBF68, 0x227CD0C4, 0x6A4F6030, 0x525E0F9C,
	0x3F4F0A49, 0x075E65E5, 0x4F6DD511, 0x777CBABD, 0xDF0AB4F9, 0xE71BDB55, 0xAF286BA1, 0x9739040D,
	0x59F3BFF2, 0x61E2D05E, 0x29D160AA, 0x11C00F06, 0xB9B60142, 0x81A76EEE, 0xC994DE1A, 0xF185B1B6,
	0x9C94B463, 0xA485DBCF, 0xECB66B3B, 0xD4A70497, 0x7CD10AD3, 0x44C0657F, 0x0CF3D58B, 0x34E2BA27,
	0xD6D1DE21, 0xEEC0B18D, 0xA6F30179, 0x9EE26ED5, 0x36946091, 0x0E850F3D, 0x46B6BFC9, 0x7EA7D065,
	0x13B6D5B0, 0x2BA7BA1C, 0x63940AE8, 0x5B856544, 0xF3F36B00, 0xCBE204AC, 0x83D1B458, 0xBBC0DBF4,
	0x425B0AA5, 0x7A4A6509, 0x3279D5FD, 0x0A68BA51, 0xA21EB415, 0x9A0FDBB9, 0xD23C6B4D, 0xEA2D04E1,
	0x873C0134, 0xBF2D6E98, 0xF71EDE6C, 0xCF0FB1C0, 0x6779BF84, 0x5F68D028, 0x175B60DC, 0x2F4A0F70,
	0xCD796B76, 0xF56804DA, 0xBD5BB42E, 0x854ADB82, 0x2D3CD5C6, 0x152DBA6A, 0x5D1E0A9E, 0x650F6532,
	0x081E60E7, 0x300F0F4B, 0x783CBFBF, 0x402DD013, 0xE85BDE57, 0xD04AB1FB, 0x9879010F, 0xA0686EA3
};

static const uint32_t crc_tableil8_o72[256] =
{
	0x00000000, 0xEF306B19, 0xDB8CA0C3, 0x34BCCBDA, 0xB2F53777, 0x5DC55C6E, 0x697997B4, 0x8649FCAD,
	0x6006181F, 0x8F367306, 0xBB8AB8DC, 0x54BAD3C5, 0xD2F32F68, 0x3DC34471, 0x097F8FAB, 0xE64FE4B2,
	0xC00C303E, 0x2F3C5B27, 0x1B8090FD, 0xF4B0FBE4, 0x72F90749, 0x9DC96C50, 0xA975A78A, 0x4645CC93,
	0xA00A2821, 0x4F3A4338, 0x7B8688E2, 0x94B6E3FB, 0x12FF1F56, 0xFDCF744F, 0xC973BF95, 0x2643D48C,
	0x85F4168D, 0x6AC47D94, 0x5E78B64E, 0xB148DD57, 0x370121FA, 0xD8314AE3, 0xEC8D8139, 0x03BDEA20,
	0xE5F20E92, 0x0AC2658B, 0x3E7EAE51, 0xD14EC548, 0x570739E5, 0xB83752FC, 0x8C8B9926, 0x63BBF23F,
	0x45F826B3, 0xAAC84DAA, 0x9E748670, 0x7144ED69, 0xF70D11C4, 0x183D7ADD, 0x2C81B107, 0xC3B1DA1E,
	0x25FE3EAC, 0xCACE55B5, 0xFE729E6F, 0x1142F576, 0x970B09DB, 0x783B62C2, 0x4C87A918, 0xA3B7C201,
	0x0E045BEB, 0xE13430F2, 0xD588FB28, 0x3AB89031, 0xBCF16C9C, 0x53C10785, 0x677DCC5F, 0x884DA746,
	0x6E0243F4, 0x813228ED, 0xB58EE337, 0x5ABE882E, 0xDCF77483, 0x33C71F9A, 0x077BD440, 0xE84BBF59,
	0xCE086BD5, 0x213800CC, 0x1584CB16, 0xFAB4A00F, 0x7CFD5CA2, 0x93CD37BB, 0xA771FC61, 0x48419778,
	0xAE0E73CA, 0x413E18D3, 0x7582D309, 0x9AB2B810, 0x1CFB44BD, 0xF3CB2FA4, 0xC777E47E, 0x28478F67,
	0x8BF04D66, 0x64C0267F, 0x507CEDA5, 0xBF4C86BC, 0x39057A11, 0xD6351108, 0xE289DAD2, 0x0DB9B1CB,
	0xEBF65579, 0x04C63E60, 0x307AF5BA, 0xDF4A9EA3, 0x5903620E, 0xB6330917, 0x828FC2CD, 0x6DBFA9D4,
	0x4BFC7D58, 0xA4CC1641, 0x9070DD9B, 0x7F40B682, 0xF9094A2F, 0x16392136, 0x2285EAEC, 0xCDB581F5,
	0x2BFA6547, 0xC4CA0E5E, 0xF076C584, 0x1F46AE9D, 0x990F5230, 0x763F3929, 0x4283F2F3, 0xADB399EA,
	0x1C08B7D6, 0xF338DCCF, 0xC7841715, 0x28B47C0C, 0xAEFD80A1, 0x41CDEBB8, 0x75712062, 0x9A414B7B,
	0x7C0EAFC9, 0x933EC4D0, 0xA7820F0A, 0x48B26413, 0xCEFB98BE, 0x21CBF3A7, 0x1577387D, 0xFA475364,
	0xDC0487E8, 0x3334ECF1, 0x0788272B, 0xE8B84C32, 0x6EF1B09F, 0x81C1DB86, 0xB57D105C, 0x5A4D7B45,
	0xBC029FF7, 0x5332F4EE, 0x678E3F34, 0x88BE542D, 0x0EF7A880, 0xE1C7C399, 0xD57B0843, 0x3A4B635A,
	0x99FCA15B, 0x76CCCA42, 0x42700198, 0xAD406A81, 0x2B09962C, 0xC439FD35, 0xF08536EF, 0x1FB55DF6,
	0xF9FAB944, 0x16CAD25D, 0x22761987, 0xCD46729E, 0x4B0F8E33, 0xA43FE52A, 0x90832EF0, 0x7FB345E9,
	0x59F09165, 0xB6C0FA7C, 0x827C31A6, 0x6D4C5ABF, 0xEB05A612, 0x0435CD0B, 0x308906D1, 0xDFB96DC8,
	0x39F6897A, 0xD6C6E263, 0xE27A29B9, 0x0D4A42A0, 0x8B03BE0D, 0x6433D514, 0x508F1ECE, 0xBFBF75D7,
	0x120CEC3D, 0xFD3C8724, 0xC9804CFE, 0x26B027E7, 0xA0F9DB4A, 0x4FC9B053, 0x7B757B89, 0x94451090,
	0x720AF422, 0x9D3A9F3B, 0xA98654E1, 0x46B63FF8, 0xC0FFC355, 0x2FCFA84C, 0x1B736396, 0xF443088F,
	0xD200DC03, 0x3D30B71A, 0x098C7CC0, 0xE6BC17D9, 0x60F5EB74, 0x8FC5806D, 0xBB794BB7, 0x544920AE,
	0xB206C41C, 0x5D36AF05, 0x698A64DF, 0x86BA0FC6, 0x00F3F36B, 0xEFC39872, 0xDB7F53A8, 0x344F38B1,
	0x97F8FAB0, 0x78C891A9, 0x4C745A73, 0xA344316A, 0x250DCDC7, 0xCA3DA6DE, 0xFE816D04, 0x11B1061D,
	0xF7FEE2AF, 0x18CE89B6, 0x2C72426C, 0xC3422975, 0x450BD5D8, 0xAA3BBEC1, 0x9E87751B, 0x71B71E02,
	0x57F4CA8E, 0xB8C4A197, 0x8C786A4D, 0x63480154, 0xE501FDF9, 0x0A3196E0, 0x3E8D5D3A, 0xD1BD3623,
	0x37F2D291, 0xD8C2B988, 0xEC7E7252, 0x034E194B, 0x8507E5E6, 0x6A378EFF, 0x5E8B4525, 0xB1BB2E3C
};

static const uint32_t crc_tableil8_o80[256] =
{
	0x00000000, 0x68032CC8, 0xD0065990, 0xB8057558, 0xA5E0C5D1, 0xCDE3E919, 0x75E69C41, 0x1DE5B089,
	0x4E2DFD53, 0x262ED19B, 0x9E2BA4C3, 0xF628880B, 0xEBCD3882, 0x83CE144A, 0x3BCB6112, 0x53C84DDA,
	0x9C5BFAA6, 0xF458D66E, 0x4C5DA336, 0x245E8FFE, 0x39BB3F77, 0x51B813BF, 0xE9BD66E7, 0x81BE4A2F,
	0xD27607F5, 0xBA752B3D, 0x02705E65, 0x6A7372AD, 0x7796C224, 0x1F95EEEC, 0xA7909BB4, 0xCF93B77C,
	0x3D5B83BD, 0x5558AF75, 0xED5DDA2D, 0x855EF6E5, 0x98BB466C, 0xF0B86AA4, 0x48BD1FFC, 0x20BE3334,
	0x73767EEE, 0x1B755226, 0xA370277E, 0xCB730BB6, 0xD696BB3F, 0xBE9597F7, 0x0690E2AF, 0x6E93CE67,
	0xA100791B, 0xC90355D3, 0x7106208B, 0x19050C43, 0x04E0BCCA, 0x6CE39002, 0xD4E6E55A, 0xBCE5C992,
	0xEF2D8448, 0x872EA880, 0x3F2BDDD8, 0x5728F110, 0x4ACD4199, 0x22CE6D51, 0x9ACB1809, 0xF2C834C1,
	0x7AB7077A, 0x12B42BB2, 0xAAB15EEA, 0xC2B27222, 0xDF57C2AB, 0xB754EE63, 0x0F519B3B, 0x6752B7F3,
	0x349AFA29, 0x5C99D6E1, 0xE49CA3B9, 0x8C9F8F71, 0x917A3FF8, 0xF9791330, 0x417C6668, 0x297F4AA0,
	0xE6ECFDDC, 0x8EEFD114, 0x36EAA44C, 0x5EE98884, 0x430C380D, 0x2B0F14C5, 0x930A619D, 0xFB094D55,
	0xA8C1008F, 0xC0C22C47, 0x78C7591F, 0x10C475D7, 0x0D21C55E, 0x6522E996, 0xDD279CCE, 0xB524B006,
	0x47EC84C7, 0x2FEFA80F, 0x97EADD57, 0xFFE9F19F, 0xE20C4116, 0x8A0F6DDE, 0x320A1886, 0x5A09344E,
	0x09C17994, 0x61C2555C, 0xD9C72004, 0xB1C40CCC, 0xAC21BC45, 0xC422908D, 0x7C27E5D5, 0x1424C91D,
	0xDBB77E61, 0xB3B452A9, 0x0BB127F1, 0x63B20B39, 0x7E57BBB0, 0x16549778, 0xAE51E220, 0xC652CEE8,
	0x959A8332, 0xFD99AFFA, 0x459CDAA2, 0x2D9FF66A, 0x307A46E3, 0x58796A2B, 0xE07C1F73, 0x887F33BB,
	0xF56E0EF4, 0x9D6D223C, 0x25685764, 0x4D6B7BAC, 0x508ECB25, 0x388DE7ED, 0x808892B5, 0xE88BBE7D,
	0xBB43F3A7, 0xD340DF6F, 0x6B45AA37, 0x034686FF, 0x1EA33676, 0x76A01ABE, 0xCEA56FE6, 0xA6A6432E,
	0x6935F452, 0x0136D89A, 0xB933ADC2, 0xD130810A, 0xCCD53183, 0xA4D61D4B, 0x1CD36813, 0x74D044DB,
	0x27180901, 0x4F1B25C9, 0xF71E5091, 0x9F1D7C59, 0x82F8CCD0, 0xEAFBE018, 0x52FE9540, 0x3AFDB988,
	0xC8358D49, 0xA036A181, 0x1833D4D9, 0x7030F811, 0x6DD54898, 0x05D66450, 0xBDD31108, 0xD5D03DC0,
	0x8618701A, 0xEE1B5CD2, 0x561E298A, 0x3E1D0542, 0x23F8B5CB, 0x4BFB9903, 0xF3FEEC5B, 0x9BFDC093,
	0x546E77EF, 0x3C6D5B27, 0x84682E7F, 0xEC6B02B7, 0xF18EB23E, 0x998D9EF6, 0x2188EBAE, 0x498BC766,
	0x1A438ABC, 0x7240A674, 0xCA45D32C, 0xA246FFE4, 0xBFA34F6D, 0xD7A063A5, 0x6FA516FD, 0x07A63A35,
	0x8FD9098E, 0xE7DA2546, 0x5FDF501E, 0x37DC7CD6, 0x2A39CC5F, 0x423AE097, 0xFA3F95CF, 0x923CB907,
	0xC1F4F4DD, 0xA9F7D815, 0x11F2AD4D, 0x79F18185, 0x6414310C, 0x0C171DC4, 0xB412689C, 0xDC114454,
	0x1382F328, 0x7B81DFE0, 0xC384AAB8, 0xAB878670, 0xB66236F9, 0xDE611A31, 0x66646F69, 0x0E6743A1,
	0x5DAF0E7B, 0x35AC22B3, 0x8DA957EB, 0xE5AA7B23, 0xF84FCBAA, 0x904CE762, 0x2849923A, 0x404ABEF2,
	0xB2828A33, 0xDA81A6FB, 0x6284D3A3, 0x0A87FF6B, 0x17624FE2, 0x7F61632A, 0xC7641672, 0xAF673ABA,
	0xFCAF7760, 0x94AC5BA8, 0x2CA92EF0, 0x44AA0238, 0x594FB2B1, 0x314C9E79, 0x8949EB21, 0xE14AC7E9,
	0x2ED97095, 0x46DA5C5D, 0xFEDF2905, 0x96DC05CD, 0x8B39B544, 0xE33A998C, 0x5B3FECD4, 0x333CC01C,
	0x60F48DC6, 0x08F7A10E, 0xB0F2D456, 0xD8F1F89E, 0xC5144817, 0xAD1764DF, 0x15121187, 0x7D113D4F
};

static const uint32_t crc_tableil8_o88[256] =
{
	0x00000000, 0x493C7D27, 0x9278FA4E, 0xDB448769, 0x211D826D, 0x6821FF4A, 0xB3657823, 0xFA590504,
	0x423B04DA, 0x0B0779FD, 0xD043FE94, 0x997F83B3, 0x632686B7, 0x2A1AFB90, 0xF15E7CF9, 0xB86201DE,
	0x847609B4, 0xCD4A7493, 0x160EF3FA, 0x5F328EDD, 0xA56B8BD9, 0xEC57F6FE, 0x37137197, 0x7E2F0CB0,
	0xC64D0D6E, 0x8F717049, 0x5435F720, 0x1D098A07, 0xE7508F03, 0xAE6CF224, 0x7528754D, 0x3C14086A,
	0x0D006599, 0x443C18BE, 0x9F789FD7, 0xD644E2F0, 0x2C1DE7F4, 0x65219AD3, 0xBE651DBA, 0xF759609D,
	0x4F3B6143, 0x06071C64, 0xDD439B0D, 0x947FE62A, 0x6E26E32E, 0x271A9E09, 0xFC5E1960, 0xB5626447,
	0x89766C2D, 0xC04A110A, 0x1B0E9663, 0x5232EB44, 0xA86BEE40, 0xE1579367, 0x3A13140E, 0x732F6929,
	0xCB4D68F7, 0x827115D0, 0x593592B9, 0x1009EF9E, 0xEA50EA9A, 0xA36C97BD, 0x782810D4, 0x31146DF3,
	0x1A00CB32, 0x533CB615, 0x8878317C, 0xC1444C5B, 0x3B1D495F, 0x72213478, 0xA965B311, 0xE059CE36,
	0x583BCFE8, 0x1107B2CF, 0xCA4335A6, 0x837F4881, 0x79264D85, 0x301A30A2, 0xEB5EB7CB, 0xA262CAEC,
	0x9E76C286, 0xD74ABFA1, 0x0C0E38C8, 0x453245EF, 0xBF6B40EB, 0xF6573DCC, 0x2D13BAA5, 0x642FC782,
	0xDC4DC65C, 0x9571BB7B, 0x4E353C12, 0x07094135, 0xFD504431, 0xB46C3916, 0x6F28BE7F, 0x2614C358,
	0x1700AEAB, 0x5E3CD38C, 0x857854E5, 0xCC4429C2, 0x361D2CC6, 0x7F2151E1, 0xA465D688, 0xED59ABAF,
	0x553BAA71, 0x1C07D756, 0xC743503F, 0x8E7F2D18, 0x7426281C, 0x3D1A553B, 0xE65ED252, 0xAF62AF75,
	0x9376A71F, 0xDA4ADA38, 0x010E5D51, 0x48322076, 0xB26B2572, 0xFB575855, 0x2013DF3C, 0x692FA21B,
	0xD14DA3C5, 0x9871DEE2, 0x4335598B, 0x0A0924AC, 0xF05021A8, 0xB96C5C8F, 0x6228DBE6, 0x2B14A6C1,
	0x34019664, 0x7D3DEB43, 0xA6796C2A, 0xEF45110D, 0x151C1409, 0x5C20692E, 0x8764EE47, 0xCE589360,
	0x763A92BE, 0x3F06EF99, 0xE44268F0, 0xAD7E15D7, 0x572710D3, 0x1E1B6DF4, 0xC55FEA9D, 0x8C6397BA,
	0xB0779FD0, 0xF94BE2F7, 0x220F659E, 0x6B3318B9, 0x916A1DBD, 0xD856609A, 0x0312E7F3, 0x4A2E9AD4,
	0xF24C9B0A, 0xBB70E62D, 0x60346144, 0x29081C63, 0xD3511967, 0x9A6D6440, 0x4129E329, 0x08159E0E,
	0x3901F3FD, 0x703D8EDA, 0xAB7909B3, 0xE2457494, 0x181C7190, 0x51200CB7, 0x8A648BDE, 0xC358F6F9,
	0x7B3AF727, 0x32068A00, 0xE9420D69, 0xA07E704E, 0x5A27754A, 0x131B086D, 0xC85F8F04, 0x8163F223,
	0xBD77FA49, 0xF44B876E, 0x2F0F0007, 0x66337D20, 0x9C6A7824, 0xD5560503, 0x0E12826A, 0x472EFF4D,
	0xFF4CFE93, 0xB67083B4, 0x6D3404DD, 0x240879FA, 0xDE517CFE, 0x976D01D9, 0x4C2986B0, 0x0515FB97,
	0x2E015D56, 0x673D2071, 0xBC79A718, 0xF545DA3F, 0x0F1CDF3B, 0x4620A21C, 0x9D642575, 0xD4585852,
	0x6C3A598C, 0x250624AB, 0xFE42A3C2, 0xB77EDEE5, 0x4D27DBE1, 0x041BA6C6, 0xDF5F21AF, 0x96635C88,
	0xAA7754E2, 0xE34B29C5, 0x380FAEAC, 0x7133D38B, 0x8B6AD68F, 0xC256ABA8, 0x19122CC1, 0x502E51E6,
	0xE84C5038, 0xA1702D1F, 0x7A34AA76, 0x3308D751, 0xC951D255, 0x806DAF72, 0x5B29281B, 0x1215553C,
	0x230138CF, 0x6A3D45E8, 0xB179C281, 0xF845BFA6, 0x021CBAA2, 0x4B20C785, 0x906440EC, 0xD9583DCB,
	0x613A3C15, 0x28064132, 0xF342C65B, 0xBA7EBB7C, 0x4027BE78, 0x091BC35F, 0xD25F4436, 0x9B633911,
	0xA777317B, 0xEE4B4C5C, 0x350FCB35, 0x7C33B612, 0x866AB316, 0xCF56CE31, 0x14124958, 0x5D2E347F,
	0xE54C35A1, 0xAC704886, 0x7734CFEF, 0x3E08B2C8, 0xC451B7CC, 0x8D6DCAEB, 0x56294D82, 0x1F1530A5
};

static uint32_t
ss_crc32c_sw(uint32_t crc, const void *buf, int len)
{
	const char *p_buf = (const char*)buf;

	int initial_bytes = (sizeof(int32_t) - (intptr_t)p_buf) & (sizeof(int32_t) - 1);
	if (len < initial_bytes)
		initial_bytes = len;
	int li;
	for (li = 0; li < initial_bytes; li++)
		crc = crc_tableil8_o32[(crc ^ *p_buf++) & 0x000000FF] ^ (crc >> 8);

	len -= initial_bytes;
	int running_len = len & ~(sizeof(uint64_t) - 1);
	int end_bytes = len - running_len; 

	for (li = 0; li < running_len / 8; li++) {
		crc ^= *(uint32_t*)p_buf;
		p_buf += 4;
		uint32_t term1 = crc_tableil8_o88[(crc) & 0x000000FF] ^
		                 crc_tableil8_o80[(crc >> 8) & 0x000000FF];
		uint32_t term2 = crc >> 16;
		crc = term1 ^
		      crc_tableil8_o72[term2 & 0x000000FF] ^ 
		      crc_tableil8_o64[(term2 >> 8) & 0x000000FF];
		term1 = crc_tableil8_o56[(*(uint32_t*)p_buf) & 0x000000FF] ^
		        crc_tableil8_o48[((*(uint32_t*)p_buf) >> 8) & 0x000000FF];
		term2 = (*(uint32_t*)p_buf) >> 16;
		crc = crc ^ term1 ^
		      crc_tableil8_o40[term2 & 0x000000FF] ^
		      crc_tableil8_o32[(term2 >> 8) & 0x000000FF];
		p_buf += 4;
	}

    for (li = 0; li < end_bytes; li++)
        crc = crc_tableil8_o32[(crc ^ *p_buf++) & 0x000000FF] ^ (crc >> 8);
    return crc;
}

#if defined (__x86_64__) || defined (__i386__)
/*
 * Using hardware provided CRC32 instruction to accelerate the CRC32 disposal.
 *
 * CRC32 is a new instruction in Intel SSE4.2, the reference can be found at:
 * http://www.intel.com/products/processor/manuals/
 * Intel(R) 64 and IA-32 Architectures Software Developer's Manual
 * Volume 2A: Instruction Set Reference, A-M
*/
#if defined (__x86_64__)
	#define REX_PRE "0x48, "
#elif defined (__i386__)
	#define REX_PRE
#endif

static uint32_t
ss_crc32c_hw_byte(uint32_t crc, unsigned char const *data, unsigned int length)
{
	while (length--) {
		__asm__ __volatile__(
			".byte 0xf2, 0xf, 0x38, 0xf0, 0xf1"
			:"=S"(crc)
			:"0"(crc), "c"(*data)
		);
		data++;
	}
	return crc;
}

static uint32_t
ss_crc32c_hw(uint32_t crc, const void *buf, int len)
{
	unsigned int iquotient = len / sizeof(unsigned long);
	unsigned int iremainder = len % sizeof(unsigned long);
	unsigned long *ptmp = (unsigned long *)buf;
	while (iquotient--) {
		__asm__ __volatile__(
			".byte 0xf2, " REX_PRE "0xf, 0x38, 0xf1, 0xf1;"
			:"=S"(crc)
			:"0"(crc), "c"(*ptmp)
		);
		ptmp++;
	}
	if (iremainder) {
		crc = ss_crc32c_hw_byte(crc, (unsigned char const*)ptmp, iremainder);
	}
	return crc;
}
#undef REX_PRE

static int
ss_crc32c_hw_enabled(void)
{
	unsigned int ax, bx, cx, dx;
	if (__get_cpuid(1, &ax, &bx, &cx, &dx) == 0)
		return 0;
	return (cx & (1 << 20)) != 0;
}
#endif

sscrcf ss_crc32c_function(void)
{
#if defined (__x86_64__) || defined (__i386__)
	if (ss_crc32c_hw_enabled())
		return ss_crc32c_hw;
#endif
	return ss_crc32c_sw;
}
#line 1 "sophia/std/ss_file.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



int ss_fileunlink(char *path)
{
	return unlink(path);
}

int ss_filemove(char *a, char *b)
{
	return rename(a, b);
}

int ss_fileexists(char *path)
{
	struct stat st;
	int rc = lstat(path, &st);
	return rc == 0;
}

ssize_t ss_filesize(char *path)
{
	struct stat st;
	int rc = lstat(path, &st);
	if (ssunlikely(rc == -1))
		return -1;
	return st.st_size;
}

int ss_filemkdir(char *path)
{
	return mkdir(path, 0750);
}

static inline int
ss_fileopenas(ssfile *f, char *path, int flags)
{
	f->creat = (flags & O_CREAT ? 1 : 0);
	f->fd = open(path, flags, 0644);
	if (ssunlikely(f->fd == -1))
		return -1;
	f->file = ss_strdup(f->a, path);
	if (ssunlikely(f->file == NULL))
		goto err;
	f->size = 0;
	if (f->creat)
		return 0;
	struct stat st;
	int rc = lstat(path, &st);
	if (ssunlikely(rc == -1))
		goto err;
	f->size = st.st_size;
	return 0;
err:
	if (f->file) {
		ss_free(f->a, f->file);
		f->file = NULL;
	}
	close(f->fd);
	f->fd = -1;
	return -1;
}

int ss_fileopen(ssfile *f, char *path)
{
	return ss_fileopenas(f, path, O_RDWR);
}

int ss_filenew(ssfile *f, char *path)
{
	return ss_fileopenas(f, path, O_RDWR|O_CREAT);
}

int ss_filerename(ssfile *f, char *path)
{
	char *p = ss_strdup(f->a, path);
	if (ssunlikely(p == NULL))
		return -1;
	int rc = ss_filemove(f->file, p);
	if (ssunlikely(rc == -1)) {
		ss_free(f->a, p);
		return -1;
	}
	ss_free(f->a, f->file);
	f->file = p;
	return 0;
}

int ss_fileclose(ssfile *f)
{
	if (sslikely(f->file)) {
		ss_free(f->a, f->file);
		f->file = NULL;
	}
	int rc;
	if (ssunlikely(f->fd != -1)) {
		rc = close(f->fd);
		if (ssunlikely(rc == -1))
			return -1;
		f->fd = -1;
	}
	return 0;
}

int ss_filesync(ssfile *f)
{
#if defined(__APPLE__)
	return fcntl(f->fd, F_FULLFSYNC);
#elif defined(__FreeBSD__) || defined(__DragonFly__)
	return fsync(f->fd);
#else
    // at least Linux, OpenBSD and NetBSD have fdatasync().
	return fdatasync(f->fd);
#endif
}

int ss_fileresize(ssfile *f, uint64_t size)
{
	int rc = ftruncate(f->fd, size);
	if (ssunlikely(rc == -1))
		return -1;
	f->size = size;
	return 0;
}

int ss_filepread(ssfile *f, uint64_t off, void *buf, size_t size)
{
	size_t n = 0;
	do {
		ssize_t r;
		do {
			r = pread(f->fd, (char*)buf + n, size - n, off + n);
		} while (r == -1 && errno == EINTR);
		if (r <= 0)
			return -1;
		n += r;
	} while (n != size);

	return 0;
}

int ss_filewrite(ssfile *f, void *buf, size_t size)
{
	size_t n = 0;
	do {
		ssize_t r;
		do {
			r = write(f->fd, (char*)buf + n, size - n);
		} while (r == -1 && errno == EINTR);
		if (r <= 0)
			return -1;
		n += r;
	} while (n != size);
	f->size += size;
	return 0;
}

int ss_filewritev(ssfile *f, ssiov *iv)
{
	struct iovec *v = iv->v;
	int n = iv->iovc;
	uint64_t size = 0;
	do {
		int r;
		do {
			r = writev(f->fd, v, n);
		} while (r == -1 && errno == EINTR);
		if (r < 0)
			return -1;
		size += r;
		while (n > 0) {
			if (v->iov_len > (size_t)r) {
				v->iov_base = (char*)v->iov_base + r;
				v->iov_len -= r;
				break;
			} else {
				r -= v->iov_len;
				v++;
				n--;
			}
		}
	} while (n > 0);
	f->size += size;
	return 0;
}

int ss_fileseek(ssfile *f, uint64_t off)
{
	return lseek(f->fd, off, SEEK_SET);
}

#if 0
int ss_filelock(ssfile *f)
{
	struct flock l;
	memset(&l, 0, sizeof(l));
	l.l_whence = SEEK_SET;
	l.l_start = 0;
	l.l_len = 0;
	l.l_type = F_WRLCK;
	return fcntl(f->fd, F_SETLK, &l);
}

int ss_fileunlock(ssfile *f)
{
	if (ssunlikely(f->fd == -1))
		return 0;
	struct flock l;
	memset(&l, 0, sizeof(l));
	l.l_whence = SEEK_SET;
	l.l_start = 0;
	l.l_len = 0;
	l.l_type = F_UNLCK;
	return fcntl(f->fd, F_SETLK, &l);
}
#endif

int ss_filerlb(ssfile *f, uint64_t svp)
{
	if (ssunlikely(f->size == svp))
		return 0;
	int rc = ftruncate(f->fd, svp);
	if (ssunlikely(rc == -1))
		return -1;
	f->size = svp;
	rc = ss_fileseek(f, f->size);
	if (ssunlikely(rc == -1))
		return -1;
	return 0;
}
#line 1 "sophia/std/ss_lz4filter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



/*  lz4 git commit: 2d4fed5ed2a8e0231f98d79699d28af0142d0099 */

/*
	LZ4 auto-framing library
	Copyright (C) 2011-2015, Yann Collet.

	BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are
	met:

	* Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above
	copyright notice, this list of conditions and the following disclaimer
	in the documentation and/or other materials provided with the
	distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	You can contact the author at :
	- LZ4 source repository : https://github.com/Cyan4973/lz4
	- LZ4 public forum : https://groups.google.com/forum/#!forum/lz4c
*/

/*
    xxHash - Extremely Fast Hash algorithm
    Header File
    Copyright (C) 2012-2015, Yann Collet.

    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - xxHash source repository : http://code.google.com/p/xxhash/
*/


/* LZ4F is a stand-alone API to create LZ4-compressed Frames
*  in full conformance with specification v1.5.0
*  All related operations, including memory management, are handled by the library.
* */

/* lz4frame_static.h */

/* lz4frame_static.h should be used solely in the context of static linking.
 * */

/**************************************
 * Error management
 * ************************************/
#define LZ4F_LIST_ERRORS(ITEM) \
        ITEM(OK_NoError) ITEM(ERROR_GENERIC) \
        ITEM(ERROR_maxBlockSize_invalid) ITEM(ERROR_blockMode_invalid) ITEM(ERROR_contentChecksumFlag_invalid) \
        ITEM(ERROR_compressionLevel_invalid) \
        ITEM(ERROR_allocation_failed) \
        ITEM(ERROR_srcSize_tooLarge) ITEM(ERROR_dstMaxSize_tooSmall) \
        ITEM(ERROR_frameSize_wrong) \
        ITEM(ERROR_frameType_unknown) \
        ITEM(ERROR_wrongSrcPtr) \
        ITEM(ERROR_decompressionFailed) \
        ITEM(ERROR_checksum_invalid) \
        ITEM(ERROR_maxCode)

#define LZ4F_GENERATE_ENUM(ENUM) ENUM,
typedef enum { LZ4F_LIST_ERRORS(LZ4F_GENERATE_ENUM) } LZ4F_errorCodes;  /* enum is exposed, to handle specific errors; compare function result to -enum value */

/* lz4frame_static.h EOF */

/* lz4frame.h */

/**************************************
 * Error management
 * ************************************/
typedef size_t LZ4F_errorCode_t;

unsigned    LZ4F_isError(LZ4F_errorCode_t code);
const char* LZ4F_getErrorName(LZ4F_errorCode_t code);   /* return error code string; useful for debugging */


/**************************************
 * Frame compression types
 * ************************************/
typedef enum { LZ4F_default=0, max64KB=4, max256KB=5, max1MB=6, max4MB=7 } blockSizeID_t;
typedef enum { blockLinked=0, blockIndependent} blockMode_t;
typedef enum { noContentChecksum=0, contentChecksumEnabled } contentChecksum_t;
typedef enum { LZ4F_frame=0, skippableFrame } frameType_t;

typedef struct {
  blockSizeID_t      blockSizeID;           /* max64KB, max256KB, max1MB, max4MB ; 0 == default */
  blockMode_t        blockMode;             /* blockLinked, blockIndependent ; 0 == default */
  contentChecksum_t  contentChecksumFlag;   /* noContentChecksum, contentChecksumEnabled ; 0 == default  */
  frameType_t        frameType;             /* LZ4F_frame, skippableFrame ; 0 == default */
  unsigned long long frameOSize;            /* Size of uncompressed (original) content ; 0 == unknown */
  unsigned           reserved[2];           /* must be zero for forward compatibility */
} LZ4F_frameInfo_t;

typedef struct {
  LZ4F_frameInfo_t frameInfo;
  unsigned         compressionLevel;       /* 0 == default (fast mode); values above 16 count as 16 */
  unsigned         autoFlush;              /* 1 == always flush (reduce need for tmp buffer) */
  unsigned         reserved[4];            /* must be zero for forward compatibility */
} LZ4F_preferences_t;


/***********************************
 * Simple compression function
 * *********************************/
size_t LZ4F_compressFrameBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr);

size_t LZ4F_compressFrame(void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_preferences_t* preferencesPtr);
/* LZ4F_compressFrame()
 * Compress an entire srcBuffer into a valid LZ4 frame, as defined by specification v1.4.1.
 * The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
 * You can get the minimum value of dstMaxSize by using LZ4F_compressFrameBound()
 * If this condition is not respected, LZ4F_compressFrame() will fail (result is an errorCode)
 * The LZ4F_preferences_t structure is optional : you can provide NULL as argument. All preferences will be set to default.
 * The result of the function is the number of bytes written into dstBuffer.
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 */


/**********************************
 * Advanced compression functions
 * ********************************/
typedef void* LZ4F_compressionContext_t;

typedef struct {
  unsigned stableSrc;    /* 1 == src content will remain available on future calls to LZ4F_compress(); avoid saving src content within tmp buffer as future dictionary */
  unsigned reserved[3];
} LZ4F_compressOptions_t;

/* Resource Management */

#define LZ4F_VERSION 100
LZ4F_errorCode_t LZ4F_createCompressionContext(LZ4F_compressionContext_t* LZ4F_compressionContextPtr, unsigned version);
LZ4F_errorCode_t LZ4F_freeCompressionContext(LZ4F_compressionContext_t LZ4F_compressionContext);
/* LZ4F_createCompressionContext() :
 * The first thing to do is to create a compressionContext object, which will be used in all compression operations.
 * This is achieved using LZ4F_createCompressionContext(), which takes as argument a version and an LZ4F_preferences_t structure.
 * The version provided MUST be LZ4F_VERSION. It is intended to track potential version differences between different binaries.
 * The function will provide a pointer to a fully allocated LZ4F_compressionContext_t object.
 * If the result LZ4F_errorCode_t is not zero, there was an error during context creation.
 * Object can release its memory using LZ4F_freeCompressionContext();
 */


/* Compression */

size_t LZ4F_compressBegin(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_preferences_t* preferencesPtr);
/* LZ4F_compressBegin() :
 * will write the frame header into dstBuffer.
 * dstBuffer must be large enough to accommodate a header (dstMaxSize). Maximum header size is 15 bytes.
 * The LZ4F_preferences_t structure is optional : you can provide NULL as argument, all preferences will then be set to default.
 * The result of the function is the number of bytes written into dstBuffer for the header
 * or an error code (can be tested using LZ4F_isError())
 */

size_t LZ4F_compressBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr);
/* LZ4F_compressBound() :
 * Provides the minimum size of Dst buffer given srcSize to handle worst case situations.
 * preferencesPtr is optional : you can provide NULL as argument, all preferences will then be set to default.
 * Note that different preferences will produce in different results.
 */

size_t LZ4F_compressUpdate(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_compressOptions_t* compressOptionsPtr);
/* LZ4F_compressUpdate()
 * LZ4F_compressUpdate() can be called repetitively to compress as much data as necessary.
 * The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
 * If this condition is not respected, LZ4F_compress() will fail (result is an errorCode)
 * You can get the minimum value of dstMaxSize by using LZ4F_compressBound()
 * The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
 * The result of the function is the number of bytes written into dstBuffer : it can be zero, meaning input data was just buffered.
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 */

size_t LZ4F_flush(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* compressOptionsPtr);
/* LZ4F_flush()
 * Should you need to create compressed data immediately, without waiting for a block to be filled,
 * you can call LZ4_flush(), which will immediately compress any remaining data buffered within compressionContext.
 * The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
 * The result of the function is the number of bytes written into dstBuffer
 * (it can be zero, this means there was no data left within compressionContext)
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 */

size_t LZ4F_compressEnd(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* compressOptionsPtr);
/* LZ4F_compressEnd()
 * When you want to properly finish the compressed frame, just call LZ4F_compressEnd().
 * It will flush whatever data remained within compressionContext (like LZ4_flush())
 * but also properly finalize the frame, with an endMark and a checksum.
 * The result of the function is the number of bytes written into dstBuffer (necessarily >= 4 (endMark size))
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 * The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
 * compressionContext can then be used again, starting with LZ4F_compressBegin().
 */


/***********************************
 * Decompression functions
 * *********************************/

typedef void* LZ4F_decompressionContext_t;

typedef struct {
  unsigned stableDst;       /* guarantee that decompressed data will still be there on next function calls (avoid storage into tmp buffers) */
  unsigned reserved[3];
} LZ4F_decompressOptions_t;


/* Resource management */

LZ4F_errorCode_t LZ4F_createDecompressionContext(LZ4F_decompressionContext_t* ctxPtr, unsigned version);
LZ4F_errorCode_t LZ4F_freeDecompressionContext(LZ4F_decompressionContext_t ctx);
/* LZ4F_createDecompressionContext() :
 * The first thing to do is to create a decompressionContext object, which will be used in all decompression operations.
 * This is achieved using LZ4F_createDecompressionContext().
 * The version provided MUST be LZ4F_VERSION. It is intended to track potential version differences between different binaries.
 * The function will provide a pointer to a fully allocated and initialized LZ4F_decompressionContext_t object.
 * If the result LZ4F_errorCode_t is not OK_NoError, there was an error during context creation.
 * Object can release its memory using LZ4F_freeDecompressionContext();
 */


/* Decompression */

size_t LZ4F_getFrameInfo(LZ4F_decompressionContext_t ctx,
                         LZ4F_frameInfo_t* frameInfoPtr,
                         const void* srcBuffer, size_t* srcSizePtr);
/* LZ4F_getFrameInfo()
 * This function decodes frame header information, such as blockSize.
 * It is optional : you could start by calling directly LZ4F_decompress() instead.
 * The objective is to extract header information without starting decompression, typically for allocation purposes.
 * LZ4F_getFrameInfo() can also be used *after* starting decompression, on a valid LZ4F_decompressionContext_t.
 * The number of bytes read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
 * You are expected to resume decompression from where it stopped (srcBuffer + *srcSizePtr)
 * The function result is an hint of how many srcSize bytes LZ4F_decompress() expects for next call,
 *                        or an error code which can be tested using LZ4F_isError().
 */

size_t LZ4F_decompress(LZ4F_decompressionContext_t ctx,
                       void* dstBuffer, size_t* dstSizePtr,
                       const void* srcBuffer, size_t* srcSizePtr,
                       const LZ4F_decompressOptions_t* optionsPtr);
/* LZ4F_decompress()
 * Call this function repetitively to regenerate data compressed within srcBuffer.
 * The function will attempt to decode *srcSizePtr bytes from srcBuffer, into dstBuffer of maximum size *dstSizePtr.
 *
 * The number of bytes regenerated into dstBuffer will be provided within *dstSizePtr (necessarily <= original value).
 *
 * The number of bytes read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
 * If number of bytes read is < number of bytes provided, then decompression operation is not completed.
 * It typically happens when dstBuffer is not large enough to contain all decoded data.
 * LZ4F_decompress() must be called again, starting from where it stopped (srcBuffer + *srcSizePtr)
 * The function will check this condition, and refuse to continue if it is not respected.
 *
 * dstBuffer is supposed to be flushed between each call to the function, since its content will be overwritten.
 * dst arguments can be changed at will with each consecutive call to the function.
 *
 * The function result is an hint of how many srcSize bytes LZ4F_decompress() expects for next call.
 * Schematically, it's the size of the current (or remaining) compressed block + header of next block.
 * Respecting the hint provides some boost to performance, since it does skip intermediate buffers.
 * This is just a hint, you can always provide any srcSize you want.
 * When a frame is fully decoded, the function result will be 0. (no more data expected)
 * If decompression failed, function result is an error code, which can be tested using LZ4F_isError().
 */

/* lz4frame.h EOF */


/* lz4.h */

/*
 * lz4.h provides block compression functions, for optimal performance.
 * If you need to generate inter-operable compressed data (respecting LZ4 frame specification),
 * please use lz4frame.h instead.
*/

/**************************************
*  Version
**************************************/
#define LZ4_VERSION_MAJOR    1    /* for breaking interface changes  */
#define LZ4_VERSION_MINOR    6    /* for new (non-breaking) interface capabilities */
#define LZ4_VERSION_RELEASE  0    /* for tweaks, bug-fixes, or development */
#define LZ4_VERSION_NUMBER (LZ4_VERSION_MAJOR *100*100 + LZ4_VERSION_MINOR *100 + LZ4_VERSION_RELEASE)
int LZ4_versionNumber (void);

/**************************************
*  Tuning parameter
**************************************/
/*
 * LZ4_MEMORY_USAGE :
 * Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
 * Increasing memory usage improves compression ratio
 * Reduced memory usage can improve speed, due to cache effect
 * Default value is 14, for 16KB, which nicely fits into Intel x86 L1 cache
 */
#define LZ4_MEMORY_USAGE 14


/**************************************
*  Simple Functions
**************************************/

int LZ4_compress        (const char* source, char* dest, int sourceSize);
int LZ4_decompress_safe (const char* source, char* dest, int compressedSize, int maxDecompressedSize);

/*
LZ4_compress() :
    Compresses 'sourceSize' bytes from 'source' into 'dest'.
    Destination buffer must be already allocated,
    and must be sized to handle worst cases situations (input data not compressible)
    Worst case size evaluation is provided by function LZ4_compressBound()
    inputSize : Max supported value is LZ4_MAX_INPUT_SIZE
    return : the number of bytes written in buffer dest
             or 0 if the compression fails

LZ4_decompress_safe() :
    compressedSize : is obviously the source size
    maxDecompressedSize : is the size of the destination buffer, which must be already allocated.
    return : the number of bytes decompressed into the destination buffer (necessarily <= maxDecompressedSize)
             If the destination buffer is not large enough, decoding will stop and output an error code (<0).
             If the source stream is detected malformed, the function will stop decoding and return a negative result.
             This function is protected against buffer overflow exploits,
             and never writes outside of output buffer, nor reads outside of input buffer.
             It is also protected against malicious data packets.
*/


/**************************************
*  Advanced Functions
**************************************/
#define LZ4_MAX_INPUT_SIZE        0x7E000000   /* 2 113 929 216 bytes */
#define LZ4_COMPRESSBOUND(isize)  ((unsigned int)(isize) > (unsigned int)LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize)/255) + 16)

/*
LZ4_compressBound() :
    Provides the maximum size that LZ4 compression may output in a "worst case" scenario (input data not compressible)
    This function is primarily useful for memory allocation purposes (output buffer size).
    Macro LZ4_COMPRESSBOUND() is also provided for compilation-time evaluation (stack memory allocation for example).

    isize  : is the input size. Max supported value is LZ4_MAX_INPUT_SIZE
    return : maximum output size in a "worst case" scenario
             or 0, if input size is too large ( > LZ4_MAX_INPUT_SIZE)
*/
int LZ4_compressBound(int isize);


/*
LZ4_compress_limitedOutput() :
    Compress 'sourceSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
    If it cannot achieve it, compression will stop, and result of the function will be zero.
    This saves time and memory on detecting non-compressible (or barely compressible) data.
    This function never writes outside of provided output buffer.

    sourceSize  : Max supported value is LZ4_MAX_INPUT_VALUE
    maxOutputSize : is the size of the destination buffer (which must be already allocated)
    return : the number of bytes written in buffer 'dest'
             or 0 if compression fails
*/
int LZ4_compress_limitedOutput (const char* source, char* dest, int sourceSize, int maxOutputSize);


/*
LZ4_compress_withState() :
    Same compression functions, but using an externally allocated memory space to store compression state.
    Use LZ4_sizeofState() to know how much memory must be allocated,
    and then, provide it as 'void* state' to compression functions.
*/
int LZ4_sizeofState(void);
int LZ4_compress_withState               (void* state, const char* source, char* dest, int inputSize);
int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);


/*
LZ4_decompress_fast() :
    originalSize : is the original and therefore uncompressed size
    return : the number of bytes read from the source buffer (in other words, the compressed size)
             If the source stream is detected malformed, the function will stop decoding and return a negative result.
             Destination buffer must be already allocated. Its size must be a minimum of 'originalSize' bytes.
    note : This function fully respect memory boundaries for properly formed compressed data.
           It is a bit faster than LZ4_decompress_safe().
           However, it does not provide any protection against intentionally modified data stream (malicious input).
           Use this function in trusted environment only (data to decode comes from a trusted source).
*/
int LZ4_decompress_fast (const char* source, char* dest, int originalSize);


/*
LZ4_decompress_safe_partial() :
    This function decompress a compressed block of size 'compressedSize' at position 'source'
    into destination buffer 'dest' of size 'maxDecompressedSize'.
    The function tries to stop decompressing operation as soon as 'targetOutputSize' has been reached,
    reducing decompression time.
    return : the number of bytes decoded in the destination buffer (necessarily <= maxDecompressedSize)
       Note : this number can be < 'targetOutputSize' should the compressed block to decode be smaller.
             Always control how many bytes were decoded.
             If the source stream is detected malformed, the function will stop decoding and return a negative result.
             This function never writes outside of output buffer, and never reads outside of input buffer. It is therefore protected against malicious data packets
*/
int LZ4_decompress_safe_partial (const char* source, char* dest, int compressedSize, int targetOutputSize, int maxDecompressedSize);


/***********************************************
*  Streaming Compression Functions
***********************************************/

#define LZ4_STREAMSIZE_U64 ((1 << (LZ4_MEMORY_USAGE-3)) + 4)
#define LZ4_STREAMSIZE     (LZ4_STREAMSIZE_U64 * sizeof(long long))
/*
 * LZ4_stream_t
 * information structure to track an LZ4 stream.
 * important : init this structure content before first use !
 * note : only allocated directly the structure if you are statically linking LZ4
 *        If you are using liblz4 as a DLL, please use below construction methods instead.
 */
typedef struct { long long table[LZ4_STREAMSIZE_U64]; } LZ4_stream_t;

/*
 * LZ4_resetStream
 * Use this function to init an allocated LZ4_stream_t structure
 */
void LZ4_resetStream (LZ4_stream_t* LZ4_streamPtr);

/*
 * LZ4_createStream will allocate and initialize an LZ4_stream_t structure
 * LZ4_freeStream releases its memory.
 * In the context of a DLL (liblz4), please use these methods rather than the static struct.
 * They are more future proof, in case of a change of LZ4_stream_t size.
 */
LZ4_stream_t* LZ4_createStream(void);
int           LZ4_freeStream (LZ4_stream_t* LZ4_streamPtr);

/*
 * LZ4_loadDict
 * Use this function to load a static dictionary into LZ4_stream.
 * Any previous data will be forgotten, only 'dictionary' will remain in memory.
 * Loading a size of 0 is allowed.
 * Return : dictionary size, in bytes (necessarily <= 64 KB)
 */
int LZ4_loadDict (LZ4_stream_t* LZ4_streamPtr, const char* dictionary, int dictSize);

/*
 * LZ4_compress_continue
 * Compress data block 'source', using blocks compressed before as dictionary to improve compression ratio
 * Previous data blocks are assumed to still be present at their previous location.
 * dest buffer must be already allocated, and sized to at least LZ4_compressBound(inputSize)
 */
int LZ4_compress_continue (LZ4_stream_t* LZ4_streamPtr, const char* source, char* dest, int inputSize);

/*
 * LZ4_compress_limitedOutput_continue
 * Same as before, but also specify a maximum target compressed size (maxOutputSize)
 * If objective cannot be met, compression exits, and returns a zero.
 */
int LZ4_compress_limitedOutput_continue (LZ4_stream_t* LZ4_streamPtr, const char* source, char* dest, int inputSize, int maxOutputSize);

/*
 * LZ4_saveDict
 * If previously compressed data block is not guaranteed to remain available at its memory location
 * save it into a safer place (char* safeBuffer)
 * Note : you don't need to call LZ4_loadDict() afterwards,
 *        dictionary is immediately usable, you can therefore call again LZ4_compress_continue()
 * Return : saved dictionary size in bytes (necessarily <= dictSize), or 0 if error
 */
int LZ4_saveDict (LZ4_stream_t* LZ4_streamPtr, char* safeBuffer, int dictSize);


/************************************************
*  Streaming Decompression Functions
************************************************/

#define LZ4_STREAMDECODESIZE_U64  4
#define LZ4_STREAMDECODESIZE     (LZ4_STREAMDECODESIZE_U64 * sizeof(unsigned long long))
typedef struct { unsigned long long table[LZ4_STREAMDECODESIZE_U64]; } LZ4_streamDecode_t;
/*
 * LZ4_streamDecode_t
 * information structure to track an LZ4 stream.
 * init this structure content using LZ4_setStreamDecode or memset() before first use !
 *
 * In the context of a DLL (liblz4) please prefer usage of construction methods below.
 * They are more future proof, in case of a change of LZ4_streamDecode_t size in the future.
 * LZ4_createStreamDecode will allocate and initialize an LZ4_streamDecode_t structure
 * LZ4_freeStreamDecode releases its memory.
 */
LZ4_streamDecode_t* LZ4_createStreamDecode(void);
int                 LZ4_freeStreamDecode (LZ4_streamDecode_t* LZ4_stream);

/*
 * LZ4_setStreamDecode
 * Use this function to instruct where to find the dictionary.
 * Setting a size of 0 is allowed (same effect as reset).
 * Return : 1 if OK, 0 if error
 */
int LZ4_setStreamDecode (LZ4_streamDecode_t* LZ4_streamDecode, const char* dictionary, int dictSize);

/*
*_continue() :
    These decoding functions allow decompression of multiple blocks in "streaming" mode.
    Previously decoded blocks *must* remain available at the memory position where they were decoded (up to 64 KB)
    If this condition is not possible, save the relevant part of decoded data into a safe buffer,
    and indicate where is its new address using LZ4_setStreamDecode()
*/
int LZ4_decompress_safe_continue (LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int compressedSize, int maxDecompressedSize);
int LZ4_decompress_fast_continue (LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int originalSize);


/*
Advanced decoding functions :
*_usingDict() :
    These decoding functions work the same as
    a combination of LZ4_setDictDecode() followed by LZ4_decompress_x_continue()
    They are stand-alone and don't use nor update an LZ4_streamDecode_t structure.
*/
int LZ4_decompress_safe_usingDict (const char* source, char* dest, int compressedSize, int maxDecompressedSize, const char* dictStart, int dictSize);
int LZ4_decompress_fast_usingDict (const char* source, char* dest, int originalSize, const char* dictStart, int dictSize);



/**************************************
*  Obsolete Functions
**************************************/
/*
Obsolete decompression functions
These function names are deprecated and should no longer be used.
They are only provided here for compatibility with older user programs.
- LZ4_uncompress is the same as LZ4_decompress_fast
- LZ4_uncompress_unknownOutputSize is the same as LZ4_decompress_safe
These function prototypes are now disabled; uncomment them if you really need them.
It is highly recommended to stop using these functions and migrate to newer ones */
/* int LZ4_uncompress (const char* source, char* dest, int outputSize); */
/* int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize); */


/* Obsolete streaming functions; use new streaming interface whenever possible */
void* LZ4_create (const char* inputBuffer);
int   LZ4_sizeofStreamState(void);
int   LZ4_resetStreamState(void* state, const char* inputBuffer);
char* LZ4_slideInputBuffer (void* state);

/* Obsolete streaming decoding functions */
int LZ4_decompress_safe_withPrefix64k (const char* source, char* dest, int compressedSize, int maxOutputSize);
int LZ4_decompress_fast_withPrefix64k (const char* source, char* dest, int originalSize);

/* lz4.h EOF */

/* lz4hc.h */

int LZ4_compressHC (const char* source, char* dest, int inputSize);
/*
LZ4_compressHC :
    return : the number of bytes in compressed buffer dest
             or 0 if compression fails.
    note : destination buffer must be already allocated.
        To avoid any problem, size it to handle worst cases situations (input data not compressible)
        Worst case size evaluation is provided by function LZ4_compressBound() (see "lz4.h")
*/

int LZ4_compressHC_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);
/*
LZ4_compress_limitedOutput() :
    Compress 'inputSize' bytes from 'source' into an output buffer 'dest' of maximum size 'maxOutputSize'.
    If it cannot achieve it, compression will stop, and result of the function will be zero.
    This function never writes outside of provided output buffer.

    inputSize  : Max supported value is 1 GB
    maxOutputSize : is maximum allowed size into the destination buffer (which must be already allocated)
    return : the number of output bytes written in buffer 'dest'
             or 0 if compression fails.
*/


int LZ4_compressHC2 (const char* source, char* dest, int inputSize, int compressionLevel);
int LZ4_compressHC2_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);
/*
    Same functions as above, but with programmable 'compressionLevel'.
    Recommended values are between 4 and 9, although any value between 0 and 16 will work.
    'compressionLevel'==0 means use default 'compressionLevel' value.
    Values above 16 behave the same as 16.
    Equivalent variants exist for all other compression functions below.
*/

/* Note :
   Decompression functions are provided within LZ4 source code (see "lz4.h") (BSD license)
*/


/**************************************
*  Using an external allocation
**************************************/
int LZ4_sizeofStateHC(void);
int LZ4_compressHC_withStateHC               (void* state, const char* source, char* dest, int inputSize);
int LZ4_compressHC_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);

int LZ4_compressHC2_withStateHC              (void* state, const char* source, char* dest, int inputSize, int compressionLevel);
int LZ4_compressHC2_limitedOutput_withStateHC(void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);

/*
These functions are provided should you prefer to allocate memory for compression tables with your own allocation methods.
To know how much memory must be allocated for the compression tables, use :
int LZ4_sizeofStateHC();

Note that tables must be aligned for pointer (32 or 64 bits), otherwise compression will fail (return code 0).

The allocated memory can be provided to the compression functions using 'void* state' parameter.
LZ4_compress_withStateHC() and LZ4_compress_limitedOutput_withStateHC() are equivalent to previously described functions.
They just use the externally allocated memory for state instead of allocating their own (on stack, or on heap).
*/



/*****************************
*  Includes
*****************************/



/**************************************
*  Experimental Streaming Functions
**************************************/
#define LZ4_STREAMHCSIZE        262192
#define LZ4_STREAMHCSIZE_SIZET (LZ4_STREAMHCSIZE / sizeof(size_t))
typedef struct { size_t table[LZ4_STREAMHCSIZE_SIZET]; } LZ4_streamHC_t;
/*
LZ4_streamHC_t
This structure allows static allocation of LZ4 HC streaming state.
State must then be initialized using LZ4_resetStreamHC() before first use.

Static allocation should only be used with statically linked library.
If you want to use LZ4 as a DLL, please use construction functions below, which are more future-proof.
*/


LZ4_streamHC_t* LZ4_createStreamHC(void);
int             LZ4_freeStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr);
/*
These functions create and release memory for LZ4 HC streaming state.
Newly created states are already initialized.
Existing state space can be re-used anytime using LZ4_resetStreamHC().
If you use LZ4 as a DLL, please use these functions instead of direct struct allocation,
to avoid size mismatch between different versions.
*/

void LZ4_resetStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel);
int  LZ4_loadDictHC (LZ4_streamHC_t* LZ4_streamHCPtr, const char* dictionary, int dictSize);

int LZ4_compressHC_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize);
int LZ4_compressHC_limitedOutput_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize, int maxOutputSize);

int LZ4_saveDictHC (LZ4_streamHC_t* LZ4_streamHCPtr, char* safeBuffer, int maxDictSize);

/*
These functions compress data in successive blocks of any size, using previous blocks as dictionary.
One key assumption is that each previous block will remain read-accessible while compressing next block.

Before starting compression, state must be properly initialized, using LZ4_resetStreamHC().
A first "fictional block" can then be designated as initial dictionary, using LZ4_loadDictHC() (Optional).

Then, use LZ4_compressHC_continue() or LZ4_compressHC_limitedOutput_continue() to compress each successive block.
They work like usual LZ4_compressHC() or LZ4_compressHC_limitedOutput(), but use previous memory blocks to improve compression.
Previous memory blocks (including initial dictionary when present) must remain accessible and unmodified during compression.

If, for any reason, previous data block can't be preserved in memory during next compression block,
you must save it to a safer memory space,
using LZ4_saveDictHC().
*/



/**************************************
 * Deprecated Streaming Functions
 * ************************************/
/* Note : these streaming functions follows the older model, and should no longer be used */
void* LZ4_createHC (const char* inputBuffer);
char* LZ4_slideInputBufferHC (void* LZ4HC_Data);
int   LZ4_freeHC (void* LZ4HC_Data);

int   LZ4_compressHC2_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel);
int   LZ4_compressHC2_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel);

int   LZ4_sizeofStreamStateHC(void);
int   LZ4_resetStreamStateHC(void* state, const char* inputBuffer);

/* lz4hc.h EOF */

/* xxhash.h */


/* Notice extracted from xxHash homepage :

xxHash is an extremely fast Hash algorithm, running at RAM speed limits.
It also successfully passes all tests from the SMHasher suite.

Comparison (single thread, Windows Seven 32 bits, using SMHasher on a Core 2 Duo @3GHz)

Name            Speed       Q.Score   Author
xxHash          5.4 GB/s     10
CrapWow         3.2 GB/s      2       Andrew
MumurHash 3a    2.7 GB/s     10       Austin Appleby
SpookyHash      2.0 GB/s     10       Bob Jenkins
SBox            1.4 GB/s      9       Bret Mulvey
Lookup3         1.2 GB/s      9       Bob Jenkins
SuperFastHash   1.2 GB/s      1       Paul Hsieh
CityHash64      1.05 GB/s    10       Pike & Alakuijala
FNV             0.55 GB/s     5       Fowler, Noll, Vo
CRC32           0.43 GB/s     9
MD5-32          0.33 GB/s    10       Ronald L. Rivest
SHA1-32         0.28 GB/s    10

Q.Score is a measure of quality of the hash function.
It depends on successfully passing SMHasher test set.
10 is a perfect score.

A new 64-bits version, named XXH64, is available since r35.
It offers better speed for 64-bits applications.
Name     Speed on 64 bits    Speed on 32 bits
XXH64       13.8 GB/s            1.9 GB/s
XXH32        6.8 GB/s            6.0 GB/s
*/

/*****************************
*  Definitions
*****************************/
typedef enum { XXH_OK=0, XXH_ERROR } XXH_errorcode;



/*****************************
*  Simple Hash Functions
*****************************/

unsigned int       XXH32 (const void* input, size_t length, unsigned seed);
unsigned long long XXH64 (const void* input, size_t length, unsigned long long seed);

/*
XXH32() :
    Calculate the 32-bits hash of sequence "length" bytes stored at memory address "input".
    The memory between input & input+length must be valid (allocated and read-accessible).
    "seed" can be used to alter the result predictably.
    This function successfully passes all SMHasher tests.
    Speed on Core 2 Duo @ 3 GHz (single thread, SMHasher benchmark) : 5.4 GB/s
XXH64() :
    Calculate the 64-bits hash of sequence of length "len" stored at memory address "input".
    Faster on 64-bits systems. Slower on 32-bits systems.
*/



/*****************************
*  Advanced Hash Functions
*****************************/
typedef struct { long long ll[ 6]; } XXH32_state_t;
typedef struct { long long ll[11]; } XXH64_state_t;

/*
These structures allow static allocation of XXH states.
States must then be initialized using XXHnn_reset() before first use.

If you prefer dynamic allocation, please refer to functions below.
*/

XXH32_state_t* XXH32_createState(void);
XXH_errorcode  XXH32_freeState(XXH32_state_t* statePtr);

XXH64_state_t* XXH64_createState(void);
XXH_errorcode  XXH64_freeState(XXH64_state_t* statePtr);

/*
These functions create and release memory for XXH state.
States must then be initialized using XXHnn_reset() before first use.
*/


XXH_errorcode XXH32_reset  (XXH32_state_t* statePtr, unsigned seed);
XXH_errorcode XXH32_update (XXH32_state_t* statePtr, const void* input, size_t length);
unsigned int  XXH32_digest (const XXH32_state_t* statePtr);

XXH_errorcode      XXH64_reset  (XXH64_state_t* statePtr, unsigned long long seed);
XXH_errorcode      XXH64_update (XXH64_state_t* statePtr, const void* input, size_t length);
unsigned long long XXH64_digest (const XXH64_state_t* statePtr);

/*
These functions calculate the xxHash of an input provided in multiple smaller packets,
as opposed to an input provided as a single block.

XXH state space must first be allocated, using either static or dynamic method provided above.

Start a new hash by initializing state with a seed, using XXHnn_reset().

Then, feed the hash state by calling XXHnn_update() as many times as necessary.
Obviously, input must be valid, meaning allocated and read accessible.
The function returns an error code, with 0 meaning OK, and any other value meaning there is an error.

Finally, you can produce a hash anytime, by using XXHnn_digest().
This function returns the final nn-bits hash.
You can nonetheless continue feeding the hash state with more input,
and therefore get some new hashes, by calling again XXHnn_digest().

When you are done, don't forget to free XXH state space, using typically XXHnn_freeState().
*/

/* xxhash.h EOF */

/* xxhash.c */

/**************************************
*  Tuning parameters
***************************************/
/* Unaligned memory access is automatically enabled for "common" CPU, such as x86.
 * For others CPU, the compiler will be more cautious, and insert extra code to ensure aligned access is respected.
 * If you know your target CPU supports unaligned memory access, you want to force this option manually to improve performance.
 * You can also enable this parameter if you know your input data will always be aligned (boundaries of 4, for U32).
 */
#if defined(__ARM_FEATURE_UNALIGNED) || defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#  define XXH_USE_UNALIGNED_ACCESS 1
#endif

/* XXH_ACCEPT_NULL_INPUT_POINTER :
 * If the input pointer is a null pointer, xxHash default behavior is to trigger a memory access error, since it is a bad pointer.
 * When this option is enabled, xxHash output for null input pointers will be the same as a null-length input.
 * By default, this option is disabled. To enable it, uncomment below define :
 */
/* #define XXH_ACCEPT_NULL_INPUT_POINTER 1 */

/* XXH_FORCE_NATIVE_FORMAT :
 * By default, xxHash library provides endian-independant Hash values, based on little-endian convention.
 * Results are therefore identical for little-endian and big-endian CPU.
 * This comes at a performance cost for big-endian CPU, since some swapping is required to emulate little-endian format.
 * Should endian-independance be of no importance for your application, you may set the #define below to 1.
 * It will improve speed for Big-endian CPU.
 * This option has no impact on Little_Endian CPU.
 */
#define XXH_FORCE_NATIVE_FORMAT 0


/**************************************
*  Compiler Specific Options
***************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  pragma warning(disable : 4127)      /* disable: C4127: conditional expression is constant */
#  define FORCE_INLINE static __forceinline
#else
#  if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
#    ifdef __GNUC__
#      define FORCE_INLINE static inline __attribute__((always_inline))
#    else
#      define FORCE_INLINE static inline
#    endif
#  else
#    define FORCE_INLINE static
#  endif /* __STDC_VERSION__ */
#endif


/**************************************
*  Includes & Memory related functions
***************************************/
/* Modify the local functions below should you wish to use some other memory routines */
/* for malloc(), free() */
static void* XXH_malloc(size_t s) { return malloc(s); }
static void  XXH_free  (void* p)  { free(p); }
static void* XXH_memcpy(void* dest, const void* src, size_t size)
{
    return memcpy(dest,src,size);
}


/**************************************
*  Basic Types
***************************************/
#ifndef ZTYPES
#define ZTYPES 1

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
typedef  uint8_t BYTE;
typedef uint16_t U16;
typedef  int16_t S16;
typedef uint32_t U32;
typedef  int32_t S32;
typedef uint64_t U64;
typedef  int64_t S64;
#else
typedef unsigned char       BYTE;
typedef unsigned short      U16;
typedef   signed short      S16;
typedef unsigned int        U32;
typedef   signed int        S32;
typedef unsigned long long  U64;
typedef   signed long long  S64;
#endif

#endif

#if defined(__GNUC__)  && !defined(XXH_USE_UNALIGNED_ACCESS)
#  define _PACKED __attribute__ ((packed))
#else
#  define _PACKED
#endif

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  ifdef __IBMC__
#    pragma pack(1)
#  else
#    pragma pack(push, 1)
#  endif
#endif

typedef struct _U32_S
{
    U32 v;
} _PACKED U32_S;
typedef struct _U64_S
{
    U64 v;
} _PACKED U64_S;

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(pop)
#endif

#define A32(x) (((U32_S *)(x))->v)
#define A64(x) (((U64_S *)(x))->v)


/*****************************************
*  Compiler-specific Functions and Macros
******************************************/
#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

/* Note : although _rotl exists for minGW (GCC under windows), performance seems poor */
#if defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) ((x << r) | (x >> (32 - r)))
#  define XXH_rotl64(x,r) ((x << r) | (x >> (64 - r)))
#endif

#if defined(_MSC_VER)     /* Visual Studio */
#  define XXH_swap32 _byteswap_ulong
#  define XXH_swap64 _byteswap_uint64
#elif GCC_VERSION >= 403
#  define XXH_swap32 __builtin_bswap32
#  define XXH_swap64 __builtin_bswap64
#else
static U32 XXH_swap32 (U32 x)
{
    return  ((x << 24) & 0xff000000 ) |
            ((x <<  8) & 0x00ff0000 ) |
            ((x >>  8) & 0x0000ff00 ) |
            ((x >> 24) & 0x000000ff );
}
static U64 XXH_swap64 (U64 x)
{
    return  ((x << 56) & 0xff00000000000000ULL) |
            ((x << 40) & 0x00ff000000000000ULL) |
            ((x << 24) & 0x0000ff0000000000ULL) |
            ((x << 8)  & 0x000000ff00000000ULL) |
            ((x >> 8)  & 0x00000000ff000000ULL) |
            ((x >> 24) & 0x0000000000ff0000ULL) |
            ((x >> 40) & 0x000000000000ff00ULL) |
            ((x >> 56) & 0x00000000000000ffULL);
}
#endif


/**************************************
*  Constants
***************************************/
#define PRIME32_1   2654435761U
#define PRIME32_2   2246822519U
#define PRIME32_3   3266489917U
#define PRIME32_4    668265263U
#define PRIME32_5    374761393U

#define PRIME64_1 11400714785074694791ULL
#define PRIME64_2 14029467366897019727ULL
#define PRIME64_3  1609587929392839161ULL
#define PRIME64_4  9650029242287828579ULL
#define PRIME64_5  2870177450012600261ULL


/***************************************
*  Architecture Macros
****************************************/
typedef enum { XXH_bigEndian=0, XXH_littleEndian=1 } XXH_endianess;
#ifndef XXH_CPU_LITTLE_ENDIAN   /* XXH_CPU_LITTLE_ENDIAN can be defined externally, for example using a compiler switch */
static const int one = 1;
#   define XXH_CPU_LITTLE_ENDIAN   (*(char*)(&one))
#endif


/**************************************
*  Macros
***************************************/
#define XXH_STATIC_ASSERT(c)   { enum { XXH_static_assert = 1/(!!(c)) }; }    /* use only *after* variable declarations */


/****************************
*  Memory reads
*****************************/
typedef enum { XXH_aligned, XXH_unaligned } XXH_alignment;

FORCE_INLINE U32 XXH_readLE32_align(const void* ptr, XXH_endianess endian, XXH_alignment align)
{
    if (align==XXH_unaligned)
        return endian==XXH_littleEndian ? A32(ptr) : XXH_swap32(A32(ptr));
    else
        return endian==XXH_littleEndian ? *(U32*)ptr : XXH_swap32(*(U32*)ptr);
}

FORCE_INLINE U32 XXH_readLE32(const void* ptr, XXH_endianess endian)
{
    return XXH_readLE32_align(ptr, endian, XXH_unaligned);
}

FORCE_INLINE U64 XXH_readLE64_align(const void* ptr, XXH_endianess endian, XXH_alignment align)
{
    if (align==XXH_unaligned)
        return endian==XXH_littleEndian ? A64(ptr) : XXH_swap64(A64(ptr));
    else
        return endian==XXH_littleEndian ? *(U64*)ptr : XXH_swap64(*(U64*)ptr);
}

FORCE_INLINE U64 XXH_readLE64(const void* ptr, XXH_endianess endian)
{
    return XXH_readLE64_align(ptr, endian, XXH_unaligned);
}


/****************************
*  Simple Hash Functions
*****************************/
FORCE_INLINE U32 XXH32_endian_align(const void* input, size_t len, U32 seed, XXH_endianess endian, XXH_alignment align)
{
    const BYTE* p = (const BYTE*)input;
    const BYTE* bEnd = p + len;
    U32 h32;
#define XXH_get32bits(p) XXH_readLE32_align(p, endian, align)

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
    if (p==NULL)
    {
        len=0;
        bEnd=p=(const BYTE*)(size_t)16;
    }
#endif

    if (len>=16)
    {
        const BYTE* const limit = bEnd - 16;
        U32 v1 = seed + PRIME32_1 + PRIME32_2;
        U32 v2 = seed + PRIME32_2;
        U32 v3 = seed + 0;
        U32 v4 = seed - PRIME32_1;

        do
        {
            v1 += XXH_get32bits(p) * PRIME32_2;
            v1 = XXH_rotl32(v1, 13);
            v1 *= PRIME32_1;
            p+=4;
            v2 += XXH_get32bits(p) * PRIME32_2;
            v2 = XXH_rotl32(v2, 13);
            v2 *= PRIME32_1;
            p+=4;
            v3 += XXH_get32bits(p) * PRIME32_2;
            v3 = XXH_rotl32(v3, 13);
            v3 *= PRIME32_1;
            p+=4;
            v4 += XXH_get32bits(p) * PRIME32_2;
            v4 = XXH_rotl32(v4, 13);
            v4 *= PRIME32_1;
            p+=4;
        }
        while (p<=limit);

        h32 = XXH_rotl32(v1, 1) + XXH_rotl32(v2, 7) + XXH_rotl32(v3, 12) + XXH_rotl32(v4, 18);
    }
    else
    {
        h32  = seed + PRIME32_5;
    }

    h32 += (U32) len;

    while (p+4<=bEnd)
    {
        h32 += XXH_get32bits(p) * PRIME32_3;
        h32  = XXH_rotl32(h32, 17) * PRIME32_4 ;
        p+=4;
    }

    while (p<bEnd)
    {
        h32 += (*p) * PRIME32_5;
        h32 = XXH_rotl32(h32, 11) * PRIME32_1 ;
        p++;
    }

    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}


unsigned int XXH32 (const void* input, size_t len, unsigned seed)
{
#if 0
    /* Simple version, good for code maintenance, but unfortunately slow for small inputs */
    XXH32_state_t state;
    XXH32_reset(&state, seed);
    XXH32_update(&state, input, len);
    return XXH32_digest(&state);
#else
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

#  if !defined(XXH_USE_UNALIGNED_ACCESS)
    if ((((size_t)input) & 3) == 0)   /* Input is aligned, let's leverage the speed advantage */
    {
        if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
            return XXH32_endian_align(input, len, seed, XXH_littleEndian, XXH_aligned);
        else
            return XXH32_endian_align(input, len, seed, XXH_bigEndian, XXH_aligned);
    }
#  endif

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH32_endian_align(input, len, seed, XXH_littleEndian, XXH_unaligned);
    else
        return XXH32_endian_align(input, len, seed, XXH_bigEndian, XXH_unaligned);
#endif
}

FORCE_INLINE U64 XXH64_endian_align(const void* input, size_t len, U64 seed, XXH_endianess endian, XXH_alignment align)
{
    const BYTE* p = (const BYTE*)input;
    const BYTE* bEnd = p + len;
    U64 h64;
#define XXH_get64bits(p) XXH_readLE64_align(p, endian, align)

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
    if (p==NULL)
    {
        len=0;
        bEnd=p=(const BYTE*)(size_t)32;
    }
#endif

    if (len>=32)
    {
        const BYTE* const limit = bEnd - 32;
        U64 v1 = seed + PRIME64_1 + PRIME64_2;
        U64 v2 = seed + PRIME64_2;
        U64 v3 = seed + 0;
        U64 v4 = seed - PRIME64_1;

        do
        {
            v1 += XXH_get64bits(p) * PRIME64_2;
            p+=8;
            v1 = XXH_rotl64(v1, 31);
            v1 *= PRIME64_1;
            v2 += XXH_get64bits(p) * PRIME64_2;
            p+=8;
            v2 = XXH_rotl64(v2, 31);
            v2 *= PRIME64_1;
            v3 += XXH_get64bits(p) * PRIME64_2;
            p+=8;
            v3 = XXH_rotl64(v3, 31);
            v3 *= PRIME64_1;
            v4 += XXH_get64bits(p) * PRIME64_2;
            p+=8;
            v4 = XXH_rotl64(v4, 31);
            v4 *= PRIME64_1;
        }
        while (p<=limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

        v1 *= PRIME64_2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= PRIME64_1;
        h64 ^= v1;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v2 *= PRIME64_2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= PRIME64_1;
        h64 ^= v2;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v3 *= PRIME64_2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= PRIME64_1;
        h64 ^= v3;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v4 *= PRIME64_2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= PRIME64_1;
        h64 ^= v4;
        h64 = h64 * PRIME64_1 + PRIME64_4;
    }
    else
    {
        h64  = seed + PRIME64_5;
    }

    h64 += (U64) len;

    while (p+8<=bEnd)
    {
        U64 k1 = XXH_get64bits(p);
        k1 *= PRIME64_2;
        k1 = XXH_rotl64(k1,31);
        k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64,27) * PRIME64_1 + PRIME64_4;
        p+=8;
    }

    if (p+4<=bEnd)
    {
        h64 ^= (U64)(XXH_get32bits(p)) * PRIME64_1;
        h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p+=4;
    }

    while (p<bEnd)
    {
        h64 ^= (*p) * PRIME64_5;
        h64 = XXH_rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}


unsigned long long XXH64 (const void* input, size_t len, unsigned long long seed)
{
#if 0
    /* Simple version, good for code maintenance, but unfortunately slow for small inputs */
    XXH64_state_t state;
    XXH64_reset(&state, seed);
    XXH64_update(&state, input, len);
    return XXH64_digest(&state);
#else
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

#  if !defined(XXH_USE_UNALIGNED_ACCESS)
    if ((((size_t)input) & 7)==0)   /* Input is aligned, let's leverage the speed advantage */
    {
        if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
            return XXH64_endian_align(input, len, seed, XXH_littleEndian, XXH_aligned);
        else
            return XXH64_endian_align(input, len, seed, XXH_bigEndian, XXH_aligned);
    }
#  endif

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH64_endian_align(input, len, seed, XXH_littleEndian, XXH_unaligned);
    else
        return XXH64_endian_align(input, len, seed, XXH_bigEndian, XXH_unaligned);
#endif
}

/****************************************************
 *  Advanced Hash Functions
****************************************************/

/*** Allocation ***/
typedef struct
{
    U64 total_len;
    U32 seed;
    U32 v1;
    U32 v2;
    U32 v3;
    U32 v4;
    U32 mem32[4];   /* defined as U32 for alignment */
    U32 memsize;
} XXH_istate32_t;

typedef struct
{
    U64 total_len;
    U64 seed;
    U64 v1;
    U64 v2;
    U64 v3;
    U64 v4;
    U64 mem64[4];   /* defined as U64 for alignment */
    U32 memsize;
} XXH_istate64_t;


XXH32_state_t* XXH32_createState(void)
{
    XXH_STATIC_ASSERT(sizeof(XXH32_state_t) >= sizeof(XXH_istate32_t));   /* A compilation error here means XXH32_state_t is not large enough */
    return (XXH32_state_t*)XXH_malloc(sizeof(XXH32_state_t));
}
XXH_errorcode XXH32_freeState(XXH32_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
}

XXH64_state_t* XXH64_createState(void)
{
    XXH_STATIC_ASSERT(sizeof(XXH64_state_t) >= sizeof(XXH_istate64_t));   /* A compilation error here means XXH64_state_t is not large enough */
    return (XXH64_state_t*)XXH_malloc(sizeof(XXH64_state_t));
}
XXH_errorcode XXH64_freeState(XXH64_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
}


/*** Hash feed ***/

XXH_errorcode XXH32_reset(XXH32_state_t* state_in, U32 seed)
{
    XXH_istate32_t* state = (XXH_istate32_t*) state_in;
    state->seed = seed;
    state->v1 = seed + PRIME32_1 + PRIME32_2;
    state->v2 = seed + PRIME32_2;
    state->v3 = seed + 0;
    state->v4 = seed - PRIME32_1;
    state->total_len = 0;
    state->memsize = 0;
    return XXH_OK;
}

XXH_errorcode XXH64_reset(XXH64_state_t* state_in, unsigned long long seed)
{
    XXH_istate64_t* state = (XXH_istate64_t*) state_in;
    state->seed = seed;
    state->v1 = seed + PRIME64_1 + PRIME64_2;
    state->v2 = seed + PRIME64_2;
    state->v3 = seed + 0;
    state->v4 = seed - PRIME64_1;
    state->total_len = 0;
    state->memsize = 0;
    return XXH_OK;
}


FORCE_INLINE XXH_errorcode XXH32_update_endian (XXH32_state_t* state_in, const void* input, size_t len, XXH_endianess endian)
{
    XXH_istate32_t* state = (XXH_istate32_t *) state_in;
    const BYTE* p = (const BYTE*)input;
    const BYTE* const bEnd = p + len;

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
    if (input==NULL) return XXH_ERROR;
#endif

    state->total_len += len;

    if (state->memsize + len < 16)   /* fill in tmp buffer */
    {
        XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, len);
        state->memsize += (U32)len;
        return XXH_OK;
    }

    if (state->memsize)   /* some data left from previous update */
    {
        XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, 16-state->memsize);
        {
            const U32* p32 = state->mem32;
            state->v1 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v1 = XXH_rotl32(state->v1, 13);
            state->v1 *= PRIME32_1;
            p32++;
            state->v2 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v2 = XXH_rotl32(state->v2, 13);
            state->v2 *= PRIME32_1;
            p32++;
            state->v3 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v3 = XXH_rotl32(state->v3, 13);
            state->v3 *= PRIME32_1;
            p32++;
            state->v4 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v4 = XXH_rotl32(state->v4, 13);
            state->v4 *= PRIME32_1;
            p32++;
        }
        p += 16-state->memsize;
        state->memsize = 0;
    }

    if (p <= bEnd-16)
    {
        const BYTE* const limit = bEnd - 16;
        U32 v1 = state->v1;
        U32 v2 = state->v2;
        U32 v3 = state->v3;
        U32 v4 = state->v4;

        do
        {
            v1 += XXH_readLE32(p, endian) * PRIME32_2;
            v1 = XXH_rotl32(v1, 13);
            v1 *= PRIME32_1;
            p+=4;
            v2 += XXH_readLE32(p, endian) * PRIME32_2;
            v2 = XXH_rotl32(v2, 13);
            v2 *= PRIME32_1;
            p+=4;
            v3 += XXH_readLE32(p, endian) * PRIME32_2;
            v3 = XXH_rotl32(v3, 13);
            v3 *= PRIME32_1;
            p+=4;
            v4 += XXH_readLE32(p, endian) * PRIME32_2;
            v4 = XXH_rotl32(v4, 13);
            v4 *= PRIME32_1;
            p+=4;
        }
        while (p<=limit);

        state->v1 = v1;
        state->v2 = v2;
        state->v3 = v3;
        state->v4 = v4;
    }

    if (p < bEnd)
    {
        XXH_memcpy(state->mem32, p, bEnd-p);
        state->memsize = (int)(bEnd-p);
    }

    return XXH_OK;
}

XXH_errorcode XXH32_update (XXH32_state_t* state_in, const void* input, size_t len)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH32_update_endian(state_in, input, len, XXH_littleEndian);
    else
        return XXH32_update_endian(state_in, input, len, XXH_bigEndian);
}



FORCE_INLINE U32 XXH32_digest_endian (const XXH32_state_t* state_in, XXH_endianess endian)
{
    XXH_istate32_t* state = (XXH_istate32_t*) state_in;
    const BYTE * p = (const BYTE*)state->mem32;
    BYTE* bEnd = (BYTE*)(state->mem32) + state->memsize;
    U32 h32;

    if (state->total_len >= 16)
    {
        h32 = XXH_rotl32(state->v1, 1) + XXH_rotl32(state->v2, 7) + XXH_rotl32(state->v3, 12) + XXH_rotl32(state->v4, 18);
    }
    else
    {
        h32  = state->seed + PRIME32_5;
    }

    h32 += (U32) state->total_len;

    while (p+4<=bEnd)
    {
        h32 += XXH_readLE32(p, endian) * PRIME32_3;
        h32  = XXH_rotl32(h32, 17) * PRIME32_4;
        p+=4;
    }

    while (p<bEnd)
    {
        h32 += (*p) * PRIME32_5;
        h32 = XXH_rotl32(h32, 11) * PRIME32_1;
        p++;
    }

    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}


U32 XXH32_digest (const XXH32_state_t* state_in)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH32_digest_endian(state_in, XXH_littleEndian);
    else
        return XXH32_digest_endian(state_in, XXH_bigEndian);
}


FORCE_INLINE XXH_errorcode XXH64_update_endian (XXH64_state_t* state_in, const void* input, size_t len, XXH_endianess endian)
{
    XXH_istate64_t * state = (XXH_istate64_t *) state_in;
    const BYTE* p = (const BYTE*)input;
    const BYTE* const bEnd = p + len;

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
    if (input==NULL) return XXH_ERROR;
#endif

    state->total_len += len;

    if (state->memsize + len < 32)   /* fill in tmp buffer */
    {
        XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, len);
        state->memsize += (U32)len;
        return XXH_OK;
    }

    if (state->memsize)   /* some data left from previous update */
    {
        XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, 32-state->memsize);
        {
            const U64* p64 = state->mem64;
            state->v1 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v1 = XXH_rotl64(state->v1, 31);
            state->v1 *= PRIME64_1;
            p64++;
            state->v2 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v2 = XXH_rotl64(state->v2, 31);
            state->v2 *= PRIME64_1;
            p64++;
            state->v3 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v3 = XXH_rotl64(state->v3, 31);
            state->v3 *= PRIME64_1;
            p64++;
            state->v4 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v4 = XXH_rotl64(state->v4, 31);
            state->v4 *= PRIME64_1;
            p64++;
        }
        p += 32-state->memsize;
        state->memsize = 0;
    }

    if (p+32 <= bEnd)
    {
        const BYTE* const limit = bEnd - 32;
        U64 v1 = state->v1;
        U64 v2 = state->v2;
        U64 v3 = state->v3;
        U64 v4 = state->v4;

        do
        {
            v1 += XXH_readLE64(p, endian) * PRIME64_2;
            v1 = XXH_rotl64(v1, 31);
            v1 *= PRIME64_1;
            p+=8;
            v2 += XXH_readLE64(p, endian) * PRIME64_2;
            v2 = XXH_rotl64(v2, 31);
            v2 *= PRIME64_1;
            p+=8;
            v3 += XXH_readLE64(p, endian) * PRIME64_2;
            v3 = XXH_rotl64(v3, 31);
            v3 *= PRIME64_1;
            p+=8;
            v4 += XXH_readLE64(p, endian) * PRIME64_2;
            v4 = XXH_rotl64(v4, 31);
            v4 *= PRIME64_1;
            p+=8;
        }
        while (p<=limit);

        state->v1 = v1;
        state->v2 = v2;
        state->v3 = v3;
        state->v4 = v4;
    }

    if (p < bEnd)
    {
        XXH_memcpy(state->mem64, p, bEnd-p);
        state->memsize = (int)(bEnd-p);
    }

    return XXH_OK;
}

XXH_errorcode XXH64_update (XXH64_state_t* state_in, const void* input, size_t len)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH64_update_endian(state_in, input, len, XXH_littleEndian);
    else
        return XXH64_update_endian(state_in, input, len, XXH_bigEndian);
}



FORCE_INLINE U64 XXH64_digest_endian (const XXH64_state_t* state_in, XXH_endianess endian)
{
    XXH_istate64_t * state = (XXH_istate64_t *) state_in;
    const BYTE * p = (const BYTE*)state->mem64;
    BYTE* bEnd = (BYTE*)state->mem64 + state->memsize;
    U64 h64;

    if (state->total_len >= 32)
    {
        U64 v1 = state->v1;
        U64 v2 = state->v2;
        U64 v3 = state->v3;
        U64 v4 = state->v4;

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

        v1 *= PRIME64_2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= PRIME64_1;
        h64 ^= v1;
        h64 = h64*PRIME64_1 + PRIME64_4;

        v2 *= PRIME64_2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= PRIME64_1;
        h64 ^= v2;
        h64 = h64*PRIME64_1 + PRIME64_4;

        v3 *= PRIME64_2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= PRIME64_1;
        h64 ^= v3;
        h64 = h64*PRIME64_1 + PRIME64_4;

        v4 *= PRIME64_2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= PRIME64_1;
        h64 ^= v4;
        h64 = h64*PRIME64_1 + PRIME64_4;
    }
    else
    {
        h64  = state->seed + PRIME64_5;
    }

    h64 += (U64) state->total_len;

    while (p+8<=bEnd)
    {
        U64 k1 = XXH_readLE64(p, endian);
        k1 *= PRIME64_2;
        k1 = XXH_rotl64(k1,31);
        k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64,27) * PRIME64_1 + PRIME64_4;
        p+=8;
    }

    if (p+4<=bEnd)
    {
        h64 ^= (U64)(XXH_readLE32(p, endian)) * PRIME64_1;
        h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p+=4;
    }

    while (p<bEnd)
    {
        h64 ^= (*p) * PRIME64_5;
        h64 = XXH_rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}


unsigned long long XXH64_digest (const XXH64_state_t* state_in)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH64_digest_endian(state_in, XXH_littleEndian);
    else
        return XXH64_digest_endian(state_in, XXH_bigEndian);
}

/* xxhash.c EOF */

/* lz4.c */

/**************************************
   Tuning parameters
**************************************/
/*
 * HEAPMODE :
 * Select how default compression functions will allocate memory for their hash table,
 * in memory stack (0:default, fastest), or in memory heap (1:requires malloc()).
 */
#define HEAPMODE 0

/*
 * CPU_HAS_EFFICIENT_UNALIGNED_MEMORY_ACCESS :
 * By default, the source code expects the compiler to correctly optimize
 * 4-bytes and 8-bytes read on architectures able to handle it efficiently.
 * This is not always the case. In some circumstances (ARM notably),
 * the compiler will issue cautious code even when target is able to correctly handle unaligned memory accesses.
 *
 * You can force the compiler to use unaligned memory access by uncommenting the line below.
 * One of the below scenarios will happen :
 * 1 - Your target CPU correctly handle unaligned access, and was not well optimized by compiler (good case).
 *     You will witness large performance improvements (+50% and up).
 *     Keep the line uncommented and send a word to upstream (https://groups.google.com/forum/#!forum/lz4c)
 *     The goal is to automatically detect such situations by adding your target CPU within an exception list.
 * 2 - Your target CPU correctly handle unaligned access, and was already already optimized by compiler
 *     No change will be experienced.
 * 3 - Your target CPU inefficiently handle unaligned access.
 *     You will experience a performance loss. Comment back the line.
 * 4 - Your target CPU does not handle unaligned access.
 *     Program will crash.
 * If uncommenting results in better performance (case 1)
 * please report your configuration to upstream (https://groups.google.com/forum/#!forum/lz4c)
 * This way, an automatic detection macro can be added to match your case within later versions of the library.
 */
/* #define CPU_HAS_EFFICIENT_UNALIGNED_MEMORY_ACCESS 1 */


/**************************************
   CPU Feature Detection
**************************************/
/*
 * Automated efficient unaligned memory access detection
 * Based on known hardware architectures
 * This list will be updated thanks to feedbacks
 */
#if defined(CPU_HAS_EFFICIENT_UNALIGNED_MEMORY_ACCESS) \
    || defined(__ARM_FEATURE_UNALIGNED) \
    || defined(__i386__) || defined(__x86_64__) \
    || defined(_M_IX86) || defined(_M_X64) \
    || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_8__) \
    || (defined(_M_ARM) && (_M_ARM >= 7))
#  define LZ4_UNALIGNED_ACCESS 1
#else
#  define LZ4_UNALIGNED_ACCESS 0
#endif

/*
 * LZ4_FORCE_SW_BITCOUNT
 * Define this parameter if your target system or compiler does not support hardware bit count
 */
#if defined(_MSC_VER) && defined(_WIN32_WCE)   /* Visual Studio for Windows CE does not support Hardware bit count */
#  define LZ4_FORCE_SW_BITCOUNT
#endif


/**************************************
*  Compiler Options
**************************************/
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
/* "restrict" is a known keyword */
#else
#  define restrict /* Disable restrict */
#endif

#if 0
#ifdef _MSC_VER    /* Visual Studio */
#  define FORCE_INLINE static __forceinline
#  include <intrin.h>
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#  pragma warning(disable : 4293)        /* disable: C4293: too large shift (32-bits) */
#else
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
#    ifdef __GNUC__
#      define FORCE_INLINE static inline __attribute__((always_inline))
#    else
#      define FORCE_INLINE static inline
#    endif
#  else
#    define FORCE_INLINE static
#  endif   /* __STDC_VERSION__ */
#endif  /* _MSC_VER */
#endif

#ifndef GCC_VERSION
# define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#endif

#if (GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
#  define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
#  define expect(expr,value)    (expr)
#endif

#define likely(expr)     expect((expr) != 0, 1)
#define unlikely(expr)   expect((expr) != 0, 0)


/**************************************
   Memory routines
**************************************/
#define ALLOCATOR2(n,s) calloc(n,s)
#define FREEMEM2        free
#define MEM_INIT       memset


#if 0
/**************************************
   Basic Types
**************************************/
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
# include <stdint.h>
  typedef  uint8_t BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      U16;
  typedef unsigned int        U32;
  typedef   signed int        S32;
  typedef unsigned long long  U64;
#endif
#endif

/**************************************
   Reading and writing into memory
**************************************/
#define STEPSIZE sizeof(size_t)

static unsigned LZ4_64bits(void) { return sizeof(void*)==8; }

static unsigned LZ4_isLittleEndian(void)
{
    const union { U32 i; BYTE c[4]; } one = { 1 };   /* don't use static : performance detrimental  */
    return one.c[0];
}


static U16 LZ4_readLE16(const void* memPtr)
{
    if ((LZ4_UNALIGNED_ACCESS) && (LZ4_isLittleEndian()))
        return *(U16*)memPtr;
    else
    {
        const BYTE* p = (const BYTE*)memPtr;
        return (U16)((U16)p[0] + (p[1]<<8));
    }
}

static void LZ4_writeLE16(void* memPtr, U16 value)
{
    if ((LZ4_UNALIGNED_ACCESS) && (LZ4_isLittleEndian()))
    {
        *(U16*)memPtr = value;
        return;
    }
    else
    {
        BYTE* p = (BYTE*)memPtr;
        p[0] = (BYTE) value;
        p[1] = (BYTE)(value>>8);
    }
}


static U16 LZ4_read16(const void* memPtr)
{
    if (LZ4_UNALIGNED_ACCESS)
        return *(U16*)memPtr;
    else
    {
        U16 val16;
        memcpy(&val16, memPtr, 2);
        return val16;
    }
}

static U32 LZ4_read32(const void* memPtr)
{
    if (LZ4_UNALIGNED_ACCESS)
        return *(U32*)memPtr;
    else
    {
        U32 val32;
        memcpy(&val32, memPtr, 4);
        return val32;
    }
}

static U64 LZ4_read64(const void* memPtr)
{
    if (LZ4_UNALIGNED_ACCESS)
        return *(U64*)memPtr;
    else
    {
        U64 val64;
        memcpy(&val64, memPtr, 8);
        return val64;
    }
}

static size_t LZ4_read_ARCH(const void* p)
{
    if (LZ4_64bits())
        return (size_t)LZ4_read64(p);
    else
        return (size_t)LZ4_read32(p);
}


static void LZ4_copy4(void* dstPtr, const void* srcPtr)
{
    if (LZ4_UNALIGNED_ACCESS)
    {
        *(U32*)dstPtr = *(U32*)srcPtr;
        return;
    }
    memcpy(dstPtr, srcPtr, 4);
}

static void LZ4_copy8(void* dstPtr, const void* srcPtr)
{
#if GCC_VERSION!=409  /* disabled on GCC 4.9, as it generates invalid opcode (crash) */
    if (LZ4_UNALIGNED_ACCESS)
    {
        if (LZ4_64bits())
            *(U64*)dstPtr = *(U64*)srcPtr;
        else
            ((U32*)dstPtr)[0] = ((U32*)srcPtr)[0],
            ((U32*)dstPtr)[1] = ((U32*)srcPtr)[1];
        return;
    }
#endif
    memcpy(dstPtr, srcPtr, 8);
}

/* customized version of memcpy, which may overwrite up to 7 bytes beyond dstEnd */
static void LZ4_wildCopy(void* dstPtr, const void* srcPtr, void* dstEnd)
{
    BYTE* d = (BYTE*)dstPtr;
    const BYTE* s = (const BYTE*)srcPtr;
    BYTE* e = (BYTE*)dstEnd;
    do { LZ4_copy8(d,s); d+=8; s+=8; } while (d<e);
}


/**************************************
   Common Constants
**************************************/
#define MINMATCH 4

#define COPYLENGTH 8
#define LASTLITERALS 5
#define MFLIMIT (COPYLENGTH+MINMATCH)
static const int LZ4_minLength = (MFLIMIT+1);

#define KB *(1 <<10)
#define MB *(1 <<20)
#define GB *(1U<<30)

#define MAXD_LOG 16
#define MAX_DISTANCE ((1 << MAXD_LOG) - 1)

#define ML_BITS  4
#define ML_MASK  ((1U<<ML_BITS)-1)
#define RUN_BITS (8-ML_BITS)
#define RUN_MASK ((1U<<RUN_BITS)-1)


/**************************************
*  Common Utils
**************************************/
#define LZ4_STATIC_ASSERT(c)    { enum { LZ4_static_assert = 1/(int)(!!(c)) }; }   /* use only *after* variable declarations */


/**************************************
*  Common functions
**************************************/
static unsigned LZ4_NbCommonBytes (register size_t val)
{
    if (LZ4_isLittleEndian())
    {
        if (LZ4_64bits())
        {
#       if defined(_MSC_VER) && defined(_WIN64) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r = 0;
            _BitScanForward64( &r, (U64)val );
            return (int)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_ctzll((U64)val) >> 3);
#       else
            static const int DeBruijnBytePos[64] = { 0, 0, 0, 0, 0, 1, 1, 2, 0, 3, 1, 3, 1, 4, 2, 7, 0, 2, 3, 6, 1, 5, 3, 5, 1, 3, 4, 4, 2, 5, 6, 7, 7, 0, 1, 2, 3, 3, 4, 6, 2, 6, 5, 5, 3, 4, 5, 6, 7, 1, 2, 4, 6, 4, 4, 5, 7, 2, 6, 5, 7, 6, 7, 7 };
            return DeBruijnBytePos[((U64)((val & -(long long)val) * 0x0218A392CDABBD3FULL)) >> 58];
#       endif
        }
        else /* 32 bits */
        {
#       if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r;
            _BitScanForward( &r, (U32)val );
            return (int)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_ctz((U32)val) >> 3);
#       else
            static const int DeBruijnBytePos[32] = { 0, 0, 3, 0, 3, 1, 3, 0, 3, 2, 2, 1, 3, 2, 0, 1, 3, 3, 1, 2, 2, 2, 2, 0, 3, 1, 2, 0, 1, 0, 1, 1 };
            return DeBruijnBytePos[((U32)((val & -(S32)val) * 0x077CB531U)) >> 27];
#       endif
        }
    }
    else   /* Big Endian CPU */
    {
        if (LZ4_64bits())
        {
#       if defined(_MSC_VER) && defined(_WIN64) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r = 0;
            _BitScanReverse64( &r, val );
            return (unsigned)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_clzll(val) >> 3);
#       else
            unsigned r;
            if (!(val>>32)) { r=4; } else { r=0; val>>=32; }
            if (!(val>>16)) { r+=2; val>>=8; } else { val>>=24; }
            r += (!val);
            return r;
#       endif
        }
        else /* 32 bits */
        {
#       if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r = 0;
            _BitScanReverse( &r, (unsigned long)val );
            return (unsigned)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_clz(val) >> 3);
#       else
            unsigned r;
            if (!(val>>16)) { r=2; val>>=8; } else { r=0; val>>=24; }
            r += (!val);
            return r;
#       endif
        }
    }
}

static unsigned LZ4_count(const BYTE* pIn, const BYTE* pMatch, const BYTE* pInLimit)
{
    const BYTE* const pStart = pIn;

    while (likely(pIn<pInLimit-(STEPSIZE-1)))
    {
        size_t diff = LZ4_read_ARCH(pMatch) ^ LZ4_read_ARCH(pIn);
        if (!diff) { pIn+=STEPSIZE; pMatch+=STEPSIZE; continue; }
        pIn += LZ4_NbCommonBytes(diff);
        return (unsigned)(pIn - pStart);
    }

    if (LZ4_64bits()) if ((pIn<(pInLimit-3)) && (LZ4_read32(pMatch) == LZ4_read32(pIn))) { pIn+=4; pMatch+=4; }
    if ((pIn<(pInLimit-1)) && (LZ4_read16(pMatch) == LZ4_read16(pIn))) { pIn+=2; pMatch+=2; }
    if ((pIn<pInLimit) && (*pMatch == *pIn)) pIn++;
    return (unsigned)(pIn - pStart);
}


#ifndef LZ4_COMMONDEFS_ONLY
/**************************************
*  Local Constants
**************************************/
#define LZ4_HASHLOG   (LZ4_MEMORY_USAGE-2)
#define HASHTABLESIZE (1 << LZ4_MEMORY_USAGE)
#define HASH_SIZE_U32 (1 << LZ4_HASHLOG)       /* required as macro for static allocation */

static const int LZ4_64Klimit = ((64 KB) + (MFLIMIT-1));
static const U32 LZ4_skipTrigger = 6;  /* Increase this value ==> compression run slower on incompressible data */


/**************************************
*  Local Utils
**************************************/
int LZ4_versionNumber (void) { return LZ4_VERSION_NUMBER; }
int LZ4_compressBound(int isize)  { return LZ4_COMPRESSBOUND(isize); }


/**************************************
*  Local Structures and types
**************************************/
typedef struct {
    U32 hashTable[HASH_SIZE_U32];
    U32 currentOffset;
    U32 initCheck;
    const BYTE* dictionary;
    const BYTE* bufferStart;
    U32 dictSize;
} LZ4_stream_t_internal;

typedef enum { notLimited = 0, limitedOutput = 1 } limitedOutput_directive;
typedef enum { byPtr, byU32, byU16 } tableType_t;

typedef enum { noDict = 0, withPrefix64k, usingExtDict } dict_directive;
typedef enum { noDictIssue = 0, dictSmall } dictIssue_directive;

typedef enum { endOnOutputSize = 0, endOnInputSize = 1 } endCondition_directive;
typedef enum { full = 0, partial = 1 } earlyEnd_directive;



/********************************
*  Compression functions
********************************/

static U32 LZ4_hashSequence(U32 sequence, tableType_t const tableType)
{
    if (tableType == byU16)
        return (((sequence) * 2654435761U) >> ((MINMATCH*8)-(LZ4_HASHLOG+1)));
    else
        return (((sequence) * 2654435761U) >> ((MINMATCH*8)-LZ4_HASHLOG));
}

static U32 LZ4_hashPosition(const BYTE* p, tableType_t tableType) { return LZ4_hashSequence(LZ4_read32(p), tableType); }

static void LZ4_putPositionOnHash(const BYTE* p, U32 h, void* tableBase, tableType_t const tableType, const BYTE* srcBase)
{
    switch (tableType)
    {
    case byPtr: { const BYTE** hashTable = (const BYTE**)tableBase; hashTable[h] = p; return; }
    case byU32: { U32* hashTable = (U32*) tableBase; hashTable[h] = (U32)(p-srcBase); return; }
    case byU16: { U16* hashTable = (U16*) tableBase; hashTable[h] = (U16)(p-srcBase); return; }
    }
}

static void LZ4_putPosition(const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
    U32 h = LZ4_hashPosition(p, tableType);
    LZ4_putPositionOnHash(p, h, tableBase, tableType, srcBase);
}

static const BYTE* LZ4_getPositionOnHash(U32 h, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
    if (tableType == byPtr) { const BYTE** hashTable = (const BYTE**) tableBase; return hashTable[h]; }
    if (tableType == byU32) { U32* hashTable = (U32*) tableBase; return hashTable[h] + srcBase; }
    { U16* hashTable = (U16*) tableBase; return hashTable[h] + srcBase; }   /* default, to ensure a return */
}

static const BYTE* LZ4_getPosition(const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase)
{
    U32 h = LZ4_hashPosition(p, tableType);
    return LZ4_getPositionOnHash(h, tableBase, tableType, srcBase);
}

static int LZ4_compress_generic(
                 void* ctx,
                 const char* source,
                 char* dest,
                 int inputSize,
                 int maxOutputSize,
                 limitedOutput_directive outputLimited,
                 tableType_t const tableType,
                 dict_directive dict,
                 dictIssue_directive dictIssue)
{
    LZ4_stream_t_internal* const dictPtr = (LZ4_stream_t_internal*)ctx;

    const BYTE* ip = (const BYTE*) source;
    const BYTE* base;
    const BYTE* lowLimit;
    const BYTE* const lowRefLimit = ip - dictPtr->dictSize;
    const BYTE* const dictionary = dictPtr->dictionary;
    const BYTE* const dictEnd = dictionary + dictPtr->dictSize;
    const size_t dictDelta = dictEnd - (const BYTE*)source;
    const BYTE* anchor = (const BYTE*) source;
    const BYTE* const iend = ip + inputSize;
    const BYTE* const mflimit = iend - MFLIMIT;
    const BYTE* const matchlimit = iend - LASTLITERALS;

    BYTE* op = (BYTE*) dest;
    BYTE* const olimit = op + maxOutputSize;

    U32 forwardH;
    size_t refDelta=0;

    /* Init conditions */
    if ((U32)inputSize > (U32)LZ4_MAX_INPUT_SIZE) return 0;          /* Unsupported input size, too large (or negative) */
    switch(dict)
    {
    case noDict:
    default:
        base = (const BYTE*)source;
        lowLimit = (const BYTE*)source;
        break;
    case withPrefix64k:
        base = (const BYTE*)source - dictPtr->currentOffset;
        lowLimit = (const BYTE*)source - dictPtr->dictSize;
        break;
    case usingExtDict:
        base = (const BYTE*)source - dictPtr->currentOffset;
        lowLimit = (const BYTE*)source;
        break;
    }
    if ((tableType == byU16) && (inputSize>=LZ4_64Klimit)) return 0;   /* Size too large (not within 64K limit) */
    if (inputSize<LZ4_minLength) goto _last_literals;                  /* Input too small, no compression (all literals) */

    /* First Byte */
    LZ4_putPosition(ip, ctx, tableType, base);
    ip++; forwardH = LZ4_hashPosition(ip, tableType);

    /* Main Loop */
    for ( ; ; )
    {
        const BYTE* match;
        BYTE* token;
        {
            const BYTE* forwardIp = ip;
            unsigned step=1;
            unsigned searchMatchNb = (1U << LZ4_skipTrigger);

            /* Find a match */
            do {
                U32 h = forwardH;
                ip = forwardIp;
                forwardIp += step;
                step = searchMatchNb++ >> LZ4_skipTrigger;

                if (unlikely(forwardIp > mflimit)) goto _last_literals;

                match = LZ4_getPositionOnHash(h, ctx, tableType, base);
                if (dict==usingExtDict)
                {
                    if (match<(const BYTE*)source)
                    {
                        refDelta = dictDelta;
                        lowLimit = dictionary;
                    }
                    else
                    {
                        refDelta = 0;
                        lowLimit = (const BYTE*)source;
                    }
                }
                forwardH = LZ4_hashPosition(forwardIp, tableType);
                LZ4_putPositionOnHash(ip, h, ctx, tableType, base);

            } while ( ((dictIssue==dictSmall) ? (match < lowRefLimit) : 0)
                || ((tableType==byU16) ? 0 : (match + MAX_DISTANCE < ip))
                || (LZ4_read32(match+refDelta) != LZ4_read32(ip)) );
        }

        /* Catch up */
        while ((ip>anchor) && (match+refDelta > lowLimit) && (unlikely(ip[-1]==match[refDelta-1]))) { ip--; match--; }

        {
            /* Encode Literal length */
            unsigned litLength = (unsigned)(ip - anchor);
            token = op++;
            if ((outputLimited) && (unlikely(op + litLength + (2 + 1 + LASTLITERALS) + (litLength/255) > olimit)))
                return 0;   /* Check output limit */
            if (litLength>=RUN_MASK)
            {
                int len = (int)litLength-RUN_MASK;
                *token=(RUN_MASK<<ML_BITS);
                for(; len >= 255 ; len-=255) *op++ = 255;
                *op++ = (BYTE)len;
            }
            else *token = (BYTE)(litLength<<ML_BITS);

            /* Copy Literals */
            LZ4_wildCopy(op, anchor, op+litLength);
            op+=litLength;
        }

_next_match:
        /* Encode Offset */
        LZ4_writeLE16(op, (U16)(ip-match)); op+=2;

        /* Encode MatchLength */
        {
            unsigned matchLength;

            if ((dict==usingExtDict) && (lowLimit==dictionary))
            {
                const BYTE* limit;
                match += refDelta;
                limit = ip + (dictEnd-match);
                if (limit > matchlimit) limit = matchlimit;
                matchLength = LZ4_count(ip+MINMATCH, match+MINMATCH, limit);
                ip += MINMATCH + matchLength;
                if (ip==limit)
                {
                    unsigned more = LZ4_count(ip, (const BYTE*)source, matchlimit);
                    matchLength += more;
                    ip += more;
                }
            }
            else
            {
                matchLength = LZ4_count(ip+MINMATCH, match+MINMATCH, matchlimit);
                ip += MINMATCH + matchLength;
            }

            if ((outputLimited) && (unlikely(op + (1 + LASTLITERALS) + (matchLength>>8) > olimit)))
                return 0;    /* Check output limit */
            if (matchLength>=ML_MASK)
            {
                *token += ML_MASK;
                matchLength -= ML_MASK;
                for (; matchLength >= 510 ; matchLength-=510) { *op++ = 255; *op++ = 255; }
                if (matchLength >= 255) { matchLength-=255; *op++ = 255; }
                *op++ = (BYTE)matchLength;
            }
            else *token += (BYTE)(matchLength);
        }

        anchor = ip;

        /* Test end of chunk */
        if (ip > mflimit) break;

        /* Fill table */
        LZ4_putPosition(ip-2, ctx, tableType, base);

        /* Test next position */
        match = LZ4_getPosition(ip, ctx, tableType, base);
        if (dict==usingExtDict)
        {
            if (match<(const BYTE*)source)
            {
                refDelta = dictDelta;
                lowLimit = dictionary;
            }
            else
            {
                refDelta = 0;
                lowLimit = (const BYTE*)source;
            }
        }
        LZ4_putPosition(ip, ctx, tableType, base);
        if ( ((dictIssue==dictSmall) ? (match>=lowRefLimit) : 1)
            && (match+MAX_DISTANCE>=ip)
            && (LZ4_read32(match+refDelta)==LZ4_read32(ip)) )
        { token=op++; *token=0; goto _next_match; }

        /* Prepare next loop */
        forwardH = LZ4_hashPosition(++ip, tableType);
    }

_last_literals:
    /* Encode Last Literals */
    {
        int lastRun = (int)(iend - anchor);
        if ((outputLimited) && (((char*)op - dest) + lastRun + 1 + ((lastRun+255-RUN_MASK)/255) > (U32)maxOutputSize))
            return 0;   /* Check output limit */
        if (lastRun>=(int)RUN_MASK) { *op++=(RUN_MASK<<ML_BITS); lastRun-=RUN_MASK; for(; lastRun >= 255 ; lastRun-=255) *op++ = 255; *op++ = (BYTE) lastRun; }
        else *op++ = (BYTE)(lastRun<<ML_BITS);
        memcpy(op, anchor, iend - anchor);
        op += iend-anchor;
    }

    /* End */
    return (int) (((char*)op)-dest);
}


int LZ4_compress(const char* source, char* dest, int inputSize)
{
#if (HEAPMODE)
    void* ctx = ALLOCATOR2(LZ4_STREAMSIZE_U64, 8);   /* Aligned on 8-bytes boundaries */
#else
    U64 ctx[LZ4_STREAMSIZE_U64] = {0};      /* Ensure data is aligned on 8-bytes boundaries */
#endif
    int result;

    if (inputSize < LZ4_64Klimit)
        result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, 0, notLimited, byU16, noDict, noDictIssue);
    else
        result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, 0, notLimited, LZ4_64bits() ? byU32 : byPtr, noDict, noDictIssue);

#if (HEAPMODE)
    FREEMEM2(ctx);
#endif
    return result;
}

int LZ4_compress_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize)
{
#if (HEAPMODE)
    void* ctx = ALLOCATOR2(LZ4_STREAMSIZE_U64, 8);   /* Aligned on 8-bytes boundaries */
#else
    U64 ctx[LZ4_STREAMSIZE_U64] = {0};      /* Ensure data is aligned on 8-bytes boundaries */
#endif
    int result;

    if (inputSize < LZ4_64Klimit)
        result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, maxOutputSize, limitedOutput, byU16, noDict, noDictIssue);
    else
        result = LZ4_compress_generic((void*)ctx, source, dest, inputSize, maxOutputSize, limitedOutput, LZ4_64bits() ? byU32 : byPtr, noDict, noDictIssue);

#if (HEAPMODE)
    FREEMEM2(ctx);
#endif
    return result;
}


/*****************************************
*  Experimental : Streaming functions
*****************************************/

/*
 * LZ4_initStream
 * Use this function once, to init a newly allocated LZ4_stream_t structure
 * Return : 1 if OK, 0 if error
 */
void LZ4_resetStream (LZ4_stream_t* LZ4_stream)
{
    MEM_INIT(LZ4_stream, 0, sizeof(LZ4_stream_t));
}

LZ4_stream_t* LZ4_createStream(void)
{
    LZ4_stream_t* lz4s = (LZ4_stream_t*)ALLOCATOR2(8, LZ4_STREAMSIZE_U64);
    LZ4_STATIC_ASSERT(LZ4_STREAMSIZE >= sizeof(LZ4_stream_t_internal));    /* A compilation error here means LZ4_STREAMSIZE is not large enough */
    LZ4_resetStream(lz4s);
    return lz4s;
}

int LZ4_freeStream (LZ4_stream_t* LZ4_stream)
{
    FREEMEM2(LZ4_stream);
    return (0);
}


int LZ4_loadDict (LZ4_stream_t* LZ4_dict, const char* dictionary, int dictSize)
{
    LZ4_stream_t_internal* dict = (LZ4_stream_t_internal*) LZ4_dict;
    const BYTE* p = (const BYTE*)dictionary;
    const BYTE* const dictEnd = p + dictSize;
    const BYTE* base;

    if (dict->initCheck) LZ4_resetStream(LZ4_dict);                         /* Uninitialized structure detected */

    if (dictSize < MINMATCH)
    {
        dict->dictionary = NULL;
        dict->dictSize = 0;
        return 0;
    }

    if (p <= dictEnd - 64 KB) p = dictEnd - 64 KB;
    base = p - dict->currentOffset;
    dict->dictionary = p;
    dict->dictSize = (U32)(dictEnd - p);
    dict->currentOffset += dict->dictSize;

    while (p <= dictEnd-MINMATCH)
    {
        LZ4_putPosition(p, dict, byU32, base);
        p+=3;
    }

    return dict->dictSize;
}


static void LZ4_renormDictT(LZ4_stream_t_internal* LZ4_dict, const BYTE* src)
{
    if ((LZ4_dict->currentOffset > 0x80000000) ||
        ((size_t)LZ4_dict->currentOffset > (size_t)src))   /* address space overflow */
    {
        /* rescale hash table */
        U32 delta = LZ4_dict->currentOffset - 64 KB;
        const BYTE* dictEnd = LZ4_dict->dictionary + LZ4_dict->dictSize;
        int i;
        for (i=0; i<HASH_SIZE_U32; i++)
        {
            if (LZ4_dict->hashTable[i] < delta) LZ4_dict->hashTable[i]=0;
            else LZ4_dict->hashTable[i] -= delta;
        }
        LZ4_dict->currentOffset = 64 KB;
        if (LZ4_dict->dictSize > 64 KB) LZ4_dict->dictSize = 64 KB;
        LZ4_dict->dictionary = dictEnd - LZ4_dict->dictSize;
    }
}


FORCE_INLINE int LZ4_compress_continue_generic (void* LZ4_stream, const char* source, char* dest, int inputSize,
                                                int maxOutputSize, limitedOutput_directive limit)
{
    LZ4_stream_t_internal* streamPtr = (LZ4_stream_t_internal*)LZ4_stream;
    const BYTE* const dictEnd = streamPtr->dictionary + streamPtr->dictSize;

    const BYTE* smallest = (const BYTE*) source;
    if (streamPtr->initCheck) return 0;   /* Uninitialized structure detected */
    if ((streamPtr->dictSize>0) && (smallest>dictEnd)) smallest = dictEnd;
    LZ4_renormDictT(streamPtr, smallest);

    /* Check overlapping input/dictionary space */
    {
        const BYTE* sourceEnd = (const BYTE*) source + inputSize;
        if ((sourceEnd > streamPtr->dictionary) && (sourceEnd < dictEnd))
        {
            streamPtr->dictSize = (U32)(dictEnd - sourceEnd);
            if (streamPtr->dictSize > 64 KB) streamPtr->dictSize = 64 KB;
            if (streamPtr->dictSize < 4) streamPtr->dictSize = 0;
            streamPtr->dictionary = dictEnd - streamPtr->dictSize;
        }
    }

    /* prefix mode : source data follows dictionary */
    if (dictEnd == (const BYTE*)source)
    {
        int result;
        if ((streamPtr->dictSize < 64 KB) && (streamPtr->dictSize < streamPtr->currentOffset))
            result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, withPrefix64k, dictSmall);
        else
            result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, withPrefix64k, noDictIssue);
        streamPtr->dictSize += (U32)inputSize;
        streamPtr->currentOffset += (U32)inputSize;
        return result;
    }

    /* external dictionary mode */
    {
        int result;
        if ((streamPtr->dictSize < 64 KB) && (streamPtr->dictSize < streamPtr->currentOffset))
            result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, usingExtDict, dictSmall);
        else
            result = LZ4_compress_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limit, byU32, usingExtDict, noDictIssue);
        streamPtr->dictionary = (const BYTE*)source;
        streamPtr->dictSize = (U32)inputSize;
        streamPtr->currentOffset += (U32)inputSize;
        return result;
    }
}

int LZ4_compress_continue (LZ4_stream_t* LZ4_stream, const char* source, char* dest, int inputSize)
{
    return LZ4_compress_continue_generic(LZ4_stream, source, dest, inputSize, 0, notLimited);
}

int LZ4_compress_limitedOutput_continue (LZ4_stream_t* LZ4_stream, const char* source, char* dest, int inputSize, int maxOutputSize)
{
    return LZ4_compress_continue_generic(LZ4_stream, source, dest, inputSize, maxOutputSize, limitedOutput);
}


/* Hidden debug function, to force separate dictionary mode */
int LZ4_compress_forceExtDict (LZ4_stream_t* LZ4_dict, const char* source, char* dest, int inputSize)
{
    LZ4_stream_t_internal* streamPtr = (LZ4_stream_t_internal*)LZ4_dict;
    int result;
    const BYTE* const dictEnd = streamPtr->dictionary + streamPtr->dictSize;

    const BYTE* smallest = dictEnd;
    if (smallest > (const BYTE*) source) smallest = (const BYTE*) source;
    LZ4_renormDictT((LZ4_stream_t_internal*)LZ4_dict, smallest);

    result = LZ4_compress_generic(LZ4_dict, source, dest, inputSize, 0, notLimited, byU32, usingExtDict, noDictIssue);

    streamPtr->dictionary = (const BYTE*)source;
    streamPtr->dictSize = (U32)inputSize;
    streamPtr->currentOffset += (U32)inputSize;

    return result;
}


int LZ4_saveDict (LZ4_stream_t* LZ4_dict, char* safeBuffer, int dictSize)
{
    LZ4_stream_t_internal* dict = (LZ4_stream_t_internal*) LZ4_dict;
    const BYTE* previousDictEnd = dict->dictionary + dict->dictSize;

    if ((U32)dictSize > 64 KB) dictSize = 64 KB;   /* useless to define a dictionary > 64 KB */
    if ((U32)dictSize > dict->dictSize) dictSize = dict->dictSize;

    memmove(safeBuffer, previousDictEnd - dictSize, dictSize);

    dict->dictionary = (const BYTE*)safeBuffer;
    dict->dictSize = (U32)dictSize;

    return dictSize;
}



/*******************************
*  Decompression functions
*******************************/
/*
 * This generic decompression function cover all use cases.
 * It shall be instantiated several times, using different sets of directives
 * Note that it is essential this generic function is really inlined,
 * in order to remove useless branches during compilation optimization.
 */
FORCE_INLINE int LZ4_decompress_generic(
                 const char* const source,
                 char* const dest,
                 int inputSize,
                 int outputSize,         /* If endOnInput==endOnInputSize, this value is the max size of Output Buffer. */

                 int endOnInput,         /* endOnOutputSize, endOnInputSize */
                 int partialDecoding,    /* full, partial */
                 int targetOutputSize,   /* only used if partialDecoding==partial */
                 int dict,               /* noDict, withPrefix64k, usingExtDict */
                 const BYTE* const lowPrefix,  /* == dest if dict == noDict */
                 const BYTE* const dictStart,  /* only if dict==usingExtDict */
                 const size_t dictSize         /* note : = 0 if noDict */
                 )
{
    /* Local Variables */
    const BYTE* restrict ip = (const BYTE*) source;
    const BYTE* const iend = ip + inputSize;

    BYTE* op = (BYTE*) dest;
    BYTE* const oend = op + outputSize;
    BYTE* cpy;
    BYTE* oexit = op + targetOutputSize;
    const BYTE* const lowLimit = lowPrefix - dictSize;

    const BYTE* const dictEnd = (const BYTE*)dictStart + dictSize;
    const size_t dec32table[] = {4, 1, 2, 1, 4, 4, 4, 4};
    const size_t dec64table[] = {0, 0, 0, (size_t)-1, 0, 1, 2, 3};

    const int safeDecode = (endOnInput==endOnInputSize);
    const int checkOffset = ((safeDecode) && (dictSize < (int)(64 KB)));


    /* Special cases */
    if ((partialDecoding) && (oexit> oend-MFLIMIT)) oexit = oend-MFLIMIT;                         /* targetOutputSize too high => decode everything */
    if ((endOnInput) && (unlikely(outputSize==0))) return ((inputSize==1) && (*ip==0)) ? 0 : -1;  /* Empty output buffer */
    if ((!endOnInput) && (unlikely(outputSize==0))) return (*ip==0?1:-1);


    /* Main Loop */
    while (1)
    {
        unsigned token;
        size_t length;
        const BYTE* match;

        /* get literal length */
        token = *ip++;
        if ((length=(token>>ML_BITS)) == RUN_MASK)
        {
            unsigned s;
            do
            {
                s = *ip++;
                length += s;
            }
            while (likely((endOnInput)?ip<iend-RUN_MASK:1) && (s==255));
            if ((safeDecode) && unlikely((size_t)(op+length)<(size_t)(op))) goto _output_error;   /* overflow detection */
            if ((safeDecode) && unlikely((size_t)(ip+length)<(size_t)(ip))) goto _output_error;   /* overflow detection */
        }

        /* copy literals */
        cpy = op+length;
        if (((endOnInput) && ((cpy>(partialDecoding?oexit:oend-MFLIMIT)) || (ip+length>iend-(2+1+LASTLITERALS))) )
            || ((!endOnInput) && (cpy>oend-COPYLENGTH)))
        {
            if (partialDecoding)
            {
                if (cpy > oend) goto _output_error;                           /* Error : write attempt beyond end of output buffer */
                if ((endOnInput) && (ip+length > iend)) goto _output_error;   /* Error : read attempt beyond end of input buffer */
            }
            else
            {
                if ((!endOnInput) && (cpy != oend)) goto _output_error;       /* Error : block decoding must stop exactly there */
                if ((endOnInput) && ((ip+length != iend) || (cpy > oend))) goto _output_error;   /* Error : input must be consumed */
            }
            memcpy(op, ip, length);
            ip += length;
            op += length;
            break;     /* Necessarily EOF, due to parsing restrictions */
        }
        LZ4_wildCopy(op, ip, cpy);
        ip += length; op = cpy;

        /* get offset */
        match = cpy - LZ4_readLE16(ip); ip+=2;
        if ((checkOffset) && (unlikely(match < lowLimit))) goto _output_error;   /* Error : offset outside destination buffer */

        /* get matchlength */
        length = token & ML_MASK;
        if (length == ML_MASK)
        {
            unsigned s;
            do
            {
                if ((endOnInput) && (ip > iend-LASTLITERALS)) goto _output_error;
                s = *ip++;
                length += s;
            } while (s==255);
            if ((safeDecode) && unlikely((size_t)(op+length)<(size_t)op)) goto _output_error;   /* overflow detection */
        }
        length += MINMATCH;

        /* check external dictionary */
        if ((dict==usingExtDict) && (match < lowPrefix))
        {
            if (unlikely(op+length > oend-LASTLITERALS)) goto _output_error;   /* doesn't respect parsing restriction */

            if (length <= (size_t)(lowPrefix-match))
            {
                /* match can be copied as a single segment from external dictionary */
                match = dictEnd - (lowPrefix-match);
                memcpy(op, match, length);
                op += length;
            }
            else
            {
                /* match encompass external dictionary and current segment */
                size_t copySize = (size_t)(lowPrefix-match);
                memcpy(op, dictEnd - copySize, copySize);
                op += copySize;
                copySize = length - copySize;
                if (copySize > (size_t)(op-lowPrefix))   /* overlap within current segment */
                {
                    BYTE* const endOfMatch = op + copySize;
                    const BYTE* copyFrom = lowPrefix;
                    while (op < endOfMatch) *op++ = *copyFrom++;
                }
                else
                {
                    memcpy(op, lowPrefix, copySize);
                    op += copySize;
                }
            }
            continue;
        }

        /* copy repeated sequence */
        cpy = op + length;
        if (unlikely((op-match)<8))
        {
            const size_t dec64 = dec64table[op-match];
            op[0] = match[0];
            op[1] = match[1];
            op[2] = match[2];
            op[3] = match[3];
            match += dec32table[op-match];
            LZ4_copy4(op+4, match);
            op += 8; match -= dec64;
        } else { LZ4_copy8(op, match); op+=8; match+=8; }

        if (unlikely(cpy>oend-12))
        {
            if (cpy > oend-LASTLITERALS) goto _output_error;    /* Error : last LASTLITERALS bytes must be literals */
            if (op < oend-8)
            {
                LZ4_wildCopy(op, match, oend-8);
                match += (oend-8) - op;
                op = oend-8;
            }
            while (op<cpy) *op++ = *match++;
        }
        else
            LZ4_wildCopy(op, match, cpy);
        op=cpy;   /* correction */
    }

    /* end of decoding */
    if (endOnInput)
       return (int) (((char*)op)-dest);     /* Nb of output bytes decoded */
    else
       return (int) (((char*)ip)-source);   /* Nb of input bytes read */

    /* Overflow error detected */
_output_error:
    return (int) (-(((char*)ip)-source))-1;
}


int LZ4_decompress_safe(const char* source, char* dest, int compressedSize, int maxDecompressedSize)
{
    return LZ4_decompress_generic(source, dest, compressedSize, maxDecompressedSize, endOnInputSize, full, 0, noDict, (BYTE*)dest, NULL, 0);
}

int LZ4_decompress_safe_partial(const char* source, char* dest, int compressedSize, int targetOutputSize, int maxDecompressedSize)
{
    return LZ4_decompress_generic(source, dest, compressedSize, maxDecompressedSize, endOnInputSize, partial, targetOutputSize, noDict, (BYTE*)dest, NULL, 0);
}

int LZ4_decompress_fast(const char* source, char* dest, int originalSize)
{
    return LZ4_decompress_generic(source, dest, 0, originalSize, endOnOutputSize, full, 0, withPrefix64k, (BYTE*)(dest - 64 KB), NULL, 64 KB);
}


/* streaming decompression functions */

typedef struct
{
    BYTE* externalDict;
    size_t extDictSize;
    BYTE* prefixEnd;
    size_t prefixSize;
} LZ4_streamDecode_t_internal;

/*
 * If you prefer dynamic allocation methods,
 * LZ4_createStreamDecode()
 * provides a pointer (void*) towards an initialized LZ4_streamDecode_t structure.
 */
LZ4_streamDecode_t* LZ4_createStreamDecode(void)
{
    LZ4_streamDecode_t* lz4s = (LZ4_streamDecode_t*) ALLOCATOR2(1, sizeof(LZ4_streamDecode_t));
    return lz4s;
}

int LZ4_freeStreamDecode (LZ4_streamDecode_t* LZ4_stream)
{
    FREEMEM2(LZ4_stream);
    return 0;
}

/*
 * LZ4_setStreamDecode
 * Use this function to instruct where to find the dictionary
 * This function is not necessary if previous data is still available where it was decoded.
 * Loading a size of 0 is allowed (same effect as no dictionary).
 * Return : 1 if OK, 0 if error
 */
int LZ4_setStreamDecode (LZ4_streamDecode_t* LZ4_streamDecode, const char* dictionary, int dictSize)
{
    LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*) LZ4_streamDecode;
    lz4sd->prefixSize = (size_t) dictSize;
    lz4sd->prefixEnd = (BYTE*) dictionary + dictSize;
    lz4sd->externalDict = NULL;
    lz4sd->extDictSize  = 0;
    return 1;
}

/*
*_continue() :
    These decoding functions allow decompression of multiple blocks in "streaming" mode.
    Previously decoded blocks must still be available at the memory position where they were decoded.
    If it's not possible, save the relevant part of decoded data into a safe buffer,
    and indicate where it stands using LZ4_setStreamDecode()
*/
int LZ4_decompress_safe_continue (LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int compressedSize, int maxOutputSize)
{
    LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*) LZ4_streamDecode;
    int result;

    if (lz4sd->prefixEnd == (BYTE*)dest)
    {
        result = LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize,
                                        endOnInputSize, full, 0,
                                        usingExtDict, lz4sd->prefixEnd - lz4sd->prefixSize, lz4sd->externalDict, lz4sd->extDictSize);
        if (result <= 0) return result;
        lz4sd->prefixSize += result;
        lz4sd->prefixEnd  += result;
    }
    else
    {
        lz4sd->extDictSize = lz4sd->prefixSize;
        lz4sd->externalDict = lz4sd->prefixEnd - lz4sd->extDictSize;
        result = LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize,
                                        endOnInputSize, full, 0,
                                        usingExtDict, (BYTE*)dest, lz4sd->externalDict, lz4sd->extDictSize);
        if (result <= 0) return result;
        lz4sd->prefixSize = result;
        lz4sd->prefixEnd  = (BYTE*)dest + result;
    }

    return result;
}

int LZ4_decompress_fast_continue (LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int originalSize)
{
    LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*) LZ4_streamDecode;
    int result;

    if (lz4sd->prefixEnd == (BYTE*)dest)
    {
        result = LZ4_decompress_generic(source, dest, 0, originalSize,
                                        endOnOutputSize, full, 0,
                                        usingExtDict, lz4sd->prefixEnd - lz4sd->prefixSize, lz4sd->externalDict, lz4sd->extDictSize);
        if (result <= 0) return result;
        lz4sd->prefixSize += originalSize;
        lz4sd->prefixEnd  += originalSize;
    }
    else
    {
        lz4sd->extDictSize = lz4sd->prefixSize;
        lz4sd->externalDict = (BYTE*)dest - lz4sd->extDictSize;
        result = LZ4_decompress_generic(source, dest, 0, originalSize,
                                        endOnOutputSize, full, 0,
                                        usingExtDict, (BYTE*)dest, lz4sd->externalDict, lz4sd->extDictSize);
        if (result <= 0) return result;
        lz4sd->prefixSize = originalSize;
        lz4sd->prefixEnd  = (BYTE*)dest + originalSize;
    }

    return result;
}


/*
Advanced decoding functions :
*_usingDict() :
    These decoding functions work the same as "_continue" ones,
    the dictionary must be explicitly provided within parameters
*/

FORCE_INLINE int LZ4_decompress_usingDict_generic(const char* source, char* dest, int compressedSize, int maxOutputSize, int safe, const char* dictStart, int dictSize)
{
    if (dictSize==0)
        return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, safe, full, 0, noDict, (BYTE*)dest, NULL, 0);
    if (dictStart+dictSize == dest)
    {
        if (dictSize >= (int)(64 KB - 1))
            return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, safe, full, 0, withPrefix64k, (BYTE*)dest-64 KB, NULL, 0);
        return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, safe, full, 0, noDict, (BYTE*)dest-dictSize, NULL, 0);
    }
    return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, safe, full, 0, usingExtDict, (BYTE*)dest, (BYTE*)dictStart, dictSize);
}

int LZ4_decompress_safe_usingDict(const char* source, char* dest, int compressedSize, int maxOutputSize, const char* dictStart, int dictSize)
{
    return LZ4_decompress_usingDict_generic(source, dest, compressedSize, maxOutputSize, 1, dictStart, dictSize);
}

int LZ4_decompress_fast_usingDict(const char* source, char* dest, int originalSize, const char* dictStart, int dictSize)
{
    return LZ4_decompress_usingDict_generic(source, dest, 0, originalSize, 0, dictStart, dictSize);
}

/* debug function */
int LZ4_decompress_safe_forceExtDict(const char* source, char* dest, int compressedSize, int maxOutputSize, const char* dictStart, int dictSize)
{
    return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, usingExtDict, (BYTE*)dest, (BYTE*)dictStart, dictSize);
}


/***************************************************
*  Obsolete Functions
***************************************************/
/*
These function names are deprecated and should no longer be used.
They are only provided here for compatibility with older user programs.
- LZ4_uncompress is totally equivalent to LZ4_decompress_fast
- LZ4_uncompress_unknownOutputSize is totally equivalent to LZ4_decompress_safe
*/
int LZ4_uncompress (const char* source, char* dest, int outputSize) { return LZ4_decompress_fast(source, dest, outputSize); }
int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize) { return LZ4_decompress_safe(source, dest, isize, maxOutputSize); }


/* Obsolete Streaming functions */

int LZ4_sizeofStreamState() { return LZ4_STREAMSIZE; }

static void LZ4_init(LZ4_stream_t_internal* lz4ds, const BYTE* base)
{
    MEM_INIT(lz4ds, 0, LZ4_STREAMSIZE);
    lz4ds->bufferStart = base;
}

int LZ4_resetStreamState(void* state, const char* inputBuffer)
{
    if ((((size_t)state) & 3) != 0) return 1;   /* Error : pointer is not aligned on 4-bytes boundary */
    LZ4_init((LZ4_stream_t_internal*)state, (const BYTE*)inputBuffer);
    return 0;
}

void* LZ4_create (const char* inputBuffer)
{
    void* lz4ds = ALLOCATOR2(8, LZ4_STREAMSIZE_U64);
    LZ4_init ((LZ4_stream_t_internal*)lz4ds, (const BYTE*)inputBuffer);
    return lz4ds;
}

char* LZ4_slideInputBuffer (void* LZ4_Data)
{
    LZ4_stream_t_internal* ctx = (LZ4_stream_t_internal*)LZ4_Data;
    int dictSize = LZ4_saveDict((LZ4_stream_t*)LZ4_Data, (char*)ctx->bufferStart, 64 KB);
    return (char*)(ctx->bufferStart + dictSize);
}

/*  Obsolete compresson functions using User-allocated state */

int LZ4_sizeofState() { return LZ4_STREAMSIZE; }

int LZ4_compress_withState (void* state, const char* source, char* dest, int inputSize)
{
    if (((size_t)(state)&3) != 0) return 0;   /* Error : state is not aligned on 4-bytes boundary */
    MEM_INIT(state, 0, LZ4_STREAMSIZE);

    if (inputSize < LZ4_64Klimit)
        return LZ4_compress_generic(state, source, dest, inputSize, 0, notLimited, byU16, noDict, noDictIssue);
    else
        return LZ4_compress_generic(state, source, dest, inputSize, 0, notLimited, LZ4_64bits() ? byU32 : byPtr, noDict, noDictIssue);
}

int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize)
{
    if (((size_t)(state)&3) != 0) return 0;   /* Error : state is not aligned on 4-bytes boundary */
    MEM_INIT(state, 0, LZ4_STREAMSIZE);

    if (inputSize < LZ4_64Klimit)
        return LZ4_compress_generic(state, source, dest, inputSize, maxOutputSize, limitedOutput, byU16, noDict, noDictIssue);
    else
        return LZ4_compress_generic(state, source, dest, inputSize, maxOutputSize, limitedOutput, LZ4_64bits() ? byU32 : byPtr, noDict, noDictIssue);
}

/* Obsolete streaming decompression functions */

int LZ4_decompress_safe_withPrefix64k(const char* source, char* dest, int compressedSize, int maxOutputSize)
{
    return LZ4_decompress_generic(source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, withPrefix64k, (BYTE*)dest - 64 KB, NULL, 64 KB);
}

int LZ4_decompress_fast_withPrefix64k(const char* source, char* dest, int originalSize)
{
    return LZ4_decompress_generic(source, dest, 0, originalSize, endOnOutputSize, full, 0, withPrefix64k, (BYTE*)dest - 64 KB, NULL, 64 KB);
}

#endif   /* LZ4_COMMONDEFS_ONLY */

/* lz4.c EOF */

/* lz4hc.c */
/*
LZ4 HC - High Compression Mode of LZ4
Copyright (C) 2011-2015, Yann Collet.

BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You can contact the author at :
   - LZ4 source repository : https://github.com/Cyan4973/lz4
   - LZ4 public forum : https://groups.google.com/forum/#!forum/lz4c
*/



/**************************************
   Tuning Parameter
**************************************/
static const int LZ4HC_compressionLevel_default = 8;


/**************************************
   Common LZ4 definition
**************************************/
#define LZ4_COMMONDEFS_ONLY

/**************************************
  Local Constants
**************************************/
#define DICTIONARY_LOGSIZE 16
#define MAXD (1<<DICTIONARY_LOGSIZE)
#define MAXD_MASK ((U32)(MAXD - 1))

#define HASH_LOG2 (DICTIONARY_LOGSIZE-1)
#define HASHTABLESIZE2 (1 << HASH_LOG2)
#define HASH_MASK2 (HASHTABLESIZE2 - 1)

#define OPTIMAL_ML (int)((ML_MASK-1)+MINMATCH)

static const int g_maxCompressionLevel = 16;

/**************************************
   Local Types
**************************************/
typedef struct
{
    U32 hashTable[HASHTABLESIZE2];
    U16   chainTable[MAXD];
    const BYTE* end;        /* next block here to continue on current prefix */
    const BYTE* base;       /* All index relative to this position */
    const BYTE* dictBase;   /* alternate base for extDict */
    const BYTE* inputBuffer;/* deprecated */
    U32   dictLimit;        /* below that point, need extDict */
    U32   lowLimit;         /* below that point, no more dict */
    U32   nextToUpdate;
    U32   compressionLevel;
} LZ4HC_Data_Structure;


/**************************************
   Local Macros
**************************************/
#define HASH_FUNCTION(i)       (((i) * 2654435761U) >> ((MINMATCH*8)-HASH_LOG2))
#define DELTANEXT(p)           chainTable[(size_t)(p) & MAXD_MASK]
#define GETNEXT(p)             ((p) - (size_t)DELTANEXT(p))

static U32 LZ4HC_hashPtr(const void* ptr) { return HASH_FUNCTION(LZ4_read32(ptr)); }



/**************************************
   HC Compression
**************************************/
static void LZ4HC_init (LZ4HC_Data_Structure* hc4, const BYTE* start)
{
    MEM_INIT((void*)hc4->hashTable, 0, sizeof(hc4->hashTable));
    MEM_INIT(hc4->chainTable, 0xFF, sizeof(hc4->chainTable));
    hc4->nextToUpdate = 64 KB;
    hc4->base = start - 64 KB;
    hc4->inputBuffer = start;
    hc4->end = start;
    hc4->dictBase = start - 64 KB;
    hc4->dictLimit = 64 KB;
    hc4->lowLimit = 64 KB;
}


/* Update chains up to ip (excluded) */
FORCE_INLINE void LZ4HC_Insert (LZ4HC_Data_Structure* hc4, const BYTE* ip)
{
    U16* chainTable = hc4->chainTable;
    U32* HashTable  = hc4->hashTable;
    const BYTE* const base = hc4->base;
    const U32 target = (U32)(ip - base);
    U32 idx = hc4->nextToUpdate;

    while(idx < target)
    {
        U32 h = LZ4HC_hashPtr(base+idx);
        size_t delta = idx - HashTable[h];
        if (delta>MAX_DISTANCE) delta = MAX_DISTANCE;
        chainTable[idx & 0xFFFF] = (U16)delta;
        HashTable[h] = idx;
        idx++;
    }

    hc4->nextToUpdate = target;
}


FORCE_INLINE int LZ4HC_InsertAndFindBestMatch (LZ4HC_Data_Structure* hc4,   /* Index table will be updated */
                                               const BYTE* ip, const BYTE* const iLimit,
                                               const BYTE** matchpos,
                                               const int maxNbAttempts)
{
    U16* const chainTable = hc4->chainTable;
    U32* const HashTable = hc4->hashTable;
    const BYTE* const base = hc4->base;
    const BYTE* const dictBase = hc4->dictBase;
    const U32 dictLimit = hc4->dictLimit;
    const U32 lowLimit = (hc4->lowLimit + 64 KB > (U32)(ip-base)) ? hc4->lowLimit : (U32)(ip - base) - (64 KB - 1);
    U32 matchIndex;
    const BYTE* match;
    int nbAttempts=maxNbAttempts;
    size_t ml=0;

    /* HC4 match finder */
    LZ4HC_Insert(hc4, ip);
    matchIndex = HashTable[LZ4HC_hashPtr(ip)];

    while ((matchIndex>=lowLimit) && (nbAttempts))
    {
        nbAttempts--;
        if (matchIndex >= dictLimit)
        {
            match = base + matchIndex;
            if (*(match+ml) == *(ip+ml)
                && (LZ4_read32(match) == LZ4_read32(ip)))
            {
                size_t mlt = LZ4_count(ip+MINMATCH, match+MINMATCH, iLimit) + MINMATCH;
                if (mlt > ml) { ml = mlt; *matchpos = match; }
            }
        }
        else
        {
            match = dictBase + matchIndex;
            if (LZ4_read32(match) == LZ4_read32(ip))
            {
                size_t mlt;
                const BYTE* vLimit = ip + (dictLimit - matchIndex);
                if (vLimit > iLimit) vLimit = iLimit;
                mlt = LZ4_count(ip+MINMATCH, match+MINMATCH, vLimit) + MINMATCH;
                if ((ip+mlt == vLimit) && (vLimit < iLimit))
                    mlt += LZ4_count(ip+mlt, base+dictLimit, iLimit);
                if (mlt > ml) { ml = mlt; *matchpos = base + matchIndex; }   /* virtual matchpos */
            }
        }
        matchIndex -= chainTable[matchIndex & 0xFFFF];
    }

    return (int)ml;
}


FORCE_INLINE int LZ4HC_InsertAndGetWiderMatch (
    LZ4HC_Data_Structure* hc4,
    const BYTE* ip,
    const BYTE* iLowLimit,
    const BYTE* iHighLimit,
    int longest,
    const BYTE** matchpos,
    const BYTE** startpos,
    const int maxNbAttempts)
{
    U16* const chainTable = hc4->chainTable;
    U32* const HashTable = hc4->hashTable;
    const BYTE* const base = hc4->base;
    const U32 dictLimit = hc4->dictLimit;
    const U32 lowLimit = (hc4->lowLimit + 64 KB > (U32)(ip-base)) ? hc4->lowLimit : (U32)(ip - base) - (64 KB - 1);
    const BYTE* const dictBase = hc4->dictBase;
    const BYTE* match;
    U32   matchIndex;
    int nbAttempts = maxNbAttempts;
    int delta = (int)(ip-iLowLimit);


    /* First Match */
    LZ4HC_Insert(hc4, ip);
    matchIndex = HashTable[LZ4HC_hashPtr(ip)];

    while ((matchIndex>=lowLimit) && (nbAttempts))
    {
        nbAttempts--;
        if (matchIndex >= dictLimit)
        {
            match = base + matchIndex;
            if (*(iLowLimit + longest) == *(match - delta + longest))
                if (LZ4_read32(match) == LZ4_read32(ip))
                {
                    const BYTE* startt = ip;
                    const BYTE* tmpMatch = match;
                    const BYTE* const matchEnd = ip + MINMATCH + LZ4_count(ip+MINMATCH, match+MINMATCH, iHighLimit);

                    while ((startt>iLowLimit) && (tmpMatch > iLowLimit) && (startt[-1] == tmpMatch[-1])) {startt--; tmpMatch--;}

                    if ((matchEnd-startt) > longest)
                    {
                        longest = (int)(matchEnd-startt);
                        *matchpos = tmpMatch;
                        *startpos = startt;
                    }
                }
        }
        else
        {
            match = dictBase + matchIndex;
            if (LZ4_read32(match) == LZ4_read32(ip))
            {
                size_t mlt;
                int back=0;
                const BYTE* vLimit = ip + (dictLimit - matchIndex);
                if (vLimit > iHighLimit) vLimit = iHighLimit;
                mlt = LZ4_count(ip+MINMATCH, match+MINMATCH, vLimit) + MINMATCH;
                if ((ip+mlt == vLimit) && (vLimit < iHighLimit))
                    mlt += LZ4_count(ip+mlt, base+dictLimit, iHighLimit);
                while ((ip+back > iLowLimit) && (matchIndex+back > lowLimit) && (ip[back-1] == match[back-1])) back--;
                mlt -= back;
                if ((int)mlt > longest) { longest = (int)mlt; *matchpos = base + matchIndex + back; *startpos = ip+back; }
            }
        }
        matchIndex -= chainTable[matchIndex & 0xFFFF];
    }

    return longest;
}


enum { noLimit = 0 };

/*typedef enum { noLimit = 0, limitedOutput = 1 } limitedOutput_directive;*/

#define LZ4HC_DEBUG 0
#if LZ4HC_DEBUG
static unsigned debug = 0;
#endif

FORCE_INLINE int LZ4HC_encodeSequence (
    const BYTE** ip,
    BYTE** op,
    const BYTE** anchor,
    int matchLength,
    const BYTE* const match,
    limitedOutput_directive limitedOutputBuffer,
    BYTE* oend)
{
    int length;
    BYTE* token;

#if LZ4HC_DEBUG
    if (debug) printf("literal : %u  --  match : %u  --  offset : %u\n", (U32)(*ip - *anchor), (U32)matchLength, (U32)(*ip-match));
#endif

    /* Encode Literal length */
    length = (int)(*ip - *anchor);
    token = (*op)++;
    if ((limitedOutputBuffer) && ((*op + (length>>8) + length + (2 + 1 + LASTLITERALS)) > oend)) return 1;   /* Check output limit */
    if (length>=(int)RUN_MASK) { int len; *token=(RUN_MASK<<ML_BITS); len = length-RUN_MASK; for(; len > 254 ; len-=255) *(*op)++ = 255;  *(*op)++ = (BYTE)len; }
    else *token = (BYTE)(length<<ML_BITS);

    /* Copy Literals */
    LZ4_wildCopy(*op, *anchor, (*op) + length);
    *op += length;

    /* Encode Offset */
    LZ4_writeLE16(*op, (U16)(*ip-match)); *op += 2;

    /* Encode MatchLength */
    length = (int)(matchLength-MINMATCH);
    if ((limitedOutputBuffer) && (*op + (length>>8) + (1 + LASTLITERALS) > oend)) return 1;   /* Check output limit */
    if (length>=(int)ML_MASK) { *token+=ML_MASK; length-=ML_MASK; for(; length > 509 ; length-=510) { *(*op)++ = 255; *(*op)++ = 255; } if (length > 254) { length-=255; *(*op)++ = 255; } *(*op)++ = (BYTE)length; }
    else *token += (BYTE)(length);

    /* Prepare next loop */
    *ip += matchLength;
    *anchor = *ip;

    return 0;
}


static int LZ4HC_compress_generic (
    void* ctxvoid,
    const char* source,
    char* dest,
    int inputSize,
    int maxOutputSize,
    int compressionLevel,
    limitedOutput_directive limit
    )
{
    LZ4HC_Data_Structure* ctx = (LZ4HC_Data_Structure*) ctxvoid;
    const BYTE* ip = (const BYTE*) source;
    const BYTE* anchor = ip;
    const BYTE* const iend = ip + inputSize;
    const BYTE* const mflimit = iend - MFLIMIT;
    const BYTE* const matchlimit = (iend - LASTLITERALS);

    BYTE* op = (BYTE*) dest;
    BYTE* const oend = op + maxOutputSize;

    unsigned maxNbAttempts;
    int   ml, ml2, ml3, ml0;
    const BYTE* ref=NULL;
    const BYTE* start2=NULL;
    const BYTE* ref2=NULL;
    const BYTE* start3=NULL;
    const BYTE* ref3=NULL;
    const BYTE* start0;
    const BYTE* ref0;


    /* init */
    if (compressionLevel > g_maxCompressionLevel) compressionLevel = g_maxCompressionLevel;
    if (compressionLevel < 1) compressionLevel = LZ4HC_compressionLevel_default;
    maxNbAttempts = 1 << (compressionLevel-1);
    ctx->end += inputSize;

    ip++;

    /* Main Loop */
    while (ip < mflimit)
    {
        ml = LZ4HC_InsertAndFindBestMatch (ctx, ip, matchlimit, (&ref), maxNbAttempts);
        if (!ml) { ip++; continue; }

        /* saved, in case we would skip too much */
        start0 = ip;
        ref0 = ref;
        ml0 = ml;

_Search2:
        if (ip+ml < mflimit)
            ml2 = LZ4HC_InsertAndGetWiderMatch(ctx, ip + ml - 2, ip + 1, matchlimit, ml, &ref2, &start2, maxNbAttempts);
        else ml2 = ml;

        if (ml2 == ml)  /* No better match */
        {
            if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;
            continue;
        }

        if (start0 < ip)
        {
            if (start2 < ip + ml0)   /* empirical */
            {
                ip = start0;
                ref = ref0;
                ml = ml0;
            }
        }

        /* Here, start0==ip */
        if ((start2 - ip) < 3)   /* First Match too small : removed */
        {
            ml = ml2;
            ip = start2;
            ref =ref2;
            goto _Search2;
        }

_Search3:
        /*
        * Currently we have :
        * ml2 > ml1, and
        * ip1+3 <= ip2 (usually < ip1+ml1)
        */
        if ((start2 - ip) < OPTIMAL_ML)
        {
            int correction;
            int new_ml = ml;
            if (new_ml > OPTIMAL_ML) new_ml = OPTIMAL_ML;
            if (ip+new_ml > start2 + ml2 - MINMATCH) new_ml = (int)(start2 - ip) + ml2 - MINMATCH;
            correction = new_ml - (int)(start2 - ip);
            if (correction > 0)
            {
                start2 += correction;
                ref2 += correction;
                ml2 -= correction;
            }
        }
        /* Now, we have start2 = ip+new_ml, with new_ml = min(ml, OPTIMAL_ML=18) */

        if (start2 + ml2 < mflimit)
            ml3 = LZ4HC_InsertAndGetWiderMatch(ctx, start2 + ml2 - 3, start2, matchlimit, ml2, &ref3, &start3, maxNbAttempts);
        else ml3 = ml2;

        if (ml3 == ml2) /* No better match : 2 sequences to encode */
        {
            /* ip & ref are known; Now for ml */
            if (start2 < ip+ml)  ml = (int)(start2 - ip);
            /* Now, encode 2 sequences */
            if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;
            ip = start2;
            if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml2, ref2, limit, oend)) return 0;
            continue;
        }

        if (start3 < ip+ml+3) /* Not enough space for match 2 : remove it */
        {
            if (start3 >= (ip+ml)) /* can write Seq1 immediately ==> Seq2 is removed, so Seq3 becomes Seq1 */
            {
                if (start2 < ip+ml)
                {
                    int correction = (int)(ip+ml - start2);
                    start2 += correction;
                    ref2 += correction;
                    ml2 -= correction;
                    if (ml2 < MINMATCH)
                    {
                        start2 = start3;
                        ref2 = ref3;
                        ml2 = ml3;
                    }
                }

                if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;
                ip  = start3;
                ref = ref3;
                ml  = ml3;

                start0 = start2;
                ref0 = ref2;
                ml0 = ml2;
                goto _Search2;
            }

            start2 = start3;
            ref2 = ref3;
            ml2 = ml3;
            goto _Search3;
        }

        /*
        * OK, now we have 3 ascending matches; let's write at least the first one
        * ip & ref are known; Now for ml
        */
        if (start2 < ip+ml)
        {
            if ((start2 - ip) < (int)ML_MASK)
            {
                int correction;
                if (ml > OPTIMAL_ML) ml = OPTIMAL_ML;
                if (ip + ml > start2 + ml2 - MINMATCH) ml = (int)(start2 - ip) + ml2 - MINMATCH;
                correction = ml - (int)(start2 - ip);
                if (correction > 0)
                {
                    start2 += correction;
                    ref2 += correction;
                    ml2 -= correction;
                }
            }
            else
            {
                ml = (int)(start2 - ip);
            }
        }
        if (LZ4HC_encodeSequence(&ip, &op, &anchor, ml, ref, limit, oend)) return 0;

        ip = start2;
        ref = ref2;
        ml = ml2;

        start2 = start3;
        ref2 = ref3;
        ml2 = ml3;

        goto _Search3;
    }

    /* Encode Last Literals */
    {
        int lastRun = (int)(iend - anchor);
        if ((limit) && (((char*)op - dest) + lastRun + 1 + ((lastRun+255-RUN_MASK)/255) > (U32)maxOutputSize)) return 0;  /* Check output limit */
        if (lastRun>=(int)RUN_MASK) { *op++=(RUN_MASK<<ML_BITS); lastRun-=RUN_MASK; for(; lastRun > 254 ; lastRun-=255) *op++ = 255; *op++ = (BYTE) lastRun; }
        else *op++ = (BYTE)(lastRun<<ML_BITS);
        memcpy(op, anchor, iend - anchor);
        op += iend-anchor;
    }

    /* End */
    return (int) (((char*)op)-dest);
}


int LZ4_compressHC2(const char* source, char* dest, int inputSize, int compressionLevel)
{
    LZ4HC_Data_Structure ctx;
    LZ4HC_init(&ctx, (const BYTE*)source);
    return LZ4HC_compress_generic (&ctx, source, dest, inputSize, 0, compressionLevel, noLimit);
}

int LZ4_compressHC(const char* source, char* dest, int inputSize) { return LZ4_compressHC2(source, dest, inputSize, 0); }

int LZ4_compressHC2_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel)
{
    LZ4HC_Data_Structure ctx;
    LZ4HC_init(&ctx, (const BYTE*)source);
    return LZ4HC_compress_generic (&ctx, source, dest, inputSize, maxOutputSize, compressionLevel, limitedOutput);
}

int LZ4_compressHC_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize)
{
    return LZ4_compressHC2_limitedOutput(source, dest, inputSize, maxOutputSize, 0);
}


/*****************************
 * Using external allocation
 * ***************************/
int LZ4_sizeofStateHC(void) { return sizeof(LZ4HC_Data_Structure); }


int LZ4_compressHC2_withStateHC (void* state, const char* source, char* dest, int inputSize, int compressionLevel)
{
    if (((size_t)(state)&(sizeof(void*)-1)) != 0) return 0;   /* Error : state is not aligned for pointers (32 or 64 bits) */
    LZ4HC_init ((LZ4HC_Data_Structure*)state, (const BYTE*)source);
    return LZ4HC_compress_generic (state, source, dest, inputSize, 0, compressionLevel, noLimit);
}

int LZ4_compressHC_withStateHC (void* state, const char* source, char* dest, int inputSize)
{ return LZ4_compressHC2_withStateHC (state, source, dest, inputSize, 0); }


int LZ4_compressHC2_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel)
{
    if (((size_t)(state)&(sizeof(void*)-1)) != 0) return 0;   /* Error : state is not aligned for pointers (32 or 64 bits) */
    LZ4HC_init ((LZ4HC_Data_Structure*)state, (const BYTE*)source);
    return LZ4HC_compress_generic (state, source, dest, inputSize, maxOutputSize, compressionLevel, limitedOutput);
}

int LZ4_compressHC_limitedOutput_withStateHC (void* state, const char* source, char* dest, int inputSize, int maxOutputSize)
{ return LZ4_compressHC2_limitedOutput_withStateHC (state, source, dest, inputSize, maxOutputSize, 0); }



/**************************************
 * Streaming Functions
 * ************************************/
/* allocation */
LZ4_streamHC_t* LZ4_createStreamHC(void) { return (LZ4_streamHC_t*)malloc(sizeof(LZ4_streamHC_t)); }
int LZ4_freeStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr) { free(LZ4_streamHCPtr); return 0; }


/* initialization */
void LZ4_resetStreamHC (LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel)
{
    LZ4_STATIC_ASSERT(sizeof(LZ4HC_Data_Structure) <= sizeof(LZ4_streamHC_t));   /* if compilation fails here, LZ4_STREAMHCSIZE must be increased */
    ((LZ4HC_Data_Structure*)LZ4_streamHCPtr)->base = NULL;
    ((LZ4HC_Data_Structure*)LZ4_streamHCPtr)->compressionLevel = (unsigned)compressionLevel;
}

int LZ4_loadDictHC (LZ4_streamHC_t* LZ4_streamHCPtr, const char* dictionary, int dictSize)
{
    LZ4HC_Data_Structure* ctxPtr = (LZ4HC_Data_Structure*) LZ4_streamHCPtr;
    if (dictSize > 64 KB)
    {
        dictionary += dictSize - 64 KB;
        dictSize = 64 KB;
    }
    LZ4HC_init (ctxPtr, (const BYTE*)dictionary);
    if (dictSize >= 4) LZ4HC_Insert (ctxPtr, (const BYTE*)dictionary +(dictSize-3));
    ctxPtr->end = (const BYTE*)dictionary + dictSize;
    return dictSize;
}


/* compression */

static void LZ4HC_setExternalDict(LZ4HC_Data_Structure* ctxPtr, const BYTE* newBlock)
{
    if (ctxPtr->end >= ctxPtr->base + 4)
        LZ4HC_Insert (ctxPtr, ctxPtr->end-3);   /* Referencing remaining dictionary content */
    /* Only one memory segment for extDict, so any previous extDict is lost at this stage */
    ctxPtr->lowLimit  = ctxPtr->dictLimit;
    ctxPtr->dictLimit = (U32)(ctxPtr->end - ctxPtr->base);
    ctxPtr->dictBase  = ctxPtr->base;
    ctxPtr->base = newBlock - ctxPtr->dictLimit;
    ctxPtr->end  = newBlock;
    ctxPtr->nextToUpdate = ctxPtr->dictLimit;   /* match referencing will resume from there */
}

static int LZ4_compressHC_continue_generic (LZ4HC_Data_Structure* ctxPtr,
                                            const char* source, char* dest,
                                            int inputSize, int maxOutputSize, limitedOutput_directive limit)
{
    /* auto-init if forgotten */
    if (ctxPtr->base == NULL)
        LZ4HC_init (ctxPtr, (const BYTE*) source);

    /* Check overflow */
    if ((size_t)(ctxPtr->end - ctxPtr->base) > 2 GB)
    {
        size_t dictSize = (size_t)(ctxPtr->end - ctxPtr->base) - ctxPtr->dictLimit;
        if (dictSize > 64 KB) dictSize = 64 KB;

        LZ4_loadDictHC((LZ4_streamHC_t*)ctxPtr, (const char*)(ctxPtr->end) - dictSize, (int)dictSize);
    }

    /* Check if blocks follow each other */
    if ((const BYTE*)source != ctxPtr->end) LZ4HC_setExternalDict(ctxPtr, (const BYTE*)source);

    /* Check overlapping input/dictionary space */
    {
        const BYTE* sourceEnd = (const BYTE*) source + inputSize;
        const BYTE* dictBegin = ctxPtr->dictBase + ctxPtr->lowLimit;
        const BYTE* dictEnd   = ctxPtr->dictBase + ctxPtr->dictLimit;
        if ((sourceEnd > dictBegin) && ((BYTE*)source < dictEnd))
        {
            if (sourceEnd > dictEnd) sourceEnd = dictEnd;
            ctxPtr->lowLimit = (U32)(sourceEnd - ctxPtr->dictBase);
            if (ctxPtr->dictLimit - ctxPtr->lowLimit < 4) ctxPtr->lowLimit = ctxPtr->dictLimit;
        }
    }

    return LZ4HC_compress_generic (ctxPtr, source, dest, inputSize, maxOutputSize, ctxPtr->compressionLevel, limit);
}

int LZ4_compressHC_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize)
{
    return LZ4_compressHC_continue_generic ((LZ4HC_Data_Structure*)LZ4_streamHCPtr, source, dest, inputSize, 0, noLimit);
}

int LZ4_compressHC_limitedOutput_continue (LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize, int maxOutputSize)
{
    return LZ4_compressHC_continue_generic ((LZ4HC_Data_Structure*)LZ4_streamHCPtr, source, dest, inputSize, maxOutputSize, limitedOutput);
}


/* dictionary saving */

int LZ4_saveDictHC (LZ4_streamHC_t* LZ4_streamHCPtr, char* safeBuffer, int dictSize)
{
    LZ4HC_Data_Structure* streamPtr = (LZ4HC_Data_Structure*)LZ4_streamHCPtr;
    int prefixSize = (int)(streamPtr->end - (streamPtr->base + streamPtr->dictLimit));
    if (dictSize > 64 KB) dictSize = 64 KB;
    if (dictSize < 4) dictSize = 0;
    if (dictSize > prefixSize) dictSize = prefixSize;
    memcpy(safeBuffer, streamPtr->end - dictSize, dictSize);
    {
        U32 endIndex = (U32)(streamPtr->end - streamPtr->base);
        streamPtr->end = (const BYTE*)safeBuffer + dictSize;
        streamPtr->base = streamPtr->end - endIndex;
        streamPtr->dictLimit = endIndex - dictSize;
        streamPtr->lowLimit = endIndex - dictSize;
        if (streamPtr->nextToUpdate < streamPtr->dictLimit) streamPtr->nextToUpdate = streamPtr->dictLimit;
    }
    return dictSize;
}


/***********************************
 * Deprecated Functions
 ***********************************/
int LZ4_sizeofStreamStateHC(void) { return LZ4_STREAMHCSIZE; }

int LZ4_resetStreamStateHC(void* state, const char* inputBuffer)
{
    if ((((size_t)state) & (sizeof(void*)-1)) != 0) return 1;   /* Error : pointer is not aligned for pointer (32 or 64 bits) */
    LZ4HC_init((LZ4HC_Data_Structure*)state, (const BYTE*)inputBuffer);
    return 0;
}

void* LZ4_createHC (const char* inputBuffer)
{
    void* hc4 = ALLOCATOR2(1, sizeof(LZ4HC_Data_Structure));
    LZ4HC_init ((LZ4HC_Data_Structure*)hc4, (const BYTE*)inputBuffer);
    return hc4;
}

int LZ4_freeHC (void* LZ4HC_Data)
{
    FREEMEM2(LZ4HC_Data);
    return (0);
}

/*
int LZ4_compressHC_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize)
{
return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, 0, 0, noLimit);
}
int LZ4_compressHC_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize)
{
return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, maxOutputSize, 0, limitedOutput);
}
*/

int LZ4_compressHC2_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel)
{
    return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, 0, compressionLevel, noLimit);
}

int LZ4_compressHC2_limitedOutput_continue (void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel)
{
    return LZ4HC_compress_generic (LZ4HC_Data, source, dest, inputSize, maxOutputSize, compressionLevel, limitedOutput);
}

char* LZ4_slideInputBufferHC(void* LZ4HC_Data)
{
    LZ4HC_Data_Structure* hc4 = (LZ4HC_Data_Structure*)LZ4HC_Data;
    int dictSize = LZ4_saveDictHC((LZ4_streamHC_t*)LZ4HC_Data, (char*)(hc4->inputBuffer), 64 KB);
    return (char*)(hc4->inputBuffer + dictSize);
}
/* lz4hc.c EOF */

/* lz4frame.c */

/**************************************
*  Memory routines
**************************************/
#define ALLOCATOR(s)   calloc(1,s)
#define FREEMEM        free
#define MEM_INIT       memset

/**************************************
*  Constants
**************************************/
#ifndef KB
#define KB *(1<<10)
#endif
#ifndef MB
#define MB *(1<<20)
#endif
#ifndef GB
#define GB *(1<<30)
#endif

#define _1BIT  0x01
#define _2BITS 0x03
#define _3BITS 0x07
#define _4BITS 0x0F
#define _8BITS 0xFF

#define LZ4F_MAGIC_SKIPPABLE_START 0x184D2A50U
#define LZ4F_MAGICNUMBER 0x184D2204U
#define LZ4F_BLOCKUNCOMPRESSED_FLAG 0x80000000U
#define LZ4F_MAXHEADERFRAME_SIZE 15
#define LZ4F_BLOCKSIZEID_DEFAULT max64KB

static const size_t minFHSize = 5;
static const U32 minHClevel = 3;

/**************************************
*  Structures and local types
**************************************/
typedef struct
{
    LZ4F_preferences_t prefs;
    U32    version;
    U32    cStage;
    size_t maxBlockSize;
    size_t maxBufferSize;
    BYTE*  tmpBuff;
    BYTE*  tmpIn;
    size_t tmpInSize;
    U64    totalInSize;
    XXH32_state_t xxh;
    void*  lz4CtxPtr;
    U32    lz4CtxLevel;     /* 0: unallocated;  1: LZ4_stream_t;  3: LZ4_streamHC_t */
} LZ4F_cctx_internal_t;

typedef struct
{
    LZ4F_frameInfo_t frameInfo;
    U32    version;
    U32    dStage;
    size_t maxBlockSize;
    size_t maxBufferSize;
    const BYTE* srcExpect;
    BYTE*  tmpIn;
    size_t tmpInSize;
    size_t tmpInTarget;
    BYTE*  tmpOutBuffer;
    BYTE*  dict;
    size_t dictSize;
    BYTE*  tmpOut;
    size_t tmpOutSize;
    size_t tmpOutStart;
    XXH32_state_t xxh;
    BYTE   header[16];
} LZ4F_dctx_internal_t;


/**************************************
*  Error management
**************************************/
#define LZ4F_GENERATE_STRING(STRING) #STRING,
static const char* LZ4F_errorStrings[] = { LZ4F_LIST_ERRORS(LZ4F_GENERATE_STRING) };


unsigned LZ4F_isError(LZ4F_errorCode_t code)
{
    return (code > (LZ4F_errorCode_t)(-ERROR_maxCode));
}

const char* LZ4F_getErrorName(LZ4F_errorCode_t code)
{
    static const char* codeError = "Unspecified error code";
    if (LZ4F_isError(code)) return LZ4F_errorStrings[-(int)(code)];
    return codeError;
}


/**************************************
*  Private functions
**************************************/
static size_t LZ4F_getBlockSize(unsigned blockSizeID)
{
    static const size_t blockSizes[4] = { 64 KB, 256 KB, 1 MB, 4 MB };

    if (blockSizeID == 0) blockSizeID = LZ4F_BLOCKSIZEID_DEFAULT;
    blockSizeID -= 4;
    if (blockSizeID > 3) return (size_t)-ERROR_maxBlockSize_invalid;
    return blockSizes[blockSizeID];
}


/* unoptimized version; solves endianess & alignment issues */
static U32 LZ4F_readLE32 (const BYTE* srcPtr)
{
    U32 value32 = srcPtr[0];
    value32 += (srcPtr[1]<<8);
    value32 += (srcPtr[2]<<16);
    value32 += (srcPtr[3]<<24);
    return value32;
}

static void LZ4F_writeLE32 (BYTE* dstPtr, U32 value32)
{
    dstPtr[0] = (BYTE)value32;
    dstPtr[1] = (BYTE)(value32 >> 8);
    dstPtr[2] = (BYTE)(value32 >> 16);
    dstPtr[3] = (BYTE)(value32 >> 24);
}

static U64 LZ4F_readLE64 (const BYTE* srcPtr)
{
    U64 value64 = srcPtr[0];
    value64 += (srcPtr[1]<<8);
    value64 += (srcPtr[2]<<16);
    value64 += (srcPtr[3]<<24);
    value64 += ((U64)srcPtr[4]<<32);
    value64 += ((U64)srcPtr[5]<<40);
    value64 += ((U64)srcPtr[6]<<48);
    value64 += ((U64)srcPtr[7]<<56);
    return value64;
}

static void LZ4F_writeLE64 (BYTE* dstPtr, U64 value64)
{
    dstPtr[0] = (BYTE)value64;
    dstPtr[1] = (BYTE)(value64 >> 8);
    dstPtr[2] = (BYTE)(value64 >> 16);
    dstPtr[3] = (BYTE)(value64 >> 24);
    dstPtr[4] = (BYTE)(value64 >> 32);
    dstPtr[5] = (BYTE)(value64 >> 40);
    dstPtr[6] = (BYTE)(value64 >> 48);
    dstPtr[7] = (BYTE)(value64 >> 56);
}


static BYTE LZ4F_headerChecksum (const void* header, size_t length)
{
    U32 xxh = XXH32(header, length, 0);
    return (BYTE)(xxh >> 8);
}


/**************************************
*  Simple compression functions
**************************************/
static blockSizeID_t LZ4F_optimalBSID(const blockSizeID_t requestedBSID, const size_t srcSize)
{
    blockSizeID_t proposedBSID = max64KB;
    size_t maxBlockSize = 64 KB;
    while (requestedBSID > proposedBSID)
    {
        if (srcSize <= maxBlockSize)
            return proposedBSID;
        proposedBSID = (blockSizeID_t)((int)proposedBSID + 1);
        maxBlockSize <<= 2;
    }
    return requestedBSID;
}


size_t LZ4F_compressFrameBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_preferences_t prefs;
    size_t headerSize;
    size_t streamSize;

    if (preferencesPtr!=NULL) prefs = *preferencesPtr;
    else memset(&prefs, 0, sizeof(prefs));

    prefs.frameInfo.blockSizeID = LZ4F_optimalBSID(prefs.frameInfo.blockSizeID, srcSize);
    prefs.autoFlush = 1;

    headerSize = 15;      /* header size, including magic number and frame content size*/
    streamSize = LZ4F_compressBound(srcSize, &prefs);

    return headerSize + streamSize;
}


/* LZ4F_compressFrame()
* Compress an entire srcBuffer into a valid LZ4 frame, as defined by specification v1.5.0, in a single step.
* The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
* You can get the minimum value of dstMaxSize by using LZ4F_compressFrameBound()
* If this condition is not respected, LZ4F_compressFrame() will fail (result is an errorCode)
* The LZ4F_preferences_t structure is optional : you can provide NULL as argument. All preferences will then be set to default.
* The result of the function is the number of bytes written into dstBuffer.
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
*/
size_t LZ4F_compressFrame(void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_cctx_internal_t cctxI;
    LZ4_stream_t lz4ctx;
    LZ4F_preferences_t prefs;
    LZ4F_compressOptions_t options;
    LZ4F_errorCode_t errorCode;
    BYTE* const dstStart = (BYTE*) dstBuffer;
    BYTE* dstPtr = dstStart;
    BYTE* const dstEnd = dstStart + dstMaxSize;

    memset(&cctxI, 0, sizeof(cctxI));   /* works because no allocation */
    memset(&options, 0, sizeof(options));

    cctxI.version = LZ4F_VERSION;
    cctxI.maxBufferSize = 5 MB;   /* mess with real buffer size to prevent allocation; works because autoflush==1 & stableSrc==1 */

    if (preferencesPtr!=NULL) prefs = *preferencesPtr;
    else
    {
        memset(&prefs, 0, sizeof(prefs));
        prefs.frameInfo.frameOSize = (U64)srcSize;
    }
    if (prefs.frameInfo.frameOSize != 0)
        prefs.frameInfo.frameOSize = (U64)srcSize;   /* correct frame size if selected (!=0) */

    if (prefs.compressionLevel < minHClevel)
    {
        cctxI.lz4CtxPtr = &lz4ctx;
        cctxI.lz4CtxLevel = 1;
    }

    prefs.frameInfo.blockSizeID = LZ4F_optimalBSID(prefs.frameInfo.blockSizeID, srcSize);
    prefs.autoFlush = 1;
    if (srcSize <= LZ4F_getBlockSize(prefs.frameInfo.blockSizeID))
        prefs.frameInfo.blockMode = blockIndependent;   /* no need for linked blocks */

    options.stableSrc = 1;

    if (dstMaxSize < LZ4F_compressFrameBound(srcSize, &prefs))
        return (size_t)-ERROR_dstMaxSize_tooSmall;

    errorCode = LZ4F_compressBegin(&cctxI, dstBuffer, dstMaxSize, &prefs);  /* write header */
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;   /* header size */

    errorCode = LZ4F_compressUpdate(&cctxI, dstPtr, dstEnd-dstPtr, srcBuffer, srcSize, &options);
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;

    errorCode = LZ4F_compressEnd(&cctxI, dstPtr, dstEnd-dstPtr, &options);   /* flush last block, and generate suffix */
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;

    if (prefs.compressionLevel >= minHClevel)   /* no allocation necessary with lz4 fast */
        FREEMEM(cctxI.lz4CtxPtr);

    return (dstPtr - dstStart);
}


/***********************************
* Advanced compression functions
* *********************************/

/* LZ4F_createCompressionContext() :
* The first thing to do is to create a compressionContext object, which will be used in all compression operations.
* This is achieved using LZ4F_createCompressionContext(), which takes as argument a version and an LZ4F_preferences_t structure.
* The version provided MUST be LZ4F_VERSION. It is intended to track potential version differences between different binaries.
* The function will provide a pointer to an allocated LZ4F_compressionContext_t object.
* If the result LZ4F_errorCode_t is not OK_NoError, there was an error during context creation.
* Object can release its memory using LZ4F_freeCompressionContext();
*/
LZ4F_errorCode_t LZ4F_createCompressionContext(LZ4F_compressionContext_t* LZ4F_compressionContextPtr, unsigned version)
{
    LZ4F_cctx_internal_t* cctxPtr;

    cctxPtr = (LZ4F_cctx_internal_t*)ALLOCATOR(sizeof(LZ4F_cctx_internal_t));
    if (cctxPtr==NULL) return (LZ4F_errorCode_t)(-ERROR_allocation_failed);

    cctxPtr->version = version;
    cctxPtr->cStage = 0;   /* Next stage : write header */

    *LZ4F_compressionContextPtr = (LZ4F_compressionContext_t)cctxPtr;

    return OK_NoError;
}


LZ4F_errorCode_t LZ4F_freeCompressionContext(LZ4F_compressionContext_t LZ4F_compressionContext)
{
    LZ4F_cctx_internal_t* cctxPtr = (LZ4F_cctx_internal_t*)LZ4F_compressionContext;

    FREEMEM(cctxPtr->lz4CtxPtr);
    FREEMEM(cctxPtr->tmpBuff);
    FREEMEM(LZ4F_compressionContext);

    return OK_NoError;
}


/* LZ4F_compressBegin() :
* will write the frame header into dstBuffer.
* dstBuffer must be large enough to accommodate a header (dstMaxSize). Maximum header size is LZ4F_MAXHEADERFRAME_SIZE bytes.
* The result of the function is the number of bytes written into dstBuffer for the header
* or an error code (can be tested using LZ4F_isError())
*/
size_t LZ4F_compressBegin(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_preferences_t prefNull;
    LZ4F_cctx_internal_t* cctxPtr = (LZ4F_cctx_internal_t*)compressionContext;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    BYTE* headerStart;
    size_t requiredBuffSize;

    if (dstMaxSize < LZ4F_MAXHEADERFRAME_SIZE) return (size_t)-ERROR_dstMaxSize_tooSmall;
    if (cctxPtr->cStage != 0) return (size_t)-ERROR_GENERIC;
    memset(&prefNull, 0, sizeof(prefNull));
    if (preferencesPtr == NULL) preferencesPtr = &prefNull;
    cctxPtr->prefs = *preferencesPtr;

    /* ctx Management */
    {
        U32 tableID = cctxPtr->prefs.compressionLevel<minHClevel ? 1 : 2;  /* 0:nothing ; 1:LZ4 table ; 2:HC tables */
        if (cctxPtr->lz4CtxLevel < tableID)
        {
            FREEMEM(cctxPtr->lz4CtxPtr);
            if (cctxPtr->prefs.compressionLevel<minHClevel)
                cctxPtr->lz4CtxPtr = (void*)LZ4_createStream();
            else
                cctxPtr->lz4CtxPtr = (void*)LZ4_createStreamHC();
            cctxPtr->lz4CtxLevel = tableID;
        }
    }

    /* Buffer Management */
    if (cctxPtr->prefs.frameInfo.blockSizeID == 0) cctxPtr->prefs.frameInfo.blockSizeID = LZ4F_BLOCKSIZEID_DEFAULT;
    cctxPtr->maxBlockSize = LZ4F_getBlockSize(cctxPtr->prefs.frameInfo.blockSizeID);

    requiredBuffSize = cctxPtr->maxBlockSize + ((cctxPtr->prefs.frameInfo.blockMode == blockLinked) * 128 KB);
    if (preferencesPtr->autoFlush)
        requiredBuffSize = (cctxPtr->prefs.frameInfo.blockMode == blockLinked) * 64 KB;   /* just needs dict */

    if (cctxPtr->maxBufferSize < requiredBuffSize)
    {
        cctxPtr->maxBufferSize = requiredBuffSize;
        FREEMEM(cctxPtr->tmpBuff);
        cctxPtr->tmpBuff = (BYTE*)ALLOCATOR(requiredBuffSize);
        if (cctxPtr->tmpBuff == NULL) return (size_t)-ERROR_allocation_failed;
    }
    cctxPtr->tmpIn = cctxPtr->tmpBuff;
    cctxPtr->tmpInSize = 0;
    XXH32_reset(&(cctxPtr->xxh), 0);
    if (cctxPtr->prefs.compressionLevel < minHClevel)
        LZ4_resetStream((LZ4_stream_t*)(cctxPtr->lz4CtxPtr));
    else
        LZ4_resetStreamHC((LZ4_streamHC_t*)(cctxPtr->lz4CtxPtr), cctxPtr->prefs.compressionLevel);

    /* Magic Number */
    LZ4F_writeLE32(dstPtr, LZ4F_MAGICNUMBER);
    dstPtr += 4;
    headerStart = dstPtr;

    /* FLG Byte */
    *dstPtr++ = ((1 & _2BITS) << 6)    /* Version('01') */
        + ((cctxPtr->prefs.frameInfo.blockMode & _1BIT ) << 5)    /* Block mode */
        + (BYTE)((cctxPtr->prefs.frameInfo.contentChecksumFlag & _1BIT ) << 2)   /* Frame checksum */
        + (BYTE)((cctxPtr->prefs.frameInfo.frameOSize > 0) << 3);   /* Frame content size */
    /* BD Byte */
    *dstPtr++ = (BYTE)((cctxPtr->prefs.frameInfo.blockSizeID & _3BITS) << 4);
    /* Optional Frame content size field */
    if (cctxPtr->prefs.frameInfo.frameOSize)
    {
        LZ4F_writeLE64(dstPtr, cctxPtr->prefs.frameInfo.frameOSize);
        dstPtr += 8;
        cctxPtr->totalInSize = 0;
    }
    /* CRC Byte */
    *dstPtr = LZ4F_headerChecksum(headerStart, dstPtr - headerStart);
    dstPtr++;

    cctxPtr->cStage = 1;   /* header written, now request input data block */

    return (dstPtr - dstStart);
}


/* LZ4F_compressBound() : gives the size of Dst buffer given a srcSize to handle worst case situations.
*                        The LZ4F_frameInfo_t structure is optional :
*                        you can provide NULL as argument, all preferences will then be set to default.
* */
size_t LZ4F_compressBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_preferences_t prefsNull;
    memset(&prefsNull, 0, sizeof(prefsNull));
    {
        const LZ4F_preferences_t* prefsPtr = (preferencesPtr==NULL) ? &prefsNull : preferencesPtr;
        blockSizeID_t bid = prefsPtr->frameInfo.blockSizeID;
        size_t blockSize = LZ4F_getBlockSize(bid);
        unsigned nbBlocks = (unsigned)(srcSize / blockSize) + 1;
        size_t lastBlockSize = prefsPtr->autoFlush ? srcSize % blockSize : blockSize;
        size_t blockInfo = 4;   /* default, without block CRC option */
        size_t frameEnd = 4 + (prefsPtr->frameInfo.contentChecksumFlag*4);

        return (blockInfo * nbBlocks) + (blockSize * (nbBlocks-1)) + lastBlockSize + frameEnd;;
    }
}


typedef int (*compressFunc_t)(void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level);

static size_t LZ4F_compressBlock(void* dst, const void* src, size_t srcSize, compressFunc_t compress, void* lz4ctx, int level)
{
    /* compress one block */
    BYTE* cSizePtr = (BYTE*)dst;
    U32 cSize;
    cSize = (U32)compress(lz4ctx, (const char*)src, (char*)(cSizePtr+4), (int)(srcSize), (int)(srcSize-1), level);
    LZ4F_writeLE32(cSizePtr, cSize);
    if (cSize == 0)   /* compression failed */
    {
        cSize = (U32)srcSize;
        LZ4F_writeLE32(cSizePtr, cSize + LZ4F_BLOCKUNCOMPRESSED_FLAG);
        memcpy(cSizePtr+4, src, srcSize);
    }
    return cSize + 4;
}


static int LZ4F_localLZ4_compress_limitedOutput_withState(void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level)
{
    (void) level;
    return LZ4_compress_limitedOutput_withState(ctx, src, dst, srcSize, dstSize);
}

static int LZ4F_localLZ4_compress_limitedOutput_continue(void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level)
{
    (void) level;
    return LZ4_compress_limitedOutput_continue((LZ4_stream_t*)ctx, src, dst, srcSize, dstSize);
}

static int LZ4F_localLZ4_compressHC_limitedOutput_continue(void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level)
{
    (void) level;
    return LZ4_compressHC_limitedOutput_continue((LZ4_streamHC_t*)ctx, src, dst, srcSize, dstSize);
}

static compressFunc_t LZ4F_selectCompression(blockMode_t blockMode, U32 level)
{
    if (level < minHClevel)
    {
        if (blockMode == blockIndependent) return LZ4F_localLZ4_compress_limitedOutput_withState;
        return LZ4F_localLZ4_compress_limitedOutput_continue;
    }
    if (blockMode == blockIndependent) return LZ4_compressHC2_limitedOutput_withStateHC;
    return LZ4F_localLZ4_compressHC_limitedOutput_continue;
}

static int LZ4F_localSaveDict(LZ4F_cctx_internal_t* cctxPtr)
{
    if (cctxPtr->prefs.compressionLevel < minHClevel)
        return LZ4_saveDict ((LZ4_stream_t*)(cctxPtr->lz4CtxPtr), (char*)(cctxPtr->tmpBuff), 64 KB);
    return LZ4_saveDictHC ((LZ4_streamHC_t*)(cctxPtr->lz4CtxPtr), (char*)(cctxPtr->tmpBuff), 64 KB);
}

typedef enum { notDone, fromTmpBuffer, fromSrcBuffer } LZ4F_lastBlockStatus;

/* LZ4F_compressUpdate()
* LZ4F_compressUpdate() can be called repetitively to compress as much data as necessary.
* The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
* If this condition is not respected, LZ4F_compress() will fail (result is an errorCode)
* You can get the minimum value of dstMaxSize by using LZ4F_compressBound()
* The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
* The result of the function is the number of bytes written into dstBuffer : it can be zero, meaning input data was just buffered.
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
*/
size_t LZ4F_compressUpdate(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_compressOptions_t* compressOptionsPtr)
{
    LZ4F_compressOptions_t cOptionsNull;
    LZ4F_cctx_internal_t* cctxPtr = (LZ4F_cctx_internal_t*)compressionContext;
    size_t blockSize = cctxPtr->maxBlockSize;
    const BYTE* srcPtr = (const BYTE*)srcBuffer;
    const BYTE* const srcEnd = srcPtr + srcSize;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    LZ4F_lastBlockStatus lastBlockCompressed = notDone;
    compressFunc_t compress;


    if (cctxPtr->cStage != 1) return (size_t)-ERROR_GENERIC;
    if (dstMaxSize < LZ4F_compressBound(srcSize, &(cctxPtr->prefs))) return (size_t)-ERROR_dstMaxSize_tooSmall;
    memset(&cOptionsNull, 0, sizeof(cOptionsNull));
    if (compressOptionsPtr == NULL) compressOptionsPtr = &cOptionsNull;

    /* select compression function */
    compress = LZ4F_selectCompression(cctxPtr->prefs.frameInfo.blockMode, cctxPtr->prefs.compressionLevel);

    /* complete tmp buffer */
    if (cctxPtr->tmpInSize > 0)   /* some data already within tmp buffer */
    {
        size_t sizeToCopy = blockSize - cctxPtr->tmpInSize;
        if (sizeToCopy > srcSize)
        {
            /* add src to tmpIn buffer */
            memcpy(cctxPtr->tmpIn + cctxPtr->tmpInSize, srcBuffer, srcSize);
            srcPtr = srcEnd;
            cctxPtr->tmpInSize += srcSize;
            /* still needs some CRC */
        }
        else
        {
            /* complete tmpIn block and then compress it */
            lastBlockCompressed = fromTmpBuffer;
            memcpy(cctxPtr->tmpIn + cctxPtr->tmpInSize, srcBuffer, sizeToCopy);
            srcPtr += sizeToCopy;

            dstPtr += LZ4F_compressBlock(dstPtr, cctxPtr->tmpIn, blockSize, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);

            if (cctxPtr->prefs.frameInfo.blockMode==blockLinked) cctxPtr->tmpIn += blockSize;
            cctxPtr->tmpInSize = 0;
        }
    }

    while ((size_t)(srcEnd - srcPtr) >= blockSize)
    {
        /* compress full block */
        lastBlockCompressed = fromSrcBuffer;
        dstPtr += LZ4F_compressBlock(dstPtr, srcPtr, blockSize, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);
        srcPtr += blockSize;
    }

    if ((cctxPtr->prefs.autoFlush) && (srcPtr < srcEnd))
    {
        /* compress remaining input < blockSize */
        lastBlockCompressed = fromSrcBuffer;
        dstPtr += LZ4F_compressBlock(dstPtr, srcPtr, srcEnd - srcPtr, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);
        srcPtr  = srcEnd;
    }

    /* preserve dictionary if necessary */
    if ((cctxPtr->prefs.frameInfo.blockMode==blockLinked) && (lastBlockCompressed==fromSrcBuffer))
    {
        if (compressOptionsPtr->stableSrc)
        {
            cctxPtr->tmpIn = cctxPtr->tmpBuff;
        }
        else
        {
            int realDictSize = LZ4F_localSaveDict(cctxPtr);
            if (realDictSize==0) return (size_t)-ERROR_GENERIC;
            cctxPtr->tmpIn = cctxPtr->tmpBuff + realDictSize;
        }
    }

    /* keep tmpIn within limits */
    if ((cctxPtr->tmpIn + blockSize) > (cctxPtr->tmpBuff + cctxPtr->maxBufferSize)   /* necessarily blockLinked && lastBlockCompressed==fromTmpBuffer */
        && !(cctxPtr->prefs.autoFlush))
    {
        LZ4F_localSaveDict(cctxPtr);
        cctxPtr->tmpIn = cctxPtr->tmpBuff + 64 KB;
    }

    /* some input data left, necessarily < blockSize */
    if (srcPtr < srcEnd)
    {
        /* fill tmp buffer */
        size_t sizeToCopy = srcEnd - srcPtr;
        memcpy(cctxPtr->tmpIn, srcPtr, sizeToCopy);
        cctxPtr->tmpInSize = sizeToCopy;
    }

    if (cctxPtr->prefs.frameInfo.contentChecksumFlag == contentChecksumEnabled)
        XXH32_update(&(cctxPtr->xxh), srcBuffer, srcSize);

    cctxPtr->totalInSize += srcSize;
    return dstPtr - dstStart;
}


/* LZ4F_flush()
* Should you need to create compressed data immediately, without waiting for a block to be filled,
* you can call LZ4_flush(), which will immediately compress any remaining data stored within compressionContext.
* The result of the function is the number of bytes written into dstBuffer
* (it can be zero, this means there was no data left within compressionContext)
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
* The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
*/
size_t LZ4F_flush(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* compressOptionsPtr)
{
    LZ4F_cctx_internal_t* cctxPtr = (LZ4F_cctx_internal_t*)compressionContext;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    compressFunc_t compress;


    if (cctxPtr->tmpInSize == 0) return 0;   /* nothing to flush */
    if (cctxPtr->cStage != 1) return (size_t)-ERROR_GENERIC;
    if (dstMaxSize < (cctxPtr->tmpInSize + 16)) return (size_t)-ERROR_dstMaxSize_tooSmall;
    (void)compressOptionsPtr;   /* not yet useful */

    /* select compression function */
    compress = LZ4F_selectCompression(cctxPtr->prefs.frameInfo.blockMode, cctxPtr->prefs.compressionLevel);

    /* compress tmp buffer */
    dstPtr += LZ4F_compressBlock(dstPtr, cctxPtr->tmpIn, cctxPtr->tmpInSize, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);
    if (cctxPtr->prefs.frameInfo.blockMode==blockLinked) cctxPtr->tmpIn += cctxPtr->tmpInSize;
    cctxPtr->tmpInSize = 0;

    /* keep tmpIn within limits */
    if ((cctxPtr->tmpIn + cctxPtr->maxBlockSize) > (cctxPtr->tmpBuff + cctxPtr->maxBufferSize))   /* necessarily blockLinked */
    {
        LZ4F_localSaveDict(cctxPtr);
        cctxPtr->tmpIn = cctxPtr->tmpBuff + 64 KB;
    }

    return dstPtr - dstStart;
}


/* LZ4F_compressEnd()
* When you want to properly finish the compressed frame, just call LZ4F_compressEnd().
* It will flush whatever data remained within compressionContext (like LZ4_flush())
* but also properly finalize the frame, with an endMark and a checksum.
* The result of the function is the number of bytes written into dstBuffer (necessarily >= 4 (endMark size))
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
* The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
* compressionContext can then be used again, starting with LZ4F_compressBegin(). The preferences will remain the same.
*/
size_t LZ4F_compressEnd(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* compressOptionsPtr)
{
    LZ4F_cctx_internal_t* cctxPtr = (LZ4F_cctx_internal_t*)compressionContext;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    size_t errorCode;

    errorCode = LZ4F_flush(compressionContext, dstBuffer, dstMaxSize, compressOptionsPtr);
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;

    LZ4F_writeLE32(dstPtr, 0);
    dstPtr+=4;   /* endMark */

    if (cctxPtr->prefs.frameInfo.contentChecksumFlag == contentChecksumEnabled)
    {
        U32 xxh = XXH32_digest(&(cctxPtr->xxh));
        LZ4F_writeLE32(dstPtr, xxh);
        dstPtr+=4;   /* content Checksum */
    }

    cctxPtr->cStage = 0;   /* state is now re-usable (with identical preferences) */

    if (cctxPtr->prefs.frameInfo.frameOSize)
    {
        if (cctxPtr->prefs.frameInfo.frameOSize != cctxPtr->totalInSize)
            return (size_t)-ERROR_frameSize_wrong;
    }

    return dstPtr - dstStart;
}


/**********************************
*  Decompression functions
**********************************/

/* Resource management */

/* LZ4F_createDecompressionContext() :
* The first thing to do is to create a decompressionContext object, which will be used in all decompression operations.
* This is achieved using LZ4F_createDecompressionContext().
* The function will provide a pointer to a fully allocated and initialized LZ4F_decompressionContext object.
* If the result LZ4F_errorCode_t is not zero, there was an error during context creation.
* Object can release its memory using LZ4F_freeDecompressionContext();
*/
LZ4F_errorCode_t LZ4F_createDecompressionContext(LZ4F_decompressionContext_t* LZ4F_decompressionContextPtr, unsigned versionNumber)
{
    LZ4F_dctx_internal_t* dctxPtr;

    dctxPtr = (LZ4F_dctx_internal_t*)ALLOCATOR(sizeof(LZ4F_dctx_internal_t));
    if (dctxPtr==NULL) return (LZ4F_errorCode_t)-ERROR_GENERIC;

    dctxPtr->version = versionNumber;
    *LZ4F_decompressionContextPtr = (LZ4F_compressionContext_t)dctxPtr;
    return OK_NoError;
}

LZ4F_errorCode_t LZ4F_freeDecompressionContext(LZ4F_decompressionContext_t LZ4F_decompressionContext)
{
    LZ4F_dctx_internal_t* dctxPtr = (LZ4F_dctx_internal_t*)LZ4F_decompressionContext;
    FREEMEM(dctxPtr->tmpIn);
    FREEMEM(dctxPtr->tmpOutBuffer);
    FREEMEM(dctxPtr);
    return OK_NoError;
}


/* ******************************************************************** */
/* ********************* Decompression ******************************** */
/* ******************************************************************** */

typedef enum { dstage_getHeader=0, dstage_storeHeader,
    dstage_getCBlockSize, dstage_storeCBlockSize,
    dstage_copyDirect,
    dstage_getCBlock, dstage_storeCBlock, dstage_decodeCBlock,
    dstage_decodeCBlock_intoDst, dstage_decodeCBlock_intoTmp, dstage_flushOut,
    dstage_getSuffix, dstage_storeSuffix,
    dstage_getSFrameSize, dstage_storeSFrameSize,
    dstage_skipSkippable
} dStage_t;


/* LZ4F_decodeHeader
   return : nb Bytes read from srcVoidPtr (necessarily <= srcSize)
            or an error code (testable with LZ4F_isError())
   output : set internal values of dctx, such as
            dctxPtr->frameInfo and dctxPtr->dStage.
*/
static size_t LZ4F_decodeHeader(LZ4F_dctx_internal_t* dctxPtr, const void* srcVoidPtr, size_t srcSize)
{
    BYTE FLG, BD, HC;
    unsigned version, blockMode, blockChecksumFlag, contentSizeFlag, contentChecksumFlag, blockSizeID;
    size_t bufferNeeded;
    size_t frameHeaderSize;
    const BYTE* srcPtr = (const BYTE*)srcVoidPtr;

    /* need to decode header to get frameInfo */
    if (srcSize < minFHSize) return (size_t)-ERROR_GENERIC;   /* minimal header size */
    memset(&(dctxPtr->frameInfo), 0, sizeof(dctxPtr->frameInfo));

    /* skippable frames */
    if ((LZ4F_readLE32(srcPtr) & 0xFFFFFFF0U) == LZ4F_MAGIC_SKIPPABLE_START)
    {
        dctxPtr->frameInfo.frameType = skippableFrame;
        if (srcVoidPtr == (void*)(dctxPtr->header))
        {
            dctxPtr->tmpInSize = srcSize;
            dctxPtr->tmpInTarget = 8;
            dctxPtr->dStage = dstage_storeSFrameSize;
            return srcSize;
        }
        else
        {
            dctxPtr->dStage = dstage_getSFrameSize;
            return 4;
        }
    }

    /* control magic number */
    if (LZ4F_readLE32(srcPtr) != LZ4F_MAGICNUMBER) return (size_t)-ERROR_frameType_unknown;
    dctxPtr->frameInfo.frameType = LZ4F_frame;

    /* Flags */
    FLG = srcPtr[4];
    version = (FLG>>6) & _2BITS;
    blockMode = (FLG>>5) & _1BIT;
    blockChecksumFlag = (FLG>>4) & _1BIT;
    contentSizeFlag = (FLG>>3) & _1BIT;
    contentChecksumFlag = (FLG>>2) & _1BIT;

    /* Frame Header Size */
    frameHeaderSize = contentSizeFlag ? 15 : 7;

    if (srcSize < frameHeaderSize)
    {
        if (srcPtr != dctxPtr->header)
            memcpy(dctxPtr->header, srcPtr, srcSize);
        dctxPtr->tmpInSize = srcSize;
        dctxPtr->tmpInTarget = frameHeaderSize;
        dctxPtr->dStage = dstage_storeHeader;
        return srcSize;
    }

    BD = srcPtr[5];
    blockSizeID = (BD>>4) & _3BITS;

    /* validate */
    if (version != 1) return (size_t)-ERROR_GENERIC;           /* Version Number, only supported value */
    if (blockChecksumFlag != 0) return (size_t)-ERROR_GENERIC; /* Only supported value for the time being */
    if (((FLG>>0)&_2BITS) != 0) return (size_t)-ERROR_GENERIC; /* Reserved bits */
    if (((BD>>7)&_1BIT) != 0) return (size_t)-ERROR_GENERIC;   /* Reserved bit */
    if (blockSizeID < 4) return (size_t)-ERROR_GENERIC;        /* 4-7 only supported values for the time being */
    if (((BD>>0)&_4BITS) != 0) return (size_t)-ERROR_GENERIC;  /* Reserved bits */

    /* check */
    HC = LZ4F_headerChecksum(srcPtr+4, frameHeaderSize-5);
    if (HC != srcPtr[frameHeaderSize-1]) return (size_t)-ERROR_GENERIC;   /* Bad header checksum error */

    /* save */
    dctxPtr->frameInfo.blockMode = (blockMode_t)blockMode;
    dctxPtr->frameInfo.contentChecksumFlag = (contentChecksum_t)contentChecksumFlag;
    dctxPtr->frameInfo.blockSizeID = (blockSizeID_t)blockSizeID;
    dctxPtr->maxBlockSize = LZ4F_getBlockSize(blockSizeID);
    if (contentSizeFlag)
        dctxPtr->frameInfo.frameOSize = LZ4F_readLE64(srcPtr+6);

    /* init */
    if (contentChecksumFlag) XXH32_reset(&(dctxPtr->xxh), 0);

    /* alloc */
    bufferNeeded = dctxPtr->maxBlockSize + ((dctxPtr->frameInfo.blockMode==blockLinked) * 128 KB);
    if (bufferNeeded > dctxPtr->maxBufferSize)   /* tmp buffers too small */
    {
        FREEMEM(dctxPtr->tmpIn);
        FREEMEM(dctxPtr->tmpOutBuffer);
        dctxPtr->maxBufferSize = bufferNeeded;
        dctxPtr->tmpIn = (BYTE*)ALLOCATOR(dctxPtr->maxBlockSize);
        if (dctxPtr->tmpIn == NULL) return (size_t)-ERROR_GENERIC;
        dctxPtr->tmpOutBuffer= (BYTE*)ALLOCATOR(dctxPtr->maxBufferSize);
        if (dctxPtr->tmpOutBuffer== NULL) return (size_t)-ERROR_GENERIC;
    }
    dctxPtr->tmpInSize = 0;
    dctxPtr->tmpInTarget = 0;
    dctxPtr->dict = dctxPtr->tmpOutBuffer;
    dctxPtr->dictSize = 0;
    dctxPtr->tmpOut = dctxPtr->tmpOutBuffer;
    dctxPtr->tmpOutStart = 0;
    dctxPtr->tmpOutSize = 0;

    dctxPtr->dStage = dstage_getCBlockSize;

    return frameHeaderSize;
}


/* LZ4F_getFrameInfo()
* This function decodes frame header information, such as blockSize.
* It is optional : you could start by calling directly LZ4F_decompress() instead.
* The objective is to extract header information without starting decompression, typically for allocation purposes.
* LZ4F_getFrameInfo() can also be used *after* starting decompression, on a valid LZ4F_decompressionContext_t.
* The number of bytes read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
* You are expected to resume decompression from where it stopped (srcBuffer + *srcSizePtr)
* The function result is an hint of the better srcSize to use for next call to LZ4F_decompress,
* or an error code which can be tested using LZ4F_isError().
*/
LZ4F_errorCode_t LZ4F_getFrameInfo(LZ4F_decompressionContext_t decompressionContext, LZ4F_frameInfo_t* frameInfoPtr, const void* srcBuffer, size_t* srcSizePtr)
{
    LZ4F_dctx_internal_t* dctxPtr = (LZ4F_dctx_internal_t*)decompressionContext;

    if (dctxPtr->dStage == dstage_getHeader)
    {
        LZ4F_errorCode_t errorCode = LZ4F_decodeHeader(dctxPtr, srcBuffer, *srcSizePtr);
        if (LZ4F_isError(errorCode)) return errorCode;
        *srcSizePtr = errorCode;   /* nb Bytes consumed */
        *frameInfoPtr = dctxPtr->frameInfo;
        dctxPtr->srcExpect = NULL;
        return 4;   /* nextSrcSizeHint : 4 == block header size */
    }

    /* frameInfo already decoded */
    *srcSizePtr = 0;
    *frameInfoPtr = dctxPtr->frameInfo;
    return 0;
}


/* redirector, with common prototype */
static int LZ4F_decompress_safe (const char* source, char* dest, int compressedSize, int maxDecompressedSize, const char* dictStart, int dictSize)
{
    (void)dictStart; (void)dictSize;
    return LZ4_decompress_safe (source, dest, compressedSize, maxDecompressedSize);
}


static void LZ4F_updateDict(LZ4F_dctx_internal_t* dctxPtr, const BYTE* dstPtr, size_t dstSize, const BYTE* dstPtr0, unsigned withinTmp)
{
    if (dctxPtr->dictSize==0)
        dctxPtr->dict = (BYTE*)dstPtr;   /* priority to dictionary continuity */

    if (dctxPtr->dict + dctxPtr->dictSize == dstPtr)   /* dictionary continuity */
    {
        dctxPtr->dictSize += dstSize;
        return;
    }

    if (dstPtr - dstPtr0 + dstSize >= 64 KB)   /* dstBuffer large enough to become dictionary */
    {
        dctxPtr->dict = (BYTE*)dstPtr0;
        dctxPtr->dictSize = dstPtr - dstPtr0 + dstSize;
        return;
    }

    if ((withinTmp) && (dctxPtr->dict == dctxPtr->tmpOutBuffer))
    {
        /* assumption : dctxPtr->dict + dctxPtr->dictSize == dctxPtr->tmpOut + dctxPtr->tmpOutStart */
        dctxPtr->dictSize += dstSize;
        return;
    }

    if (withinTmp) /* copy relevant dict portion in front of tmpOut within tmpOutBuffer */
    {
        size_t preserveSize = dctxPtr->tmpOut - dctxPtr->tmpOutBuffer;
        size_t copySize = 64 KB - dctxPtr->tmpOutSize;
        BYTE* oldDictEnd = dctxPtr->dict + dctxPtr->dictSize - dctxPtr->tmpOutStart;
        if (dctxPtr->tmpOutSize > 64 KB) copySize = 0;
        if (copySize > preserveSize) copySize = preserveSize;

        memcpy(dctxPtr->tmpOutBuffer + preserveSize - copySize, oldDictEnd - copySize, copySize);

        dctxPtr->dict = dctxPtr->tmpOutBuffer;
        dctxPtr->dictSize = preserveSize + dctxPtr->tmpOutStart + dstSize;
        return;
    }

    if (dctxPtr->dict == dctxPtr->tmpOutBuffer)     /* copy dst into tmp to complete dict */
    {
        if (dctxPtr->dictSize + dstSize > dctxPtr->maxBufferSize)   /* tmp buffer not large enough */
        {
            size_t preserveSize = 64 KB - dstSize;   /* note : dstSize < 64 KB */
            memcpy(dctxPtr->dict, dctxPtr->dict + dctxPtr->dictSize - preserveSize, preserveSize);
            dctxPtr->dictSize = preserveSize;
        }
        memcpy(dctxPtr->dict + dctxPtr->dictSize, dstPtr, dstSize);
        dctxPtr->dictSize += dstSize;
        return;
    }

    /* join dict & dest into tmp */
    {
        size_t preserveSize = 64 KB - dstSize;   /* note : dstSize < 64 KB */
        if (preserveSize > dctxPtr->dictSize) preserveSize = dctxPtr->dictSize;
        memcpy(dctxPtr->tmpOutBuffer, dctxPtr->dict + dctxPtr->dictSize - preserveSize, preserveSize);
        memcpy(dctxPtr->tmpOutBuffer + preserveSize, dstPtr, dstSize);
        dctxPtr->dict = dctxPtr->tmpOutBuffer;
        dctxPtr->dictSize = preserveSize + dstSize;
    }
}



/* LZ4F_decompress()
* Call this function repetitively to regenerate data compressed within srcBuffer.
* The function will attempt to decode *srcSizePtr from srcBuffer, into dstBuffer of maximum size *dstSizePtr.
*
* The number of bytes regenerated into dstBuffer will be provided within *dstSizePtr (necessarily <= original value).
*
* The number of bytes effectively read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
* If the number of bytes read is < number of bytes provided, then the decompression operation is not complete.
* You will have to call it again, continuing from where it stopped.
*
* The function result is an hint of the better srcSize to use for next call to LZ4F_decompress.
* Basically, it's the size of the current (or remaining) compressed block + header of next block.
* Respecting the hint provides some boost to performance, since it allows less buffer shuffling.
* Note that this is just a hint, you can always provide any srcSize you want.
* When a frame is fully decoded, the function result will be 0.
* If decompression failed, function result is an error code which can be tested using LZ4F_isError().
*/
size_t LZ4F_decompress(LZ4F_decompressionContext_t decompressionContext,
                       void* dstBuffer, size_t* dstSizePtr,
                       const void* srcBuffer, size_t* srcSizePtr,
                       const LZ4F_decompressOptions_t* decompressOptionsPtr)
{
    LZ4F_dctx_internal_t* dctxPtr = (LZ4F_dctx_internal_t*)decompressionContext;
    LZ4F_decompressOptions_t optionsNull;
    const BYTE* const srcStart = (const BYTE*)srcBuffer;
    const BYTE* const srcEnd = srcStart + *srcSizePtr;
    const BYTE* srcPtr = srcStart;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* const dstEnd = dstStart + *dstSizePtr;
    BYTE* dstPtr = dstStart;
    const BYTE* selectedIn = NULL;
    unsigned doAnotherStage = 1;
    size_t nextSrcSizeHint = 1;


    memset(&optionsNull, 0, sizeof(optionsNull));
    if (decompressOptionsPtr==NULL) decompressOptionsPtr = &optionsNull;
    *srcSizePtr = 0;
    *dstSizePtr = 0;

    /* expect to continue decoding src buffer where it left previously */
    if (dctxPtr->srcExpect != NULL)
    {
        if (srcStart != dctxPtr->srcExpect) return (size_t)-ERROR_wrongSrcPtr;
    }

    /* programmed as a state machine */

    while (doAnotherStage)
    {

        switch(dctxPtr->dStage)
        {

        case dstage_getHeader:
            {
                if (srcEnd-srcPtr >= 7)
                {
                    LZ4F_errorCode_t errorCode = LZ4F_decodeHeader(dctxPtr, srcPtr, srcEnd-srcPtr);
                    if (LZ4F_isError(errorCode)) return errorCode;
                    srcPtr += errorCode;
                    break;
                }
                dctxPtr->tmpInSize = 0;
                dctxPtr->tmpInTarget = 7;
                dctxPtr->dStage = dstage_storeHeader;
            }

        case dstage_storeHeader:
            {
                size_t sizeToCopy = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy =  srcEnd - srcPtr;
                memcpy(dctxPtr->header + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                dctxPtr->tmpInSize += sizeToCopy;
                srcPtr += sizeToCopy;
                if (dctxPtr->tmpInSize < dctxPtr->tmpInTarget)
                {
                    nextSrcSizeHint = (dctxPtr->tmpInTarget - dctxPtr->tmpInSize) + 4;
                    doAnotherStage = 0;   /* not enough src data, ask for some more */
                    break;
                }
                LZ4F_errorCode_t errorCode = LZ4F_decodeHeader(dctxPtr, dctxPtr->header, dctxPtr->tmpInTarget);
                if (LZ4F_isError(errorCode)) return errorCode;
                break;
            }

        case dstage_getCBlockSize:
            {
                if ((srcEnd - srcPtr) >= 4)
                {
                    selectedIn = srcPtr;
                    srcPtr += 4;
                }
                else
                {
                /* not enough input to read cBlockSize field */
                    dctxPtr->tmpInSize = 0;
                    dctxPtr->dStage = dstage_storeCBlockSize;
                }
            }

            if (dctxPtr->dStage == dstage_storeCBlockSize)
        case dstage_storeCBlockSize:
            {
                size_t sizeToCopy = 4 - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy = srcEnd - srcPtr;
                memcpy(dctxPtr->tmpIn + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                srcPtr += sizeToCopy;
                dctxPtr->tmpInSize += sizeToCopy;
                if (dctxPtr->tmpInSize < 4) /* not enough input to get full cBlockSize; wait for more */
                {
                    nextSrcSizeHint = 4 - dctxPtr->tmpInSize;
                    doAnotherStage=0;
                    break;
                }
                selectedIn = dctxPtr->tmpIn;
            }

        /* case dstage_decodeCBlockSize: */   /* no more direct access, to prevent scan-build warning */
            {
                size_t nextCBlockSize = LZ4F_readLE32(selectedIn) & 0x7FFFFFFFU;
                if (nextCBlockSize==0)   /* frameEnd signal, no more CBlock */
                {
                    dctxPtr->dStage = dstage_getSuffix;
                    break;
                }
                if (nextCBlockSize > dctxPtr->maxBlockSize) return (size_t)-ERROR_GENERIC;   /* invalid cBlockSize */
                dctxPtr->tmpInTarget = nextCBlockSize;
                if (LZ4F_readLE32(selectedIn) & LZ4F_BLOCKUNCOMPRESSED_FLAG)
                {
                    dctxPtr->dStage = dstage_copyDirect;
                    break;
                }
                dctxPtr->dStage = dstage_getCBlock;
                if (dstPtr==dstEnd)
                {
                    nextSrcSizeHint = nextCBlockSize + 4;
                    doAnotherStage = 0;
                }
                break;
            }

        case dstage_copyDirect:   /* uncompressed block */
            {
                size_t sizeToCopy = dctxPtr->tmpInTarget;
                if ((size_t)(srcEnd-srcPtr) < sizeToCopy) sizeToCopy = srcEnd - srcPtr;  /* not enough input to read full block */
                if ((size_t)(dstEnd-dstPtr) < sizeToCopy) sizeToCopy = dstEnd - dstPtr;
                memcpy(dstPtr, srcPtr, sizeToCopy);
                if (dctxPtr->frameInfo.contentChecksumFlag) XXH32_update(&(dctxPtr->xxh), srcPtr, (U32)sizeToCopy);

                /* dictionary management */
                if (dctxPtr->frameInfo.blockMode==blockLinked)
                    LZ4F_updateDict(dctxPtr, dstPtr, sizeToCopy, dstStart, 0);

                srcPtr += sizeToCopy;
                dstPtr += sizeToCopy;
                if (sizeToCopy == dctxPtr->tmpInTarget)   /* all copied */
                {
                    dctxPtr->dStage = dstage_getCBlockSize;
                    break;
                }
                dctxPtr->tmpInTarget -= sizeToCopy;   /* still need to copy more */
                nextSrcSizeHint = dctxPtr->tmpInTarget + 4;
                doAnotherStage = 0;
                break;
            }

        case dstage_getCBlock:   /* entry from dstage_decodeCBlockSize */
            {
                if ((size_t)(srcEnd-srcPtr) < dctxPtr->tmpInTarget)
                {
                    dctxPtr->tmpInSize = 0;
                    dctxPtr->dStage = dstage_storeCBlock;
                    break;
                }
                selectedIn = srcPtr;
                srcPtr += dctxPtr->tmpInTarget;
                dctxPtr->dStage = dstage_decodeCBlock;
                break;
            }

        case dstage_storeCBlock:
            {
                size_t sizeToCopy = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd-srcPtr)) sizeToCopy = srcEnd-srcPtr;
                memcpy(dctxPtr->tmpIn + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                dctxPtr->tmpInSize += sizeToCopy;
                srcPtr += sizeToCopy;
                if (dctxPtr->tmpInSize < dctxPtr->tmpInTarget)  /* need more input */
                {
                    nextSrcSizeHint = (dctxPtr->tmpInTarget - dctxPtr->tmpInSize) + 4;
                    doAnotherStage=0;
                    break;
                }
                selectedIn = dctxPtr->tmpIn;
                dctxPtr->dStage = dstage_decodeCBlock;
                break;
            }

        case dstage_decodeCBlock:
            {
                if ((size_t)(dstEnd-dstPtr) < dctxPtr->maxBlockSize)   /* not enough place into dst : decode into tmpOut */
                    dctxPtr->dStage = dstage_decodeCBlock_intoTmp;
                else
                    dctxPtr->dStage = dstage_decodeCBlock_intoDst;
                break;
            }

        case dstage_decodeCBlock_intoDst:
            {
                int (*decoder)(const char*, char*, int, int, const char*, int);
                int decodedSize;

                if (dctxPtr->frameInfo.blockMode == blockLinked)
                    decoder = LZ4_decompress_safe_usingDict;
                else
                    decoder = LZ4F_decompress_safe;

                decodedSize = decoder((const char*)selectedIn, (char*)dstPtr, (int)dctxPtr->tmpInTarget, (int)dctxPtr->maxBlockSize, (const char*)dctxPtr->dict, (int)dctxPtr->dictSize);
                if (decodedSize < 0) return (size_t)-ERROR_GENERIC;   /* decompression failed */
                if (dctxPtr->frameInfo.contentChecksumFlag) XXH32_update(&(dctxPtr->xxh), dstPtr, decodedSize);

                /* dictionary management */
                if (dctxPtr->frameInfo.blockMode==blockLinked)
                    LZ4F_updateDict(dctxPtr, dstPtr, decodedSize, dstStart, 0);

                dstPtr += decodedSize;
                dctxPtr->dStage = dstage_getCBlockSize;
                break;
            }

        case dstage_decodeCBlock_intoTmp:
            {
                /* not enough place into dst : decode into tmpOut */
                int (*decoder)(const char*, char*, int, int, const char*, int);
                int decodedSize;

                if (dctxPtr->frameInfo.blockMode == blockLinked)
                    decoder = LZ4_decompress_safe_usingDict;
                else
                    decoder = LZ4F_decompress_safe;

                /* ensure enough place for tmpOut */
                if (dctxPtr->frameInfo.blockMode == blockLinked)
                {
                    if (dctxPtr->dict == dctxPtr->tmpOutBuffer)
                    {
                        if (dctxPtr->dictSize > 128 KB)
                        {
                            memcpy(dctxPtr->dict, dctxPtr->dict + dctxPtr->dictSize - 64 KB, 64 KB);
                            dctxPtr->dictSize = 64 KB;
                        }
                        dctxPtr->tmpOut = dctxPtr->dict + dctxPtr->dictSize;
                    }
                    else   /* dict not within tmp */
                    {
                        size_t reservedDictSpace = dctxPtr->dictSize;
                        if (reservedDictSpace > 64 KB) reservedDictSpace = 64 KB;
                        dctxPtr->tmpOut = dctxPtr->tmpOutBuffer + reservedDictSpace;
                    }
                }

                /* Decode */
                decodedSize = decoder((const char*)selectedIn, (char*)dctxPtr->tmpOut, (int)dctxPtr->tmpInTarget, (int)dctxPtr->maxBlockSize, (const char*)dctxPtr->dict, (int)dctxPtr->dictSize);
                if (decodedSize < 0) return (size_t)-ERROR_decompressionFailed;   /* decompression failed */
                if (dctxPtr->frameInfo.contentChecksumFlag) XXH32_update(&(dctxPtr->xxh), dctxPtr->tmpOut, decodedSize);
                dctxPtr->tmpOutSize = decodedSize;
                dctxPtr->tmpOutStart = 0;
                dctxPtr->dStage = dstage_flushOut;
                break;
            }

        case dstage_flushOut:  /* flush decoded data from tmpOut to dstBuffer */
            {
                size_t sizeToCopy = dctxPtr->tmpOutSize - dctxPtr->tmpOutStart;
                if (sizeToCopy > (size_t)(dstEnd-dstPtr)) sizeToCopy = dstEnd-dstPtr;
                memcpy(dstPtr, dctxPtr->tmpOut + dctxPtr->tmpOutStart, sizeToCopy);

                /* dictionary management */
                if (dctxPtr->frameInfo.blockMode==blockLinked)
                    LZ4F_updateDict(dctxPtr, dstPtr, sizeToCopy, dstStart, 1);

                dctxPtr->tmpOutStart += sizeToCopy;
                dstPtr += sizeToCopy;

                /* end of flush ? */
                if (dctxPtr->tmpOutStart == dctxPtr->tmpOutSize)
                {
                    dctxPtr->dStage = dstage_getCBlockSize;
                    break;
                }
                nextSrcSizeHint = 4;
                doAnotherStage = 0;   /* still some data to flush */
                break;
            }

        case dstage_getSuffix:
            {
                size_t suffixSize = dctxPtr->frameInfo.contentChecksumFlag * 4;
                if (suffixSize == 0)   /* frame completed */
                {
                    nextSrcSizeHint = 0;
                    dctxPtr->dStage = dstage_getHeader;
                    doAnotherStage = 0;
                    break;
                }
                if ((srcEnd - srcPtr) < 4)   /* not enough size for entire CRC */
                {
                    dctxPtr->tmpInSize = 0;
                    dctxPtr->dStage = dstage_storeSuffix;
                }
                else
                {
                    selectedIn = srcPtr;
                    srcPtr += 4;
                }
            }

            if (dctxPtr->dStage == dstage_storeSuffix)
        case dstage_storeSuffix:
            {
                size_t sizeToCopy = 4 - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy = srcEnd - srcPtr;
                memcpy(dctxPtr->tmpIn + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                srcPtr += sizeToCopy;
                dctxPtr->tmpInSize += sizeToCopy;
                if (dctxPtr->tmpInSize < 4)  /* not enough input to read complete suffix */
                {
                    nextSrcSizeHint = 4 - dctxPtr->tmpInSize;
                    doAnotherStage=0;
                    break;
                }
                selectedIn = dctxPtr->tmpIn;
            }

        /* case dstage_checkSuffix: */   /* no direct call, to avoid scan-build warning */
            {
                U32 readCRC = LZ4F_readLE32(selectedIn);
                U32 resultCRC = XXH32_digest(&(dctxPtr->xxh));
                if (readCRC != resultCRC) return (size_t)-ERROR_checksum_invalid;
                nextSrcSizeHint = 0;
                dctxPtr->dStage = dstage_getHeader;
                doAnotherStage = 0;
                break;
            }

        case dstage_getSFrameSize:
            {
                if ((srcEnd - srcPtr) >= 4)
                {
                    selectedIn = srcPtr;
                    srcPtr += 4;
                }
                else
                {
                /* not enough input to read cBlockSize field */
                    dctxPtr->tmpInSize = 4;
                    dctxPtr->tmpInTarget = 8;
                    dctxPtr->dStage = dstage_storeSFrameSize;
                }
            }

            if (dctxPtr->dStage == dstage_storeSFrameSize)
        case dstage_storeSFrameSize:
            {
                size_t sizeToCopy = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy = srcEnd - srcPtr;
                memcpy(dctxPtr->header + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                srcPtr += sizeToCopy;
                dctxPtr->tmpInSize += sizeToCopy;
                if (dctxPtr->tmpInSize < dctxPtr->tmpInTarget) /* not enough input to get full sBlockSize; wait for more */
                {
                    nextSrcSizeHint = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                    doAnotherStage = 0;
                    break;
                }
                selectedIn = dctxPtr->header + 4;
            }

        /* case dstage_decodeSBlockSize: */   /* no direct access */
            {
                size_t SFrameSize = LZ4F_readLE32(selectedIn);
                dctxPtr->frameInfo.frameOSize = SFrameSize;
                dctxPtr->tmpInTarget = SFrameSize;
                dctxPtr->dStage = dstage_skipSkippable;
                break;
            }

        case dstage_skipSkippable:
            {
                size_t skipSize = dctxPtr->tmpInTarget;
                if (skipSize > (size_t)(srcEnd-srcPtr)) skipSize = srcEnd-srcPtr;
                srcPtr += skipSize;
                dctxPtr->tmpInTarget -= skipSize;
                doAnotherStage = 0;
                nextSrcSizeHint = dctxPtr->tmpInTarget;
                if (nextSrcSizeHint) break;
                dctxPtr->dStage = dstage_getHeader;
                break;
            }
        }
    }

    /* preserve dictionary within tmp if necessary */
    if ( (dctxPtr->frameInfo.blockMode==blockLinked)
        &&(dctxPtr->dict != dctxPtr->tmpOutBuffer)
        &&(!decompressOptionsPtr->stableDst)
        &&((unsigned)(dctxPtr->dStage-1) < (unsigned)(dstage_getSuffix-1))
        )
    {
        if (dctxPtr->dStage == dstage_flushOut)
        {
            size_t preserveSize = dctxPtr->tmpOut - dctxPtr->tmpOutBuffer;
            size_t copySize = 64 KB - dctxPtr->tmpOutSize;
            BYTE* oldDictEnd = dctxPtr->dict + dctxPtr->dictSize - dctxPtr->tmpOutStart;
            if (dctxPtr->tmpOutSize > 64 KB) copySize = 0;
            if (copySize > preserveSize) copySize = preserveSize;

            memcpy(dctxPtr->tmpOutBuffer + preserveSize - copySize, oldDictEnd - copySize, copySize);

            dctxPtr->dict = dctxPtr->tmpOutBuffer;
            dctxPtr->dictSize = preserveSize + dctxPtr->tmpOutStart;
        }
        else
        {
            size_t newDictSize = dctxPtr->dictSize;
            BYTE* oldDictEnd = dctxPtr->dict + dctxPtr->dictSize;
            if ((newDictSize) > 64 KB) newDictSize = 64 KB;

            memcpy(dctxPtr->tmpOutBuffer, oldDictEnd - newDictSize, newDictSize);

            dctxPtr->dict = dctxPtr->tmpOutBuffer;
            dctxPtr->dictSize = newDictSize;
            dctxPtr->tmpOut = dctxPtr->tmpOutBuffer + newDictSize;
        }
    }

    /* require function to be called again from position where it stopped */
    if (srcPtr<srcEnd)
        dctxPtr->srcExpect = srcPtr;
    else
        dctxPtr->srcExpect = NULL;

    *srcSizePtr = (srcPtr - srcStart);
    *dstSizePtr = (dstPtr - dstStart);
    return nextSrcSizeHint;
}

/* lz4frame.c EOF */

typedef struct sslz4filter sslz4filter;

struct sslz4filter {
	void *ctx;
} sspacked;

static int
ss_lz4filter_init(ssfilter *f, va_list args ssunused)
{
	sslz4filter *z = (sslz4filter*)f->priv;
	LZ4F_errorCode_t rc = -1;
	switch (f->op) {
	case SS_FINPUT:
		rc = LZ4F_createCompressionContext(&z->ctx, LZ4F_VERSION);
		break;	
	case SS_FOUTPUT:
		rc = LZ4F_createDecompressionContext(&z->ctx, LZ4F_VERSION);
		break;	
	}
	if (ssunlikely(rc != 0))
		return -1;
	return 0;
}

static int
ss_lz4filter_free(ssfilter *f)
{
	sslz4filter *z = (sslz4filter*)f->priv;
	(void)z;
	switch (f->op) {
	case SS_FINPUT:
		LZ4F_freeCompressionContext(z->ctx);
		break;	
	case SS_FOUTPUT:
		LZ4F_freeDecompressionContext(z->ctx);
		break;	
	}
	return 0;
}

static int
ss_lz4filter_reset(ssfilter *f)
{
	sslz4filter *z = (sslz4filter*)f->priv;
	(void)z;
	switch (f->op) {
	case SS_FINPUT:
		break;	
	case SS_FOUTPUT:
		break;
	}
	return 0;
}

static int
ss_lz4filter_start(ssfilter *f, ssbuf *dest)
{
	sslz4filter *z = (sslz4filter*)f->priv;
	int rc;
	size_t block;
	size_t sz;
	switch (f->op) {
	case SS_FINPUT:;
		block = LZ4F_MAXHEADERFRAME_SIZE;
		rc = ss_bufensure(dest, f->a, block);
		if (ssunlikely(rc == -1))
			return -1;
		sz = LZ4F_compressBegin(z->ctx, dest->p, block, NULL);
		if (ssunlikely(LZ4F_isError(sz)))
			return -1;
		ss_bufadvance(dest, sz);
		break;	
	case SS_FOUTPUT:
		/* do nothing */
		break;
	}
	return 0;
}

static int
ss_lz4filter_next(ssfilter *f, ssbuf *dest, char *buf, int size)
{
	sslz4filter *z = (sslz4filter*)f->priv;
	if (ssunlikely(size == 0))
		return 0;
	int rc;
	switch (f->op) {
	case SS_FINPUT:;
		size_t block = LZ4F_compressBound(size, NULL);
		rc = ss_bufensure(dest, f->a, block);
		if (ssunlikely(rc == -1))
			return -1;
		size_t sz = LZ4F_compressUpdate(z->ctx, dest->p, block, buf, size, NULL);
		if (ssunlikely(LZ4F_isError(sz)))
			return -1;
		ss_bufadvance(dest, sz);
		break;	
	case SS_FOUTPUT:;
		/* do a single-pass decompression.
		 *
		 * Assume that destination buffer is allocated to
		 * original size.
		 */
		size_t pos = 0;
		while (pos < (size_t)size)
		{
			size_t o_size = ss_bufunused(dest);
			size_t i_size = size - pos;
			LZ4F_errorCode_t rc;
			rc = LZ4F_decompress(z->ctx, dest->p, &o_size, buf + pos, &i_size, NULL);
			if (LZ4F_isError(rc))
				return -1;
			ss_bufadvance(dest, o_size);
			pos += i_size;
		}
		break;
	}
	return 0;
}

static int
ss_lz4filter_complete(ssfilter *f, ssbuf *dest)
{
	sslz4filter *z = (sslz4filter*)f->priv;
	int rc;
	switch (f->op) {
	case SS_FINPUT:;
    	LZ4F_cctx_internal_t* cctxPtr = z->ctx;
		size_t block = (cctxPtr->tmpInSize + 16);
		rc = ss_bufensure(dest, f->a, block);
		if (ssunlikely(rc == -1))
			return -1;
		size_t sz = LZ4F_compressEnd(z->ctx, dest->p, block, NULL);
		if (ssunlikely(LZ4F_isError(sz)))
			return -1;
		ss_bufadvance(dest, sz);
		break;	
	case SS_FOUTPUT:
		/* do nothing */
		break;
	}
	return 0;
}

ssfilterif ss_lz4filter =
{
	.name     = "lz4",
	.init     = ss_lz4filter_init,
	.free     = ss_lz4filter_free,
	.reset    = ss_lz4filter_reset,
	.start    = ss_lz4filter_start,
	.next     = ss_lz4filter_next,
	.complete = ss_lz4filter_complete
};
#line 1 "sophia/std/ss_mmap.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



int ss_mmap(ssmmap *m, int fd, uint64_t size, int ro)
{
	int flags = PROT_READ;
	if (! ro)
		flags |= PROT_WRITE;
	m->p = mmap(NULL, size, flags, MAP_SHARED, fd, 0);
	if (m->p == MAP_FAILED) {
		m->p = NULL;
		return -1;
	}
	m->size = size;
	return 0;
}

int ss_munmap(ssmmap *m)
{
	if (ssunlikely(m->p == NULL))
		return 0;
	int rc = munmap(m->p, m->size);
	m->p = NULL;
	return rc;
}
#line 1 "sophia/std/ss_nonefilter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



static int
ss_nonefilter_init(ssfilter *f ssunused, va_list args ssunused)
{
	return 0;
}

static int
ss_nonefilter_free(ssfilter *f ssunused)
{
	return 0;
}

static int
ss_nonefilter_reset(ssfilter *f ssunused)
{
	return 0;
}

static int
ss_nonefilter_start(ssfilter *f ssunused, ssbuf *dest ssunused)
{
	return 0;
}

static int
ss_nonefilter_next(ssfilter *f ssunused,
                   ssbuf *dest ssunused,
                   char *buf ssunused, int size ssunused)
{
	return 0;
}

static int
ss_nonefilter_complete(ssfilter *f ssunused, ssbuf *dest ssunused)
{
	return 0;
}

ssfilterif ss_nonefilter =
{
	.name     = "none",
	.init     = ss_nonefilter_init,
	.free     = ss_nonefilter_free,
	.reset    = ss_nonefilter_reset,
	.start    = ss_nonefilter_start,
	.next     = ss_nonefilter_next,
	.complete = ss_nonefilter_complete
};
#line 1 "sophia/std/ss_pager.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



void ss_pagerinit(sspager *p, uint32_t pool_count, uint32_t page_size)
{
	p->page_size  = sizeof(sspage) + page_size;
	p->pool_count = pool_count;
	p->pool_size  = sizeof(sspagepool) + pool_count * p->page_size;
	p->pools      = 0;
	p->pp         = NULL;
	p->p          = NULL;
}

void ss_pagerfree(sspager *p)
{
	sspagepool *pp_next, *pp = p->pp;
	while (pp) {
		pp_next = pp->next;
		munmap(pp, p->pool_size);
		pp = pp_next;
	}
}

static inline void
ss_pagerprefetch(sspager *p, sspagepool *pp)
{
	register sspage *start =
		(sspage*)((char*)pp + sizeof(sspagepool));
	register sspage *prev = start;
	register uint32_t i = 1;
	start->pool = pp;
	while (i < p->pool_count) {
		sspage *page =
			(sspage*)((char*)start + i * p->page_size);
		page->pool = pp;
		prev->next = page;
		prev = page;
		i++;
	}
	prev->next = NULL;
	p->p = start;
}

int ss_pageradd(sspager *p)
{
	sspagepool *pp =
		mmap(NULL, p->pool_size, PROT_READ|PROT_WRITE|PROT_EXEC,
	         MAP_PRIVATE|MAP_ANON, -1, 0);
	if (ssunlikely(p == MAP_FAILED))
		return -1;
	pp->used = 0;
	pp->next = p->pp;
	p->pp = pp;
	p->pools++;
	ss_pagerprefetch(p, pp);
	return 0;
}

void *ss_pagerpop(sspager *p)
{
	if (p->p)
		goto fetch;
	if (ssunlikely(ss_pageradd(p) == -1))
		return NULL;
fetch:;
	sspage *page = p->p;
	p->p = page->next;
	page->pool->used++;
	return page;
}

void ss_pagerpush(sspager *p, sspage *page)
{
	page->pool->used--;
	page->next = p->p;
	p->p = page;
}
#line 1 "sophia/std/ss_quota.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



int ss_quotainit(ssquota *q)
{
	q->enable = 0;
	q->wait   = 0;
	q->limit  = 0;
	q->used   = 0;
	ss_mutexinit(&q->lock);
	ss_condinit(&q->cond);
	return 0;
}

int ss_quotaset(ssquota *q, uint64_t limit)
{
	q->limit = limit;
	return 0;
}

int ss_quotaenable(ssquota *q, int v)
{
	q->enable = v;
	return 0;
}

int ss_quotafree(ssquota *q)
{
	ss_mutexfree(&q->lock);
	ss_condfree(&q->cond);
	return 0;
}

int ss_quota(ssquota *q, ssquotaop op, uint64_t v)
{
	ss_mutexlock(&q->lock);
	switch (op) {
	case SS_QADD:
		if (ssunlikely(!q->enable || q->limit == 0)) {
			/* .. */
		} else {
			if (ssunlikely((q->used + v) >= q->limit)) {
				q->wait++;
				ss_condwait(&q->cond, &q->lock);
			}
		}
	case SS_QGROW:
		q->used += v;
		break;
	case SS_QREMOVE:
		q->used -= v;
		if (ssunlikely(q->wait)) {
			q->wait--;
			ss_condsignal(&q->cond);
		}
		break;
	}
	ss_mutexunlock(&q->lock);
	return 0;
}
#line 1 "sophia/std/ss_rb.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



#define SS_RBBLACK 0
#define SS_RBRED   1
#define SS_RBUNDEF 2

ssrbnode *ss_rbmin(ssrb *t)
{
	ssrbnode *n = t->root;
	if (ssunlikely(n == NULL))
		return NULL;
	while (n->l)
		n = n->l;
	return n;
}

ssrbnode *ss_rbmax(ssrb *t)
{
	ssrbnode *n = t->root;
	if (ssunlikely(n == NULL))
		return NULL;
	while (n->r)
		n = n->r;
	return n;
}

ssrbnode *ss_rbnext(ssrb *t, ssrbnode *n)
{
	if (ssunlikely(n == NULL))
		return ss_rbmin(t);
	if (n->r) {
		n = n->r;
		while (n->l)
			n = n->l;
		return n;
	}
	ssrbnode *p;
	while ((p = n->p) && p->r == n)
		n = p;
	return p;
}

ssrbnode *ss_rbprev(ssrb *t, ssrbnode *n)
{
	if (ssunlikely(n == NULL))
		return ss_rbmax(t);
	if (n->l) {
		n = n->l;
		while (n->r)
			n = n->r;
		return n;
	}
	ssrbnode *p;
	while ((p = n->p) && p->l == n)
		n = p;
	return p;
}

static inline void
ss_rbrotate_left(ssrb *t, ssrbnode *n)
{
	ssrbnode *p = n;
	ssrbnode *q = n->r;
	ssrbnode *parent = n->p;
	if (sslikely(p->p != NULL)) {
		if (parent->l == p)
			parent->l = q;
		else
			parent->r = q;
	} else {
		t->root = q;
	}
	q->p = parent;
	p->p = q;
	p->r = q->l;
	if (p->r)
		p->r->p = p;
	q->l = p;
}

static inline void
ss_rbrotate_right(ssrb *t, ssrbnode *n)
{
	ssrbnode *p = n;
	ssrbnode *q = n->l;
	ssrbnode *parent = n->p;
	if (sslikely(p->p != NULL)) {
		if (parent->l == p)
			parent->l = q;
		else
			parent->r = q;
	} else {
		t->root = q;
	}
	q->p = parent;
	p->p = q;
	p->l = q->r;
	if (p->l)
		p->l->p = p;
	q->r = p;
}

static inline void
ss_rbset_fixup(ssrb *t, ssrbnode *n)
{
	ssrbnode *p;
	while ((p = n->p) && (p->color == SS_RBRED))
	{
		ssrbnode *g = p->p;
		if (p == g->l) {
			ssrbnode *u = g->r;
			if (u && u->color == SS_RBRED) {
				g->color = SS_RBRED;
				p->color = SS_RBBLACK;
				u->color = SS_RBBLACK;
				n = g;
			} else {
				if (n == p->r) {
					ss_rbrotate_left(t, p);
					n = p;
					p = n->p;
				}
				g->color = SS_RBRED;
				p->color = SS_RBBLACK;
				ss_rbrotate_right(t, g);
			}
		} else {
			ssrbnode *u = g->l;
			if (u && u->color == SS_RBRED) {
				g->color = SS_RBRED;
				p->color = SS_RBBLACK;
				u->color = SS_RBBLACK;
				n = g;
			} else {
				if (n == p->l) {
					ss_rbrotate_right(t, p);
					n = p;
					p = n->p;
				}
				g->color = SS_RBRED;
				p->color = SS_RBBLACK;
				ss_rbrotate_left(t, g);
			}
		}
	}
	t->root->color = SS_RBBLACK;
}

void ss_rbset(ssrb *t, ssrbnode *p, int prel, ssrbnode *n)
{
	n->color = SS_RBRED;
	n->p     = p;
	n->l     = NULL;
	n->r     = NULL;
	if (sslikely(p)) {
		assert(prel != 0);
		if (prel > 0)
			p->l = n;
		else
			p->r = n;
	} else {
		t->root = n;
	}
	ss_rbset_fixup(t, n);
}

void ss_rbreplace(ssrb *t, ssrbnode *o, ssrbnode *n)
{
	ssrbnode *p = o->p;
	if (p) {
		if (p->l == o) {
			p->l = n;
		} else {
			p->r = n;
		}
	} else {
		t->root = n;
	}
	if (o->l)
		o->l->p = n;
	if (o->r)
		o->r->p = n;
	*n = *o;
}

void ss_rbremove(ssrb *t, ssrbnode *n)
{
	if (ssunlikely(n->color == SS_RBUNDEF))
		return;
	ssrbnode *l = n->l;
	ssrbnode *r = n->r;
	ssrbnode *x = NULL;
	if (l == NULL) {
		x = r;
	} else
	if (r == NULL) {
		x = l;
	} else {
		x = r;
		while (x->l)
			x = x->l;
	}
	ssrbnode *p = n->p;
	if (p) {
		if (p->l == n) {
			p->l = x;
		} else {
			p->r = x;
		}
	} else {
		t->root = x;
	}
	uint8_t color;
	if (l && r) {
		color    = x->color;
		x->color = n->color;
		x->l     = l;
		l->p     = x;
		if (x != r) {
			p    = x->p;
			x->p = n->p;
			n    = x->r;
			p->l = n;
			x->r = r;
			r->p = x;
		} else {
			x->p = p;
			p    = x;
			n    = x->r;
		}
	} else {
		color = n->color;
		n     = x;
	}
	if (n)
		n->p = p;

	if (color == SS_RBRED)
		return;
	if (n && n->color == SS_RBRED) {
		n->color = SS_RBBLACK;
		return;
	}

	ssrbnode *s;
	do {
		if (ssunlikely(n == t->root))
			break;

		if (n == p->l) {
			s = p->r;
			if (s->color == SS_RBRED)
			{
				s->color = SS_RBBLACK;
				p->color = SS_RBRED;
				ss_rbrotate_left(t, p);
				s = p->r;
			}
			if ((!s->l || (s->l->color == SS_RBBLACK)) &&
			    (!s->r || (s->r->color == SS_RBBLACK)))
			{
				s->color = SS_RBRED;
				n = p;
				p = p->p;
				continue;
			}
			if ((!s->r || (s->r->color == SS_RBBLACK)))
			{
				s->l->color = SS_RBBLACK;
				s->color    = SS_RBRED;
				ss_rbrotate_right(t, s);
				s = p->r;
			}
			s->color    = p->color;
			p->color    = SS_RBBLACK;
			s->r->color = SS_RBBLACK;
			ss_rbrotate_left(t, p);
			n = t->root;
			break;
		} else {
			s = p->l;
			if (s->color == SS_RBRED)
			{
				s->color = SS_RBBLACK;
				p->color = SS_RBRED;
				ss_rbrotate_right(t, p);
				s = p->l;
			}
			if ((!s->l || (s->l->color == SS_RBBLACK)) &&
				(!s->r || (s->r->color == SS_RBBLACK)))
			{
				s->color = SS_RBRED;
				n = p;
				p = p->p;
				continue;
			}
			if ((!s->l || (s->l->color == SS_RBBLACK)))
			{
				s->r->color = SS_RBBLACK;
				s->color    = SS_RBRED;
				ss_rbrotate_left(t, s);
				s = p->l;
			}
			s->color    = p->color;
			p->color    = SS_RBBLACK;
			s->l->color = SS_RBBLACK;
			ss_rbrotate_right(t, p);
			n = t->root;
			break;
		}
	} while (n->color == SS_RBBLACK);
	if (n)
		n->color = SS_RBBLACK;
}
#line 1 "sophia/std/ss_slaba.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



typedef struct sssachunk sssachunk;
typedef struct sssa sssa;

struct sssachunk {
	sssachunk *next;
};

struct sssa {
	uint32_t chunk_max;
	uint32_t chunk_count;
	uint32_t chunk_used;
	uint32_t chunk_size;
	sssachunk *chunk;
	sspage *pu;
	sspager *pager;
} sspacked;

static inline int
ss_sagrow(sssa *s)
{
	register sspage *page = ss_pagerpop(s->pager);
	if (ssunlikely(page == NULL))
		return -1;
	page->next = s->pu;
	s->pu = page;
	s->chunk_used = 0;
	return 0;
}

static inline int
ss_slabaclose(ssa *a)
{
	sssa *s = (sssa*)a->priv;
	sspage *p_next, *p;
	p = s->pu;
	while (p) {
		p_next = p->next;
		ss_pagerpush(s->pager, p);
		p = p_next;
	}
	return 0;
}

static inline int
ss_slabaopen(ssa *a, va_list args) {
	assert(sizeof(sssa) <= sizeof(a->priv));
	sssa *s = (sssa*)a->priv;
	memset(s, 0, sizeof(*s));
	s->pager       = va_arg(args, sspager*);
	s->chunk_size  = va_arg(args, uint32_t);
	s->chunk_count = 0;
	s->chunk_max   =
		(s->pager->page_size - sizeof(sspage)) /
	     (sizeof(sssachunk) + s->chunk_size);
	s->chunk_used  = 0;
	s->chunk       = NULL;
	s->pu          = NULL;
	int rc = ss_sagrow(s);
	if (ssunlikely(rc == -1)) {
		ss_slabaclose(a);
		return -1;
	}
	return 0;
}

sshot static inline void*
ss_slabamalloc(ssa *a, int size ssunused)
{
	sssa *s = (sssa*)a->priv;
	if (sslikely(s->chunk)) {
		register sssachunk *c = s->chunk;
		s->chunk = c->next;
		s->chunk_count++;
		c->next = NULL;
		return (char*)c + sizeof(sssachunk);
	}
	if (ssunlikely(s->chunk_used == s->chunk_max)) {
		if (ssunlikely(ss_sagrow(s) == -1))
			return NULL;
	}
	register int off = sizeof(sspage) +
		s->chunk_used * (sizeof(sssachunk) + s->chunk_size);
	register sssachunk *n =
		(sssachunk*)((char*)s->pu + off);
	s->chunk_used++;
	s->chunk_count++;
	n->next = NULL;
	return (char*)n + sizeof(sssachunk);
}

sshot static inline void
ss_slabafree(ssa *a, void *ptr)
{
	sssa *s = (sssa*)a->priv;
	register sssachunk *c =
		(sssachunk*)((char*)ptr - sizeof(sssachunk));
	c->next = s->chunk;
	s->chunk = c;
	s->chunk_count--;
}

ssaif ss_slaba =
{
	.open    = ss_slabaopen,
	.close   = ss_slabaclose,
	.malloc  = ss_slabamalloc,
	.realloc = NULL,
	.free    = ss_slabafree 
};
#line 1 "sophia/std/ss_stda.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



static inline int
ss_stdaopen(ssa *a ssunused, va_list args ssunused) {
	return 0;
}

static inline int
ss_stdaclose(ssa *a ssunused) {
	return 0;
}

static inline void*
ss_stdamalloc(ssa *a ssunused, int size) {
	return malloc(size);
}

static inline void*
ss_stdarealloc(ssa *a ssunused, void *ptr, int size) {
	return realloc(ptr,  size);
}

static inline void
ss_stdafree(ssa *a ssunused, void *ptr) {
	assert(ptr != NULL);
	free(ptr);
}

ssaif ss_stda =
{
	.open    = ss_stdaopen,
	.close   = ss_stdaclose,
	.malloc  = ss_stdamalloc,
	.realloc = ss_stdarealloc,
	.free    = ss_stdafree 
};
#line 1 "sophia/std/ss_thread.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



int ss_threadnew(ssthread *t, ssthreadf f, void *arg)
{
	t->arg = arg;
	return pthread_create(&t->id, NULL, f, t);
}

int ss_threadjoin(ssthread *t)
{
	return pthread_join(t->id, NULL);
}
#line 1 "sophia/std/ss_time.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



void ss_sleep(uint64_t ns)
{
	struct timespec ts;
	ts.tv_sec  = 0;
	ts.tv_nsec = ns;
	nanosleep(&ts, NULL);
}

uint64_t ss_utime(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000000ULL + t.tv_usec;
}
#line 1 "sophia/std/ss_trigger.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/


#line 1 "sophia/std/ss_zstdfilter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



/* zstd git commit: 765207c54934d478488c236749b01c7d6fc63d70 */

/*
    zstd - standard compression library
    Copyright (C) 2014-2015, Yann Collet.

    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - zstd source repository : https://github.com/Cyan4973/zstd
    - ztsd public forum : https://groups.google.com/forum/#!forum/lz4c
*/

/*
    FSE : Finite State Entropy coder
    Copyright (C) 2013-2015, Yann Collet.

    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
    - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
    - Public forum : https://groups.google.com/forum/#!forum/lz4c
*/

/* >>>>> zstd.h */

/**************************************
*  Version
**************************************/
#define ZSTD_VERSION_MAJOR    0    /* for breaking interface changes  */
#define ZSTD_VERSION_MINOR    0    /* for new (non-breaking) interface capabilities */
#define ZSTD_VERSION_RELEASE  2    /* for tweaks, bug-fixes, or development */
#define ZSTD_VERSION_NUMBER (ZSTD_VERSION_MAJOR *100*100 + ZSTD_VERSION_MINOR *100 + ZSTD_VERSION_RELEASE)
unsigned ZSTD_versionNumber (void);


/**************************************
*  Simple one-step functions
**************************************/
size_t ZSTD_compress(   void* dst, size_t maxDstSize,
                  const void* src, size_t srcSize);

size_t ZSTD_decompress( void* dst, size_t maxOriginalSize,
                  const void* src, size_t compressedSize);

/*
ZSTD_compress() :
    Compresses 'srcSize' bytes from buffer 'src' into buffer 'dst', of maximum size 'dstSize'.
    Destination buffer should be sized to handle worst cases situations (input data not compressible).
    Worst case size evaluation is provided by function ZSTD_compressBound().
    return : the number of bytes written into buffer 'dst'
             or an error code if it fails (which can be tested using ZSTD_isError())

ZSTD_decompress() :
    compressedSize : is obviously the source size
    maxOriginalSize : is the size of the 'dst' buffer, which must be already allocated.
                      It must be equal or larger than originalSize, otherwise decompression will fail.
    return : the number of bytes decompressed into destination buffer (originalSize)
             or an errorCode if it fails (which can be tested using ZSTD_isError())
*/


/**************************************
*  Tool functions
**************************************/
size_t      ZSTD_compressBound(size_t srcSize);   /* maximum compressed size */

/* Error Management */
unsigned    ZSTD_isError(size_t code);         /* tells if a return value is an error code */
const char* ZSTD_getErrorName(size_t code);    /* provides error code string (useful for debugging) */

/* <<<<< zstd.h EOF */

/* >>>>> zstd_static.h */

/**************************************
*  Streaming functions
**************************************/
typedef void* ZSTD_cctx_t;
ZSTD_cctx_t ZSTD_createCCtx(void);
size_t      ZSTD_freeCCtx(ZSTD_cctx_t cctx);

size_t ZSTD_compressBegin(ZSTD_cctx_t cctx, void* dst, size_t maxDstSize);
size_t ZSTD_compressContinue(ZSTD_cctx_t cctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize);
size_t ZSTD_compressEnd(ZSTD_cctx_t cctx, void* dst, size_t maxDstSize);

typedef void* ZSTD_dctx_t;
ZSTD_dctx_t ZSTD_createDCtx(void);
size_t      ZSTD_freeDCtx(ZSTD_dctx_t dctx);

size_t ZSTD_nextSrcSizeToDecompress(ZSTD_dctx_t dctx);
size_t ZSTD_decompressContinue(ZSTD_dctx_t dctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize);
/*
  Use above functions alternatively.
  ZSTD_nextSrcSizeToDecompress() tells how much bytes to provide as input to ZSTD_decompressContinue().
  This value is expected to be provided, precisely, as 'srcSize'.
  Otherwise, compression will fail (result is an error code, which can be tested using ZSTD_isError() )
  ZSTD_decompressContinue() result is the number of bytes regenerated within 'dst'.
  It can be zero, which is not an error; it just means ZSTD_decompressContinue() has decoded some header.
*/

/**************************************
*  Error management
**************************************/
#define ZSTD_LIST_ERRORS(ITEM) \
        ITEM(ZSTD_OK_NoError) ITEM(ZSTD_ERROR_GENERIC) \
        ITEM(ZSTD_ERROR_wrongMagicNumber) \
        ITEM(ZSTD_ERROR_wrongSrcSize) ITEM(ZSTD_ERROR_maxDstSize_tooSmall) \
        ITEM(ZSTD_ERROR_wrongLBlockSize) \
        ITEM(ZSTD_ERROR_maxCode)

#define ZSTD_GENERATE_ENUM(ENUM) ENUM,
typedef enum { ZSTD_LIST_ERRORS(ZSTD_GENERATE_ENUM) } ZSTD_errorCodes;   /* exposed list of errors; static linking only */

/* <<<<< zstd_static.h EOF */

/* >>>>> fse.h */

/******************************************
*  FSE simple functions
******************************************/
size_t FSE_compress(void* dst, size_t maxDstSize,
              const void* src, size_t srcSize);
size_t FSE_decompress(void* dst, size_t maxDstSize,
                const void* cSrc, size_t cSrcSize);
/*
FSE_compress():
    Compress content of buffer 'src', of size 'srcSize', into destination buffer 'dst'.
    'dst' buffer must be already allocated, and sized to handle worst case situations.
    Worst case size evaluation is provided by FSE_compressBound().
    return : size of compressed data
    Special values : if result == 0, data is uncompressible => Nothing is stored within cSrc !!
                     if result == 1, data is one constant element x srcSize times. Use RLE compression.
                     if FSE_isError(result), it's an error code.

FSE_decompress():
    Decompress FSE data from buffer 'cSrc', of size 'cSrcSize',
    into already allocated destination buffer 'dst', of size 'maxDstSize'.
    ** Important ** : This function doesn't decompress uncompressed nor RLE data !
    return : size of regenerated data (<= maxDstSize)
             or an error code, which can be tested using FSE_isError()
*/


size_t FSE_decompressRLE(void* dst, size_t originalSize,
                   const void* cSrc, size_t cSrcSize);
/*
FSE_decompressRLE():
    Decompress specific RLE corner case (equivalent to memset()).
    cSrcSize must be == 1. originalSize must be exact.
    return : size of regenerated data (==originalSize)
             or an error code, which can be tested using FSE_isError()

Note : there is no function provided for uncompressed data, as it's just a simple memcpy()
*/


/******************************************
*  Tool functions
******************************************/
size_t FSE_compressBound(size_t size);       /* maximum compressed size */

/* Error Management */
unsigned    FSE_isError(size_t code);        /* tells if a return value is an error code */
const char* FSE_getErrorName(size_t code);   /* provides error code string (useful for debugging) */


/******************************************
*  FSE advanced functions
******************************************/
/*
FSE_compress2():
    Same as FSE_compress(), but allows the selection of 'maxSymbolValue' and 'tableLog'
    Both parameters can be defined as '0' to mean : use default value
    return : size of compressed data
             or -1 if there is an error
*/
size_t FSE_compress2 (void* dst, size_t dstSize, const void* src, size_t srcSize, unsigned maxSymbolValue, unsigned tableLog);


/******************************************
   FSE detailed API
******************************************/
/*
int FSE_compress(char* dest, const char* source, int inputSize) does the following:
1. count symbol occurrence from source[] into table count[]
2. normalize counters so that sum(count[]) == Power_of_2 (2^tableLog)
3. save normalized counters to memory buffer using writeHeader()
4. build encoding table 'CTable' from normalized counters
5. encode the data stream using encoding table

int FSE_decompress(char* dest, int originalSize, const char* compressed) performs:
1. read normalized counters with readHeader()
2. build decoding table 'DTable' from normalized counters
3. decode the data stream using decoding table

The following API allows triggering specific sub-functions.
*/

/* *** COMPRESSION *** */

size_t FSE_count(unsigned* count, const unsigned char* src, size_t srcSize, unsigned* maxSymbolValuePtr);

unsigned FSE_optimalTableLog(unsigned tableLog, size_t srcSize, unsigned maxSymbolValue);
size_t FSE_normalizeCount(short* normalizedCounter, unsigned tableLog, const unsigned* count, size_t total, unsigned maxSymbolValue);

size_t FSE_headerBound(unsigned maxSymbolValue, unsigned tableLog);
size_t FSE_writeHeader (void* headerBuffer, size_t headerBufferSize, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);

void*  FSE_createCTable (unsigned tableLog, unsigned maxSymbolValue);
void   FSE_freeCTable (void* CTable);
size_t FSE_buildCTable(void* CTable, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);

size_t FSE_compress_usingCTable (void* dst, size_t dstSize, const void* src, size_t srcSize, const void* CTable);

/*
The first step is to count all symbols. FSE_count() provides one quick way to do this job.
Result will be saved into 'count', a table of unsigned int, which must be already allocated, and have '*maxSymbolValuePtr+1' cells.
'source' is a table of char of size 'sourceSize'. All values within 'src' MUST be <= *maxSymbolValuePtr
*maxSymbolValuePtr will be updated, with its real value (necessarily <= original value)
FSE_count() will return the number of occurrence of the most frequent symbol.
If there is an error, the function will return an ErrorCode (which can be tested using FSE_isError()).

The next step is to normalize the frequencies.
FSE_normalizeCount() will ensure that sum of frequencies is == 2 ^'tableLog'.
It also guarantees a minimum of 1 to any Symbol which frequency is >= 1.
You can use input 'tableLog'==0 to mean "use default tableLog value".
If you are unsure of which tableLog value to use, you can optionally call FSE_optimalTableLog(),
which will provide the optimal valid tableLog given sourceSize, maxSymbolValue, and a user-defined maximum (0 means "default").

The result of FSE_normalizeCount() will be saved into a table,
called 'normalizedCounter', which is a table of signed short.
'normalizedCounter' must be already allocated, and have at least 'maxSymbolValue+1' cells.
The return value is tableLog if everything proceeded as expected.
It is 0 if there is a single symbol within distribution.
If there is an error(typically, invalid tableLog value), the function will return an ErrorCode (which can be tested using FSE_isError()).

'normalizedCounter' can be saved in a compact manner to a memory area using FSE_writeHeader().
'header' buffer must be already allocated.
For guaranteed success, buffer size must be at least FSE_headerBound().
The result of the function is the number of bytes written into 'header'.
If there is an error, the function will return an ErrorCode (which can be tested using FSE_isError()) (for example, buffer size too small).

'normalizedCounter' can then be used to create the compression tables 'CTable'.
The space required by 'CTable' must be already allocated. Its size is provided by FSE_sizeof_CTable().
'CTable' must be aligned of 4 bytes boundaries.
You can then use FSE_buildCTable() to fill 'CTable'.
In both cases, if there is an error, the function will return an ErrorCode (which can be tested using FSE_isError()).

'CTable' can then be used to compress 'source', with FSE_compress_usingCTable().
Similar to FSE_count(), the convention is that 'source' is assumed to be a table of char of size 'sourceSize'
The function returns the size of compressed data (without header), or -1 if failed.
*/


/* *** DECOMPRESSION *** */

size_t FSE_readHeader (short* normalizedCounter, unsigned* maxSymbolValuePtr, unsigned* tableLogPtr, const void* headerBuffer, size_t hbSize);

void*  FSE_createDTable(unsigned tableLog);
void   FSE_freeDTable(void* DTable);
size_t FSE_buildDTable (void* DTable, const short* const normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);

size_t FSE_decompress_usingDTable(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize, const void* DTable, size_t fastMode);

/*
If the block is RLE compressed, or uncompressed, use the relevant specific functions.

The first step is to obtain the normalized frequencies of symbols.
This can be performed by reading a header with FSE_readHeader().
'normalizedCounter' must be already allocated, and have at least '*maxSymbolValuePtr+1' cells of short.
In practice, that means it's necessary to know 'maxSymbolValue' beforehand,
or size the table to handle worst case situations (typically 256).
FSE_readHeader will provide 'tableLog' and 'maxSymbolValue' stored into the header.
The result of FSE_readHeader() is the number of bytes read from 'header'.
The following values have special meaning :
return 2 : there is only a single symbol value. The value is provided into the second byte of header.
return 1 : data is uncompressed
If there is an error, the function will return an error code, which can be tested using FSE_isError().

The next step is to create the decompression tables 'DTable' from 'normalizedCounter'.
This is performed by the function FSE_buildDTable().
The space required by 'DTable' must be already allocated and properly aligned.
One can create a DTable using FSE_createDTable().
The function will return 1 if DTable is compatible with fastMode, 0 otherwise.
If there is an error, the function will return an error code, which can be tested using FSE_isError().

'DTable' can then be used to decompress 'compressed', with FSE_decompress_usingDTable().
Only trigger fastMode if it was authorized by result of FSE_buildDTable(), otherwise decompression will fail.
cSrcSize must be correct, otherwise decompression will fail.
FSE_decompress_usingDTable() result will tell how many bytes were regenerated.
If there is an error, the function will return an error code, which can be tested using FSE_isError().
*/


/******************************************
*  FSE streaming compression API
******************************************/
typedef struct
{
    size_t bitContainer;
    int    bitPos;
    char*  startPtr;
    char*  ptr;
} FSE_CStream_t;

typedef struct
{
    ptrdiff_t   value;
    const void* stateTable;
    const void* symbolTT;
    unsigned    stateLog;
} FSE_CState_t;

void   FSE_initCStream(FSE_CStream_t* bitC, void* dstBuffer);
void   FSE_initCState(FSE_CState_t* CStatePtr, const void* CTable);

void   FSE_encodeByte(FSE_CStream_t* bitC, FSE_CState_t* CStatePtr, unsigned char symbol);
void   FSE_addBits(FSE_CStream_t* bitC, size_t value, unsigned nbBits);
void   FSE_flushBits(FSE_CStream_t* bitC);

void   FSE_flushCState(FSE_CStream_t* bitC, const FSE_CState_t* CStatePtr);
size_t FSE_closeCStream(FSE_CStream_t* bitC);

/*
These functions are inner components of FSE_compress_usingCTable().
They allow creation of custom streams, mixing multiple tables and bit sources.

A key property to keep in mind is that encoding and decoding are done **in reverse direction**.
So the first symbol you will encode is the last you will decode, like a lifo stack.

You will need a few variables to track your CStream. They are :

void* CTable;           // Provided by FSE_buildCTable()
FSE_CStream_t bitC;     // bitStream tracking structure
FSE_CState_t state;     // State tracking structure


The first thing to do is to init the bitStream, and the state.
    FSE_initCStream(&bitC, dstBuffer);
    FSE_initState(&state, CTable);

You can then encode your input data, byte after byte.
FSE_encodeByte() outputs a maximum of 'tableLog' bits at a time.
Remember decoding will be done in reverse direction.
    FSE_encodeByte(&bitStream, &state, symbol);

At any time, you can add any bit sequence.
Note : maximum allowed nbBits is 25, for compatibility with 32-bits decoders
    FSE_addBits(&bitStream, bitField, nbBits);

The above methods don't commit data to memory, they just store it into local register, for speed.
Local register size is 64-bits on 64-bits systems, 32-bits on 32-bits systems (size_t).
Writing data to memory is a manual operation, performed by the flushBits function.
    FSE_flushBits(&bitStream);

Your last FSE encoding operation shall be to flush your last state value(s).
    FSE_flushState(&bitStream, &state);

You must then close the bitStream if you opened it with FSE_initCStream().
It's possible to embed some user-info into the header, as an optionalId [0-31].
The function returns the size in bytes of CStream.
If there is an error, it returns an errorCode (which can be tested using FSE_isError()).
    size_t size = FSE_closeCStream(&bitStream, optionalId);
*/


/******************************************
*  FSE streaming decompression API
******************************************/
//typedef unsigned int bitD_t;
typedef size_t bitD_t;

typedef struct
{
    bitD_t   bitContainer;
    unsigned bitsConsumed;
    const char* ptr;
    const char* start;
} FSE_DStream_t;

typedef struct
{
    bitD_t      state;
    const void* table;
} FSE_DState_t;


size_t FSE_initDStream(FSE_DStream_t* bitD, const void* srcBuffer, size_t srcSize);
void   FSE_initDState(FSE_DState_t* DStatePtr, FSE_DStream_t* bitD, const void* DTable);

unsigned char FSE_decodeSymbol(FSE_DState_t* DStatePtr, FSE_DStream_t* bitD);
bitD_t        FSE_readBits(FSE_DStream_t* bitD, unsigned nbBits);
unsigned int  FSE_reloadDStream(FSE_DStream_t* bitD);

unsigned FSE_endOfDStream(const FSE_DStream_t* bitD);
unsigned FSE_endOfDState(const FSE_DState_t* DStatePtr);

/*
Let's now decompose FSE_decompress_usingDTable() into its unitary elements.
You will decode FSE-encoded symbols from the bitStream,
and also any other bitFields you put in, **in reverse order**.

You will need a few variables to track your bitStream. They are :

FSE_DStream_t DStream;    // Stream context
FSE_DState_t DState;      // State context. Multiple ones are possible
const void* DTable;       // Decoding table, provided by FSE_buildDTable()
U32 tableLog;             // Provided by FSE_readHeader()

The first thing to do is to init the bitStream.
    errorCode = FSE_initDStream(&DStream, &optionalId, srcBuffer, srcSize);

You should then retrieve your initial state(s) (multiple ones are possible) :
    errorCode = FSE_initDState(&DState, &DStream, DTable, tableLog);

You can then decode your data, symbol after symbol.
For information the maximum number of bits read by FSE_decodeSymbol() is 'tableLog'.
Keep in mind that symbols are decoded in reverse order, like a lifo stack (last in, first out).
    unsigned char symbol = FSE_decodeSymbol(&DState, &DStream);

You can retrieve any bitfield you eventually stored into the bitStream (in reverse order)
Note : maximum allowed nbBits is 25
    unsigned int bitField = FSE_readBits(&DStream, nbBits);

All above operations only read from local register (which size is controlled by bitD_t==32 bits).
Reading data from memory is manually performed by the reload method.
    endSignal = FSE_reloadDStream(&DStream);

FSE_reloadDStream() result tells if there is still some more data to read from DStream.
0 : there is still some data left into the DStream.
1 Dstream reached end of buffer, but is not yet fully extracted. It will not load data from memory any more.
2 Dstream reached its exact end, corresponding in general to decompression completed.
3 Dstream went too far. Decompression result is corrupted.

When reaching end of buffer(1), progress slowly if you decode multiple symbols per loop,
to properly detect the exact end of stream.
After each decoded symbol, check if DStream is fully consumed using this simple test :
    FSE_reloadDStream(&DStream) >= 2

When it's done, verify decompression is fully completed, by checking both DStream and the relevant states.
Checking if DStream has reached its end is performed by :
    FSE_endOfDStream(&DStream);
Check also the states. There might be some entropy left there, still able to decode some high probability symbol.
    FSE_endOfDState(&DState);
*/

/* <<<<< fse.h EOF */


/* >>>>> fse_static.h */

/******************************************
*  Tool functions
******************************************/
#define FSE_MAX_HEADERSIZE 512
#define FSE_COMPRESSBOUND(size) (size + (size>>7) + FSE_MAX_HEADERSIZE)   /* Macro can be useful for static allocation */


/******************************************
*  Static allocation
******************************************/
/* You can statically allocate a CTable as a table of U32 using below macro */
#define FSE_CTABLE_SIZE_U32(maxTableLog, maxSymbolValue)   (1 + (1<<(maxTableLog-1)) + ((maxSymbolValue+1)*2))
#define FSE_DTABLE_SIZE_U32(maxTableLog)                   ((1<<maxTableLog)+1)

/******************************************
*  Error Management
******************************************/
#define FSE_LIST_ERRORS(ITEM) \
        ITEM(FSE_OK_NoError) ITEM(FSE_ERROR_GENERIC) \
        ITEM(FSE_ERROR_tableLog_tooLarge) ITEM(FSE_ERROR_maxSymbolValue_tooLarge) \
        ITEM(FSE_ERROR_dstSize_tooSmall) ITEM(FSE_ERROR_srcSize_wrong)\
        ITEM(FSE_ERROR_corruptionDetected) \
        ITEM(FSE_ERROR_maxCode)

#define FSE_GENERATE_ENUM(ENUM) ENUM,
typedef enum { FSE_LIST_ERRORS(FSE_GENERATE_ENUM) } FSE_errorCodes;  /* enum is exposed, to detect & handle specific errors; compare function result to -enum value */


/******************************************
*  FSE advanced API
******************************************/
size_t FSE_countFast(unsigned* count, const unsigned char* src, size_t srcSize, unsigned* maxSymbolValuePtr);
/* same as FSE_count(), but won't check if input really respect that all values within src are <= *maxSymbolValuePtr */

size_t FSE_buildCTable_raw (void* CTable, unsigned nbBits);
/* create a fake CTable, designed to not compress an input where each element uses nbBits */

size_t FSE_buildCTable_rle (void* CTable, unsigned char symbolValue);
/* create a fake CTable, designed to compress a single identical value */

size_t FSE_buildDTable_raw (void* DTable, unsigned nbBits);
/* create a fake DTable, designed to read an uncompressed bitstream where each element uses nbBits */

size_t FSE_buildDTable_rle (void* DTable, unsigned char symbolValue);
/* create a fake DTable, designed to always generate the same symbolValue */


/******************************************
*  FSE streaming API
******************************************/
bitD_t FSE_readBitsFast(FSE_DStream_t* bitD, unsigned nbBits);
/* faster, but works only if nbBits >= 1 (otherwise, result will be corrupted) */

unsigned char FSE_decodeSymbolFast(FSE_DState_t* DStatePtr, FSE_DStream_t* bitD);
/* faster, but works only if nbBits >= 1 (otherwise, result will be corrupted) */

/* <<<<< fse_static.h EOF */

/* >>>>> fse.c */

#ifndef FSE_COMMONDEFS_ONLY

/****************************************************************
*  Tuning parameters
****************************************************************/
/* MEMORY_USAGE :
*  Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
*  Increasing memory usage improves compression ratio
*  Reduced memory usage can improve speed, due to cache effect
*  Recommended max value is 14, for 16KB, which nicely fits into Intel x86 L1 cache */
#define FSE_MAX_MEMORY_USAGE 14
#define FSE_DEFAULT_MEMORY_USAGE 13

/* FSE_MAX_SYMBOL_VALUE :
*  Maximum symbol value authorized.
*  Required for proper stack allocation */
#define FSE_MAX_SYMBOL_VALUE 255


/****************************************************************
*  Generic function type & suffix (C template emulation)
****************************************************************/
#define FSE_FUNCTION_TYPE BYTE
#define FSE_FUNCTION_EXTENSION

#endif   /* !FSE_COMMONDEFS_ONLY */


/****************************************************************
*  Compiler specifics
****************************************************************/
#  define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#  ifdef __GNUC__
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif

#ifndef MEM_ACCESS_MODULE
#define MEM_ACCESS_MODULE
/****************************************************************
*  Basic Types
*****************************************************************/
#ifndef ZTYPES
#define ZTYPES 1

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
typedef  uint8_t BYTE;
typedef uint16_t U16;
typedef  int16_t S16;
typedef uint32_t U32;
typedef  int32_t S32;
typedef uint64_t U64;
typedef  int64_t S64;
#else
typedef unsigned char       BYTE;
typedef unsigned short      U16;
typedef   signed short      S16;
typedef unsigned int        U32;
typedef   signed int        S32;
typedef unsigned long long  U64;
typedef   signed long long  S64;
#endif

#endif

#endif   /* MEM_ACCESS_MODULE */

/****************************************************************
*  Memory I/O
*****************************************************************/
static unsigned FSE_isLittleEndian(void)
{
    const union { U32 i; BYTE c[4]; } one = { 1 };   /* don't use static : performance detrimental  */
    return one.c[0];
}

static U32 FSE_read32(const void* memPtr)
{
    U32 val32;
    memcpy(&val32, memPtr, 4);
    return val32;
}

static U32 FSE_readLE32(const void* memPtr)
{
    if (FSE_isLittleEndian())
        return FSE_read32(memPtr);
    else
    {
        const BYTE* p = (const BYTE*)memPtr;
        return (U32)((U32)p[0] + ((U32)p[1]<<8) + ((U32)p[2]<<16) + ((U32)p[3]<<24));
    }
}

static void FSE_writeLE32(void* memPtr, U32 val32)
{
    if (FSE_isLittleEndian())
    {
        memcpy(memPtr, &val32, 4);
    }
    else
    {
        BYTE* p = (BYTE*)memPtr;
        p[0] = (BYTE)val32;
        p[1] = (BYTE)(val32>>8);
        p[2] = (BYTE)(val32>>16);
        p[3] = (BYTE)(val32>>24);
    }
}

static U64 FSE_read64(const void* memPtr)
{
    U64 val64;
    memcpy(&val64, memPtr, 8);
    return val64;
}

static U64 FSE_readLE64(const void* memPtr)
{
    if (FSE_isLittleEndian())
        return FSE_read64(memPtr);
    else
    {
        const BYTE* p = (const BYTE*)memPtr;
        return (U64)((U64)p[0] + ((U64)p[1]<<8) + ((U64)p[2]<<16) + ((U64)p[3]<<24)
                     + ((U64)p[4]<<32) + ((U64)p[5]<<40) + ((U64)p[6]<<48) + ((U64)p[7]<<56));
    }
}

static void FSE_writeLE64(void* memPtr, U64 val64)
{
    if (FSE_isLittleEndian())
    {
        memcpy(memPtr, &val64, 8);
    }
    else
    {
        BYTE* p = (BYTE*)memPtr;
        p[0] = (BYTE)val64;
        p[1] = (BYTE)(val64>>8);
        p[2] = (BYTE)(val64>>16);
        p[3] = (BYTE)(val64>>24);
        p[4] = (BYTE)(val64>>32);
        p[5] = (BYTE)(val64>>40);
        p[6] = (BYTE)(val64>>48);
        p[7] = (BYTE)(val64>>56);
    }
}

static size_t FSE_readLEST(const void* memPtr)
{
    if (sizeof(size_t)==4)
        return (size_t)FSE_readLE32(memPtr);
    else
        return (size_t)FSE_readLE64(memPtr);
}

static void FSE_writeLEST(void* memPtr, size_t val)
{
    if (sizeof(size_t)==4)
        FSE_writeLE32(memPtr, (U32)val);
    else
        FSE_writeLE64(memPtr, (U64)val);
}


/****************************************************************
*  Constants
*****************************************************************/
#define FSE_MAX_TABLELOG  (FSE_MAX_MEMORY_USAGE-2)
#define FSE_MAX_TABLESIZE (1U<<FSE_MAX_TABLELOG)
#define FSE_MAXTABLESIZE_MASK (FSE_MAX_TABLESIZE-1)
#define FSE_DEFAULT_TABLELOG (FSE_DEFAULT_MEMORY_USAGE-2)
#define FSE_MIN_TABLELOG 5

#define FSE_TABLELOG_ABSOLUTE_MAX 15
#if FSE_MAX_TABLELOG > FSE_TABLELOG_ABSOLUTE_MAX
#error "FSE_MAX_TABLELOG > FSE_TABLELOG_ABSOLUTE_MAX is not supported"
#endif


/****************************************************************
*  Error Management
****************************************************************/
#define FSE_STATIC_ASSERT(c) { enum { FSE_static_assert = 1/(int)(!!(c)) }; }   /* use only *after* variable declarations */


/****************************************************************
*  Complex types
****************************************************************/
typedef struct
{
    int  deltaFindState;
    U16  maxState;
    BYTE minBitsOut;
    /* one byte padding */
} FSE_symbolCompressionTransform;

typedef struct
{
    U32 fakeTable[FSE_CTABLE_SIZE_U32(FSE_MAX_TABLELOG, FSE_MAX_SYMBOL_VALUE)];   /* compatible with FSE_compressU16() */
} CTable_max_t;


/****************************************************************
*  Internal functions
****************************************************************/
FORCE_INLINE unsigned FSE_highbit32 (register U32 val)
{
#   if defined(_MSC_VER)   /* Visual */
    unsigned long r;
    _BitScanReverse ( &r, val );
    return (unsigned) r;
#   elif defined(__GNUC__) && (GCC_VERSION >= 304)   /* GCC Intrinsic */
    return 31 - __builtin_clz (val);
#   else   /* Software version */
    static const unsigned DeBruijnClz[32] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 };
    U32 v = val;
    unsigned r;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    r = DeBruijnClz[ (U32) (v * 0x07C4ACDDU) >> 27];
    return r;
#   endif
}


#ifndef FSE_COMMONDEFS_ONLY

unsigned FSE_isError(size_t code) { return (code > (size_t)(-FSE_ERROR_maxCode)); }

#define FSE_GENERATE_STRING(STRING) #STRING,
static const char* FSE_errorStrings[] = { FSE_LIST_ERRORS(FSE_GENERATE_STRING) };

const char* FSE_getErrorName(size_t code)
{
    static const char* codeError = "Unspecified error code";
    if (FSE_isError(code)) return FSE_errorStrings[-(int)(code)];
    return codeError;
}

static short FSE_abs(short a)
{
    return a<0? -a : a;
}


/****************************************************************
*  Header bitstream management
****************************************************************/
size_t FSE_headerBound(unsigned maxSymbolValue, unsigned tableLog)
{
    size_t maxHeaderSize = (((maxSymbolValue+1) * tableLog) >> 3) + 1;
    return maxSymbolValue ? maxHeaderSize : FSE_MAX_HEADERSIZE;
}

static size_t FSE_writeHeader_generic (void* header, size_t headerBufferSize,
                                       const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog,
                                       unsigned safeWrite)
{
    BYTE* const ostart = (BYTE*) header;
    BYTE* out = ostart;
    BYTE* const oend = ostart + headerBufferSize;
    int nbBits;
    const int tableSize = 1 << tableLog;
    int remaining;
    int threshold;
    U32 bitStream;
    int bitCount;
    unsigned charnum = 0;
    int previous0 = 0;

    bitStream = 0;
    bitCount  = 0;
    /* Table Size */
    bitStream += (tableLog-FSE_MIN_TABLELOG) << bitCount;
    bitCount  += 4;

    /* Init */
    remaining = tableSize+1;   /* +1 for extra accuracy */
    threshold = tableSize;
    nbBits = tableLog+1;

    while (remaining>1)   /* stops at 1 */
    {
        if (previous0)
        {
            unsigned start = charnum;
            while (!normalizedCounter[charnum]) charnum++;
            while (charnum >= start+24)
            {
                start+=24;
                bitStream += 0xFFFF<<bitCount;
                if ((!safeWrite) && (out > oend-2)) return (size_t)-FSE_ERROR_GENERIC;   /* Buffer overflow */
                out[0] = (BYTE)bitStream;
                out[1] = (BYTE)(bitStream>>8);
                out+=2;
                bitStream>>=16;
            }
            while (charnum >= start+3)
            {
                start+=3;
                bitStream += 3 << bitCount;
                bitCount += 2;
            }
            bitStream += (charnum-start) << bitCount;
            bitCount += 2;
            if (bitCount>16)
            {
                if ((!safeWrite) && (out > oend - 2)) return (size_t)-FSE_ERROR_GENERIC;   /* Buffer overflow */
                out[0] = (BYTE)bitStream;
                out[1] = (BYTE)(bitStream>>8);
                out += 2;
                bitStream >>= 16;
                bitCount -= 16;
            }
        }
        {
            short count = normalizedCounter[charnum++];
            const short max = (short)((2*threshold-1)-remaining);
            remaining -= FSE_abs(count);
            if (remaining<0) return (size_t)-FSE_ERROR_GENERIC;
            count++;   /* +1 for extra accuracy */
            if (count>=threshold) count += max;   /* [0..max[ [max..threshold[ (...) [threshold+max 2*threshold[ */
            bitStream += count << bitCount;
            bitCount  += nbBits;
            bitCount  -= (count<max);
            previous0 = (count==1);
            while (remaining<threshold) nbBits--, threshold>>=1;
        }
        if (bitCount>16)
        {
            if ((!safeWrite) && (out > oend - 2)) return (size_t)-FSE_ERROR_GENERIC;   /* Buffer overflow */
            out[0] = (BYTE)bitStream;
            out[1] = (BYTE)(bitStream>>8);
            out += 2;
            bitStream >>= 16;
            bitCount -= 16;
        }
    }

    /* flush remaining bitStream */
    if ((!safeWrite) && (out > oend - 2)) return (size_t)-FSE_ERROR_GENERIC;   /* Buffer overflow */
    out[0] = (BYTE)bitStream;
    out[1] = (BYTE)(bitStream>>8);
    out+= (bitCount+7) /8;

    if (charnum > maxSymbolValue + 1) return (size_t)-FSE_ERROR_GENERIC;   /* Too many symbols written (a bit too late?) */

    return (out-ostart);
}


size_t FSE_writeHeader (void* header, size_t headerBufferSize, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog)
{
    if (tableLog > FSE_MAX_TABLELOG) return (size_t)-FSE_ERROR_GENERIC;   /* Unsupported */
    if (tableLog < FSE_MIN_TABLELOG) return (size_t)-FSE_ERROR_GENERIC;   /* Unsupported */

    if (headerBufferSize < FSE_headerBound(maxSymbolValue, tableLog))
        return FSE_writeHeader_generic(header, headerBufferSize, normalizedCounter, maxSymbolValue, tableLog, 0);

    return FSE_writeHeader_generic(header, headerBufferSize, normalizedCounter, maxSymbolValue, tableLog, 1);
}


size_t FSE_readHeader (short* normalizedCounter, unsigned* maxSVPtr, unsigned* tableLogPtr,
                 const void* headerBuffer, size_t hbSize)
{
    const BYTE* const istart = (const BYTE*) headerBuffer;
    const BYTE* ip = istart;
    int nbBits;
    int remaining;
    int threshold;
    U32 bitStream;
    int bitCount;
    unsigned charnum = 0;
    int previous0 = 0;

    bitStream = FSE_readLE32(ip);
    nbBits = (bitStream & 0xF) + FSE_MIN_TABLELOG;   /* extract tableLog */
    if (nbBits > FSE_TABLELOG_ABSOLUTE_MAX) return (size_t)-FSE_ERROR_tableLog_tooLarge;
    bitStream >>= 4;
    bitCount = 4;
    *tableLogPtr = nbBits;
    remaining = (1<<nbBits)+1;
    threshold = 1<<nbBits;
    nbBits++;

    while ((remaining>1) && (charnum<=*maxSVPtr))
    {
        if (previous0)
        {
            unsigned n0 = charnum;
            while ((bitStream & 0xFFFF) == 0xFFFF)
            {
                n0+=24;
                ip+=2;
                bitStream = FSE_readLE32(ip) >> bitCount;
            }
            while ((bitStream & 3) == 3)
            {
                n0+=3;
                bitStream>>=2;
                bitCount+=2;
            }
            n0 += bitStream & 3;
            bitCount += 2;
            if (n0 > *maxSVPtr) return (size_t)-FSE_ERROR_GENERIC;
            while (charnum < n0) normalizedCounter[charnum++] = 0;
            ip += bitCount>>3;
            bitCount &= 7;
            bitStream = FSE_readLE32(ip) >> bitCount;
        }
        {
            const short max = (short)((2*threshold-1)-remaining);
            short count;

            if ((bitStream & (threshold-1)) < (U32)max)
            {
                count = (short)(bitStream & (threshold-1));
                bitCount   += nbBits-1;
            }
            else
            {
                count = (short)(bitStream & (2*threshold-1));
                if (count >= threshold) count -= max;
                bitCount   += nbBits;
            }

            count--;   /* extra accuracy */
            remaining -= FSE_abs(count);
            normalizedCounter[charnum++] = count;
            previous0 = !count;
            while (remaining < threshold)
            {
                nbBits--;
                threshold >>= 1;
            }

            ip += bitCount>>3;
            bitCount &= 7;
            bitStream = FSE_readLE32(ip) >> bitCount;
        }
    }
    if (remaining != 1) return (size_t)-FSE_ERROR_GENERIC;
    *maxSVPtr = charnum-1;

    ip += bitCount>0;
    if ((size_t)(ip-istart) >= hbSize) return (size_t)-FSE_ERROR_srcSize_wrong;   /* arguably a bit late , tbd */
    return ip-istart;
}


/****************************************************************
*  FSE Compression Code
****************************************************************/
/*
CTable is a variable size structure which contains :
    U16 tableLog;
    U16 maxSymbolValue;
    U16 nextStateNumber[1 << tableLog];                         // This size is variable
    FSE_symbolCompressionTransform symbolTT[maxSymbolValue+1];  // This size is variable
Allocation is manual, since C standard does not support variable-size structures.
*/

size_t FSE_sizeof_CTable (unsigned maxSymbolValue, unsigned tableLog)
{
    size_t size;
    FSE_STATIC_ASSERT((size_t)FSE_CTABLE_SIZE_U32(FSE_MAX_TABLELOG, FSE_MAX_SYMBOL_VALUE)*4 >= sizeof(CTable_max_t));   /* A compilation error here means FSE_CTABLE_SIZE_U32 is not large enough */
    if (tableLog > FSE_MAX_TABLELOG) return (size_t)-FSE_ERROR_GENERIC;
    size = FSE_CTABLE_SIZE_U32 (tableLog, maxSymbolValue) * sizeof(U32);
    return size;
}

void* FSE_createCTable (unsigned maxSymbolValue, unsigned tableLog)
{
    size_t size;
    if (tableLog > FSE_TABLELOG_ABSOLUTE_MAX) tableLog = FSE_TABLELOG_ABSOLUTE_MAX;
    size = FSE_CTABLE_SIZE_U32 (tableLog, maxSymbolValue) * sizeof(U32);
    return malloc(size);
}

void  FSE_freeCTable (void* CTable)
{
    free(CTable);
}


unsigned FSE_optimalTableLog(unsigned maxTableLog, size_t srcSize, unsigned maxSymbolValue)
{
    U32 tableLog = maxTableLog;
    if (tableLog==0) tableLog = FSE_DEFAULT_TABLELOG;
    if ((FSE_highbit32((U32)(srcSize - 1)) - 2) < tableLog) tableLog = FSE_highbit32((U32)(srcSize - 1)) - 2;   /* Accuracy can be reduced */
    if ((FSE_highbit32(maxSymbolValue+1)+1) > tableLog) tableLog = FSE_highbit32(maxSymbolValue+1)+1;   /* Need a minimum to safely represent all symbol values */
    if (tableLog < FSE_MIN_TABLELOG) tableLog = FSE_MIN_TABLELOG;
    if (tableLog > FSE_MAX_TABLELOG) tableLog = FSE_MAX_TABLELOG;
    return tableLog;
}


typedef struct
{
    U32 id;
    U32 count;
} rank_t;

int FSE_compareRankT(const void* r1, const void* r2)
{
    const rank_t* R1 = (const rank_t*)r1;
    const rank_t* R2 = (const rank_t*)r2;

    return 2 * (R1->count < R2->count) - 1;
}


#if 0
static size_t FSE_adjustNormSlow(short* norm, int pointsToRemove, const unsigned* count, U32 maxSymbolValue)
{
    rank_t rank[FSE_MAX_SYMBOL_VALUE+2];
    U32 s;

    /* Init */
    for (s=0; s<=maxSymbolValue; s++)
    {
        rank[s].id = s;
        rank[s].count = count[s];
        if (norm[s] <= 1) rank[s].count = 0;
    }
    rank[maxSymbolValue+1].id = 0;
    rank[maxSymbolValue+1].count = 0;   /* ensures comparison ends here in worst case */

    /* Sort according to count */
    qsort(rank, maxSymbolValue+1, sizeof(rank_t), FSE_compareRankT);

    while(pointsToRemove)
    {
        int newRank = 1;
        rank_t savedR;
        if (norm[rank[0].id] == 1)
            return (size_t)-FSE_ERROR_GENERIC;
        norm[rank[0].id]--;
        pointsToRemove--;
        rank[0].count -= (rank[0].count + 6) >> 3;
        if (norm[rank[0].id] == 1)
            rank[0].count=0;
        savedR = rank[0];
        while (rank[newRank].count > savedR.count)
        {
            rank[newRank-1] = rank[newRank];
            newRank++;
        }
        rank[newRank-1] = savedR;
    }

    return 0;
}

#else

/* Secondary normalization method.
   To be used when primary method fails. */

static size_t FSE_normalizeM2(short* norm, U32 tableLog, const unsigned* count, size_t total, U32 maxSymbolValue)
{
    U32 s;
    U32 distributed = 0;
    U32 ToDistribute;

    /* Init */
    U32 lowThreshold = (U32)(total >> tableLog);
    U32 lowOne = (U32)((total * 3) >> (tableLog + 1));

    for (s=0; s<=maxSymbolValue; s++)
    {
        if (count[s] == 0)
        {
            norm[s]=0;
            continue;
        }
        if (count[s] <= lowThreshold)
        {
            norm[s] = -1;
            distributed++;
            total -= count[s];
            continue;
        }
        if (count[s] <= lowOne)
        {
            norm[s] = 1;
            distributed++;
            total -= count[s];
            continue;
        }
        norm[s]=-2;
    }
    ToDistribute = (1 << tableLog) - distributed;

    if ((total / ToDistribute) > lowOne)
    {
        /* risk of rounding to zero */
        lowOne = (U32)((total * 3) / (ToDistribute * 2));
        for (s=0; s<=maxSymbolValue; s++)
        {
            if ((norm[s] == -2) && (count[s] <= lowOne))
            {
                norm[s] = 1;
                distributed++;
                total -= count[s];
                continue;
            }
        }
        ToDistribute = (1 << tableLog) - distributed;
    }

    if (distributed == maxSymbolValue+1)
    {
        /* all values are pretty poor;
           probably incompressible data (should have already been detected);
           find max, then give all remaining points to max */
        U32 maxV = 0, maxC =0;
        for (s=0; s<=maxSymbolValue; s++)
            if (count[s] > maxC) maxV=s, maxC=count[s];
        norm[maxV] += ToDistribute;
        return 0;
    }

    {
        U64 const vStepLog = 62 - tableLog;
        U64 const mid = (1ULL << (vStepLog-1)) - 1;
        U64 const rStep = ((((U64)1<<vStepLog) * ToDistribute) + mid) / total;   /* scale on remaining */
        U64 tmpTotal = mid;
        for (s=0; s<=maxSymbolValue; s++)
        {
            if (norm[s]==-2)
            {
                U64 end = tmpTotal + (count[s] * rStep);
                U32 sStart = (U32)(tmpTotal >> vStepLog);
                U32 sEnd = (U32)(end >> vStepLog);
                U32 weight = sEnd - sStart;
                if (weight < 1)
                    return (size_t)-FSE_ERROR_GENERIC;
                norm[s] = weight;
                tmpTotal = end;
            }
        }
    }

    return 0;
}
#endif


size_t FSE_normalizeCount (short* normalizedCounter, unsigned tableLog,
                           const unsigned* count, size_t total,
                           unsigned maxSymbolValue)
{
    /* Sanity checks */
    if (tableLog==0) tableLog = FSE_DEFAULT_TABLELOG;
    if (tableLog < FSE_MIN_TABLELOG) return (size_t)-FSE_ERROR_GENERIC;   /* Unsupported size */
    if (tableLog > FSE_MAX_TABLELOG) return (size_t)-FSE_ERROR_GENERIC;   /* Unsupported size */
    if ((1U<<tableLog) <= maxSymbolValue) return (size_t)-FSE_ERROR_GENERIC;   /* Too small tableLog, compression potentially impossible */

    {
        U32 const rtbTable[] = {     0, 473195, 504333, 520860, 550000, 700000, 750000, 830000 };
        U64 const scale = 62 - tableLog;
        U64 const step = ((U64)1<<62) / total;   /* <== here, one division ! */
        U64 const vStep = 1ULL<<(scale-20);
        int stillToDistribute = 1<<tableLog;
        unsigned s;
        unsigned largest=0;
        short largestP=0;
        U32 lowThreshold = (U32)(total >> tableLog);

        for (s=0; s<=maxSymbolValue; s++)
        {
            if (count[s] == total) return 0;
            if (count[s] == 0)
            {
                normalizedCounter[s]=0;
                continue;
            }
            if (count[s] <= lowThreshold)
            {
                normalizedCounter[s] = -1;
                stillToDistribute--;
            }
            else
            {
                short proba = (short)((count[s]*step) >> scale);
                if (proba<8)
                {
                    U64 restToBeat = vStep * rtbTable[proba];
                    proba += (count[s]*step) - ((U64)proba<<scale) > restToBeat;
                }
                if (proba > largestP)
                {
                    largestP=proba;
                    largest=s;
                }
                normalizedCounter[s] = proba;
                stillToDistribute -= proba;
            }
        }
        if (-stillToDistribute >= (normalizedCounter[largest] >> 1))
        {
            /* corner case, need another normalization method */
            size_t errorCode = FSE_normalizeM2(normalizedCounter, tableLog, count, total, maxSymbolValue);
            if (FSE_isError(errorCode)) return errorCode;
        }
        else normalizedCounter[largest] += (short)stillToDistribute;
    }

#if 0
    {   /* Print Table (debug) */
        U32 s;
        U32 nTotal = 0;
        for (s=0; s<=maxSymbolValue; s++)
            printf("%3i: %4i \n", s, normalizedCounter[s]);
        for (s=0; s<=maxSymbolValue; s++)
            nTotal += abs(normalizedCounter[s]);
        if (nTotal != (1U<<tableLog))
            printf("Warning !!! Total == %u != %u !!!", nTotal, 1U<<tableLog);
        getchar();
    }
#endif

    return tableLog;
}


/* fake CTable, for raw (uncompressed) input */
size_t FSE_buildCTable_raw (void* CTable, unsigned nbBits)
{
    const unsigned tableSize = 1 << nbBits;
    const unsigned tableMask = tableSize - 1;
    const unsigned maxSymbolValue = tableMask;
    U16* tableU16 = ( (U16*) CTable) + 2;
    FSE_symbolCompressionTransform* symbolTT = (FSE_symbolCompressionTransform*) ((((U32*)CTable)+1) + (tableSize>>1));
    unsigned s;

    /* Sanity checks */
    if (nbBits < 1) return (size_t)-FSE_ERROR_GENERIC;             /* min size */
    if (((size_t)CTable) & 3) return (size_t)-FSE_ERROR_GENERIC;   /* Must be allocated of 4 bytes boundaries */

    /* header */
    tableU16[-2] = (U16) nbBits;
    tableU16[-1] = (U16) maxSymbolValue;

    /* Build table */
    for (s=0; s<tableSize; s++)
        tableU16[s] = (U16)(tableSize + s);

    /* Build Symbol Transformation Table */
    for (s=0; s<=maxSymbolValue; s++)
    {
        symbolTT[s].minBitsOut = (BYTE)nbBits;
        symbolTT[s].deltaFindState = s-1;
        symbolTT[s].maxState = (U16)( (tableSize*2) - 1);   /* ensures state <= maxState */
    }

    return 0;
}


/* fake CTable, for rle (100% always same symbol) input */
size_t FSE_buildCTable_rle (void* CTable, BYTE symbolValue)
{
    const unsigned tableSize = 1;
    U16* tableU16 = ( (U16*) CTable) + 2;
    FSE_symbolCompressionTransform* symbolTT = (FSE_symbolCompressionTransform*) ((U32*)CTable + 2);

    /* safety checks */
    if (((size_t)CTable) & 3) return (size_t)-FSE_ERROR_GENERIC;   /* Must be 4 bytes aligned */

    /* header */
    tableU16[-2] = (U16) 0;
    tableU16[-1] = (U16) symbolValue;

    /* Build table */
    tableU16[0] = 0;
    tableU16[1] = 0;   /* just in case */

    /* Build Symbol Transformation Table */
    {
        symbolTT[symbolValue].minBitsOut = 0;
        symbolTT[symbolValue].deltaFindState = 0;
        symbolTT[symbolValue].maxState = (U16)(2*tableSize-1);   /* ensures state <= maxState */
    }

    return 0;
}


void FSE_initCStream(FSE_CStream_t* bitC, void* start)
{
    bitC->bitContainer = 0;
    bitC->bitPos = 0;   /* reserved for unusedBits */
    bitC->startPtr = (char*)start;
    bitC->ptr = bitC->startPtr;
}

void FSE_initCState(FSE_CState_t* statePtr, const void* CTable)
{
    const U32 tableLog = ( (U16*) CTable) [0];
    statePtr->value = (ptrdiff_t)1<<tableLog;
    statePtr->stateTable = ((const U16*) CTable) + 2;
    statePtr->symbolTT = (const U32*)CTable + 1 + (tableLog ? (1<<(tableLog-1)) : 1);
    statePtr->stateLog = tableLog;
}

void FSE_addBits(FSE_CStream_t* bitC, size_t value, unsigned nbBits)
{
    static const unsigned mask[] = { 0, 1, 3, 7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF, 0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF,  0xFFFFFF, 0x1FFFFFF };   /* up to 25 bits */
    bitC->bitContainer |= (value & mask[nbBits]) << bitC->bitPos;
    bitC->bitPos += nbBits;
}

void FSE_encodeByte(FSE_CStream_t* bitC, FSE_CState_t* statePtr, BYTE symbol)
{
    const FSE_symbolCompressionTransform* const symbolTT = (const FSE_symbolCompressionTransform*) statePtr->symbolTT;
    const U16* const stateTable = (const U16*) statePtr->stateTable;
    int nbBitsOut  = symbolTT[symbol].minBitsOut;
    nbBitsOut -= (int)((symbolTT[symbol].maxState - statePtr->value) >> 31);
    FSE_addBits(bitC, statePtr->value, nbBitsOut);
    statePtr->value = stateTable[ (statePtr->value >> nbBitsOut) + symbolTT[symbol].deltaFindState];
}

void FSE_flushBits(FSE_CStream_t* bitC)
{
    size_t nbBytes = bitC->bitPos >> 3;
    FSE_writeLEST(bitC->ptr, bitC->bitContainer);
    bitC->bitPos &= 7;
    bitC->ptr += nbBytes;
    bitC->bitContainer >>= nbBytes*8;
}

void FSE_flushCState(FSE_CStream_t* bitC, const FSE_CState_t* statePtr)
{
    FSE_addBits(bitC, statePtr->value, statePtr->stateLog);
    FSE_flushBits(bitC);
}


size_t FSE_closeCStream(FSE_CStream_t* bitC)
{
    char* endPtr;

    FSE_addBits(bitC, 1, 1);
    FSE_flushBits(bitC);

    endPtr = bitC->ptr;
    endPtr += bitC->bitPos > 0;

    return (endPtr - bitC->startPtr);
}


size_t FSE_compress_usingCTable (void* dst, size_t dstSize,
                           const void* src, size_t srcSize,
                           const void* CTable)
{
    const BYTE* const istart = (const BYTE*) src;
    const BYTE* ip;
    const BYTE* const iend = istart + srcSize;

    FSE_CStream_t bitC;
    FSE_CState_t CState1, CState2;


    /* init */
    (void)dstSize;   /* objective : ensure it fits into dstBuffer (Todo) */
    FSE_initCStream(&bitC, dst);
    FSE_initCState(&CState1, CTable);
    CState2 = CState1;

    ip=iend;

    /* join to even */
    if (srcSize & 1)
    {
        FSE_encodeByte(&bitC, &CState1, *--ip);
        FSE_flushBits(&bitC);
    }

    /* join to mod 4 */
    if ((sizeof(size_t)*8 > FSE_MAX_TABLELOG*4+7 ) && (srcSize & 2))   /* test bit 2 */
    {
        FSE_encodeByte(&bitC, &CState2, *--ip);
        FSE_encodeByte(&bitC, &CState1, *--ip);
        FSE_flushBits(&bitC);
    }

    /* 2 or 4 encoding per loop */
    while (ip>istart)
    {
        FSE_encodeByte(&bitC, &CState2, *--ip);

        if (sizeof(size_t)*8 < FSE_MAX_TABLELOG*2+7 )   /* this test must be static */
            FSE_flushBits(&bitC);

        FSE_encodeByte(&bitC, &CState1, *--ip);

        if (sizeof(size_t)*8 > FSE_MAX_TABLELOG*4+7 )   /* this test must be static */
        {
            FSE_encodeByte(&bitC, &CState2, *--ip);
            FSE_encodeByte(&bitC, &CState1, *--ip);
        }

        FSE_flushBits(&bitC);
    }

    FSE_flushCState(&bitC, &CState2);
    FSE_flushCState(&bitC, &CState1);
    return FSE_closeCStream(&bitC);
}


size_t FSE_compressBound(size_t size) { return FSE_COMPRESSBOUND(size); }


size_t FSE_compress2 (void* dst, size_t dstSize, const void* src, size_t srcSize, unsigned maxSymbolValue, unsigned tableLog)
{
    const BYTE* const istart = (const BYTE*) src;
    const BYTE* ip = istart;

    BYTE* const ostart = (BYTE*) dst;
    BYTE* op = ostart;
    BYTE* const oend = ostart + dstSize;

    U32   count[FSE_MAX_SYMBOL_VALUE+1];
    S16   norm[FSE_MAX_SYMBOL_VALUE+1];
    CTable_max_t CTable;
    size_t errorCode;

    /* early out */
    if (dstSize < FSE_compressBound(srcSize)) return (size_t)-FSE_ERROR_dstSize_tooSmall;
    if (srcSize <= 1) return srcSize;  /* Uncompressed or RLE */
    if (!maxSymbolValue) maxSymbolValue = FSE_MAX_SYMBOL_VALUE;
    if (!tableLog) tableLog = FSE_DEFAULT_TABLELOG;

    /* Scan input and build symbol stats */
    errorCode = FSE_count (count, ip, srcSize, &maxSymbolValue);
    if (FSE_isError(errorCode)) return errorCode;
    if (errorCode == srcSize) return 1;
    if (errorCode < (srcSize >> 7)) return 0;   /* Heuristic : not compressible enough */

    tableLog = FSE_optimalTableLog(tableLog, srcSize, maxSymbolValue);
    errorCode = FSE_normalizeCount (norm, tableLog, count, srcSize, maxSymbolValue);
    if (FSE_isError(errorCode)) return errorCode;

    /* Write table description header */
    errorCode = FSE_writeHeader (op, FSE_MAX_HEADERSIZE, norm, maxSymbolValue, tableLog);
    if (FSE_isError(errorCode)) return errorCode;
    op += errorCode;

    /* Compress */
    errorCode = FSE_buildCTable (&CTable, norm, maxSymbolValue, tableLog);
    if (FSE_isError(errorCode)) return errorCode;
    op += FSE_compress_usingCTable(op, oend - op, ip, srcSize, &CTable);

    /* check compressibility */
    if ( (size_t)(op-ostart) >= srcSize-1 )
        return 0;

    return op-ostart;
}


size_t FSE_compress (void* dst, size_t dstSize, const void* src, size_t srcSize)
{
    return FSE_compress2(dst, dstSize, src, (U32)srcSize, FSE_MAX_SYMBOL_VALUE, FSE_DEFAULT_TABLELOG);
}


/*********************************************************
*  Decompression (Byte symbols)
*********************************************************/
typedef struct
{
    U16  newState;
    BYTE symbol;
    BYTE nbBits;
} FSE_decode_t;   /* size == U32 */

/* Specific corner case : RLE compression */
size_t FSE_decompressRLE(void* dst, size_t originalSize,
                   const void* cSrc, size_t cSrcSize)
{
    if (cSrcSize != 1) return (size_t)-FSE_ERROR_srcSize_wrong;
    memset(dst, *(BYTE*)cSrc, originalSize);
    return originalSize;
}


size_t FSE_buildDTable_rle (void* DTable, BYTE symbolValue)
{
    U32* const base32 = (U32*)DTable;
    FSE_decode_t* const cell = (FSE_decode_t*)(base32 + 1);

    /* Sanity check */
    if (((size_t)DTable) & 3) return (size_t)-FSE_ERROR_GENERIC;   /* Must be allocated of 4 bytes boundaries */

    base32[0] = 0;

    cell->newState = 0;
    cell->symbol = symbolValue;
    cell->nbBits = 0;

    return 0;
}


size_t FSE_buildDTable_raw (void* DTable, unsigned nbBits)
{
    U32* const base32 = (U32*)DTable;
    FSE_decode_t* dinfo = (FSE_decode_t*)(base32 + 1);
    const unsigned tableSize = 1 << nbBits;
    const unsigned tableMask = tableSize - 1;
    const unsigned maxSymbolValue = tableMask;
    unsigned s;

    /* Sanity checks */
    if (nbBits < 1) return (size_t)-FSE_ERROR_GENERIC;             /* min size */
    if (((size_t)DTable) & 3) return (size_t)-FSE_ERROR_GENERIC;   /* Must be allocated of 4 bytes boundaries */

    /* Build Decoding Table */
    base32[0] = nbBits;
    for (s=0; s<=maxSymbolValue; s++)
    {
        dinfo[s].newState = 0;
        dinfo[s].symbol = (BYTE)s;
        dinfo[s].nbBits = (BYTE)nbBits;
    }

    return 0;
}


/* FSE_initDStream
 * Initialize a FSE_DStream_t.
 * srcBuffer must point at the beginning of an FSE block.
 * The function result is the size of the FSE_block (== srcSize).
 * If srcSize is too small, the function will return an errorCode;
 */
size_t FSE_initDStream(FSE_DStream_t* bitD, const void* srcBuffer, size_t srcSize)
{
    if (srcSize < 1) return (size_t)-FSE_ERROR_srcSize_wrong;

    if (srcSize >=  sizeof(bitD_t))
    {
        U32 contain32;
        bitD->start = (char*)srcBuffer;
        bitD->ptr   = (char*)srcBuffer + srcSize - sizeof(bitD_t);
        bitD->bitContainer = FSE_readLEST(bitD->ptr);
        contain32 = ((BYTE*)srcBuffer)[srcSize-1];
        if (contain32 == 0) return (size_t)-FSE_ERROR_GENERIC;   /* stop bit not present */
        bitD->bitsConsumed = 8 - FSE_highbit32(contain32);
    }
    else
    {
        U32 contain32;
        bitD->start = (char*)srcBuffer;
        bitD->ptr   = bitD->start;
        bitD->bitContainer = *(BYTE*)(bitD->start);
        switch(srcSize)
        {
            case 7: bitD->bitContainer += (bitD_t)(((BYTE*)(bitD->start))[6]) << (sizeof(bitD_t)*8 - 16);
            case 6: bitD->bitContainer += (bitD_t)(((BYTE*)(bitD->start))[5]) << (sizeof(bitD_t)*8 - 24);
            case 5: bitD->bitContainer += (bitD_t)(((BYTE*)(bitD->start))[4]) << (sizeof(bitD_t)*8 - 32);
            case 4: bitD->bitContainer += (bitD_t)(((BYTE*)(bitD->start))[3]) << 24;
            case 3: bitD->bitContainer += (bitD_t)(((BYTE*)(bitD->start))[2]) << 16;
            case 2: bitD->bitContainer += (bitD_t)(((BYTE*)(bitD->start))[1]) <<  8;
            default:;
        }
        contain32 = ((BYTE*)srcBuffer)[srcSize-1];
        if (contain32 == 0) return (size_t)-FSE_ERROR_GENERIC;   /* stop bit not present */
        bitD->bitsConsumed = 8 - FSE_highbit32(contain32);
        bitD->bitsConsumed += (U32)(sizeof(bitD_t) - srcSize)*8;
    }

    return srcSize;
}


/* FSE_readBits
 * Read next n bits from the bitContainer.
 * Use the fast variant *only* if n > 0.
 * Note : for this function to work properly on 32-bits, don't read more than maxNbBits==25
 * return : value extracted.
 */
bitD_t FSE_readBits(FSE_DStream_t* bitD, U32 nbBits)
{
    bitD_t value = ((bitD->bitContainer << bitD->bitsConsumed) >> 1) >> (((sizeof(bitD_t)*8)-1)-nbBits);
    bitD->bitsConsumed += nbBits;
    return value;
}

bitD_t FSE_readBitsFast(FSE_DStream_t* bitD, U32 nbBits)   /* only if nbBits >= 1 */
{
    bitD_t value = (bitD->bitContainer << bitD->bitsConsumed) >> ((sizeof(bitD_t)*8)-nbBits);
    bitD->bitsConsumed += nbBits;
    return value;
}

unsigned FSE_reloadDStream(FSE_DStream_t* bitD)
{
    if (bitD->ptr >= bitD->start + sizeof(bitD_t))
    {
        bitD->ptr -= bitD->bitsConsumed >> 3;
        bitD->bitsConsumed &= 7;
        bitD->bitContainer = FSE_readLEST(bitD->ptr);
        return 0;
    }
    if (bitD->ptr == bitD->start)
    {
        if (bitD->bitsConsumed < sizeof(bitD_t)*8) return 1;
        if (bitD->bitsConsumed == sizeof(bitD_t)*8) return 2;
        return 3;
    }
    {
        U32 nbBytes = bitD->bitsConsumed >> 3;
        if (bitD->ptr - nbBytes < bitD->start)
            nbBytes = (U32)(bitD->ptr - bitD->start);  /* note : necessarily ptr > start */
        bitD->ptr -= nbBytes;
        bitD->bitsConsumed -= nbBytes*8;
        bitD->bitContainer = FSE_readLEST(bitD->ptr);   /* note : necessarily srcSize > sizeof(bitD) */
        return (bitD->ptr == bitD->start);
    }
}


void FSE_initDState(FSE_DState_t* DStatePtr, FSE_DStream_t* bitD, const void* DTable)
{
    const U32* const base32 = (const U32*)DTable;
    DStatePtr->state = FSE_readBits(bitD, base32[0]);
    FSE_reloadDStream(bitD);
    DStatePtr->table = base32 + 1;
}

BYTE FSE_decodeSymbol(FSE_DState_t* DStatePtr, FSE_DStream_t* bitD)
{
    const FSE_decode_t DInfo = ((const FSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    const U32  nbBits = DInfo.nbBits;
    BYTE symbol = DInfo.symbol;
    bitD_t lowBits = FSE_readBits(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}

BYTE FSE_decodeSymbolFast(FSE_DState_t* DStatePtr, FSE_DStream_t* bitD)
{
    const FSE_decode_t DInfo = ((const FSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    const U32 nbBits = DInfo.nbBits;
    BYTE symbol = DInfo.symbol;
    bitD_t lowBits = FSE_readBitsFast(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}

/* FSE_endOfDStream
   Tells if bitD has reached end of bitStream or not */

unsigned FSE_endOfDStream(const FSE_DStream_t* bitD)
{
    return FSE_reloadDStream((FSE_DStream_t*)bitD)==2;
}

unsigned FSE_endOfDState(const FSE_DState_t* statePtr)
{
    return statePtr->state == 0;
}


FORCE_INLINE size_t FSE_decompress_usingDTable_generic(
          void* dst, size_t maxDstSize,
    const void* cSrc, size_t cSrcSize,
    const void* DTable, unsigned fast)
{
    BYTE* const ostart = (BYTE*) dst;
    BYTE* op = ostart;
    BYTE* const omax = op + maxDstSize;
    BYTE* const olimit = omax-3;

    FSE_DStream_t bitD;
    FSE_DState_t state1, state2;
    size_t errorCode;

    /* Init */
    errorCode = FSE_initDStream(&bitD, cSrc, cSrcSize);   /* replaced last arg by maxCompressed Size */
    if (FSE_isError(errorCode)) return errorCode;

    FSE_initDState(&state1, &bitD, DTable);
    FSE_initDState(&state2, &bitD, DTable);


    /* 2 symbols per loop */
    while (!FSE_reloadDStream(&bitD) && (op<olimit))
    {
        *op++ = fast ? FSE_decodeSymbolFast(&state1, &bitD) : FSE_decodeSymbol(&state1, &bitD);

        if (FSE_MAX_TABLELOG*2+7 > sizeof(bitD_t)*8)    /* This test must be static */
            FSE_reloadDStream(&bitD);

        *op++ = fast ? FSE_decodeSymbolFast(&state2, &bitD) : FSE_decodeSymbol(&state2, &bitD);

        if (FSE_MAX_TABLELOG*4+7 < sizeof(bitD_t)*8)    /* This test must be static */
        {
            *op++ = fast ? FSE_decodeSymbolFast(&state1, &bitD) : FSE_decodeSymbol(&state1, &bitD);
            *op++ = fast ? FSE_decodeSymbolFast(&state2, &bitD) : FSE_decodeSymbol(&state2, &bitD);
        }
    }

    /* tail */
    while (1)
    {
        if ( (FSE_reloadDStream(&bitD)>2) || (op==omax) || (FSE_endOfDState(&state1) && FSE_endOfDStream(&bitD)) )
            break;

        *op++ = fast ? FSE_decodeSymbolFast(&state1, &bitD) : FSE_decodeSymbol(&state1, &bitD);

        if ( (FSE_reloadDStream(&bitD)>2) || (op==omax) || (FSE_endOfDState(&state2) && FSE_endOfDStream(&bitD)) )
            break;

        *op++ = fast ? FSE_decodeSymbolFast(&state2, &bitD) : FSE_decodeSymbol(&state2, &bitD);
    }

    /* end ? */
    if (FSE_endOfDStream(&bitD) && FSE_endOfDState(&state1) && FSE_endOfDState(&state2) )
        return op-ostart;

    if (op==omax) return (size_t)-FSE_ERROR_dstSize_tooSmall;   /* dst buffer is full, but cSrc unfinished */

    return (size_t)-FSE_ERROR_corruptionDetected;
}


size_t FSE_decompress_usingDTable(void* dst, size_t originalSize,
                            const void* cSrc, size_t cSrcSize,
                            const void* DTable, size_t fastMode)
{
    /* select fast mode (static) */
    if (fastMode) return FSE_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, DTable, 1);
    return FSE_decompress_usingDTable_generic(dst, originalSize, cSrc, cSrcSize, DTable, 0);
}


size_t FSE_decompress(void* dst, size_t maxDstSize, const void* cSrc, size_t cSrcSize)
{
    const BYTE* const istart = (const BYTE*)cSrc;
    const BYTE* ip = istart;
    short counting[FSE_MAX_SYMBOL_VALUE+1];
    FSE_decode_t DTable[FSE_DTABLE_SIZE_U32(FSE_MAX_TABLELOG)];
    unsigned maxSymbolValue = FSE_MAX_SYMBOL_VALUE;
    unsigned tableLog;
    size_t errorCode, fastMode;

    if (cSrcSize<2) return (size_t)-FSE_ERROR_srcSize_wrong;   /* too small input size */

    /* normal FSE decoding mode */
    errorCode = FSE_readHeader (counting, &maxSymbolValue, &tableLog, istart, cSrcSize);
    if (FSE_isError(errorCode)) return errorCode;
    if (errorCode >= cSrcSize) return (size_t)-FSE_ERROR_srcSize_wrong;   /* too small input size */
    ip += errorCode;
    cSrcSize -= errorCode;

    fastMode = FSE_buildDTable (DTable, counting, maxSymbolValue, tableLog);
    if (FSE_isError(fastMode)) return fastMode;

    /* always return, even if it is an error code */
    return FSE_decompress_usingDTable (dst, maxDstSize, ip, cSrcSize, DTable, fastMode);
}


#endif   /* FSE_COMMONDEFS_ONLY */

/*
  2nd part of the file
  designed to be included
  for type-specific functions (template equivalent in C)
  Objective is to write such functions only once, for better maintenance
*/

/* safety checks */
#ifndef FSE_FUNCTION_EXTENSION
#  error "FSE_FUNCTION_EXTENSION must be defined"
#endif
#ifndef FSE_FUNCTION_TYPE
#  error "FSE_FUNCTION_TYPE must be defined"
#endif

/* Function names */
#define FSE_CAT(X,Y) X##Y
#define FSE_FUNCTION_NAME(X,Y) FSE_CAT(X,Y)
#define FSE_TYPE_NAME(X,Y) FSE_CAT(X,Y)


/* Function templates */
size_t FSE_FUNCTION_NAME(FSE_count_generic, FSE_FUNCTION_EXTENSION) (unsigned* count, const FSE_FUNCTION_TYPE* source, size_t sourceSize, unsigned* maxSymbolValuePtr, unsigned safe)
{
    const FSE_FUNCTION_TYPE* ip = source;
    const FSE_FUNCTION_TYPE* const iend = ip+sourceSize;
    unsigned maxSymbolValue = *maxSymbolValuePtr;
    unsigned max=0;
    int s;

    U32 Counting1[FSE_MAX_SYMBOL_VALUE+1] = { 0 };
    U32 Counting2[FSE_MAX_SYMBOL_VALUE+1] = { 0 };
    U32 Counting3[FSE_MAX_SYMBOL_VALUE+1] = { 0 };
    U32 Counting4[FSE_MAX_SYMBOL_VALUE+1] = { 0 };

    /* safety checks */
    if (!sourceSize)
    {
        memset(count, 0, (maxSymbolValue + 1) * sizeof(FSE_FUNCTION_TYPE));
        *maxSymbolValuePtr = 0;
        return 0;
    }
    if (maxSymbolValue > FSE_MAX_SYMBOL_VALUE) return (size_t)-FSE_ERROR_GENERIC;   /* maxSymbolValue too large : unsupported */
    if (!maxSymbolValue) maxSymbolValue = FSE_MAX_SYMBOL_VALUE;            /* 0 == default */

    if ((safe) || (sizeof(FSE_FUNCTION_TYPE)>1))
    {
        /* check input values, to avoid count table overflow */
        while (ip < iend-3)
        {
            if (*ip>maxSymbolValue) return (size_t)-FSE_ERROR_GENERIC; Counting1[*ip++]++;
            if (*ip>maxSymbolValue) return (size_t)-FSE_ERROR_GENERIC; Counting2[*ip++]++;
            if (*ip>maxSymbolValue) return (size_t)-FSE_ERROR_GENERIC; Counting3[*ip++]++;
            if (*ip>maxSymbolValue) return (size_t)-FSE_ERROR_GENERIC; Counting4[*ip++]++;
        }
    }
    else
    {
        U32 cached = FSE_read32(ip); ip += 4;
        while (ip < iend-15)
        {
            U32 c = cached; cached = FSE_read32(ip); ip += 4;
            Counting1[(BYTE) c     ]++;
            Counting2[(BYTE)(c>>8) ]++;
            Counting3[(BYTE)(c>>16)]++;
            Counting4[       c>>24 ]++;
            c = cached; cached = FSE_read32(ip); ip += 4;
            Counting1[(BYTE) c     ]++;
            Counting2[(BYTE)(c>>8) ]++;
            Counting3[(BYTE)(c>>16)]++;
            Counting4[       c>>24 ]++;
            c = cached; cached = FSE_read32(ip); ip += 4;
            Counting1[(BYTE) c     ]++;
            Counting2[(BYTE)(c>>8) ]++;
            Counting3[(BYTE)(c>>16)]++;
            Counting4[       c>>24 ]++;
            c = cached; cached = FSE_read32(ip); ip += 4;
            Counting1[(BYTE) c     ]++;
            Counting2[(BYTE)(c>>8) ]++;
            Counting3[(BYTE)(c>>16)]++;
            Counting4[       c>>24 ]++;
        }
        ip-=4;
    }

    /* finish last symbols */
    while (ip<iend) { if ((safe) && (*ip>maxSymbolValue)) return (size_t)-FSE_ERROR_GENERIC; Counting1[*ip++]++; }

    for (s=0; s<=(int)maxSymbolValue; s++)
    {
        count[s] = Counting1[s] + Counting2[s] + Counting3[s] + Counting4[s];
        if (count[s] > max) max = count[s];
    }

    while (!count[maxSymbolValue]) maxSymbolValue--;
    *maxSymbolValuePtr = maxSymbolValue;
    return (int)max;
}

/* hidden fast variant (unsafe) */
size_t FSE_FUNCTION_NAME(FSE_countFast, FSE_FUNCTION_EXTENSION) (unsigned* count, const FSE_FUNCTION_TYPE* source, size_t sourceSize, unsigned* maxSymbolValuePtr)
{
    return FSE_FUNCTION_NAME(FSE_count_generic, FSE_FUNCTION_EXTENSION) (count, source, sourceSize, maxSymbolValuePtr, 0);
}

size_t FSE_FUNCTION_NAME(FSE_count, FSE_FUNCTION_EXTENSION) (unsigned* count, const FSE_FUNCTION_TYPE* source, size_t sourceSize, unsigned* maxSymbolValuePtr)
{
    if ((sizeof(FSE_FUNCTION_TYPE)==1) && (*maxSymbolValuePtr >= 255))
    {
        *maxSymbolValuePtr = 255;
        return FSE_FUNCTION_NAME(FSE_count_generic, FSE_FUNCTION_EXTENSION) (count, source, sourceSize, maxSymbolValuePtr, 0);
    }
    return FSE_FUNCTION_NAME(FSE_count_generic, FSE_FUNCTION_EXTENSION) (count, source, sourceSize, maxSymbolValuePtr, 1);
}


static U32 FSE_tableStep(U32 tableSize) { return (tableSize>>1) + (tableSize>>3) + 3; }

size_t FSE_FUNCTION_NAME(FSE_buildCTable, FSE_FUNCTION_EXTENSION)
(void* CTable, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog)
{
    const unsigned tableSize = 1 << tableLog;
    const unsigned tableMask = tableSize - 1;
    U16* tableU16 = ( (U16*) CTable) + 2;
    FSE_symbolCompressionTransform* symbolTT = (FSE_symbolCompressionTransform*) (((U32*)CTable) + 1 + (tableLog ? tableSize>>1 : 1) );
    const unsigned step = FSE_tableStep(tableSize);
    unsigned cumul[FSE_MAX_SYMBOL_VALUE+2];
    U32 position = 0;
    FSE_FUNCTION_TYPE tableSymbol[FSE_MAX_TABLESIZE];
    U32 highThreshold = tableSize-1;
    unsigned symbol;
    unsigned i;

    /* safety checks */
    if (((size_t)CTable) & 3) return (size_t)-FSE_ERROR_GENERIC;   /* Must be allocated of 4 bytes boundaries */

    /* header */
    tableU16[-2] = (U16) tableLog;
    tableU16[-1] = (U16) maxSymbolValue;

    /* For explanations on how to distribute symbol values over the table :
    *  http://fastcompression.blogspot.fr/2014/02/fse-distributing-symbol-values.html */

    /* symbol start positions */
    cumul[0] = 0;
    for (i=1; i<=maxSymbolValue+1; i++)
    {
        if (normalizedCounter[i-1]==-1)   /* Low prob symbol */
        {
            cumul[i] = cumul[i-1] + 1;
            tableSymbol[highThreshold--] = (FSE_FUNCTION_TYPE)(i-1);
        }
        else
            cumul[i] = cumul[i-1] + normalizedCounter[i-1];
    }
    cumul[maxSymbolValue+1] = tableSize+1;

    /* Spread symbols */
    for (symbol=0; symbol<=maxSymbolValue; symbol++)
    {
        int nbOccurences;
        for (nbOccurences=0; nbOccurences<normalizedCounter[symbol]; nbOccurences++)
        {
            tableSymbol[position] = (FSE_FUNCTION_TYPE)symbol;
            position = (position + step) & tableMask;
            while (position > highThreshold) position = (position + step) & tableMask;   /* Lowprob area */
        }
    }

    if (position!=0) return (size_t)-FSE_ERROR_GENERIC;   /* Must have gone through all positions */

    /* Build table */
    for (i=0; i<tableSize; i++)
    {
        FSE_FUNCTION_TYPE s = tableSymbol[i];
        tableU16[cumul[s]++] = (U16) (tableSize+i);   // Table U16 : sorted by symbol order; gives next state value
    }

    // Build Symbol Transformation Table
    {
        unsigned s;
        unsigned total = 0;
        for (s=0; s<=maxSymbolValue; s++)
        {
            switch (normalizedCounter[s])
            {
            case 0:
                break;
            case -1:
            case 1:
                symbolTT[s].minBitsOut = (BYTE)tableLog;
                symbolTT[s].deltaFindState = total - 1;
                total ++;
                symbolTT[s].maxState = (U16)( (tableSize*2) - 1);   /* ensures state <= maxState */
                break;
            default :
                symbolTT[s].minBitsOut = (BYTE)( (tableLog-1) - FSE_highbit32 (normalizedCounter[s]-1) );
                symbolTT[s].deltaFindState = total - normalizedCounter[s];
                total +=  normalizedCounter[s];
                symbolTT[s].maxState = (U16)( (normalizedCounter[s] << (symbolTT[s].minBitsOut+1)) - 1);
            }
        }
    }

    return 0;
}


#define FSE_DECODE_TYPE FSE_TYPE_NAME(FSE_decode_t, FSE_FUNCTION_EXTENSION)

void* FSE_FUNCTION_NAME(FSE_createDTable, FSE_FUNCTION_EXTENSION) (unsigned tableLog)
{
    if (tableLog > FSE_TABLELOG_ABSOLUTE_MAX) tableLog = FSE_TABLELOG_ABSOLUTE_MAX;
    return malloc( ((size_t)1<<tableLog) * sizeof (FSE_DECODE_TYPE) );
}

void FSE_FUNCTION_NAME(FSE_freeDTable, FSE_FUNCTION_EXTENSION) (void* DTable)
{
    free(DTable);
}


size_t FSE_FUNCTION_NAME(FSE_buildDTable, FSE_FUNCTION_EXTENSION)
(void* DTable, const short* const normalizedCounter, unsigned maxSymbolValue, unsigned tableLog)
{
    U32* const base32 = (U32*)DTable;
    FSE_DECODE_TYPE* const tableDecode = (FSE_DECODE_TYPE*) (base32+1);
    const U32 tableSize = 1 << tableLog;
    const U32 tableMask = tableSize-1;
    const U32 step = FSE_tableStep(tableSize);
    U16 symbolNext[FSE_MAX_SYMBOL_VALUE+1];
    U32 position = 0;
    U32 highThreshold = tableSize-1;
    const S16 largeLimit= 1 << (tableLog-1);
    U32 noLarge = 1;
    U32 s;

    /* Sanity Checks */
    if (maxSymbolValue > FSE_MAX_SYMBOL_VALUE) return (size_t)-FSE_ERROR_maxSymbolValue_tooLarge;
    if (tableLog > FSE_MAX_TABLELOG) return (size_t)-FSE_ERROR_tableLog_tooLarge;

    /* Init, lay down lowprob symbols */
    base32[0] = tableLog;
    for (s=0; s<=maxSymbolValue; s++)
    {
        if (normalizedCounter[s]==-1)
        {
            tableDecode[highThreshold--].symbol = (FSE_FUNCTION_TYPE)s;
            symbolNext[s] = 1;
        }
        else
        {
            if (normalizedCounter[s] >= largeLimit) noLarge=0;
            symbolNext[s] = normalizedCounter[s];
        }
    }

    /* Spread symbols */
    for (s=0; s<=maxSymbolValue; s++)
    {
        int i;
        for (i=0; i<normalizedCounter[s]; i++)
        {
            tableDecode[position].symbol = (FSE_FUNCTION_TYPE)s;
            position = (position + step) & tableMask;
            while (position > highThreshold) position = (position + step) & tableMask;   /* lowprob area */
        }
    }

    if (position!=0) return (size_t)-FSE_ERROR_GENERIC;   /* position must reach all cells once, otherwise normalizedCounter is incorrect */

    /* Build Decoding table */
    {
        U32 i;
        for (i=0; i<tableSize; i++)
        {
            FSE_FUNCTION_TYPE symbol = tableDecode[i].symbol;
            U16 nextState = symbolNext[symbol]++;
            tableDecode[i].nbBits = (BYTE) (tableLog - FSE_highbit32 ((U32)nextState) );
            tableDecode[i].newState = (U16) ( (nextState << tableDecode[i].nbBits) - tableSize);
        }
    }

    return noLarge;
}

/* <<<<< fse.c EOF */


/* >>>>> zstd.c */

/****************************************************************
*  Tuning parameters
*****************************************************************/
/* MEMORY_USAGE :
*  Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
*  Increasing memory usage improves compression ratio
*  Reduced memory usage can improve speed, due to cache effect */
#define ZSTD_MEMORY_USAGE 17


/**************************************
   CPU Feature Detection
**************************************/
/*
 * Automated efficient unaligned memory access detection
 * Based on known hardware architectures
 * This list will be updated thanks to feedbacks
 */
#if defined(CPU_HAS_EFFICIENT_UNALIGNED_MEMORY_ACCESS) \
    || defined(__ARM_FEATURE_UNALIGNED) \
    || defined(__i386__) || defined(__x86_64__) \
    || defined(_M_IX86) || defined(_M_X64) \
    || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_8__) \
    || (defined(_M_ARM) && (_M_ARM >= 7))
#  define ZSTD_UNALIGNED_ACCESS 1
#else
#  define ZSTD_UNALIGNED_ACCESS 0
#endif

#ifndef MEM_ACCESS_MODULE
#define MEM_ACCESS_MODULE
/********************************************************
*  Basic Types
*********************************************************/
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
typedef  uint8_t BYTE;
typedef uint16_t U16;
typedef  int16_t S16;
typedef uint32_t U32;
typedef  int32_t S32;
typedef uint64_t U64;
#else
typedef unsigned char       BYTE;
typedef unsigned short      U16;
typedef   signed short      S16;
typedef unsigned int        U32;
typedef   signed int        S32;
typedef unsigned long long  U64;
#endif

#endif   /* MEM_ACCESS_MODULE */

/********************************************************
*  Constants
*********************************************************/
static const U32 ZSTD_magicNumber = 0xFD2FB51C;   /* Initial (limited) frame format */

#define HASH_LOG (ZSTD_MEMORY_USAGE - 2)
#define HASH_TABLESIZE (1 << HASH_LOG)
#define HASH_MASK (HASH_TABLESIZE - 1)

#define KNUTH 2654435761

#define BIT7 128
#define BIT6  64
#define BIT5  32
#define BIT4  16

#ifndef KB
#define KB *(1 <<10)
#endif
#ifndef MB
#define MB *(1 <<20)
#endif
#ifndef GB
#define GB *(1U<<30)
#endif

#define BLOCKSIZE (128 KB)                 /* define, for static allocation */
static const U32 g_maxDistance = 4 * BLOCKSIZE;
static const U32 g_maxLimit = 1 GB;
static const U32 g_searchStrength = 8;

#define WORKPLACESIZE (BLOCKSIZE*11/4)
#define MINMATCH 4
#define MLbits   7
#define LLbits   6
#define Offbits  5
#define MaxML  ((1<<MLbits )-1)
#define MaxLL  ((1<<LLbits )-1)
#define MaxOff ((1<<Offbits)-1)
#define LitFSELog  11
#define MLFSELog   10
#define LLFSELog   10
#define OffFSELog   9

#define LITERAL_NOENTROPY 63
#define COMMAND_NOENTROPY 7   /* to remove */

static const size_t ZSTD_blockHeaderSize = 3;
static const size_t ZSTD_frameHeaderSize = 4;


/********************************************************
*  Memory operations
*********************************************************/
static unsigned ZSTD_32bits(void) { return sizeof(void*)==4; }
static unsigned ZSTD_64bits(void) { return sizeof(void*)==8; }

static unsigned ZSTD_isLittleEndian(void)
{
    const union { U32 i; BYTE c[4]; } one = { 1 };   /* don't use static : performance detrimental  */
    return one.c[0];
}

static U16    ZSTD_read16(const void* p) { return *(U16*)p; }

static U32    ZSTD_read32(const void* p) { return *(U32*)p; }

static size_t ZSTD_read_ARCH(const void* p) { return *(size_t*)p; }

static void   ZSTD_copy4(void* dst, const void* src) { memcpy(dst, src, 4); }

static void   ZSTD_copy8(void* dst, const void* src) { memcpy(dst, src, 8); }

#define COPY8(d,s)    { ZSTD_copy8(d,s); d+=8; s+=8; }

static void ZSTD_wildcopy(void* dst, const void* src, size_t length)
{
    const BYTE* ip = (const BYTE*)src;
    BYTE* op = (BYTE*)dst;
    BYTE* const oend = op + length;
    while (op < oend) COPY8(op, ip);
}

static U32 ZSTD_readLE32(const void* memPtr)
{
    if (ZSTD_isLittleEndian())
        return ZSTD_read32(memPtr);
    else
    {
        const BYTE* p = (const BYTE*)memPtr;
        return (U32)((U32)p[0] + ((U32)p[1]<<8) + ((U32)p[2]<<16) + ((U32)p[3]<<24));
    }
}

static void ZSTD_writeLE32(void* memPtr, U32 val32)
{
    if (ZSTD_isLittleEndian())
    {
        memcpy(memPtr, &val32, 4);
    }
    else
    {
        BYTE* p = (BYTE*)memPtr;
        p[0] = (BYTE)val32;
        p[1] = (BYTE)(val32>>8);
        p[2] = (BYTE)(val32>>16);
        p[3] = (BYTE)(val32>>24);
    }
}

static U32 ZSTD_readBE32(const void* memPtr)
{
    const BYTE* p = (const BYTE*)memPtr;
    return (U32)(((U32)p[0]<<24) + ((U32)p[1]<<16) + ((U32)p[2]<<8) + ((U32)p[3]<<0));
}

static void ZSTD_writeBE32(void* memPtr, U32 value)
{
    BYTE* const p = (BYTE* const) memPtr;
    p[0] = (BYTE)(value>>24);
    p[1] = (BYTE)(value>>16);
    p[2] = (BYTE)(value>>8);
    p[3] = (BYTE)(value>>0);
}

static size_t ZSTD_writeProgressive(void* ptr, size_t value)
{
    BYTE* const bStart = (BYTE* const)ptr;
    BYTE* byte = bStart;

    do
    {
        BYTE l = value & 127;
        value >>= 7;
        if (value) l += 128;
        *byte++ = l;
    } while (value);

    return byte - bStart;
}


static size_t ZSTD_readProgressive(size_t* result, const void* ptr)
{
    const BYTE* const bStart = (const BYTE* const)ptr;
    const BYTE* byte = bStart;
    size_t r = 0;
    U32 shift = 0;

    do
    {
        r += (*byte & 127) << shift;
        shift += 7;
    } while (*byte++ & 128);

    *result = r;
    return byte - bStart;
}


/**************************************
*  Local structures
***************************************/
typedef enum { bt_compressed, bt_raw, bt_rle, bt_end } blockType_t;

typedef struct
{
    blockType_t blockType;
    U32 origSize;
} blockProperties_t;

typedef struct {
    void* buffer;
    U32*  offsetStart;
    U32*  offset;
    BYTE* litStart;
    BYTE* lit;
    BYTE* litLengthStart;
    BYTE* litLength;
    BYTE* matchLengthStart;
    BYTE* matchLength;
    BYTE* dumpsStart;
    BYTE* dumps;
} seqStore_t;

void ZSTD_resetSeqStore(seqStore_t* ssPtr)
{
    ssPtr->offset = ssPtr->offsetStart;
    ssPtr->lit = ssPtr->litStart;
    ssPtr->litLength = ssPtr->litLengthStart;
    ssPtr->matchLength = ssPtr->matchLengthStart;
    ssPtr->dumps = ssPtr->dumpsStart;
}


typedef struct
{
    const BYTE* base;
    U32 current;
    U32 nextUpdate;
    seqStore_t seqStore;
#ifdef __AVX2__
    __m256i hashTable[HASH_TABLESIZE>>3];
#else
    U32 hashTable[HASH_TABLESIZE];
#endif
} cctxi_t;


ZSTD_cctx_t ZSTD_createCCtx(void)
{
    cctxi_t* ctx = (cctxi_t*) malloc( sizeof(cctxi_t) );
    ctx->seqStore.buffer = malloc(WORKPLACESIZE);
    ctx->seqStore.offsetStart = (U32*) (ctx->seqStore.buffer);
    ctx->seqStore.litStart = (BYTE*) (ctx->seqStore.offsetStart + (BLOCKSIZE>>2));
    ctx->seqStore.litLengthStart =  ctx->seqStore.litStart + BLOCKSIZE;
    ctx->seqStore.matchLengthStart = ctx->seqStore.litLengthStart + (BLOCKSIZE>>2);
    ctx->seqStore.dumpsStart = ctx->seqStore.matchLengthStart + (BLOCKSIZE>>2);
    return (ZSTD_cctx_t)ctx;
}

void ZSTD_resetCCtx(ZSTD_cctx_t cctx)
{
    cctxi_t* ctx = (cctxi_t*)cctx;
    ctx->base = NULL;
    memset(ctx->hashTable, 0, HASH_TABLESIZE*4);
}

size_t ZSTD_freeCCtx(ZSTD_cctx_t cctx)
{
    cctxi_t* ctx = (cctxi_t*) (cctx);
    free(ctx->seqStore.buffer);
    free(ctx);
    return 0;
}


/**************************************
*  Error Management
**************************************/
/* tells if a return value is an error code */
unsigned ZSTD_isError(size_t code)
{
    return (code > (size_t)(-ZSTD_ERROR_maxCode));
}

#define ZSTD_GENERATE_STRING(STRING) #STRING,
static const char* ZSTD_errorStrings[] = { ZSTD_LIST_ERRORS(ZSTD_GENERATE_STRING) };

/* provides error code string (useful for debugging) */
const char* ZSTD_getErrorName(size_t code)
{
    static const char* codeError = "Unspecified error code";
    if (ZSTD_isError(code)) return ZSTD_errorStrings[-(int)(code)];
    return codeError;
}


/**************************************
*  Tool functions
**************************************/
unsigned ZSTD_versionNumber (void) { return ZSTD_VERSION_NUMBER; }

static unsigned ZSTD_highbit(U32 val)
{
#   if defined(_MSC_VER)   /* Visual */
    unsigned long r;
    _BitScanReverse(&r, val);
    return (unsigned)r;
#   elif defined(__GNUC__) && (GCC_VERSION >= 304)   /* GCC Intrinsic */
    return 31 - __builtin_clz(val);
#   else   /* Software version */
    static const int DeBruijnClz[32] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 };
    U32 v = val;
    int r;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    r = DeBruijnClz[(U32)(v * 0x07C4ACDDU) >> 27];
    return r;
#   endif
}

static unsigned ZSTD_NbCommonBytes (register size_t val)
{
    if (ZSTD_isLittleEndian())
    {
        if (ZSTD_64bits())
        {
#       if defined(_MSC_VER) && defined(_WIN64) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r = 0;
            _BitScanForward64( &r, (U64)val );
            return (int)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_ctzll((U64)val) >> 3);
#       else
            static const int DeBruijnBytePos[64] = { 0, 0, 0, 0, 0, 1, 1, 2, 0, 3, 1, 3, 1, 4, 2, 7, 0, 2, 3, 6, 1, 5, 3, 5, 1, 3, 4, 4, 2, 5, 6, 7, 7, 0, 1, 2, 3, 3, 4, 6, 2, 6, 5, 5, 3, 4, 5, 6, 7, 1, 2, 4, 6, 4, 4, 5, 7, 2, 6, 5, 7, 6, 7, 7 };
            return DeBruijnBytePos[((U64)((val & -(long long)val) * 0x0218A392CDABBD3FULL)) >> 58];
#       endif
        }
        else /* 32 bits */
        {
#       if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r;
            _BitScanForward( &r, (U32)val );
            return (int)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_ctz((U32)val) >> 3);
#       else
            static const int DeBruijnBytePos[32] = { 0, 0, 3, 0, 3, 1, 3, 0, 3, 2, 2, 1, 3, 2, 0, 1, 3, 3, 1, 2, 2, 2, 2, 0, 3, 1, 2, 0, 1, 0, 1, 1 };
            return DeBruijnBytePos[((U32)((val & -(S32)val) * 0x077CB531U)) >> 27];
#       endif
        }
    }
    else   /* Big Endian CPU */
    {
        if (ZSTD_64bits())
        {
#       if defined(_MSC_VER) && defined(_WIN64) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r = 0;
            _BitScanReverse64( &r, val );
            return (unsigned)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_clzll(val) >> 3);
#       else
            unsigned r;
            const unsigned n32 = sizeof(size_t)*4;   /* calculate this way due to compiler complaining in 32-bits mode */
            if (!(val>>n32)) { r=4; } else { r=0; val>>=n32; }
            if (!(val>>16)) { r+=2; val>>=8; } else { val>>=24; }
            r += (!val);
            return r;
#       endif
        }
        else /* 32 bits */
        {
#       if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
            unsigned long r = 0;
            _BitScanReverse( &r, (unsigned long)val );
            return (unsigned)(r>>3);
#       elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
            return (__builtin_clz(val) >> 3);
#       else
            unsigned r;
            if (!(val>>16)) { r=2; val>>=8; } else { r=0; val>>=24; }
            r += (!val);
            return r;
#       endif
        }
    }
}

static unsigned ZSTD_count(const BYTE* pIn, const BYTE* pMatch, const BYTE* pInLimit)
{
    const BYTE* const pStart = pIn;

    while ((pIn<pInLimit-(sizeof(size_t)-1)))
    {
        size_t diff = ZSTD_read_ARCH(pMatch) ^ ZSTD_read_ARCH(pIn);
        if (!diff) { pIn+=sizeof(size_t); pMatch+=sizeof(size_t); continue; }
        pIn += ZSTD_NbCommonBytes(diff);
        return (unsigned)(pIn - pStart);
    }

    if (ZSTD_64bits()) if ((pIn<(pInLimit-3)) && (ZSTD_read32(pMatch) == ZSTD_read32(pIn))) { pIn+=4; pMatch+=4; }
    if ((pIn<(pInLimit-1)) && (ZSTD_read16(pMatch) == ZSTD_read16(pIn))) { pIn+=2; pMatch+=2; }
    if ((pIn<pInLimit) && (*pMatch == *pIn)) pIn++;
    return (unsigned)(pIn - pStart);
}


/********************************************************
*  Compression
*********************************************************/
size_t ZSTD_compressBound(size_t srcSize)   /* maximum compressed size */
{
    return FSE_compressBound(srcSize) + 12;
}


static size_t ZSTD_compressRle (void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    BYTE* const ostart = (BYTE* const)dst;

    /* at this stage : dstSize >= FSE_compressBound(srcSize) > (ZSTD_blockHeaderSize+1) (checked by ZSTD_compressLiterals()) */
    (void)maxDstSize;

    ostart[ZSTD_blockHeaderSize] = *(BYTE*)src;

    /* Build header */
    ostart[0]  = (BYTE)(srcSize>>16);
    ostart[1]  = (BYTE)(srcSize>>8);
    ostart[2]  = (BYTE)srcSize;
    ostart[0] += (BYTE)(bt_rle<<6);

    return ZSTD_blockHeaderSize+1;
}


static size_t ZSTD_noCompressBlock (void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    BYTE* const ostart = (BYTE* const)dst;

    if (srcSize + ZSTD_blockHeaderSize > maxDstSize) return (size_t)-ZSTD_ERROR_maxDstSize_tooSmall;
    memcpy(ostart + ZSTD_blockHeaderSize, src, srcSize);

    /* Build header */
    ostart[0] = (BYTE)(srcSize>>16);
    ostart[1] = (BYTE)(srcSize>>8);
    ostart[2] = (BYTE)srcSize;
    ostart[0] += (BYTE)(bt_raw<<6);   /* is a raw (uncompressed) block */

    return ZSTD_blockHeaderSize+srcSize;
}


/* return : size of CStream in bits */
static size_t ZSTD_compressLiterals_usingCTable(void* dst, size_t dstSize,
                                          const void* src, size_t srcSize,
                                          const void* CTable)
{
    const BYTE* const istart = (const BYTE*)src;
    const BYTE* ip = istart;
    const BYTE* const iend = istart + srcSize;
    FSE_CStream_t bitC;
    FSE_CState_t CState1, CState2;

    /* init */
    (void)dstSize;   // objective : ensure it fits into dstBuffer (Todo)
    FSE_initCStream(&bitC, dst);
    FSE_initCState(&CState1, CTable);
    CState2 = CState1;

    /* Note : at this stage, srcSize > LITERALS_NOENTROPY (checked by ZSTD_compressLiterals()) */
    // join to mod 2
    if (srcSize & 1)
    {
        FSE_encodeByte(&bitC, &CState1, *ip++);
        FSE_flushBits(&bitC);
    }

    // join to mod 4
    if ((sizeof(size_t)*8 > LitFSELog*4+7 ) && (srcSize & 2))   // test bit 2
    {
        FSE_encodeByte(&bitC, &CState2, *ip++);
        FSE_encodeByte(&bitC, &CState1, *ip++);
        FSE_flushBits(&bitC);
    }

    // 2 or 4 encoding per loop
    while (ip<iend)
    {
        FSE_encodeByte(&bitC, &CState2, *ip++);

        if (sizeof(size_t)*8 < LitFSELog*2+7 )   // this test must be static
            FSE_flushBits(&bitC);

        FSE_encodeByte(&bitC, &CState1, *ip++);

        if (sizeof(size_t)*8 > LitFSELog*4+7 )   // this test must be static
        {
            FSE_encodeByte(&bitC, &CState2, *ip++);
            FSE_encodeByte(&bitC, &CState1, *ip++);
        }

        FSE_flushBits(&bitC);
    }

    FSE_flushCState(&bitC, &CState2);
    FSE_flushCState(&bitC, &CState1);
    return FSE_closeCStream(&bitC);
}


size_t ZSTD_minGain(size_t srcSize)
{
    return (srcSize >> 6) + 1;
}


static size_t ZSTD_compressLiterals (void* dst, size_t dstSize,
                                     const void* src, size_t srcSize)
{
    const BYTE* const istart = (const BYTE*) src;
    const BYTE* ip = istart;

    BYTE* const ostart = (BYTE*) dst;
    BYTE* op = ostart + ZSTD_blockHeaderSize;
    BYTE* const oend = ostart + dstSize;

    U32 maxSymbolValue = 256;
    U32 tableLog = LitFSELog;
    U32 count[256];
    S16 norm[256];
    U32 CTable[ FSE_CTABLE_SIZE_U32(LitFSELog, 256) ];
    size_t errorCode;
    const size_t minGain = ZSTD_minGain(srcSize);

    /* early out */
    if (dstSize < FSE_compressBound(srcSize)) return (size_t)-ZSTD_ERROR_maxDstSize_tooSmall;

    /* Scan input and build symbol stats */
    errorCode = FSE_count (count, ip, srcSize, &maxSymbolValue);
    if (FSE_isError(errorCode)) return (size_t)-ZSTD_ERROR_GENERIC;
    if (errorCode == srcSize) return 1;
    //if (errorCode < ((srcSize * 7) >> 10)) return 0;
    //if (errorCode < (srcSize >> 7)) return 0;
    if (errorCode < (srcSize >> 6)) return 0;   /* heuristic : probably not compressible enough */

    tableLog = FSE_optimalTableLog(tableLog, srcSize, maxSymbolValue);
    errorCode = (int)FSE_normalizeCount (norm, tableLog, count, srcSize, maxSymbolValue);
    if (FSE_isError(errorCode)) return (size_t)-ZSTD_ERROR_GENERIC;

    /* Write table description header */
    errorCode = FSE_writeHeader (op, FSE_MAX_HEADERSIZE, norm, maxSymbolValue, tableLog);
    if (FSE_isError(errorCode)) return (size_t)-ZSTD_ERROR_GENERIC;
    op += errorCode;

    /* Compress */
    errorCode = FSE_buildCTable (&CTable, norm, maxSymbolValue, tableLog);
    if (FSE_isError(errorCode)) return (size_t)-ZSTD_ERROR_GENERIC;
    errorCode = ZSTD_compressLiterals_usingCTable(op, oend - op, ip, srcSize, &CTable);
    if (ZSTD_isError(errorCode)) return errorCode;
    op += errorCode;

    /* check compressibility */
    if ( (size_t)(op-ostart) >= srcSize-minGain)
        return 0;

    /* Build header */
    {
        size_t totalSize;
        totalSize  = op - ostart - ZSTD_blockHeaderSize;
        ostart[0]  = (BYTE)(totalSize>>16);
        ostart[1]  = (BYTE)(totalSize>>8);
        ostart[2]  = (BYTE)totalSize;
        ostart[0] += (BYTE)(bt_compressed<<6); /* is a block, is compressed */
    }

    return op-ostart;
}


static size_t ZSTD_compressSequences(BYTE* dst, size_t maxDstSize,
                                     const seqStore_t* seqStorePtr,
                                     size_t lastLLSize, size_t srcSize)
{
    FSE_CStream_t blockStream;
    U32 count[256];
    S16 norm[256];
    size_t mostFrequent;
    U32 max = 255;
    U32 tableLog = 11;
    U32 CTable_LitLength  [FSE_CTABLE_SIZE_U32(LLFSELog, MaxLL )];
    U32 CTable_OffsetBits [FSE_CTABLE_SIZE_U32(OffFSELog,MaxOff)];
    U32 CTable_MatchLength[FSE_CTABLE_SIZE_U32(MLFSELog, MaxML )];
    U32 LLtype, Offtype, MLtype;
    const BYTE* const op_lit_start = seqStorePtr->litStart;
    const BYTE* op_lit = seqStorePtr->lit;
    const BYTE* const op_litLength_start = seqStorePtr->litLengthStart;
    const BYTE* op_litLength = seqStorePtr->litLength;
    const U32*  op_offset = seqStorePtr->offset;
    const BYTE* op_matchLength = seqStorePtr->matchLength;
    const size_t nbSeq = op_litLength - op_litLength_start;
    BYTE* op;
    BYTE offsetBits_start[BLOCKSIZE / 4];
    BYTE* offsetBitsPtr = offsetBits_start;
    const size_t minGain = ZSTD_minGain(srcSize);
    const size_t maxCSize = srcSize - minGain;
    const size_t minSeqSize = 1 /*lastL*/ + 2 /*dHead*/ + 2 /*dumpsIn*/ + 5 /*SeqHead*/ + 3 /*SeqIn*/ + 1 /*margin*/ + ZSTD_blockHeaderSize;
    const size_t maxLSize = maxCSize > minSeqSize ? maxCSize - minSeqSize : 0;
    BYTE* seqHead;


    /* init */
    op = dst;

    /* Encode literals */
    {
        size_t cSize;
        size_t litSize = op_lit - op_lit_start;
        if (litSize <= LITERAL_NOENTROPY) cSize = ZSTD_noCompressBlock (op, maxDstSize, op_lit_start, litSize);
        else
        {
            cSize = ZSTD_compressLiterals(op, maxDstSize, op_lit_start, litSize);
            if (cSize == 1) cSize = ZSTD_compressRle (op, maxDstSize, op_lit_start, litSize);
            else if (cSize == 0)
            {
                if (litSize >= maxLSize) return 0;   /* block not compressible enough */
                cSize = ZSTD_noCompressBlock (op, maxDstSize, op_lit_start, litSize);
            }
        }
        if (ZSTD_isError(cSize)) return cSize;
        op += cSize;
    }

    /* Encode Sequences */

    /* seqHeader */
    op += ZSTD_writeProgressive(op, lastLLSize);
    seqHead = op;

    /* dumps */
    {
        size_t dumpsLength = seqStorePtr->dumps - seqStorePtr->dumpsStart;
        if (dumpsLength < 512)
        {
            op[0] = (BYTE)(dumpsLength >> 8);
            op[1] = (BYTE)(dumpsLength);
            op += 2;
        }
        else
        {
            op[0] = 2;
            op[1] = (BYTE)(dumpsLength>>8);
            op[2] = (BYTE)(dumpsLength);
            op += 3;
        }
        memcpy(op, seqStorePtr->dumpsStart, dumpsLength);
        op += dumpsLength;
    }

    /* Encoding table of Literal Lengths */
    max = MaxLL;
    mostFrequent = FSE_countFast(count, seqStorePtr->litLengthStart, nbSeq, &max);
    if (mostFrequent == nbSeq)
    {
        *op++ = *(seqStorePtr->litLengthStart);
        FSE_buildCTable_rle(CTable_LitLength, (BYTE)max);
        LLtype = bt_rle;
    }
    else if ((nbSeq < 64) || (mostFrequent < (nbSeq >> (LLbits-1))))
    {
        FSE_buildCTable_raw(CTable_LitLength, LLbits);
        LLtype = bt_raw;
    }
    else
    {
        tableLog = FSE_optimalTableLog(LLFSELog, nbSeq, max);
        FSE_normalizeCount(norm, tableLog, count, nbSeq, max);
        op += FSE_writeHeader(op, maxDstSize, norm, max, tableLog);
        FSE_buildCTable(CTable_LitLength, norm, max, tableLog);
        LLtype = bt_compressed;
    }

    /* Encoding table of Offsets */
    {
        /* create OffsetBits */
        size_t i;
        const U32* const op_offset_start = seqStorePtr->offsetStart;
        max = MaxOff;
        for (i=0; i<nbSeq; i++)
        {
            offsetBits_start[i] = (BYTE)ZSTD_highbit(op_offset_start[i]) + 1;
            if (op_offset_start[i]==0) offsetBits_start[i]=0;
        }
        offsetBitsPtr += nbSeq;
        mostFrequent = FSE_countFast(count, offsetBits_start, nbSeq, &max);
    }
    if (mostFrequent == nbSeq)
    {
        *op++ = *offsetBits_start;
        FSE_buildCTable_rle(CTable_OffsetBits, (BYTE)max);
        Offtype = bt_rle;
    }
    else if ((nbSeq < 64) || (mostFrequent < (nbSeq >> (Offbits-1))))
    {
        FSE_buildCTable_raw(CTable_OffsetBits, Offbits);
        Offtype = bt_raw;
    }
    else
    {
        tableLog = FSE_optimalTableLog(OffFSELog, nbSeq, max);
        FSE_normalizeCount(norm, tableLog, count, nbSeq, max);
        op += FSE_writeHeader(op, maxDstSize, norm, max, tableLog);
        FSE_buildCTable(CTable_OffsetBits, norm, max, tableLog);
        Offtype = bt_compressed;
    }

    /* Encoding Table of MatchLengths */
    max = MaxML;
    mostFrequent = FSE_countFast(count, seqStorePtr->matchLengthStart, nbSeq, &max);
    if (mostFrequent == nbSeq)
    {
        *op++ = *seqStorePtr->matchLengthStart;
        FSE_buildCTable_rle(CTable_MatchLength, (BYTE)max);
        MLtype = bt_rle;
    }
    else if ((nbSeq < 64) || (mostFrequent < (nbSeq >> (MLbits-1))))
    {
        FSE_buildCTable_raw(CTable_MatchLength, MLbits);
        MLtype = bt_raw;
    }
    else
    {
        tableLog = FSE_optimalTableLog(MLFSELog, nbSeq, max);
        FSE_normalizeCount(norm, tableLog, count, nbSeq, max);
        op += FSE_writeHeader(op, maxDstSize, norm, max, tableLog);
        FSE_buildCTable(CTable_MatchLength, norm, max, tableLog);
        MLtype = bt_compressed;
    }

    seqHead[0] += (BYTE)((LLtype<<6) + (Offtype<<4) + (MLtype<<2));

    /* Encoding */
    {
        FSE_CState_t stateMatchLength;
        FSE_CState_t stateOffsetBits;
        FSE_CState_t stateLitLength;

        FSE_initCStream(&blockStream, op);
        FSE_initCState(&stateMatchLength, CTable_MatchLength);
        FSE_initCState(&stateOffsetBits, CTable_OffsetBits);
        FSE_initCState(&stateLitLength, CTable_LitLength);

        while (op_litLength > op_litLength_start)
        {
            BYTE matchLength = *(--op_matchLength);
            U32  offset = *(--op_offset);
            BYTE offCode = *(--offsetBitsPtr);                              /* 32b*/  /* 64b*/
            U32 nbBits = (offCode-1) * (!!offCode);
            BYTE litLength = *(--op_litLength);                             /* (7)*/  /* (7)*/
            FSE_encodeByte(&blockStream, &stateMatchLength, matchLength);   /* 17 */  /* 17 */
            if (ZSTD_32bits()) FSE_flushBits(&blockStream);                 /*  7 */
            FSE_addBits(&blockStream, offset, nbBits);                      /* 32 */  /* 42 */
            if (ZSTD_32bits()) FSE_flushBits(&blockStream);                 /*  7 */
            FSE_encodeByte(&blockStream, &stateOffsetBits, offCode);        /* 16 */  /* 51 */
            FSE_encodeByte(&blockStream, &stateLitLength, litLength);       /* 26 */  /* 61 */
            FSE_flushBits(&blockStream);                                    /*  7 */  /*  7 */
        }

        FSE_flushCState(&blockStream, &stateMatchLength);
        FSE_flushCState(&blockStream, &stateOffsetBits);
        FSE_flushCState(&blockStream, &stateLitLength);
    }

    op += FSE_closeCStream(&blockStream);

    /* check compressibility */
    if ((size_t)(op-dst) >= maxCSize) return 0;

    return op - dst;
}


static void ZSTD_storeSeq(seqStore_t* seqStorePtr, size_t litLength, const BYTE* literals, size_t offset, size_t matchLength)
{
    BYTE* op_lit = seqStorePtr->lit;
    BYTE* const l_end = op_lit + litLength;

    /* copy Literals */
    while (op_lit<l_end) COPY8(op_lit, literals);
    seqStorePtr->lit += litLength;

    /* literal Length */
    if (litLength >= MaxLL)
    {
        *(seqStorePtr->litLength++) = MaxLL;
        if (litLength<255 + MaxLL)
            *(seqStorePtr->dumps++) = (BYTE)(litLength - MaxLL);
        else
        {
            *(seqStorePtr->dumps++) = 255;
            ZSTD_writeLE32(seqStorePtr->dumps, (U32)litLength); seqStorePtr->dumps += 3;
        }
    }
    else *(seqStorePtr->litLength++) = (BYTE)litLength;

    /* match offset */
    *(seqStorePtr->offset++) = (U32)offset;

    /* match Length */
    if (matchLength >= MaxML)
    {
        *(seqStorePtr->matchLength++) = MaxML;
        if (matchLength < 255+MaxML)
            *(seqStorePtr->dumps++) = (BYTE)(matchLength - MaxML);
        else
        {
            *(seqStorePtr->dumps++) = 255;
            ZSTD_writeLE32(seqStorePtr->dumps, (U32)matchLength); seqStorePtr->dumps+=3;
        }
    }
    else *(seqStorePtr->matchLength++) = (BYTE)matchLength;
}


//static const U32 hashMask = (1<<HASH_LOG)-1;
//static const U64 prime5bytes =         889523592379ULL;
//static const U64 prime6bytes =      227718039650203ULL;
static const U64 prime7bytes =    58295818150454627ULL;
//static const U64 prime8bytes = 14923729446516375013ULL;

//static U32   ZSTD_hashPtr(const void* p) { return (U32) _bextr_u64(*(U64*)p * prime7bytes, (56-HASH_LOG), HASH_LOG); }
//static U32   ZSTD_hashPtr(const void* p) { return ( (*(U64*)p * prime7bytes) << 8 >> (64-HASH_LOG)); }
//static U32   ZSTD_hashPtr(const void* p) { return ( (*(U64*)p * prime7bytes) >> (56-HASH_LOG)) & ((1<<HASH_LOG)-1); }
//static U32   ZSTD_hashPtr(const void* p) { return ( ((*(U64*)p & 0xFFFFFFFFFFFFFF) * prime7bytes) >> (64-HASH_LOG)); }

//static U32   ZSTD_hashPtr(const void* p) { return ( (*(U64*)p * prime8bytes) >> (64-HASH_LOG)); }
static U32   ZSTD_hashPtr(const void* p) { return ( (*(U64*)p * prime7bytes) >> (56-HASH_LOG)) & HASH_MASK; }
//static U32   ZSTD_hashPtr(const void* p) { return ( (*(U64*)p * prime6bytes) >> (48-HASH_LOG)) & HASH_MASK; }
//static U32   ZSTD_hashPtr(const void* p) { return ( (*(U64*)p * prime5bytes) >> (40-HASH_LOG)) & HASH_MASK; }
//static U32   ZSTD_hashPtr(const void* p) { return ( (*(U32*)p * KNUTH) >> (32-HASH_LOG)); }

static void  ZSTD_addPtr(U32* table, const BYTE* p, const BYTE* start) { table[ZSTD_hashPtr(p)] = (U32)(p-start); }

static const BYTE* ZSTD_updateMatch(U32* table, const BYTE* p, const BYTE* start)
{
    U32 h = ZSTD_hashPtr(p);
    const BYTE* r;
    r = table[h] + start;
    //table[h] = (U32)(p - start);
    ZSTD_addPtr(table, p, start);
    return r;
}

static int ZSTD_checkMatch(const BYTE* match, const BYTE* ip)
{
    return ZSTD_read32(match) == ZSTD_read32(ip);
}


static size_t ZSTD_compressBlock(void* cctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    cctxi_t* ctx = (cctxi_t*) cctx;
    U32*  HashTable = (U32*)(ctx->hashTable);
    seqStore_t* seqStorePtr = &(ctx->seqStore);
    const BYTE* const base = ctx->base;

    const BYTE* const istart = (const BYTE*)src;
    const BYTE* ip = istart + 1;
    const BYTE* anchor = istart;
    const BYTE* const iend = istart + srcSize;
    const BYTE* const ilimit = iend - 16;

    size_t prevOffset=0, offset=0;
    size_t lastLLSize;


    /* init */
    ZSTD_resetSeqStore(seqStorePtr);

    /* Main Search Loop */
    while (ip < ilimit)
    {
        const BYTE* match = (BYTE*) ZSTD_updateMatch(HashTable, ip, base);

        if (!ZSTD_checkMatch(match,ip)) { ip += ((ip-anchor) >> g_searchStrength) + 1; continue; }

        /* catch up */
        while ((ip>anchor) && (match>base) && (ip[-1] == match[-1])) { ip--; match--; }

        {
            size_t litLength = ip-anchor;
            size_t matchLength = ZSTD_count(ip+MINMATCH, match+MINMATCH, iend);
            size_t offsetCode;
            if (litLength) prevOffset = offset;
            offsetCode = ip-match;
            if (offsetCode == prevOffset) offsetCode = 0;
            prevOffset = offset;
            offset = ip-match;
            ZSTD_storeSeq(seqStorePtr, litLength, anchor, offsetCode, matchLength);

            /* Fill Table */
            ZSTD_addPtr(HashTable, ip+1, base);
            ip += matchLength + MINMATCH;
            if (ip<=iend-8) ZSTD_addPtr(HashTable, ip-2, base);
            anchor = ip;
        }
    }

    /* Last Literals */
    lastLLSize = iend - anchor;
    memcpy(seqStorePtr->lit, anchor, lastLLSize);
    seqStorePtr->lit += lastLLSize;

    /* Finale compression stage */
    return ZSTD_compressSequences((BYTE*)dst, maxDstSize,
                                  seqStorePtr, lastLLSize, srcSize);
}


size_t ZSTD_compressBegin(ZSTD_cctx_t ctx, void* dst, size_t maxDstSize)
{
    /* Sanity check */
    if (maxDstSize < ZSTD_frameHeaderSize) return (size_t)-ZSTD_ERROR_maxDstSize_tooSmall;

    /* Init */
    ZSTD_resetCCtx(ctx);

    /* Write Header */
    ZSTD_writeBE32(dst, ZSTD_magicNumber);

    return ZSTD_frameHeaderSize;
}


/* this should be auto-vectorized by compiler */
static void ZSTD_scaleDownCtx(void* cctx, const U32 limit)
{
    cctxi_t* ctx = (cctxi_t*) cctx;
    int i;

#if defined(__AVX2__)   /* <immintrin.h> */
    /* AVX2 version */
    __m256i* h = ctx->hashTable;
    const __m256i limit8 = _mm256_set1_epi32(limit);
    for (i=0; i<(HASH_TABLESIZE>>3); i++)
    {
        __m256i src =_mm256_loadu_si256((const __m256i*)(h+i));
  const __m256i dec = _mm256_min_epu32(src, limit8);
                src = _mm256_sub_epi32(src, dec);
        _mm256_storeu_si256((__m256i*)(h+i), src);
    }
#else
    U32* h = ctx->hashTable;
    for (i=0; i<HASH_TABLESIZE; ++i)
    {
        U32 dec;
        if (h[i] > limit) dec = limit; else dec = h[i];
        h[i] -= dec;
    }
#endif
}


/* this should be auto-vectorized by compiler */
static void ZSTD_limitCtx(void* cctx, const U32 limit)
{
    cctxi_t* ctx = (cctxi_t*) cctx;
    int i;

    if (limit > g_maxLimit)
    {
        ZSTD_scaleDownCtx(cctx, limit);
        ctx->base += limit;
        ctx->current -= limit;
        ctx->nextUpdate -= limit;
        return;
    }

#if defined(__AVX2__)   /* <immintrin.h> */
    /* AVX2 version */
    {
        __m256i* h = ctx->hashTable;
        const __m256i limit8 = _mm256_set1_epi32(limit);
        //printf("Address h : %0X\n", (U32)h);    // address test
        for (i=0; i<(HASH_TABLESIZE>>3); i++)
        {
            __m256i src =_mm256_loadu_si256((const __m256i*)(h+i));   // Unfortunately, clang doesn't guarantee 32-bytes alignment
                    src = _mm256_max_epu32(src, limit8);
            _mm256_storeu_si256((__m256i*)(h+i), src);
        }
    }
#else
    {
        U32* h = (U32*)(ctx->hashTable);
        for (i=0; i<HASH_TABLESIZE; ++i)
        {
            if (h[i] < limit) h[i] = limit;
        }
    }
#endif
}


size_t ZSTD_compressContinue(ZSTD_cctx_t cctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    cctxi_t* ctx = (cctxi_t*) cctx;
    const BYTE* const istart = (const BYTE* const)src;
    const BYTE* ip = istart;
    BYTE* const ostart = (BYTE* const)dst;
    BYTE* op = ostart;
    const U32 updateRate = 2 * BLOCKSIZE;

    /*  Init */
    if (ctx->base==NULL)
        ctx->base = (const BYTE*)src, ctx->current=0, ctx->nextUpdate = g_maxDistance;
    if (src != ctx->base + ctx->current)   /* not contiguous */
    {
            ZSTD_resetCCtx(ctx);
            ctx->base = (const BYTE*)src;
            ctx->current = 0;
    }
    ctx->current += (U32)srcSize;

    while (srcSize)
    {
        size_t cSize;
        size_t blockSize = BLOCKSIZE;
        if (blockSize > srcSize) blockSize = srcSize;

        /* update hash table */
        if (g_maxDistance <= BLOCKSIZE)   /* static test => all blocks are independent */
        {
            ZSTD_resetCCtx(ctx);
            ctx->base = ip;
            ctx->current=0;
        }
        else if (ip >= ctx->base + ctx->nextUpdate)
        {
            ctx->nextUpdate += updateRate;
            ZSTD_limitCtx(ctx, ctx->nextUpdate - g_maxDistance);
        }

        /* compress */
        if (maxDstSize < ZSTD_blockHeaderSize) return (size_t)-ZSTD_ERROR_maxDstSize_tooSmall;
        cSize = ZSTD_compressBlock(ctx, op+ZSTD_blockHeaderSize, maxDstSize-ZSTD_blockHeaderSize, ip, blockSize);
        if (cSize == 0)
        {
            cSize = ZSTD_noCompressBlock(op, maxDstSize, ip, blockSize);   /* block is not compressible */
            if (ZSTD_isError(cSize)) return cSize;
        }
        else
        {
            if (ZSTD_isError(cSize)) return cSize;
            op[0] = (BYTE)(cSize>>16);
            op[1] = (BYTE)(cSize>>8);
            op[2] = (BYTE)cSize;
            op[0] += (BYTE)(bt_compressed << 6); /* is a compressed block */
            cSize += 3;
        }
        op += cSize;
        maxDstSize -= cSize;
        ip += blockSize;
        srcSize -= blockSize;
    }

    return op-ostart;
}


size_t ZSTD_compressEnd(ZSTD_cctx_t ctx, void* dst, size_t maxDstSize)
{
    BYTE* op = (BYTE*)dst;

    /* Sanity check */
    (void)ctx;
    if (maxDstSize < ZSTD_blockHeaderSize) return (size_t)-ZSTD_ERROR_maxDstSize_tooSmall;

    /* End of frame */
    op[0] = (BYTE)(bt_end << 6);
    op[1] = 0;
    op[2] = 0;

    return 3;
}


static size_t ZSTD_compressCCtx(void* ctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    BYTE* const ostart = (BYTE* const)dst;
    BYTE* op = ostart;

    /* Header */
    {
        size_t headerSize = ZSTD_compressBegin(ctx, dst, maxDstSize);
        if(ZSTD_isError(headerSize)) return headerSize;
        op += headerSize;
        maxDstSize -= headerSize;
    }

    /* Compression */
    {
        size_t cSize = ZSTD_compressContinue(ctx, op, maxDstSize, src, srcSize);
        if (ZSTD_isError(cSize)) return cSize;
        op += cSize;
        maxDstSize -= cSize;
    }

    /* Close frame */
    {
        size_t endSize = ZSTD_compressEnd(ctx, op, maxDstSize);
        if(ZSTD_isError(endSize)) return endSize;
        op += endSize;
    }

    return (op - ostart);
}


size_t ZSTD_compress(void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    void* ctx;
    size_t r;

    ctx = ZSTD_createCCtx();
    r = ZSTD_compressCCtx(ctx, dst, maxDstSize, src, srcSize);
    ZSTD_freeCCtx(ctx);
    return r;
}


/**************************************************************
*   Decompression code
**************************************************************/

size_t ZSTD_getcBlockSize(const void* src, size_t srcSize, blockProperties_t* bpPtr)
{
    const BYTE* const in = (const BYTE* const)src;
    BYTE headerFlags;
    U32 cSize;

    if (srcSize < 3) return (size_t)-ZSTD_ERROR_wrongSrcSize;

    headerFlags = *in;
    cSize = in[2] + (in[1]<<8) + ((in[0] & 7)<<16);

    bpPtr->blockType = (blockType_t)(headerFlags >> 6);
    bpPtr->origSize = (bpPtr->blockType == bt_rle) ? cSize : 0;

    if (bpPtr->blockType == bt_end) return 0;
    if (bpPtr->blockType == bt_rle) return 1;
    return cSize;
}


static size_t ZSTD_copyUncompressedBlock(void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    if (srcSize > maxDstSize) return (size_t)-ZSTD_ERROR_maxDstSize_tooSmall;
    memcpy(dst, src, srcSize);
    return srcSize;
}


/* force inline : 'fast' really needs to be evaluated at compile time */
FORCE_INLINE size_t ZSTD_decompressLiterals_usingDTable_generic(
                       void* const dst, size_t maxDstSize,
                 const void* src, size_t srcSize,
                 const void* DTable, U32 fast)
{
    BYTE* op = (BYTE*) dst;
    BYTE* const olimit = op;
    BYTE* const oend = op + maxDstSize;
    FSE_DStream_t bitD;
    FSE_DState_t state1, state2;
    size_t errorCode;

    /* Init */
    errorCode = FSE_initDStream(&bitD, src, srcSize);
    if (FSE_isError(errorCode)) return (size_t)-ZSTD_ERROR_GENERIC;

    FSE_initDState(&state1, &bitD, DTable);
    FSE_initDState(&state2, &bitD, DTable);
    op = oend;

    /* 2-4 symbols per loop */
    while (!FSE_reloadDStream(&bitD) && (op>olimit+3))
    {
        *--op = fast ? FSE_decodeSymbolFast(&state1, &bitD) : FSE_decodeSymbol(&state1, &bitD);

        if (LitFSELog*2+7 > sizeof(size_t)*8)    /* This test must be static */
            FSE_reloadDStream(&bitD);

        *--op = fast ? FSE_decodeSymbolFast(&state2, &bitD) : FSE_decodeSymbol(&state2, &bitD);

        if (LitFSELog*4+7 < sizeof(size_t)*8)    /* This test must be static */
        {
            *--op = fast ? FSE_decodeSymbolFast(&state1, &bitD) : FSE_decodeSymbol(&state1, &bitD);
            *--op = fast ? FSE_decodeSymbolFast(&state2, &bitD) : FSE_decodeSymbol(&state2, &bitD);
        }
    }

    /* tail */
    while (1)
    {
        if ( (FSE_reloadDStream(&bitD)>2) || (op==olimit) || (FSE_endOfDState(&state1) && FSE_endOfDStream(&bitD)) )
            break;

        *--op = fast ? FSE_decodeSymbolFast(&state1, &bitD) : FSE_decodeSymbol(&state1, &bitD);

        if ( (FSE_reloadDStream(&bitD)>2) || (op==olimit) || (FSE_endOfDState(&state2) && FSE_endOfDStream(&bitD)) )
            break;

        *--op = fast ? FSE_decodeSymbolFast(&state2, &bitD) : FSE_decodeSymbol(&state2, &bitD);
    }

    /* end ? */
    if (FSE_endOfDStream(&bitD) && FSE_endOfDState(&state1) && FSE_endOfDState(&state2) )
        return oend-op;

    if (op==olimit) return (size_t)-ZSTD_ERROR_maxDstSize_tooSmall;   /* dst buffer is full, but cSrc unfinished */

    return (size_t)-ZSTD_ERROR_GENERIC;
}

static size_t ZSTD_decompressLiterals_usingDTable(
                       void* const dst, size_t maxDstSize,
                 const void* src, size_t srcSize,
                 const void* DTable, U32 fast)
{
    if (fast) return ZSTD_decompressLiterals_usingDTable_generic(dst, maxDstSize, src, srcSize, DTable, 1);
    return ZSTD_decompressLiterals_usingDTable_generic(dst, maxDstSize, src, srcSize, DTable, 0);
}

static size_t ZSTD_decompressLiterals(void* ctx, void* dst, size_t maxDstSize,
                                const void* src, size_t srcSize)
{
    /* assumed : blockType == blockCompressed */
    const BYTE* ip = (const BYTE*)src;
    short norm[256];
    void* DTable = ctx;
    U32 maxSymbolValue = 255;
    U32 tableLog;
    U32 fastMode;
    size_t errorCode;

    if (srcSize < 2) return (size_t)-ZSTD_ERROR_wrongLBlockSize;   /* too small input size */

    errorCode = FSE_readHeader (norm, &maxSymbolValue, &tableLog, ip, srcSize);
    if (FSE_isError(errorCode)) return (size_t)-ZSTD_ERROR_GENERIC;
    ip += errorCode;
    srcSize -= errorCode;

    errorCode = FSE_buildDTable (DTable, norm, maxSymbolValue, tableLog);
    if (FSE_isError(errorCode)) return (size_t)-ZSTD_ERROR_GENERIC;
    fastMode = (U32)errorCode;

    return ZSTD_decompressLiterals_usingDTable (dst, maxDstSize, ip, srcSize, DTable, fastMode);
}


size_t ZSTD_decodeLiteralsBlock(void* ctx,
                                void* dst, size_t maxDstSize,
                          const BYTE** litPtr,
                          const void* src, size_t srcSize)
{
    const BYTE* const istart = (const BYTE* const)src;
    const BYTE* ip = istart;
    BYTE* const ostart = (BYTE* const)dst;
    BYTE* const oend = ostart + maxDstSize;
    blockProperties_t litbp;

    size_t litcSize = ZSTD_getcBlockSize(src, srcSize, &litbp);
    if (ZSTD_isError(litcSize)) return litcSize;
    if (litcSize > srcSize - ZSTD_blockHeaderSize) return (size_t)-ZSTD_ERROR_wrongLBlockSize;
    ip += ZSTD_blockHeaderSize;

    switch(litbp.blockType)
    {
    case bt_raw: *litPtr = ip; ip+= litcSize; break;
    case bt_rle:
        {
            size_t rleSize = litbp.origSize;
            memset(oend - rleSize, *ip, rleSize);
            *litPtr = oend - rleSize;
            ip++;
            break;
        }
    case bt_compressed:
        {
            size_t cSize = ZSTD_decompressLiterals(ctx, dst, maxDstSize, ip, litcSize);
            if (ZSTD_isError(cSize)) return cSize;
            *litPtr = oend - cSize;
            ip += litcSize;
            break;
        }
    default:
        return (size_t)-ZSTD_ERROR_GENERIC;
    }

    return ip-istart;
}


size_t ZSTD_decodeSeqHeaders(size_t* lastLLPtr, const BYTE** dumpsPtr,
                               void* DTableLL, void* DTableML, void* DTableOffb,
                         const void* src, size_t srcSize)
{
    const BYTE* const istart = (const BYTE* const)src;
    const BYTE* ip = istart;
    const BYTE* const iend = istart + srcSize;
    U32 LLtype, Offtype, MLtype;
    U32 LLlog, Offlog, MLlog;
    size_t dumpsLength;

    /* SeqHead */
    ip += ZSTD_readProgressive(lastLLPtr, ip);
    LLtype  = *ip >> 6;
    Offtype = (*ip >> 4) & 3;
    MLtype  = (*ip >> 2) & 3;
    if (*ip & 2)
    {
        dumpsLength  = ip[2];
        dumpsLength += ip[1] << 8;
        ip += 3;
    }
    else
    {
        dumpsLength  = ip[1];
        dumpsLength += (ip[0] & 1) << 8;
        ip += 2;
    }
    *dumpsPtr = ip;
    ip += dumpsLength;

    /* sequences */
    {
        S16 norm[MaxML+1];    /* assumption : MaxML >= MaxLL and MaxOff */
        size_t headerSize;

        /* Build DTables */
        switch(LLtype)
        {
        U32 max;
        case bt_rle :
            LLlog = 0;
            FSE_buildDTable_rle(DTableLL, *ip++); break;
        case bt_raw :
            LLlog = LLbits;
            FSE_buildDTable_raw(DTableLL, LLbits); break;
        default :
            max = MaxLL;
            headerSize = FSE_readHeader(norm, &max, &LLlog, ip, iend-ip);
            if (FSE_isError(headerSize)) return (size_t)-ZSTD_ERROR_GENERIC;
            ip += headerSize;
            FSE_buildDTable(DTableLL, norm, max, LLlog);
        }

        switch(Offtype)
        {
        U32 max;
        case bt_rle :
            Offlog = 0;
            FSE_buildDTable_rle(DTableOffb, *ip++); break;
        case bt_raw :
            Offlog = Offbits;
            FSE_buildDTable_raw(DTableOffb, Offbits); break;
        default :
            max = MaxOff;
            headerSize = FSE_readHeader(norm, &max, &Offlog, ip, iend-ip);
            if (FSE_isError(headerSize)) return (size_t)-ZSTD_ERROR_GENERIC;
            ip += headerSize;
            FSE_buildDTable(DTableOffb, norm, max, Offlog);
        }

        switch(MLtype)
        {
        U32 max;
        case bt_rle :
            MLlog = 0;
            FSE_buildDTable_rle(DTableML, *ip++); break;
        case bt_raw :
            MLlog = MLbits;
            FSE_buildDTable_raw(DTableML, MLbits); break;
        default :
            max = MaxML;
            headerSize = FSE_readHeader(norm, &max, &MLlog, ip, iend-ip);
            if (FSE_isError(headerSize)) return (size_t)-ZSTD_ERROR_GENERIC;
            ip += headerSize;
            FSE_buildDTable(DTableML, norm, max, MLlog);
        }
    }

    return ip-istart;
}


#define ZSTD_prefetch(p) { const BYTE pByte = *(volatile const BYTE*)p; }

FORCE_INLINE size_t ZSTD_decompressBlock(void* ctx, void* dst, size_t maxDstSize,
                             const void* src, size_t srcSize)
{
    const BYTE* ip = (const BYTE*)src;
    const BYTE* const iend = ip + srcSize;
    BYTE* const ostart = (BYTE* const)dst;
    BYTE* op = ostart;
    BYTE* const oend = ostart + maxDstSize;
    size_t errorCode;
    size_t lastLLSize;
    const BYTE* dumps;
    const BYTE* litPtr;
    const BYTE* litEnd;
    const size_t dec32table[] = {4, 1, 2, 1, 4, 4, 4, 4};   /* added */
    const size_t dec64table[] = {8, 8, 8, 7, 8, 9,10,11};   /* substracted */
    void* DTableML = ctx;
    void* DTableLL = ((U32*)ctx) + FSE_DTABLE_SIZE_U32(MLFSELog);
    void* DTableOffb = ((U32*)DTableLL) + FSE_DTABLE_SIZE_U32(LLFSELog);

    /* blockType == blockCompressed, srcSize is trusted */

    /* literal sub-block */
    errorCode = ZSTD_decodeLiteralsBlock(ctx, dst, maxDstSize, &litPtr, src, srcSize);
    if (ZSTD_isError(errorCode)) return errorCode;
    ip += errorCode;

    /* Build Decoding Tables */
    errorCode = ZSTD_decodeSeqHeaders(&lastLLSize, &dumps,
                                      DTableLL, DTableML, DTableOffb,
                                      ip, iend-ip);
    if (ZSTD_isError(errorCode)) return errorCode;
    /* end pos */
    if ((litPtr>=ostart) && (litPtr<=oend))
        litEnd = oend - lastLLSize;
    else
        litEnd = ip - lastLLSize;
    ip += errorCode;

    /* decompression */
    {
        FSE_DStream_t DStream;
        FSE_DState_t stateLL, stateOffb, stateML;
        size_t prevOffset = 0, offset = 0;
        size_t qutt=0;

        FSE_initDStream(&DStream, ip, iend-ip);
        FSE_initDState(&stateLL, &DStream, DTableLL);
        FSE_initDState(&stateOffb, &DStream, DTableOffb);
        FSE_initDState(&stateML, &DStream, DTableML);

        while (FSE_reloadDStream(&DStream)<2)
        {
            U32 nbBits, offsetCode;
            const BYTE* match;
            size_t litLength;
            size_t matchLength;
            size_t newOffset;

_another_round:

            /* Literals */
            litLength = FSE_decodeSymbol(&stateLL, &DStream);
            if (litLength) prevOffset = offset;
            if (litLength == MaxLL)
            {
                BYTE add = *dumps++;
                if (add < 255) litLength += add;
                else
                {
                    //litLength = (*(U32*)dumps) & 0xFFFFFF;
                    litLength = ZSTD_readLE32(dumps) & 0xFFFFFF;
                    dumps += 3;
                }
            }
            if (((size_t)(litPtr - op) < 8) || ((size_t)(oend-(litPtr+litLength)) < 8))
                memmove(op, litPtr, litLength);   /* overwrite risk */
            else
                ZSTD_wildcopy(op, litPtr, litLength);
            op += litLength;
            litPtr += litLength;

            /* Offset */
            offsetCode = FSE_decodeSymbol(&stateOffb, &DStream);
            if (ZSTD_32bits()) FSE_reloadDStream(&DStream);
            nbBits = offsetCode - 1;
            if (offsetCode==0) nbBits = 0;   /* cmove */
            newOffset = FSE_readBits(&DStream, nbBits);
            if (ZSTD_32bits()) FSE_reloadDStream(&DStream);
            newOffset += (size_t)1 << nbBits;
            if (offsetCode==0) newOffset = prevOffset;
            match = op - newOffset;
            prevOffset = offset;
            offset = newOffset;

            /* MatchLength */
            matchLength = FSE_decodeSymbol(&stateML, &DStream);
            if (matchLength == MaxML)
            {
                BYTE add = *dumps++;
                if (add < 255) matchLength += add;
                else
                {
                    matchLength = ZSTD_readLE32(dumps) & 0xFFFFFF;
                    dumps += 3;
                }
            }
            matchLength += MINMATCH;

            /* copy Match */
            {
                BYTE* const endMatch = op + matchLength;
                U64 saved[2];

                if ((size_t)(litPtr - endMatch) < 12)
                {
                    qutt = endMatch + 12 - litPtr;
                    if ((litPtr + qutt) > oend) qutt = oend-litPtr;
                    memcpy(saved, litPtr, qutt);
                }

                if (offset < 8)
                {
                    const size_t dec64 = dec64table[offset];
                    op[0] = match[0];
                    op[1] = match[1];
                    op[2] = match[2];
                    op[3] = match[3];
                    match += dec32table[offset];
                    ZSTD_copy4(op+4, match);
                    match -= dec64;
                } else { ZSTD_copy8(op, match); }

                if (endMatch > oend-12)
                {
                    if (op < oend-16)
                    {
                        ZSTD_wildcopy(op+8, match+8, (oend-8) - (op+8));
                        match += (oend-8) - op;
                        op = oend-8;
                    }
                    while (op<endMatch) *op++ = *match++;
                }
                else
                    ZSTD_wildcopy(op+8, match+8, matchLength-8);   /* works even if matchLength < 8 */

                op = endMatch;

                if ((size_t)(litPtr - endMatch) < 12)
                    memcpy((void*)litPtr, saved, qutt);
            }
        }

        /* check if reached exact end */
        if (FSE_reloadDStream(&DStream) > 2) return (size_t)-ZSTD_ERROR_GENERIC;   /* requested too much : data is corrupted */
        if (!FSE_endOfDState(&stateLL) && !FSE_endOfDState(&stateML) && !FSE_endOfDState(&stateOffb)) goto _another_round;   /* some ultra-compressible sequence remain ! */
        if (litPtr != litEnd) goto _another_round;   /* literals not entirely spent */

        /* last literal segment */
        if (op != litPtr) memmove(op, litPtr, lastLLSize);
        op += lastLLSize;
    }

    return op-ostart;
}


static size_t ZSTD_decompressDCtx(void* ctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    const BYTE* ip = (const BYTE*)src;
    const BYTE* iend = ip + srcSize;
    BYTE* const ostart = (BYTE* const)dst;
    BYTE* op = ostart;
    BYTE* const oend = ostart + maxDstSize;
    size_t remainingSize = srcSize;
    U32 magicNumber;
    size_t errorCode=0;
    blockProperties_t blockProperties;

    /* Header */
    if (srcSize < ZSTD_frameHeaderSize) return (size_t)-ZSTD_ERROR_wrongSrcSize;
    magicNumber = ZSTD_readBE32(src);
    if (magicNumber != ZSTD_magicNumber) return (size_t)-ZSTD_ERROR_wrongMagicNumber;
    ip += ZSTD_frameHeaderSize; remainingSize -= ZSTD_frameHeaderSize;

    while (1)
    {
        size_t blockSize = ZSTD_getcBlockSize(ip, iend-ip, &blockProperties);
        if (ZSTD_isError(blockSize))
            return blockSize;

        ip += ZSTD_blockHeaderSize;
        remainingSize -= ZSTD_blockHeaderSize;
        if (ip+blockSize > iend)
            return (size_t)-ZSTD_ERROR_wrongSrcSize;

        switch(blockProperties.blockType)
        {
        case bt_compressed:
            errorCode = ZSTD_decompressBlock(ctx, op, oend-op, ip, blockSize);
            break;
        case bt_raw :
            errorCode = ZSTD_copyUncompressedBlock(op, oend-op, ip, blockSize);
            break;
        case bt_rle :
            return (size_t)-ZSTD_ERROR_GENERIC;   /* not yet handled */
            break;
        case bt_end :
            /* end of frame */
            if (remainingSize) return (size_t)-ZSTD_ERROR_wrongSrcSize;
            break;
        default:
            return (size_t)-ZSTD_ERROR_GENERIC;
        }
        if (blockSize == 0) break;   /* bt_end */

        if (ZSTD_isError(errorCode)) return errorCode;
        op += errorCode;
        ip += blockSize;
        remainingSize -= blockSize;
    }

    return op-ostart;
}


size_t ZSTD_decompress(void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    U32 ctx[FSE_DTABLE_SIZE_U32(LLFSELog) + FSE_DTABLE_SIZE_U32(OffFSELog) + FSE_DTABLE_SIZE_U32(MLFSELog)];
    return ZSTD_decompressDCtx(ctx, dst, maxDstSize, src, srcSize);
}


/*******************************
*  Streaming Decompression API
*******************************/

typedef struct
{
    U32 ctx[FSE_DTABLE_SIZE_U32(LLFSELog) + FSE_DTABLE_SIZE_U32(OffFSELog) + FSE_DTABLE_SIZE_U32(MLFSELog)];
    size_t expected;
    blockType_t bType;
    U32 phase;
} dctx_t;


ZSTD_dctx_t ZSTD_createDCtx(void)
{
    dctx_t* dctx = (dctx_t*)malloc(sizeof(dctx_t));
    dctx->expected = ZSTD_frameHeaderSize;
    dctx->phase = 0;
    return (ZSTD_dctx_t)dctx;
}

size_t ZSTD_freeDCtx(ZSTD_dctx_t dctx)
{
    free(dctx);
    return 0;
}


size_t ZSTD_nextSrcSizeToDecompress(ZSTD_dctx_t dctx)
{
    return ((dctx_t*)dctx)->expected;
}

size_t ZSTD_decompressContinue(ZSTD_dctx_t dctx, void* dst, size_t maxDstSize, const void* src, size_t srcSize)
{
    dctx_t* ctx = (dctx_t*)dctx;

    /* Sanity check */
    if (srcSize != ctx->expected) return (size_t)-ZSTD_ERROR_wrongSrcSize;

    /* Decompress : frame header */
    if (ctx->phase == 0)
    {
        /* Check frame magic header */
        U32 magicNumber = ZSTD_readBE32(src);
        if (magicNumber != ZSTD_magicNumber) return (size_t)-ZSTD_ERROR_wrongMagicNumber;
        ctx->phase = 1;
        ctx->expected = ZSTD_blockHeaderSize;
        return 0;
    }

    /* Decompress : block header */
    if (ctx->phase == 1)
    {
        blockProperties_t bp;
        size_t blockSize = ZSTD_getcBlockSize(src, ZSTD_blockHeaderSize, &bp);
        if (ZSTD_isError(blockSize)) return blockSize;
        if (bp.blockType == bt_end)
        {
            ctx->expected = 0;
            ctx->phase = 0;
        }
        else
        {
            ctx->expected = blockSize;
            ctx->bType = bp.blockType;
            ctx->phase = 2;
        }

        return 0;
    }

    /* Decompress : block content */
    {
        size_t rSize;
        switch(ctx->bType)
        {
        case bt_compressed:
            rSize = ZSTD_decompressBlock(ctx, dst, maxDstSize, src, srcSize);
            break;
        case bt_raw :
            rSize = ZSTD_copyUncompressedBlock(dst, maxDstSize, src, srcSize);
            break;
        case bt_rle :
            return (size_t)-ZSTD_ERROR_GENERIC;   /* not yet handled */
            break;
        case bt_end :   /* should never happen (filtered at phase 1) */
            rSize = 0;
            break;
        default:
            return (size_t)-ZSTD_ERROR_GENERIC;
        }
        ctx->phase = 1;
        ctx->expected = ZSTD_blockHeaderSize;
        return rSize;
    }

}

/* <<<<< zstd.c EOF */

typedef struct sszstdfilter sszstdfilter;

struct sszstdfilter {
	void *ctx;
} sspacked;

static int
ss_zstdfilter_init(ssfilter *f, va_list args ssunused)
{
	sszstdfilter *z = (sszstdfilter*)f->priv;
	switch (f->op) {
	case SS_FINPUT:
		z->ctx = ZSTD_createCCtx();
		if (ssunlikely(z->ctx == NULL))
			return -1;
		break;	
	case SS_FOUTPUT:
		z->ctx = NULL;
		break;	
	}
	return 0;
}

static int
ss_zstdfilter_free(ssfilter *f)
{
	sszstdfilter *z = (sszstdfilter*)f->priv;
	switch (f->op) {
	case SS_FINPUT:
		ZSTD_freeCCtx(z->ctx);
		break;	
	case SS_FOUTPUT:
		break;	
	}
	return 0;
}

static int
ss_zstdfilter_reset(ssfilter *f)
{
	sszstdfilter *z = (sszstdfilter*)f->priv;
	switch (f->op) {
	case SS_FINPUT:
		ZSTD_resetCCtx(z->ctx);
		break;	
	case SS_FOUTPUT:
		break;
	}
	return 0;
}

static int
ss_zstdfilter_start(ssfilter *f, ssbuf *dest)
{
	sszstdfilter *z = (sszstdfilter*)f->priv;
	int rc;
	size_t block;
	size_t sz;
	switch (f->op) {
	case SS_FINPUT:;
		block = ZSTD_frameHeaderSize;
		rc = ss_bufensure(dest, f->a, block);
		if (ssunlikely(rc == -1))
			return -1;
		sz = ZSTD_compressBegin(z->ctx, dest->p, block);
		if (ssunlikely(ZSTD_isError(sz)))
			return -1;
		ss_bufadvance(dest, sz);
		break;	
	case SS_FOUTPUT:
		/* do nothing */
		break;
	}
	return 0;
}

static int
ss_zstdfilter_next(ssfilter *f, ssbuf *dest, char *buf, int size)
{
	sszstdfilter *z = (sszstdfilter*)f->priv;
	int rc;
	if (ssunlikely(size == 0))
		return 0;
	switch (f->op) {
	case SS_FINPUT:;
		size_t block = ZSTD_compressBound(size);
		rc = ss_bufensure(dest, f->a, block);
		if (ssunlikely(rc == -1))
			return -1;
		size_t sz = ZSTD_compressContinue(z->ctx, dest->p, block, buf, size);
		if (ssunlikely(ZSTD_isError(sz)))
			return -1;
		ss_bufadvance(dest, sz);
		break;	
	case SS_FOUTPUT:
		/* do a single-pass decompression.
		 *
		 * Assume that destination buffer is allocated to
		 * original size.
		 */
		sz = ZSTD_decompress(dest->p, ss_bufunused(dest), buf, size);
		if (ssunlikely(ZSTD_isError(sz)))
			return -1;
		break;
	}
	return 0;
}

static int
ss_zstdfilter_complete(ssfilter *f, ssbuf *dest)
{
	sszstdfilter *z = (sszstdfilter*)f->priv;
	int rc;
	switch (f->op) {
	case SS_FINPUT:;
		size_t block = ZSTD_blockHeaderSize;
		rc = ss_bufensure(dest, f->a, block);
		if (ssunlikely(rc == -1))
			return -1;
		size_t sz = ZSTD_compressEnd(z->ctx, dest->p, block);
		if (ssunlikely(ZSTD_isError(sz)))
			return -1;
		ss_bufadvance(dest, sz);
		break;	
	case SS_FOUTPUT:
		/* do nothing */
		break;
	}
	return 0;
}

ssfilterif ss_zstdfilter =
{
	.name     = "zstd",
	.init     = ss_zstdfilter_init,
	.free     = ss_zstdfilter_free,
	.reset    = ss_zstdfilter_reset,
	.start    = ss_zstdfilter_start,
	.next     = ss_zstdfilter_next,
	.complete = ss_zstdfilter_complete
};
#line 1 "sophia/format/sf.h"
#ifndef SF_H_
#define SF_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sfref sfref;
typedef struct sfv   sfv;

struct sfref {
	uint32_t offset;
	uint16_t size;
} sspacked;

struct sfv {
	void *part;
	char *key;
	sfref r;
};

typedef enum {
	SF_SKEYVALUE,
	SF_SRAW
} sfstorage;

typedef enum {
	SF_KV,
	SF_DOCUMENT
} sf;

static inline char*
sf_key(char *data, int pos) {
	return data + ((sfref*)data)[pos].offset;
}

static inline int
sf_keysize(char *data, int pos) {
	return ((sfref*)data)[pos].size;
}

static inline int
sf_keytotal(char *data, int count)
{
	int total = 0;
	int i = 0;
	while (i < count) {
		total += sf_keysize(data, i);
		i++;
	}
	return total + sizeof(sfref) * count;
}

static inline int
sf_keycopy(char *dest, char *src, int count)
{
	sfref *ref = (sfref*)dest;
	int offset = sizeof(sfref) * count;
	int i = 0;
	while (i < count) {
		int size = sf_keysize(src, i);
		ref->offset = offset;
		ref->size = size;
		memcpy(sf_key(dest, i), sf_key(src, i), size);
		offset += size;
		ref++;
		i++;
	}
	return offset;
}

static inline char*
sf_value(sf format, char *data, int count)
{
	assert(count > 0);
	sfref *ref = ((sfref*)data) + (count - 1);
	switch (format) {
	case SF_KV:
		return data + ref->offset + ref->size;
	case SF_DOCUMENT:
		return data + (sizeof(sfref) * count);
	}
	return NULL;
}

static inline int
sf_valuesize(sf format, char *data, int size, int count)
{
	assert(count > 0);
	switch (format) {
	case SF_KV: {
		sfref *ref = ((sfref*)data) + (count - 1);
		return size - (ref->offset + ref->size);
	}
	case SF_DOCUMENT:
		return size - (sizeof(sfref) * count);
	}
	return 0;
}

static inline int
sf_size(sf format, sfv *keys, int count, int vsize)
{
	switch (format) {
	case SF_KV: {
		int sum = 0;
		int i = 0;
		while (i < count) {
			sum += keys[i].r.size;
			i++;
		}
		return sizeof(sfref) * count + sum + vsize;
	}
	case SF_DOCUMENT:
		return sizeof(sfref) * count + vsize;
	}
	assert(0);
	return 0;
}

static inline void
sf_write(sf format, char *dest, sfv *keys, int count,
         char *v, int vsize)
{
	sfref *ref = (sfref*)dest;
	int offset = sizeof(sfref) * count;
	int i = 0;
	switch (format) {
	case SF_KV:
		while (i < count) {
			sfv *ptr = &keys[i];
			ref->offset = offset;
			ref->size = ptr->r.size;
			memcpy(dest + offset, ptr->key, ptr->r.size);
			offset += ptr->r.size;
			ref++;
			i++;
		}
		break;
	case SF_DOCUMENT:
		while (i < count) {
			sfv *ptr = &keys[i];
			ref->offset = offset + (uint32_t)(ptr->key - v);
			ref->size = ptr->r.size;
			ref++;
			i++;
		}
		break;
	}
	memcpy(dest + offset, v, vsize);
}

#endif
#line 1 "sophia/format/sf_update.h"
#ifndef SF_UPDATE_H_
#define SF_UPDATE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef int (*sfupdatef)(int, char*, int, int, char*, int, void*, void**, int*);

typedef struct {
	sfupdatef function;
	void *arg;
} sfupdate;

static inline void
sf_updateinit(sfupdate *u)
{
	memset(u, 0, sizeof(*u));
}

static inline void
sf_updateset(sfupdate *u, sfupdatef function)
{
	u->function = function;
}

static inline void
sf_updateset_arg(sfupdate *u, void *arg)
{
	u->arg = arg;
}

static inline int
sf_updatehas(sfupdate *u) {
	return u->function != NULL;
}

#endif
#line 1 "sophia/format/sf.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/



#line 1 "sophia/runtime/sr_version.h"
#ifndef SR_VERSION_H_
#define SR_VERSION_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#define SR_VERSION_MAGIC  8529643324614668147ULL
#define SR_VERSION_A '1'
#define SR_VERSION_B '2'
#define SR_VERSION_C '3'

#if defined(SOPHIA_BUILD)
# define SR_VERSION_COMMIT SOPHIA_BUILD
#else
# define SR_VERSION_COMMIT "unknown"
#endif

typedef struct srversion srversion;

struct srversion {
	uint64_t magic;
	uint8_t  a, b, c;
} sspacked;

static inline void
sr_version(srversion *v)
{
	v->magic = SR_VERSION_MAGIC;
	v->a = SR_VERSION_A;
	v->b = SR_VERSION_B;
	v->c = SR_VERSION_C;
}

static inline int
sr_versioncheck(srversion *v)
{
	if (v->magic != SR_VERSION_MAGIC)
		return 0;
	if (v->a != SR_VERSION_A)
		return 0;
	if (v->b != SR_VERSION_B)
		return 0;
	if (v->c != SR_VERSION_C)
		return 0;
	return 1;
}

#endif
#line 1 "sophia/runtime/sr_error.h"
#ifndef SR_ERROR_H_
#define SR_ERROR_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct srerror srerror;

enum {
	SR_ERROR_NONE  = 0,
	SR_ERROR = 1,
	SR_ERROR_MALFUNCTION = 2
};

struct srerror {
	ssspinlock lock;
	int type;
	const char *file;
	const char *function;
	int line;
	char error[256];
};

static inline void
sr_errorinit(srerror *e) {
	e->type = SR_ERROR_NONE;
	e->error[0] = 0;
	e->line = 0;
	e->function = NULL;
	e->file = NULL;
	ss_spinlockinit(&e->lock);
}

static inline void
sr_errorfree(srerror *e) {
	ss_spinlockfree(&e->lock);
}

static inline void
sr_errorreset(srerror *e) {
	ss_spinlock(&e->lock);
	e->type = SR_ERROR_NONE;
	e->error[0] = 0;
	e->line = 0;
	e->function = NULL;
	e->file = NULL;
	ss_spinunlock(&e->lock);
}

static inline void
sr_errorrecover(srerror *e) {
	ss_spinlock(&e->lock);
	assert(e->type == SR_ERROR_MALFUNCTION);
	e->type = SR_ERROR;
	ss_spinunlock(&e->lock);
}

static inline void
sr_malfunction_set(srerror *e) {
	ss_spinlock(&e->lock);
	e->type = SR_ERROR_MALFUNCTION;
	ss_spinunlock(&e->lock);
}

static inline int
sr_errorof(srerror *e) {
	ss_spinlock(&e->lock);
	int type = e->type;
	ss_spinunlock(&e->lock);
	return type;
}

static inline int
sr_errorcopy(srerror *e, char *buf, int bufsize) {
	ss_spinlock(&e->lock);
	int len = snprintf(buf, bufsize, "%s", e->error);
	ss_spinunlock(&e->lock);
	return len;
}

static inline void
sr_verrorset(srerror *e, int type,
             const char *file,
             const char *function, int line,
             char *fmt, va_list args)
{
	ss_spinlock(&e->lock);
	if (ssunlikely(e->type == SR_ERROR_MALFUNCTION)) {
		ss_spinunlock(&e->lock);
		return;
	}
	e->file     = file;
	e->function = function;
	e->line     = line;
	e->type     = type;
	int len;
	len = snprintf(e->error, sizeof(e->error), "%s:%d ", file, line);
	vsnprintf(e->error + len, sizeof(e->error) - len, fmt, args);
	ss_spinunlock(&e->lock);
}

static inline int
sr_errorset(srerror *e, int type,
            const char *file,
            const char *function, int line,
            char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	sr_verrorset(e, type, file, function, line, fmt, args);
	va_end(args);
	return -1;
}

#define sr_e(e, type, fmt, ...) \
	sr_errorset(e, type, __FILE__, __FUNCTION__, __LINE__, fmt, __VA_ARGS__)

#define sr_error(e, fmt, ...) \
	sr_e(e, SR_ERROR, fmt, __VA_ARGS__)

#define sr_malfunction(e, fmt, ...) \
	sr_e(e, SR_ERROR_MALFUNCTION, fmt, __VA_ARGS__)

#define sr_oom(e) \
	sr_e(e, SR_ERROR, "%s", "memory allocation failed")

#define sr_oom_malfunction(e) \
	sr_e(e, SR_ERROR_MALFUNCTION, "%s", "memory allocation failed")

#endif
#line 1 "sophia/runtime/sr_seq.h"
#ifndef SR_SEQ_H_
#define SR_SEQ_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef enum {
	SR_DSN,
	SR_DSNNEXT,
	SR_NSN,
	SR_NSNNEXT,
	SR_BSN,
	SR_BSNNEXT,
	SR_LSN,
	SR_LSNNEXT,
	SR_LFSN,
	SR_LFSNNEXT,
	SR_TSN,
	SR_TSNNEXT,
	SR_RSN,
	SR_RSNNEXT
} srseqop;

typedef struct {
	ssspinlock lock;
	uint32_t dsn;
	uint32_t nsn;
	uint32_t bsn;
	uint64_t lsn;
	uint32_t lfsn;
	uint32_t tsn;
	uint64_t rsn;
} srseq;

static inline void
sr_seqinit(srseq *n) {
	memset(n, 0, sizeof(*n));
	ss_spinlockinit(&n->lock);
}

static inline void
sr_seqfree(srseq *n) {
	ss_spinlockfree(&n->lock);
}

static inline void
sr_seqlock(srseq *n) {
	ss_spinlock(&n->lock);
}

static inline void
sr_sequnlock(srseq *n) {
	ss_spinunlock(&n->lock);
}

static inline uint64_t
sr_seqdo(srseq *n, srseqop op)
{
	uint64_t v = 0;
	switch (op) {
	case SR_LSN:      v = n->lsn;
		break;
	case SR_LSNNEXT:  v = ++n->lsn;
		break;
	case SR_TSN:      v = n->tsn;
		break;
	case SR_TSNNEXT:  v = ++n->tsn;
		break;
	case SR_RSN:      v = n->rsn;
		break;
	case SR_RSNNEXT:  v = ++n->rsn;
		break;
	case SR_NSN:      v = n->nsn;
		break;
	case SR_NSNNEXT:  v = ++n->nsn;
		break;
	case SR_LFSN:     v = n->lfsn;
		break;
	case SR_LFSNNEXT: v = ++n->lfsn;
		break;
	case SR_DSN:      v = n->dsn;
		break;
	case SR_DSNNEXT:  v = ++n->dsn;
		break;
	case SR_BSN:      v = n->bsn;
		break;
	case SR_BSNNEXT:  v = ++n->bsn;
		break;
	}
	return v;
}

static inline uint64_t
sr_seq(srseq *n, srseqop op)
{
	sr_seqlock(n);
	uint64_t v = sr_seqdo(n, op);
	sr_sequnlock(n);
	return v;
}

#endif
#line 1 "sophia/runtime/sr_scheme.h"
#ifndef SR_SCHEME_H_
#define SR_SCHEME_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct srkey srkey;
typedef struct srscheme srscheme;

typedef int (*srcmpf)(char*, int, char*, int, void*);

struct srkey {
	char *name;
	char *path;
	sstype type;
	int pos;
	srcmpf cmpprefix;
	srcmpf cmpraw;
	srcmpf cmp;
};

struct srscheme {
	srkey *parts;
	int count;
	srcmpf cmp;
	srcmpf cmpprefix;
	void *cmparg;
	void *cmpprefix_arg;
};

int sr_schemecompare_prefix(char*, int, char*, int, void*);
int sr_schemecompare(char*, int, char*, int, void*);

static inline void
sr_schemeinit(srscheme *s)
{
	s->parts = NULL;
	s->count = 0;
	s->cmp = sr_schemecompare;
	s->cmparg = s;
	s->cmpprefix = sr_schemecompare_prefix;
	s->cmpprefix_arg = s;
}

static inline void
sr_schemefree(srscheme *s, ssa *a)
{
	if (s->parts == NULL)
		return;
	int i = 0;
	while (i < s->count) {
		if (s->parts[i].name)
			ss_free(a, s->parts[i].name);
		if (s->parts[i].path)
			ss_free(a, s->parts[i].path);
		i++;
	}
	ss_free(a, s->parts);
	s->parts = NULL;
}

static inline void
sr_schemesetcmp(srscheme *s, srcmpf cmp, void *arg)
{
	s->cmp = cmp;
	s->cmparg = arg;
}

static inline void
sr_schemesetcmp_prefix(srscheme *s, srcmpf cmp, void *arg)
{
	s->cmpprefix = cmp;
	s->cmpprefix_arg = arg;
}

static inline srkey*
sr_schemeadd(srscheme *s, ssa *a)
{
	srkey *parts = ss_malloc(a, sizeof(srkey) * (s->count + 1));
	if (ssunlikely(parts == NULL))
		return NULL;
	memcpy(parts, s->parts, sizeof(srkey) * s->count);
	if (s->parts)
		ss_free(a, s->parts);
	s->parts = parts;
	int pos = s->count++;
	srkey *part = &s->parts[pos];
	memset(part, 0, sizeof(*part));
	part->pos = pos;
	return part;
}

static inline int
sr_schemedelete(srscheme *s, ssa *a, int pos)
{
	srkey *parts = ss_malloc(a, sizeof(srkey) * (s->count - 1));
	if (ssunlikely(parts == NULL))
		return -1;
	int i = 0;
	int j = 0;
	while (i < s->count)
	{
		if (i == pos) {
			if (s->parts[i].name)
				ss_free(a, s->parts[i].name);
			if (s->parts[i].path)
				ss_free(a, s->parts[i].path);
			i++;
			continue;
		}
		parts[j++] = s->parts[i];
		i++;
	}
	if (s->parts)
		ss_free(a, s->parts);
	s->parts = parts;
	s->count -= 1;
	return 0;
}

static inline srkey*
sr_schemefind(srscheme *s, char *name)
{
	int i = 0;
	while (i < s->count) {
		if (strcmp(s->parts[i].name, name) == 0)
			return &s->parts[i];
		i++;
	}
	return NULL;
}

static inline srkey*
sr_schemeof(srscheme *s, int pos)
{
	assert(pos < s->count);
	return &s->parts[pos];
}

int sr_keysetname(srkey*, ssa*, char*);
int sr_keyset(srkey*, ssa*, char*);

int sr_schemesave(srscheme*, ssa*, ssbuf*);
int sr_schemeload(srscheme*, ssa*, char*, int);

static inline int
sr_compare(srscheme *s, char *a, int asize, char *b, int bsize) {
	return s->cmp(a, asize, b, bsize, s->cmparg);
}

static inline int
sr_compareprefix(srscheme *s, char *prefix, int prefixsize, char *b, int bsize)
{
	return s->cmpprefix(prefix, prefixsize, b, bsize, s->cmpprefix_arg);
}

#endif
#line 1 "sophia/runtime/sr.h"
#ifndef SR_H_
#define SR_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sr sr;

struct sr {
	srerror *e;
	sfupdate *fmt_update;
	sfstorage fmt_storage;
	sf fmt;
	srscheme *scheme;
	srseq *seq;
	ssa *a;
	ssquota *quota;
	ssinjection *i;
	void *compression;
	sscrcf crc;
};

static inline void
sr_init(sr *r,
        srerror *e,
        ssa *a,
        ssquota *quota,
        srseq *seq,
        sf fmt,
        sfstorage fmt_storage,
        sfupdate *fmt_update,
        srscheme *scheme,
        ssinjection *i,
        sscrcf crc,
        void *compression)
{
	r->e           = e;
	r->a           = a;
	r->quota       = quota;
	r->seq         = seq;
	r->scheme      = scheme;
	r->fmt         = fmt;
	r->fmt_storage = fmt_storage;
	r->fmt_update  = fmt_update;
	r->i           = i;
	r->compression = compression;
	r->crc         = crc;
}

#endif
#line 1 "sophia/runtime/sr_meta.h"
#ifndef SR_META_H_
#define SR_META_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct srmeta srmeta;
typedef struct srmetadump srmetadump;
typedef struct srmetastmt srmetastmt;

typedef int (*srmetaf)(srmeta*, srmetastmt*);

typedef enum {
	SR_WRITE,
	SR_READ,
	SR_SERIALIZE
} srmetaop;

enum {
	SR_RO = 1,
	SR_NS = 2
};

struct srmeta {
	char    *key;
	int      flags;
	sstype   type;
	srmetaf  function;
	void    *value;
	void    *ptr;
	srmeta  *next;
};

struct srmetadump {
	uint8_t  type;
	uint16_t keysize;
	uint32_t valuesize;
} sspacked;

struct srmetastmt {
	srmetaop    op;
	const char *path;
	void       *value;
	sstype      valuetype;
	int         valuesize;
	srmeta     *match;
	ssbuf      *serialize;
	void       *ptr;
	sr         *r;
};

int sr_metaexec(srmeta*, srmetastmt*);
int sr_meta_read(srmeta*, srmetastmt*);
int sr_meta_write(srmeta*, srmetastmt*);
int sr_meta_serialize(srmeta*, srmetastmt*);

static inline srmeta*
sr_m(srmeta **link, srmeta **cp, srmetaf func,
     char *key, int type,
     void *value)
{
	srmeta *c = *cp;
	c->key      = key;
	c->function = func;
	c->flags    = 0;
	c->type     = type;
	c->value    = value;
	c->ptr      = NULL;
	c->next     = NULL;
	*cp = c + 1;
	if (sslikely(link)) {
		if (sslikely(*link))
			(*link)->next = c;
		*link = c;
	}
	return c;
}

static inline srmeta*
sr_M(srmeta **link, srmeta **cp, srmetaf func,
      char *key, int type,
      void *value, int flags, void *ptr)
{
	srmeta *c = sr_m(link, cp, func, key, type, value);
	c->flags = flags;
	c->ptr = ptr;
	return c;
}

static inline char*
sr_metakey(srmetadump *v) {
	return (char*)v + sizeof(srmetadump);
}

static inline char*
sr_metavalue(srmetadump *v) {
	return sr_metakey(v) + v->keysize;
}

static inline void*
sr_metanext(srmetadump *v) {
	return sr_metavalue(v) + v->valuesize;
}

#endif
#line 1 "sophia/runtime/sr_zone.h"
#ifndef SR_ZONE_H_
#define SR_ZONE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct srzone srzone;
typedef struct srzonemap srzonemap;

struct srzone {
	uint32_t enable;
	char     name[4];
	uint32_t mode;
	uint32_t compact_wm;
	uint32_t branch_prio;
	uint32_t branch_wm;
	uint32_t branch_age;
	uint32_t branch_age_period;
	uint32_t branch_age_wm;
	uint32_t backup_prio;
	uint32_t gc_db_prio;
	uint32_t gc_prio;
	uint32_t gc_period;
	uint32_t gc_wm;
	uint32_t async;
};

struct srzonemap {
	srzone zones[11];
};

static inline int
sr_zonemap_init(srzonemap *m) {
	memset(m->zones, 0, sizeof(m->zones));
	return 0;
}

static inline void
sr_zonemap_set(srzonemap *m, uint32_t percent, srzone *z)
{
	if (ssunlikely(percent > 100))
		percent = 100;
	percent = percent - percent % 10;
	int p = percent / 10;
	m->zones[p] = *z;
	snprintf(m->zones[p].name, sizeof(m->zones[p].name), "%d", percent);
}

static inline srzone*
sr_zonemap(srzonemap *m, uint32_t percent)
{
	if (ssunlikely(percent > 100))
		percent = 100;
	percent = percent - percent % 10;
	int p = percent / 10;
	srzone *z = &m->zones[p];
	if (!z->enable) {
		while (p >= 0) {
			z = &m->zones[p];
			if (z->enable)
				return z;
			p--;
		}
		return NULL;
	}
	return z;
}

#endif
#line 1 "sophia/runtime/sr_meta.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/





int sr_meta_read(srmeta *m, srmetastmt *s)
{
	switch (m->type) {
	case SS_U32:
		s->valuesize = sizeof(uint32_t);
		if (s->valuetype == SS_I64) {
			*(int64_t*)s->value = *(uint32_t*)m->value;
		} else
		if (s->valuetype == SS_U32) {
			*(uint32_t*)s->value = *(uint32_t*)m->value;
		} else
		if (s->valuetype == SS_U64) {
			*(uint64_t*)s->value = *(uint32_t*)m->value;
		} else {
			goto bad_type;
		}
		break;
	case SS_U64:
		s->valuesize = sizeof(uint64_t);
		if (s->valuetype == SS_I64) {
			*(int64_t*)s->value = *(uint64_t*)m->value;
		} else
		if (s->valuetype == SS_U32) {
			*(uint32_t*)s->value = *(uint64_t*)m->value;
		} else
		if (s->valuetype == SS_U64) {
			*(uint64_t*)s->value = *(uint64_t*)m->value;
		} else {
			goto bad_type;
		}
		break;
	case SS_STRING: {
		if (s->valuetype != SS_STRING)
			goto bad_type;
		char **result = s->value;
		*result = NULL;
		s->valuesize = 0;
		char *string = m->value;
		if (string == NULL)
			break;
		int size = strlen(string) + 1; 
		s->valuesize = size;
		*result = malloc(size);
		if (ssunlikely(*result == NULL))
			return sr_oom(s->r->e);
		memcpy(*result, string, size);
		break;
	}
	case SS_STRINGPTR: {
		if (s->valuetype != SS_STRING)
			goto bad_type;
		char **result = s->value;
		*result = NULL;
		s->valuesize = 0;
		char **string = m->value;
		if (*string == NULL)
			break;
		int size = strlen(*string) + 1;
		s->valuesize = size;
		*result = malloc(size);
		if (ssunlikely(*result == NULL))
			return sr_oom(s->r->e);
		memcpy(*result, *string, size);
		break;
	}
	case SS_OBJECT:
		if (s->valuetype != SS_STRING)
			goto bad_type;
		*(void**)s->value = m->value;
		s->valuesize = sizeof(void*);
		break;
	default:
		assert(0);
		return -1;
	}
	return 0;

bad_type:
	return sr_error(s->r->e, "bad meta read type (%s) -> (%s) %s",
	                ss_typeof(s->valuetype),
	                ss_typeof(m->type), s->path);
	return 0;
}

int sr_meta_write(srmeta *m, srmetastmt *s)
{
	if (m->flags & SR_RO) {
		sr_error(s->r->e, "%s is read-only", s->path);
		return -1;
	}
	switch (m->type) {
	case SS_U32:
		if (s->valuetype == SS_I64) {
			*((uint32_t*)m->value) = *(int64_t*)s->value;
		} else
		if (s->valuetype == SS_U32) {
			*((uint32_t*)m->value) = *(uint32_t*)s->value;
		} else
		if (s->valuetype == SS_U64) {
			*((uint32_t*)m->value) = *(uint64_t*)s->value;
		} else {
			goto bad_type;
		}
		break;
	case SS_U64:
		if (s->valuetype == SS_I64) {
			*((uint64_t*)m->value) = *(int64_t*)s->value;
		} else
		if (s->valuetype == SS_U32) {
			*((uint64_t*)m->value) = *(uint32_t*)s->value;
		} else
		if (s->valuetype == SS_U64) {
			*((uint64_t*)m->value) = *(uint64_t*)s->value;
		} else {
			goto bad_type;
		}
		break;
	case SS_STRINGPTR: {
		char **string = m->value;
		if (s->valuetype == SS_STRING) {
			char *sz = s->value;
			if (s->valuesize > 0) {
				sz = ss_malloc(s->r->a, s->valuesize);
				if (ssunlikely(sz == NULL))
					return sr_oom(s->r->e);
				memcpy(sz, s->value, s->valuesize);
			}
			if (*string)
				ss_free(s->r->a, *string);
			*string = sz;
		} else {
			goto bad_type;
		}
		break;
	}
	default:
		assert(0);
		return -1;
	}
	return 0;

bad_type:
	return sr_error(s->r->e, "bad meta write type (%s) for (%s) %s",
	                ss_typeof(s->valuetype),
	                ss_typeof(m->type), s->path);
}

static inline int
sr_meta_write_cast(sstype a, sstype b)
{
	switch (a) {
	case SS_U32:
		if (b == SS_I64) {
		} else
		if (b == SS_U32) {
		} else
		if (b == SS_U64) {
		} else {
			return -1;
		}
		break;
	case SS_U64:
		if (b == SS_I64) {
		} else
		if (b == SS_U32) {
		} else
		if (b == SS_U64) {
		} else {
			return -1;
		}
		break;
	case SS_STRING:
	case SS_STRINGPTR:
		if (b == SS_STRING) {
		} else {
			return -1;
		}
		break;
	default:
		return -1;
	}
	return 0;
}

int sr_meta_serialize(srmeta *m, srmetastmt *s)
{
	char buf[128];
	char name_function[] = "function";
	char name_object[] = "object";
	void *value = NULL;
	srmetadump v = {
		.type = m->type
	};
	switch (m->type) {
	case SS_U32:
		v.valuesize  = snprintf(buf, sizeof(buf), "%" PRIu32, *(uint32_t*)m->value);
		v.valuesize += 1;
		value = buf;
		break;
	case SS_U64:
		v.valuesize  = snprintf(buf, sizeof(buf), "%" PRIu64, *(uint64_t*)m->value);
		v.valuesize += 1;
		value = buf;
		break;
	case SS_I64:
		v.valuesize  = snprintf(buf, sizeof(buf), "%" PRIi64, *(int64_t*)m->value);
		v.valuesize += 1;
		value = buf;
		break;
	case SS_STRING: {
		char *string = m->value;
		if (string) {
			v.valuesize = strlen(string) + 1;
			value = string;
		} else {
			v.valuesize = 0;
		}
		break;
	}
	case SS_STRINGPTR: {
		char **string = (char**)m->value;
		if (*string) {
			v.valuesize = strlen(*string) + 1;
			value = *string;
		} else {
			v.valuesize = 0;
		}
		v.type = SS_STRING;
		break;
	}
	case SS_OBJECT:
		v.type = SS_STRING;
		v.valuesize = sizeof(name_object);
		value = name_object;
		break;
	case SS_FUNCTION:
		v.type = SS_STRING;
		v.valuesize = sizeof(name_function);
		value = name_function;
		break;
	default: assert(0);
	}
	char name[128];
	v.keysize  = snprintf(name, sizeof(name), "%s", s->path);
	v.keysize += 1;
	ssbuf *p = s->serialize;
	int size = sizeof(v) + v.keysize + v.valuesize;
	int rc = ss_bufensure(p, s->r->a, size);
	if (ssunlikely(rc == -1))
		return sr_oom(s->r->e);
	memcpy(p->p, &v, sizeof(v));
	memcpy(p->p + sizeof(v), name, v.keysize);
	memcpy(p->p + sizeof(v) + v.keysize, value, v.valuesize);
	ss_bufadvance(p, size);
	return 0;
}

static inline int
sr_metaexec_serialize(srmeta *c, srmetastmt *stmt, char *root)
{
	char path[256];
	while (c) {
		if (root)
			snprintf(path, sizeof(path), "%s.%s", root, c->key);
		else
			snprintf(path, sizeof(path), "%s", c->key);
		int rc;
		if (c->flags & SR_NS) {
			rc = sr_metaexec_serialize(c->value, stmt, path);
			if (ssunlikely(rc == -1))
				return -1;
		} else {
			stmt->path = path;
			rc = c->function(c, stmt);
			if (ssunlikely(rc == -1))
				return -1;
			stmt->path = NULL;
		}
		c = c->next;
	}
	return 0;
}

int sr_metaexec(srmeta *start, srmetastmt *s)
{
	if (s->op == SR_SERIALIZE)
		return sr_metaexec_serialize(start, s, NULL);
	char path[256];
	snprintf(path, sizeof(path), "%s", s->path);
	char *ptr = NULL;
	char *token;
	token = strtok_r(path, ".", &ptr);
	if (ssunlikely(token == NULL))
		return -1;
	srmeta *c = start;
	while (c) {
		if (strcmp(token, c->key) != 0) {
			c = c->next;
			continue;
		}
		if (c->flags & SR_NS) {
			token = strtok_r(NULL, ".", &ptr);
			if (ssunlikely(token == NULL))
			{
				if (s->op == SR_WRITE && c->type != SS_UNDEF) {
					int rc = sr_meta_write_cast(c->type, s->valuetype);
					if (ssunlikely(rc == -1))
						goto bad_type;
				}
				s->match = c;
				if (c->function)
					return c->function(c, s);
				/* not supported */
				goto bad_path;
			}
			c = (srmeta*)c->value;
			continue;
		}
		s->match = c;
		token = strtok_r(NULL, ".", &ptr);
		if (ssunlikely(token != NULL))
			goto bad_path;
		return c->function(c, s);
	}

bad_path:
	return sr_error(s->r->e, "bad metadata path: %s", s->path);

bad_type:
	return sr_error(s->r->e, "incompatible type (%s) for (%s) %s",
	                ss_typeof(s->valuetype),
	                ss_typeof(c->type), s->path);
}
#line 1 "sophia/runtime/sr_scheme.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/





static inline sshot int
sr_cmpany(char *prefix, int prefixsz,
          char *key, int keysz,
          void *arg)
{
	(void)prefix;
	(void)prefixsz;
	(void)key;
	(void)keysz;
	(void)arg;
	return 0;
}

static inline sshot int
sr_compare_u32(const char *a, const char *b)
{
	uint32_t av = *(const uint32_t*)a;
	uint32_t bv = *(const uint32_t*)b;
	if (av == bv)
		return 0;
	return (av > bv) ? 1 : -1;
}

static inline sshot int
sr_cmpu32_raw(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	return sr_compare_u32(a, b);
}

static inline sshot int
sr_cmpu32(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	int part = ((srkey*)arg)->pos;
	a = sf_key(a, part);
	b = sf_key(b, part);
	return sr_compare_u32(a, b);
}

static inline sshot int
sr_cmpu32_raw_reverse(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	return -sr_compare_u32(a, b);
}

static inline sshot int
sr_cmpu32_reverse(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg ssunused)
{
	int part = ((srkey*)arg)->pos;
	a = sf_key(a, part);
	b = sf_key(b, part);
	return -sr_compare_u32(a, b);
}

static inline sshot int sr_compare_u64(const char *a, const char *b)
{
	uint64_t av = *(const uint64_t*)a;
	uint64_t bv = *(const uint64_t*)b;
	if (av == bv)
		return 0;
	return (av > bv) ? 1 : -1;
}

static inline sshot int
sr_cmpu64_raw(char *a, int asz ssunused, char *b, int bsz ssunused,
              void *arg ssunused)
{
	return sr_compare_u64(a, b);
}

static inline sshot int
sr_cmpu64(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg)
{
	int part = ((srkey*)arg)->pos;
	a = sf_key(a, part);
	b = sf_key(b, part);
	return sr_compare_u64(a, b);
}

static inline sshot int
sr_cmpu64_raw_reverse(char *a, int asz ssunused, char *b, int bsz ssunused,
              void *arg ssunused)
{
	return -sr_compare_u64(a, b);
}

static inline sshot int
sr_cmpu64_reverse(char *a, int asz ssunused, char *b, int bsz ssunused, void *arg)
{
	int part = ((srkey*)arg)->pos;
	a = sf_key(a, part);
	b = sf_key(b, part);
	return -sr_compare_u64(a, b);
}

static inline sshot int
sr_cmpstring_prefix(char *prefix, int prefixsz, char *key, int keysz,
                    void *arg ssunused)
{
	keysz = sf_keysize(key, 0);
	key   = sf_key(key, 0);
	if (keysz < prefixsz)
		return 0;
	return (memcmp(prefix, key, prefixsz) == 0) ? 1 : 0;
}

#define sr_compare_string(a, asz, b, bsz) \
do { \
	int size = (asz < bsz) ? asz : bsz; \
	int rc = memcmp(a, b, size); \
	if (ssunlikely(rc == 0)) { \
		if (sslikely(asz == bsz)) \
			return 0; \
		return (asz < bsz) ? -1 : 1; \
	} \
	return rc > 0 ? 1 : -1; \
} while (0)

static inline sshot int
sr_cmpstring_raw(char *a, int asz, char *b, int bsz, void *arg ssunused)
{
	sr_compare_string(a, asz, b, bsz);
}

static inline sshot int
sr_cmpstring(char *a, int asz, char *b, int bsz, void *arg)
{
	int part = ((srkey*)arg)->pos;
	asz = sf_keysize(a, part);
	a   = sf_key(a, part);
	bsz = sf_keysize(b, part);
	b   = sf_key(b, part);
	sr_compare_string(a, asz, b, bsz);
}

inline sshot int
sr_schemecompare(char *a, int asize, char *b, int bsize, void *arg)
{
	srscheme *s = arg;
	srkey *part = s->parts;
	srkey *last = part + s->count;
	int rc;
	while (part < last) {
		rc = part->cmp(a, asize, b, bsize, part);
		if (rc != 0)
			return rc;
		part++;
	}
	return 0;
}

inline sshot int
sr_schemecompare_prefix(char *prefix, int prefixsize, char *key, int keysize,
                     void *arg)
{
	srkey *part = &((srscheme*)arg)->parts[0];
	return part->cmpprefix(prefix, prefixsize, key, keysize, part);
}

int sr_keysetname(srkey *part, ssa *a, char *name)
{
	char *p = ss_strdup(a, name);
	if (ssunlikely(p == NULL))
		return -1;
	if (part->name)
		ss_free(a, part->name);
	part->name = p;
	return 0;
}

int sr_keyset(srkey *part, ssa *a, char *path)
{
	sstype type;
	srcmpf cmpprefix = sr_cmpany;
	srcmpf cmp;
	srcmpf cmpraw;
	if (strcmp(path, "string") == 0) {
		type = SS_STRING;
		cmp = sr_cmpstring;
		cmpraw = sr_cmpstring_raw;
		cmpprefix = sr_cmpstring_prefix;
	} else
	if (strcmp(path, "u32") == 0) {
		type = SS_U32;
		cmp = sr_cmpu32;
		cmpraw = sr_cmpu32_raw;
	} else
	if (strcmp(path, "u32_rev") == 0) {
		type = SS_U32;
		cmp = sr_cmpu32_reverse;
		cmpraw = sr_cmpu32_raw_reverse;
	} else
	if (strcmp(path, "u64") == 0) {
		type = SS_U64;
		cmp = sr_cmpu64;
		cmpraw = sr_cmpu64_raw;
	} else
	if (strcmp(path, "u64_rev") == 0) {
		type = SS_U64;
		cmp = sr_cmpu64_reverse;
		cmpraw = sr_cmpu64_raw_reverse;
	} else {
		return -1;
	}
	char *p = ss_strdup(a, path);
	if (ssunlikely(p == NULL))
		return -1;
	if (part->path)
		ss_free(a, part->path);
	part->type = type;
	part->path = p;
	part->cmpprefix = cmpprefix;
	part->cmp = cmp;
	part->cmpraw = cmpraw;
	return 0;
}

int sr_schemesave(srscheme *s, ssa *a, ssbuf *buf)
{
	/* count */
	uint32_t v = s->count;
	int rc = ss_bufadd(buf, a, &v, sizeof(uint32_t));
	if (ssunlikely(rc == -1))
		return -1;
	int i = 0;
	while (i < s->count) {
		srkey *key = &s->parts[i];
		/* name */
		v = strlen(key->name) + 1;
		rc = ss_bufensure(buf, a, sizeof(uint32_t) + v);
		if (ssunlikely(rc == -1))
			goto error;
		memcpy(buf->p, &v, sizeof(v));
		ss_bufadvance(buf, sizeof(uint32_t));
		memcpy(buf->p, key->name, v);
		ss_bufadvance(buf, v);
		/* path */
		v = strlen(key->path) + 1;
		rc = ss_bufensure(buf, a, sizeof(uint32_t) + v);
		if (ssunlikely(rc == -1))
			goto error;
		memcpy(buf->p, &v, sizeof(v));
		ss_bufadvance(buf, sizeof(uint32_t));
		memcpy(buf->p, key->path, v);
		ss_bufadvance(buf, v);
		i++;
	}
	return 0;
error:
	ss_buffree(buf, a);
	return -1;
}

int sr_schemeload(srscheme *s, ssa *a, char *buf, int size ssunused)
{
	/* count */
	char *p = buf;
	uint32_t v = *(uint32_t*)p;
	p += sizeof(uint32_t);
	int count = v;
	int i = 0;
	int rc;
	while (i < count) {
		srkey *key = sr_schemeadd(s, a);
		if (ssunlikely(key == NULL))
			goto error;
		/* name */
		v = *(uint32_t*)p;
		p += sizeof(uint32_t);
		rc = sr_keysetname(key, a, p);
		if (ssunlikely(rc == -1))
			goto error;
		p += v;
		/* path */
		v = *(uint32_t*)p;
		p += sizeof(uint32_t);
		rc = sr_keyset(key, a, p);
		if (ssunlikely(rc == -1))
			goto error;
		p += v;
		i++;
	}
	return 0;
error:
	sr_schemefree(s, a);
	return -1;
}
#line 1 "sophia/object/so.h"
#ifndef SO_H_
#define SO_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct soif soif;
typedef struct sotype sotype;
typedef struct so so;

struct soif {
	int      (*open)(so*);
	int      (*destroy)(so*);
	int      (*error)(so*);
	void    *(*object)(so*);
	void    *(*asynchronous)(so*);
	void    *(*poll)(so*);
	int      (*drop)(so*);
	int      (*setobject)(so*, const char*, void*);
	int      (*setstring)(so*, const char*, void*, int);
	int      (*setint)(so*, const char*, int64_t);
	void    *(*getobject)(so*, const char*);
	void    *(*getstring)(so*, const char*, int*);
	int64_t  (*getint)(so*, const char*);
	int      (*set)(so*, so*);
	int      (*update)(so*, so*);
	int      (*del)(so*, so*);
	void    *(*get)(so*, so*);
	void    *(*batch)(so*);
	void    *(*begin)(so*);
	int      (*prepare)(so*);
	int      (*commit)(so*);
	void    *(*cursor)(so*);
};

struct sotype {
	uint32_t magic;
	char *name;
};

struct so {
	sotype *type;
	soif *i;
	so *parent;
	so *env;
	sslist link;
};

static inline void
so_init(so *o, sotype *type, soif *i, so *parent, so *env)
{
	o->type   = type;
	o->i      = i;
	o->parent = parent;
	o->env    = env;
	ss_listinit(&o->link);
}

static inline void*
so_cast_dynamic(void *ptr, sotype *type,
          const char *file,
          const char *function, int line)
{
	int eq = ptr != NULL && ((so*)ptr)->type == type;
	if (sslikely(eq))
		return ptr;
	fprintf(stderr, "%s:%d %s(%p) expected '%s' object\n",
	        file, line, function, ptr, type->name);
	abort();
	return NULL;
}

#define so_cast(o, cast, type) \
	((cast)so_cast_dynamic(o, type, __FILE__, __FUNCTION__, __LINE__))

#define so_open(o)         (o)->i->open(o)
#define so_destroy(o)      (o)->i->destroy(o)
#define so_error(o)        (o)->i->error(o)
#define so_object(o)       (o)->i->object(o)
#define so_asynchronous(o) (o)->i->asynchronous(o)
#define so_poll(o)         (o)->i->poll(o)
#define so_drop(o)         (o)->i->drop(o)
#define so_set(o, v)       (o)->i->set(o, v)
#define so_update(o, v)    (o)->i->update(o, v)
#define so_delete(o, v)    (o)->i->del(o, v)
#define so_get(o, v)       (o)->i->get(o, v)
#define so_batch(o)        (o)->i->batch(o)
#define so_begin(o)        (o)->i->begin(o)
#define so_prepare(o)      (o)->i->prepare(o)
#define so_commit(o)       (o)->i->commit(o)
#define so_cursor(o)       (o)->i->cursor(o)

#define so_setobject(o, path, object) \
	(o)->i->setobject(o, path, object)
#define so_setstring(o, path, pointer, size) \
	(o)->i->setstring(o, path, pointer, size)
#define so_setint(o, path, v) \
	(o)->i->setint(o, path, v)
#define so_getobject(o, path) \
	(o)->i->getobject(o, path)
#define so_getstring(o, path, sizep) \
	(o)->i->getstring(o, path, sizep)
#define so_getint(o, path) \
	(o)->i->getnumber(o, path)

#endif
#line 1 "sophia/object/so_list.h"
#ifndef SO_LIST_H_
#define SO_LIST_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct solist solist;

struct solist {
	sslist list;
	int n;
};

static inline void
so_listinit(solist *i)
{
	ss_listinit(&i->list);
	i->n = 0;
}

static inline int
so_listdestroy(solist *i)
{
	int rcret = 0;
	int rc;
	sslist *p, *n;
	ss_listforeach_safe(&i->list, p, n) {
		so *o = sscast(p, so, link);
		rc = so_destroy(o);
		if (ssunlikely(rc == -1))
			rcret = -1;
	}
	i->n = 0;
	ss_listinit(&i->list);
	return rcret;
}

static inline void
so_listadd(solist *i, so *o)
{
	ss_listappend(&i->list, &o->link);
	i->n++;
}

static inline void
so_listdel(solist *i, so *o)
{
	ss_listunlink(&o->link);
	i->n--;
}

static inline so*
so_listfirst(solist *i)
{
	assert(i->n > 0);
	return sscast(i->list.next, so, link);
}

#endif
#line 1 "sophia/object/so.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/





#line 1 "sophia/version/sv.h"
#ifndef SV_H_
#define SV_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#define SVNONE   0
#define SVDELETE 1
#define SVUPDATE 2
#define SVDUP    4
#define SVABORT  8
#define SVBEGIN  16

typedef struct svif svif;
typedef struct sv sv;

struct svif {
	uint8_t   (*flags)(sv*);
	void      (*lsnset)(sv*, uint64_t);
	uint64_t  (*lsn)(sv*);
	char     *(*pointer)(sv*);
	uint32_t  (*size)(sv*);
};

struct sv {
	svif *i;
	void *v, *arg;
} sspacked;

static inline void
sv_init(sv *v, svif *i, void *vptr, void *arg) {
	v->i   = i;
	v->v   = vptr;
	v->arg = arg;
}

static inline uint8_t
sv_flags(sv *v) {
	return v->i->flags(v);
}

static inline int
sv_is(sv *v, uint8_t flags) {
	return (sv_flags(v) & flags) > 0;
}

static inline uint64_t
sv_lsn(sv *v) {
	return v->i->lsn(v);
}

static inline void
sv_lsnset(sv *v, uint64_t lsn) {
	v->i->lsnset(v, lsn);
}

static inline char*
sv_pointer(sv *v) {
	return v->i->pointer(v);
}

static inline uint32_t
sv_size(sv *v) {
	return v->i->size(v);
}

static inline char*
sv_key(sv *v, sr *r ssunused, int part) {
	return sf_key(v->i->pointer(v), part);
}

static inline int
sv_keysize(sv *v, sr *r ssunused, int part) {
	return sf_keysize(v->i->pointer(v), part);
}

static inline char*
sv_value(sv *v, sr *r) {
	return sf_value(r->fmt, sv_pointer(v), r->scheme->count);
}

static inline int
sv_valuesize(sv *v, sr *r) {
	return sf_valuesize(r->fmt, sv_pointer(v), sv_size(v), r->scheme->count);
}

#endif
#line 1 "sophia/version/sv_v.h"
#ifndef SV_V_H_
#define SV_V_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svv svv;

struct svv {
	uint64_t  lsn;
	uint32_t  size;
	uint8_t   flags;
	void     *log;
	svv      *next;
	ssrbnode  node;
} sspacked;

extern svif sv_vif;

static inline char*
sv_vpointer(svv *v) {
	return (char*)(v) + sizeof(svv);
}

static inline uint32_t
sv_vsize(svv *v) {
	return sizeof(svv) + v->size;
}

static inline svv*
sv_vbuild(sr *r, sfv *keys, int count, char *data, int size)
{
	assert(r->scheme->count == count);
	int total = sf_size(r->fmt, keys, count, size);
	svv *v = ss_malloc(r->a, sizeof(svv) + total);
	if (ssunlikely(v == NULL))
		return NULL;
	v->size  = total;
	v->lsn   = 0;
	v->flags = 0;
	v->log   = NULL;
	v->next  = NULL;
	memset(&v->node, 0, sizeof(v->node));
	char *ptr = sv_vpointer(v);
	sf_write(r->fmt, ptr, keys, count, data, size);
	return v;
}

static inline svv*
sv_vbuildraw(ssa *a, char *src, int size)
{
	svv *v = ss_malloc(a, sizeof(svv) + size);
	if (ssunlikely(v == NULL))
		return NULL;
	v->size  = size;
	v->flags = 0;
	v->lsn   = 0;
	v->next  = NULL;
	v->log   = NULL;
	memset(&v->node, 0, sizeof(v->node));
	memcpy(sv_vpointer(v), src, size);
	return v;
}

static inline svv*
sv_vdup(ssa *a, sv *src)
{
	svv *v = sv_vbuildraw(a, sv_pointer(src), sv_size(src));
	if (ssunlikely(v == NULL))
		return NULL;
	v->flags = sv_flags(src);
	v->lsn   = sv_lsn(src);
	return v;
}

static inline void
sv_vfree(ssa *a, svv *v)
{
	while (v) {
		svv *n = v->next;
		ss_free(a, v);
		v = n;
	}
}

static inline svv*
sv_visible(svv *v, uint64_t vlsn) {
	while (v && v->lsn > vlsn)
		v = v->next;
	return v;
}

#endif
#line 1 "sophia/version/sv_compare.h"
#ifndef SV_COMPARE_H_
#define SV_COMPARE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

static inline int
sv_compare(sv *a, sv *b, srscheme *s) {
	return sr_compare(s, sv_pointer(a), sv_size(a),
	                     sv_pointer(b), sv_size(b));
}

#endif
#line 1 "sophia/version/sv_updatev.h"
#ifndef SV_UPDATEV_H_
#define SV_UPDATEV_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

extern svif sv_updatevif;

#endif
#line 1 "sophia/version/sv_update.h"
#ifndef SV_UPDATE_H_
#define SV_UPDATE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svupdatenode svupdatenode;
typedef struct svupdate svupdate;

struct svupdatenode {
	uint64_t lsn;
	uint8_t flags;
	ssbuf buf;
};

#define SV_UPDATERESRV 16

struct svupdate {
	svupdatenode reserve[SV_UPDATERESRV];
	ssbuf stack;
	int max;
	int count;
	sv result;
};

static inline void
sv_updateinit(svupdate *u)
{
	const int reserve = SV_UPDATERESRV;
	int i = 0;
	while (i < reserve) {
		ss_bufinit(&u->reserve[i].buf);
		i++;
	}
	memset(&u->result, 0, sizeof(u->result));
	u->max = reserve;
	u->count = 0;
	ss_bufinit_reserve(&u->stack, u->reserve, sizeof(u->reserve));
}

static inline void
sv_updatefree(svupdate *u, sr *r)
{
	svupdatenode *n = (svupdatenode*)u->stack.s;
	int i = 0;
	while (i < u->max) {
		ss_buffree(&n[i].buf, r->a);
		i++;
	}
	ss_buffree(&u->stack, r->a);
}

static inline void
sv_updatereset(svupdate *u)
{
	svupdatenode *n = (svupdatenode*)u->stack.s;
	int i = 0;
	while (i < u->count) {
		ss_bufreset(&n[i].buf);
		i++;
	}
	u->count = 0;
	ss_bufreset(&u->stack);
	memset(&u->result, 0, sizeof(u->result));
}

static inline int
sv_updatepush_raw(svupdate *u, sr *r, char *pointer, int size,
                  uint8_t flags, uint64_t lsn)
{
	svupdatenode *n;
	int rc;
	if (sslikely(u->max > u->count)) {
		n = (svupdatenode*)u->stack.p;
		ss_bufreset(&n->buf);
	} else {
		rc = ss_bufensure(&u->stack, r->a, sizeof(svupdatenode));
		if (ssunlikely(rc == -1))
			return -1;
		n = (svupdatenode*)u->stack.p;
		ss_bufinit(&n->buf);
		u->max++;
	}
	rc = ss_bufensure(&n->buf, r->a, size);
	if (ssunlikely(rc == -1))
		return -1;
	memcpy(n->buf.p, pointer, size);
	n->flags = flags;
	n->lsn = lsn;
	ss_bufadvance(&n->buf, size);
	ss_bufadvance(&u->stack, sizeof(svupdatenode));
	u->count++;
	return 0;
}

static inline int
sv_updatepush(svupdate *u, sr *r, sv *v)
{
	return sv_updatepush_raw(u, r, sv_pointer(v),
	                         sv_size(v),
	                         sv_flags(v), sv_lsn(v));
}

static inline svupdatenode*
sv_updatepop(svupdate *u)
{
	if (u->count == 0)
		return NULL;
	int pos = u->count - 1;
	u->count--;
	u->stack.p -= sizeof(svupdatenode);
	return ss_bufat(&u->stack, sizeof(svupdatenode), pos);
}

static inline int
sv_updatedo(svupdate *u, sr *r, svupdatenode *a, svupdatenode *b)
{
	assert(b->flags & SVUPDATE);
	int       b_flagsraw = b->flags;
	int       b_flags = b->flags & SVUPDATE;
	uint64_t  b_lsn = b->lsn;
	void     *b_pointer = b->buf.s;
	int       b_size = ss_bufused(&b->buf);
	int       a_flags;
	void     *a_pointer;
	int       a_size;
	if (sslikely(a && !(a->flags & SVDELETE)))
	{
		a_flags   = a->flags & SVUPDATE;
		a_pointer = a->buf.s;
		a_size    = ss_bufused(&a->buf);
	} else {
		/* convert delete to orphan update case */
		a_flags   = 0;
		a_pointer = NULL;
		a_size    = 0;
	}
	void *c_pointer;
	int c_size;
	int rc = r->fmt_update->function(a_flags, a_pointer, a_size,
	                                 b_flags, b_pointer, b_size,
	                                 r->fmt_update->arg,
	                                 &c_pointer, &c_size);
	if (ssunlikely(rc == -1))
		return -1;
	assert(c_pointer != NULL);
	rc = sv_updatepush_raw(u, r, c_pointer, c_size,
	                       b_flagsraw & ~SVUPDATE,
	                       b_lsn);
	free(c_pointer);
	return rc;
}

static inline int
sv_update(svupdate *u, sr *r)
{
	assert(u->count >= 1 );
	svupdatenode *f = ss_bufat(&u->stack, sizeof(svupdatenode), u->count - 1);
	int rc;
	if (f->flags & SVUPDATE) {
		f = sv_updatepop(u);
		rc = sv_updatedo(u, r, NULL, f);
		if (ssunlikely(rc == -1))
			return -1;
	}
	if (u->count == 1)
		goto done;
	while (u->count > 1) {
		svupdatenode *f = sv_updatepop(u);
		svupdatenode *s = sv_updatepop(u);
		assert(f != NULL);
		assert(s != NULL);
		rc = sv_updatedo(u, r, f, s);
		if (ssunlikely(rc == -1))
			return -1;
	}
done:
	sv_init(&u->result, &sv_updatevif, u->stack.s, NULL);
	return 0;
}

#endif
#line 1 "sophia/version/sv_log.h"
#ifndef SV_LOG_H_
#define SV_LOG_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svlogindex svlogindex;
typedef struct svlogv svlogv;
typedef struct svlog svlog;

struct svlogindex {
	uint32_t id;
	uint32_t head, tail;
	uint32_t count;
	void *ptr;
} sspacked;

struct svlogv {
	sv v;
	void *vgc;
	uint32_t id;
	uint32_t next;
} sspacked;

struct svlog {
	svlogindex reserve_i[4];
	svlogv reserve_v[16];
	ssbuf index;
	ssbuf buf;
};

static inline void
sv_logvinit(svlogv *v, uint32_t id)
{
	v->id   = id;
	v->next = UINT32_MAX;
	v->vgc  = NULL;
	v->v.v  = NULL;
	v->v.i  = NULL;
}

static inline void
sv_loginit(svlog *l)
{
	ss_bufinit_reserve(&l->index, l->reserve_i, sizeof(l->reserve_i));
	ss_bufinit_reserve(&l->buf, l->reserve_v, sizeof(l->reserve_v));
}

static inline void
sv_logfree(svlog *l, ssa *a)
{
	ss_buffree(&l->buf, a);
	ss_buffree(&l->index, a);
}

static inline int
sv_logcount(svlog *l) {
	return ss_bufused(&l->buf) / sizeof(svlogv);
}

static inline svlogv*
sv_logat(svlog *l, int pos) {
	return ss_bufat(&l->buf, sizeof(svlogv), pos);
}

static inline int
sv_logadd(svlog *l, ssa *a, svlogv *v, void *ptr)
{
	uint32_t n = sv_logcount(l);
	int rc = ss_bufadd(&l->buf, a, v, sizeof(svlogv));
	if (ssunlikely(rc == -1))
		return -1;
	svlogindex *i = (svlogindex*)l->index.s;
	while ((char*)i < l->index.p) {
		if (sslikely(i->id == v->id)) {
			svlogv *tail = sv_logat(l, i->tail);
			tail->next = n;
			i->tail = n;
			i->count++;
			return 0;
		}
		i++;
	}
	rc = ss_bufensure(&l->index, a, sizeof(svlogindex));
	if (ssunlikely(rc == -1)) {
		l->buf.p -= sizeof(svlogv);
		return -1;
	}
	i = (svlogindex*)l->index.p;
	i->id    = v->id;
	i->head  = n;
	i->tail  = n;
	i->ptr   = ptr;
	i->count = 1;
	ss_bufadvance(&l->index, sizeof(svlogindex));
	return 0;
}

static inline void
sv_logreplace(svlog *l, int n, svlogv *v)
{
	ss_bufset(&l->buf, sizeof(svlogv), n, (char*)v, sizeof(svlogv));
}

#endif
#line 1 "sophia/version/sv_merge.h"
#ifndef SV_MERGE_H_
#define SV_MERGE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svmergesrc svmergesrc;
typedef struct svmerge svmerge;

struct svmergesrc {
	ssiter *i, src;
	uint8_t dup;
	void *ptr;
} sspacked;

struct svmerge {
	svmergesrc reserve[16];
	ssbuf buf;
};

static inline void
sv_mergeinit(svmerge *m)
{
	ss_bufinit_reserve(&m->buf, m->reserve, sizeof(m->reserve));
}

static inline int
sv_mergeprepare(svmerge *m, sr *r, int count)
{
	int rc = ss_bufensure(&m->buf, r->a, sizeof(svmergesrc) * count);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	return 0;
}

static inline svmergesrc*
sv_mergenextof(svmergesrc *src)
{
	return (svmergesrc*)((char*)src + sizeof(svmergesrc));
}

static inline void
sv_mergefree(svmerge *m, ssa *a)
{
	ss_buffree(&m->buf, a);
}

static inline void
sv_mergereset(svmerge *m)
{
	m->buf.p = m->buf.s;
}

static inline svmergesrc*
sv_mergeadd(svmerge *m, ssiter *i)
{
	assert(m->buf.p < m->buf.e);
	svmergesrc *s = (svmergesrc*)m->buf.p;
	s->dup = 0;
	s->i = i;
	s->ptr = NULL;
	if (i == NULL)
		s->i = &s->src;
	ss_bufadvance(&m->buf, sizeof(svmergesrc));
	return s;
}

#endif
#line 1 "sophia/version/sv_mergeiter.h"
#ifndef SV_MERGEITER_H_
#define SV_MERGEITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

/*
 * Merge serveral sorted streams into one.
 * Track duplicates.
 *
 * Merger does not recognize duplicates from
 * a single stream, assumed that they are tracked
 * by the incoming data sources.
*/

typedef struct svmergeiter svmergeiter;

struct svmergeiter {
	ssorder order;
	svmerge *merge;
	svmergesrc *src, *end;
	svmergesrc *v;
	sr *r;
} sspacked;

static inline void
sv_mergeiter_dupreset(svmergeiter *i, svmergesrc *pos)
{
	svmergesrc *v = i->src;
	while (v != pos) {
		v->dup = 0;
		v = sv_mergenextof(v);
	}
}

static inline void
sv_mergeiter_gt(svmergeiter *i)
{
	if (i->v) {
		i->v->dup = 0;
		ss_iteratornext(i->v->i);
	}
	i->v = NULL;
	svmergesrc *min, *src;
	sv *minv;
	minv = NULL;
	min  = NULL;
	src  = i->src;
	for (; src < i->end; src = sv_mergenextof(src))
	{
		sv *v = ss_iteratorof(src->i);
		if (v == NULL)
			continue;
		if (min == NULL) {
			minv = v;
			min = src;
			continue;
		}
		int rc = sv_compare(minv, v, i->r->scheme);
		switch (rc) {
		case 0:
			/*
			assert(sv_lsn(v) < sv_lsn(minv));
			*/
			src->dup = 1;
			break;
		case 1:
			sv_mergeiter_dupreset(i, src);
			minv = v;
			min = src;
			break;
		}
	}
	if (ssunlikely(min == NULL))
		return;
	i->v = min;
}

static inline void
sv_mergeiter_lt(svmergeiter *i)
{
	if (i->v) {
		i->v->dup = 0;
		ss_iteratornext(i->v->i);
	}
	i->v = NULL;
	svmergesrc *max, *src;
	sv *maxv;
	maxv = NULL;
	max  = NULL;
	src  = i->src;
	for (; src < i->end; src = sv_mergenextof(src))
	{
		sv *v = ss_iteratorof(src->i);
		if (v == NULL)
			continue;
		if (max == NULL) {
			maxv = v;
			max = src;
			continue;
		}
		int rc = sv_compare(maxv, v, i->r->scheme);
		switch (rc) {
		case  0:
			/*
			assert(sv_lsn(v) < sv_lsn(maxv));
			*/
			src->dup = 1;
			break;
		case -1:
			sv_mergeiter_dupreset(i, src);
			maxv = v;
			max = src;
			break;
		}
	}
	if (ssunlikely(max == NULL))
		return;
	i->v = max;
}

static inline void
sv_mergeiter_next(ssiter *it)
{
	svmergeiter *im = (svmergeiter*)it->priv;
	switch (im->order) {
	case SS_GT:
	case SS_GTE:
		sv_mergeiter_gt(im);
		break;
	case SS_LT:
	case SS_LTE:
		sv_mergeiter_lt(im);
		break;
	default: assert(0);
	}
}

static inline int
sv_mergeiter_open(ssiter *i, sr *r, svmerge *m, ssorder o)
{
	svmergeiter *im = (svmergeiter*)i->priv;
	im->merge = m;
	im->r     = r;
	im->order = o;
	im->src   = (svmergesrc*)(im->merge->buf.s);
	im->end   = (svmergesrc*)(im->merge->buf.p);
	im->v     = NULL;
	sv_mergeiter_next(i);
	return 0;
}

static inline void
sv_mergeiter_close(ssiter *i ssunused)
{ }

static inline int
sv_mergeiter_has(ssiter *i)
{
	svmergeiter *im = (svmergeiter*)i->priv;
	return im->v != NULL;
}

static inline void*
sv_mergeiter_of(ssiter *i)
{
	svmergeiter *im = (svmergeiter*)i->priv;
	if (ssunlikely(im->v == NULL))
		return NULL;
	return ss_iteratorof(im->v->i);
}

static inline uint32_t
sv_mergeisdup(ssiter *i)
{
	svmergeiter *im = (svmergeiter*)i->priv;
	assert(im->v != NULL);
	if (im->v->dup)
		return SVDUP;
	return 0;
}

extern ssiterif sv_mergeiter;

#endif
#line 1 "sophia/version/sv_readiter.h"
#ifndef SV_READITER_H_
#define SV_READITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svreaditer svreaditer;

struct svreaditer {
	ssiter *merge;
	uint64_t vlsn;
	int next;
	int nextdup;
	int save_delete;
	svupdate *u;
	sr *r;
	sv *v;
} sspacked;

static inline int
sv_readiter_update(svreaditer *i)
{
	sv_updatereset(i->u);
	/* update begin */
	sv *v = ss_iterof(sv_mergeiter, i->merge);
	assert(v != NULL);
	assert(sv_flags(v) & SVUPDATE);
	int rc = sv_updatepush(i->u, i->r, v);
	if (ssunlikely(rc == -1))
		return -1;
	ss_iternext(sv_mergeiter, i->merge);
	/* iterate over update statements */
	int skip = 0;
	for (; ss_iterhas(sv_mergeiter, i->merge); ss_iternext(sv_mergeiter, i->merge))
	{
		v = ss_iterof(sv_mergeiter, i->merge);
		int dup = sv_is(v, SVDUP) || sv_mergeisdup(i->merge);
		if (! dup)
			break;
		if (skip)
			continue;
		int rc = sv_updatepush(i->u, i->r, v);
		if (ssunlikely(rc == -1))
			return -1;
		if (! (sv_flags(v) & SVUPDATE))
			skip = 1;
	}
	/* update */
	rc = sv_update(i->u, i->r);
	if (ssunlikely(rc == -1))
		return -1;
	return 0;
}

static inline void
sv_readiter_next(ssiter *i)
{
	svreaditer *im = (svreaditer*)i->priv;
	if (im->next)
		ss_iternext(sv_mergeiter, im->merge);
	im->next = 0;
	im->v = NULL;
	for (; ss_iterhas(sv_mergeiter, im->merge); ss_iternext(sv_mergeiter, im->merge))
	{
		sv *v = ss_iterof(sv_mergeiter, im->merge);
		int dup = sv_is(v, SVDUP) || sv_mergeisdup(im->merge);
		if (im->nextdup) {
			if (dup)
				continue;
			else
				im->nextdup = 0;
		}
		/* skip version out of visible range */
		if (sv_lsn(v) > im->vlsn) {
			continue;
		}
		im->nextdup = 1;
		if (ssunlikely(!im->save_delete && sv_is(v, SVDELETE)))
			continue;
		if (ssunlikely(sv_is(v, SVUPDATE))) {
			int rc = sv_readiter_update(im);
			if (ssunlikely(rc == -1))
				return;
			im->v = &im->u->result;
			im->next = 0;
		} else {
			im->v = v;
			im->next = 1;
		}
		break;
	}
}

static inline int
sv_readiter_open(ssiter *i, sr *r, ssiter *iterator, svupdate *u,
                 uint64_t vlsn, int save_delete)
{
	svreaditer *im = (svreaditer*)i->priv;
	im->r     = r;
	im->u     = u;
	im->merge = iterator;
	im->vlsn  = vlsn;
	assert(im->merge->vif == &sv_mergeiter);
	im->v = NULL;
	im->next = 0;
	im->nextdup = 0;
	im->save_delete = save_delete;
	/* iteration can start from duplicate */
	sv_readiter_next(i);
	return 0;
}

static inline void
sv_readiter_close(ssiter *i ssunused)
{ }

static inline int
sv_readiter_has(ssiter *i)
{
	svreaditer *im = (svreaditer*)i->priv;
	return im->v != NULL;
}

static inline void*
sv_readiter_of(ssiter *i)
{
	svreaditer *im = (svreaditer*)i->priv;
	if (ssunlikely(im->v == NULL))
		return NULL;
	return im->v;
}

extern ssiterif sv_readiter;

#endif
#line 1 "sophia/version/sv_writeiter.h"
#ifndef SV_WRITEITER_H_
#define SV_WRITEITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svwriteiter svwriteiter;

struct svwriteiter {
	ssiter *merge;
	uint64_t limit;
	uint64_t size;
	uint32_t sizev;
	uint64_t vlsn;
	int save_delete;
	int save_update;
	int next;
	int update;
	uint64_t prevlsn;
	sv *v;
	int vdup;
	svupdate *u;
	sr *r;
} sspacked;

static inline int
sv_writeiter_update(svwriteiter *i)
{
	/* apply update only on duplicate statements which are
	 * ready to be garbage-collected */
	sv_updatereset(i->u);

	/* update begin */
	sv *v = ss_iterof(sv_mergeiter, i->merge);
	assert(v != NULL);
	assert(sv_flags(v) & SVUPDATE);
	int rc = sv_updatepush(i->u, i->r, v);
	if (ssunlikely(rc == -1))
		return -1;
	uint64_t prevlsn = sv_lsn(v);
	ss_iternext(sv_mergeiter, i->merge);

	/* iterate over update statements */
	int last_non_upd = 0;
	for (; ss_iterhas(sv_mergeiter, i->merge); ss_iternext(sv_mergeiter, i->merge))
	{
		v = ss_iterof(sv_mergeiter, i->merge);
		int dup = sv_is(v, SVDUP) || sv_mergeisdup(i->merge);
		if (! dup) {
			break;
		}
		if (prevlsn > i->vlsn)
			break;
		/* stop forming updates on a second non-update stmt,
		 * but continue to iterate stream */
		if (last_non_upd) {
			continue;
		}
		last_non_upd = ! (sv_flags(v) & SVUPDATE);
		int rc = sv_updatepush(i->u, i->r, v);
		if (ssunlikely(rc == -1))
			return -1;
		prevlsn = sv_lsn(v);
	}

	/* update */
	rc = sv_update(i->u, i->r);
	if (ssunlikely(rc == -1))
		return -1;
	return 0;
}

static inline void
sv_writeiter_next(ssiter *i)
{
	svwriteiter *im = (svwriteiter*)i->priv;
	if (im->next)
		ss_iternext(sv_mergeiter, im->merge);
	im->next = 0;
	im->v = NULL;
	im->vdup = 0;

	for (; ss_iterhas(sv_mergeiter, im->merge); ss_iternext(sv_mergeiter, im->merge))
	{
		sv *v = ss_iterof(sv_mergeiter, im->merge);
		int dup = sv_is(v, SVDUP) || sv_mergeisdup(im->merge);
		if (im->size >= im->limit) {
			if (! dup)
				break;
		}
		uint64_t lsn = sv_lsn(v);
		if (ssunlikely(dup)) {
			/* keep atleast one visible version for <= vlsn */
			if (im->prevlsn <= im->vlsn)
			{
				if (im->update) {
					im->update = (sv_flags(v) & SVUPDATE) > 0;
				} else {
					continue;
				}
			}
		} else {
			im->update = 0;
			/* delete (stray or on branch) */
			if (!im->save_delete) {
				int del = sv_is(v, SVDELETE);
				if (ssunlikely(del && (lsn <= im->vlsn))) {
					im->prevlsn = lsn;
					continue;
				}
			}
			im->size += im->sizev + sv_size(v);
			/* update */
			if (sv_is(v, SVUPDATE)) {
				int rc;
				/* compaction */
				if (! im->save_update) {
					rc = sv_writeiter_update(im);
					if (ssunlikely(rc == -1))
						return;
					im->prevlsn = lsn;
					im->v = &im->u->result;
					im->vdup = dup;
					im->next = 0;
					break;
				}
				/* branch
				 * keep next ready-to-gc statements to
				 * apply update.
				 */
				im->update = 1;
			}
		}
		im->prevlsn = lsn;
		im->v = v;
		im->vdup = dup;
		im->next = 1;
		break;
	}
}

static inline int
sv_writeiter_open(ssiter *i, sr *r, ssiter *merge, svupdate *u,
                  uint64_t limit,
                  uint32_t sizev, uint64_t vlsn,
                  int save_delete,
                  int save_update)
{
	svwriteiter *im = (svwriteiter*)i->priv;
	im->u     = u;
	im->r     = r;
	im->merge = merge;
	im->limit = limit;
	im->size  = 0;
	im->sizev = sizev;
	im->vlsn  = vlsn;;
	im->save_delete = save_delete;
	im->save_update = save_update;
	assert(im->merge->vif == &sv_mergeiter);
	im->next  = 0;
	im->prevlsn  = 0;
	im->v = NULL;
	im->vdup = 0;
	im->update = 0;
	sv_writeiter_next(i);
	return 0;
}

static inline void
sv_writeiter_close(ssiter *i ssunused)
{ }

static inline int
sv_writeiter_has(ssiter *i)
{
	svwriteiter *im = (svwriteiter*)i->priv;
	return im->v != NULL;
}

static inline void*
sv_writeiter_of(ssiter *i)
{
	svwriteiter *im = (svwriteiter*)i->priv;
	if (ssunlikely(im->v == NULL))
		return NULL;
	return im->v;
}

static inline int
sv_writeiter_resume(ssiter *i)
{
	svwriteiter *im = (svwriteiter*)i->priv;
	im->v       = ss_iterof(sv_mergeiter, im->merge);
	if (ssunlikely(im->v == NULL))
		return 0;
	im->vdup    = sv_is(im->v, SVDUP) || sv_mergeisdup(im->merge);
	im->prevlsn = sv_lsn(im->v);
	im->next    = 1;
	im->update  = 0;
	im->size    = im->sizev + sv_size(im->v);
	return 1;
}

static inline int
sv_writeiter_is_duplicate(ssiter *i)
{
	svwriteiter *im = (svwriteiter*)i->priv;
	assert(im->v != NULL);
	return im->vdup;
}

extern ssiterif sv_writeiter;

#endif
#line 1 "sophia/version/sv_index.h"
#ifndef SC_INDEX_H_
#define SC_INDEX_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svindexpos svindexpos;
typedef struct svindex svindex;

struct svindexpos {
	ssrbnode *node;
	int rc;
};

struct svindex {
	ssrb i;
	uint32_t count;
	uint32_t used;
	uint64_t lsnmin;
} sspacked;

ss_rbget(sv_indexmatch,
         sr_compare(scheme, sv_vpointer(sscast(n, svv, node)),
                    (sscast(n, svv, node))->size,
                    key, keysize))

int  sv_indexinit(svindex*);
int  sv_indexfree(svindex*, sr*);
svv *sv_indexget(svindex*, sr*, svindexpos*, svv*);
int  sv_indexupdate(svindex*, svindexpos*, svv*);

static inline int
sv_indexset(svindex *i, sr *r, svv  *v)
{
	svindexpos pos;
	sv_indexget(i, r, &pos, v);
	sv_indexupdate(i, &pos, v);
	return 0;
}

static inline uint32_t
sv_indexused(svindex *i) {
	return i->count * sizeof(svv) + i->used;
}

#endif
#line 1 "sophia/version/sv_indexiter.h"
#ifndef SV_INDEXITER_H_
#define SV_INDEXITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svindexiter svindexiter;

struct svindexiter {
	svindex *index;
	ssrbnode *v;
	svv *vcur;
	sv current;
	ssorder order;
} sspacked;

static inline int
sv_indexiter_open(ssiter *i, sr *r, svindex *index, ssorder o, void *key, int keysize)
{
	svindexiter *ii = (svindexiter*)i->priv;
	ii->index   = index;
	ii->order   = o;
	ii->v       = NULL;
	ii->vcur    = NULL;
	memset(&ii->current, 0, sizeof(ii->current));
	int rc;
	int eq = 0;
	switch (ii->order) {
	case SS_LT:
	case SS_LTE:
		if (ssunlikely(key == NULL)) {
			ii->v = ss_rbmax(&ii->index->i);
			break;
		}
		rc = sv_indexmatch(&ii->index->i, r->scheme, key, keysize, &ii->v);
		if (ii->v == NULL)
			break;
		switch (rc) {
		case 0:
			eq = 1;
			if (ii->order == SS_LT)
				ii->v = ss_rbprev(&ii->index->i, ii->v);
			break;
		case 1:
			ii->v = ss_rbprev(&ii->index->i, ii->v);
			break;
		}
		break;
	case SS_GT:
	case SS_GTE:
		if (ssunlikely(key == NULL)) {
			ii->v = ss_rbmin(&ii->index->i);
			break;
		}
		rc = sv_indexmatch(&ii->index->i, r->scheme, key, keysize, &ii->v);
		if (ii->v == NULL)
			break;
		switch (rc) {
		case  0:
			eq = 1;
			if (ii->order == SS_GT)
				ii->v = ss_rbnext(&ii->index->i, ii->v);
			break;
		case -1:
			ii->v = ss_rbnext(&ii->index->i, ii->v);
			break;
		}
		break;
	default: assert(0);
	}
	ii->vcur = NULL;
	if (ii->v) {
		ii->vcur = sscast(ii->v, svv, node);
		sv_init(&ii->current, &sv_vif, ii->vcur, NULL);
	}
	return eq;
}

static inline void
sv_indexiter_close(ssiter *i ssunused)
{}

static inline int
sv_indexiter_has(ssiter *i)
{
	svindexiter *ii = (svindexiter*)i->priv;
	return ii->v != NULL;
}

static inline void*
sv_indexiter_of(ssiter *i)
{
	svindexiter *ii = (svindexiter*)i->priv;
	if (ssunlikely(ii->v == NULL))
		return NULL;
	return &ii->current;
}

static inline void
sv_indexiter_next(ssiter *i)
{
	svindexiter *ii = (svindexiter*)i->priv;
	if (ssunlikely(ii->v == NULL))
		return;
	assert(ii->vcur != NULL);
	svv *v = ii->vcur->next;
	if (v) {
		ii->vcur = v;
		sv_init(&ii->current, &sv_vif, ii->vcur, NULL);
		return;
	}
	switch (ii->order) {
	case SS_LT:
	case SS_LTE:
		ii->v = ss_rbprev(&ii->index->i, ii->v);
		break;
	case SS_GT:
	case SS_GTE:
		ii->v = ss_rbnext(&ii->index->i, ii->v);
		break;
	default: assert(0);
	}
	ii->vcur = NULL;
	if (ii->v) {
		ii->vcur = sscast(ii->v, svv, node);
		sv_init(&ii->current, &sv_vif, ii->vcur, NULL);
	}
}

extern ssiterif sv_indexiter;

#endif
#line 1 "sophia/version/sv_index.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







ss_rbtruncate(sv_indextruncate,
              sv_vfree((ssa*)arg, sscast(n, svv, node)))

int sv_indexinit(svindex *i)
{
	i->lsnmin = UINT64_MAX;
	i->count  = 0;
	i->used   = 0;
	ss_rbinit(&i->i);
	return 0;
}

int sv_indexfree(svindex *i, sr *r)
{
	if (i->i.root)
		sv_indextruncate(i->i.root, r->a);
	ss_rbinit(&i->i);
	return 0;
}

static inline svv*
sv_vset(svv *head, svv *v)
{
	/* default */
	if (sslikely(head->lsn < v->lsn)) {
		v->next = head;
		head->flags |= SVDUP;
		return v;
	}
	/* redistribution (starting from highest lsn) */
	svv *prev = head;
	svv *c = head->next;
	while (c) {
		assert(c->lsn != v->lsn);
		if (c->lsn < v->lsn)
			break;
		prev = c;
		c = c->next;
	}
	prev->next = v;
	v->next = c;
	v->flags |= SVDUP;
	return head;
}

svv *sv_indexget(svindex *i, sr *r, svindexpos *p, svv *v)
{
	p->rc = sv_indexmatch(&i->i, r->scheme, sv_vpointer(v), v->size, &p->node);
	if (p->rc == 0 && p->node)
		return sscast(p->node, svv, node);
	return NULL;
}

int sv_indexupdate(svindex *i, svindexpos *p, svv *v)
{
	if (p->rc == 0 && p->node)
	{
		svv *head = sscast(p->node, svv, node);
		svv *update = sv_vset(head, v);
		if (head != update)
			ss_rbreplace(&i->i, p->node, &update->node);
	} else {
		ss_rbset(&i->i, p->node, p->rc, &v->node);
	}
	if (v->lsn < i->lsnmin)
		i->lsnmin = v->lsn;
	i->count++;
	i->used += v->size;
	return 0;
}
#line 1 "sophia/version/sv_indexiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/






ssiterif sv_indexiter =
{
	.close   = sv_indexiter_close,
	.has     = sv_indexiter_has,
	.of      = sv_indexiter_of,
	.next    = sv_indexiter_next
};
#line 1 "sophia/version/sv_mergeiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/






ssiterif sv_mergeiter =
{
	.close = sv_mergeiter_close,
	.has   = sv_mergeiter_has,
	.of    = sv_mergeiter_of,
	.next  = sv_mergeiter_next
};
#line 1 "sophia/version/sv_readiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/






ssiterif sv_readiter =
{
	.close   = sv_readiter_close,
	.has     = sv_readiter_has,
	.of      = sv_readiter_of,
	.next    = sv_readiter_next
};
#line 1 "sophia/version/sv_updatev.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







static uint8_t
sv_updatevifflags(sv *v) {
	svupdatenode *n = v->v;
	return n->flags;
}

static uint64_t
sv_updateviflsn(sv *v) {
	svupdatenode *n = v->v;
	return n->lsn;
}

static void
sv_updateviflsnset(sv *v ssunused, uint64_t lsn ssunused) {
	assert(0);
}

static char*
sv_updatevifpointer(sv *v) {
	svupdatenode *n = v->v;
	return n->buf.s;
}

static uint32_t
sv_updatevifsize(sv *v) {
	svupdatenode *n = v->v;
	return ss_bufused(&n->buf);
}

svif sv_updatevif =
{
	.flags   = sv_updatevifflags,
	.lsn     = sv_updateviflsn,
	.lsnset  = sv_updateviflsnset,
	.pointer = sv_updatevifpointer,
	.size    = sv_updatevifsize
};
#line 1 "sophia/version/sv_v.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/






static uint8_t
sv_vifflags(sv *v) {
	return ((svv*)v->v)->flags;
}

static uint64_t
sv_viflsn(sv *v) {
	return ((svv*)v->v)->lsn;
}

static void
sv_viflsnset(sv *v, uint64_t lsn) {
	((svv*)v->v)->lsn = lsn;
}

static char*
sv_vifpointer(sv *v) {
	return sv_vpointer(((svv*)v->v));
}

static uint32_t
sv_vifsize(sv *v) {
	return ((svv*)v->v)->size;
}

svif sv_vif =
{
	.flags   = sv_vifflags,
	.lsn     = sv_viflsn,
	.lsnset  = sv_viflsnset,
	.pointer = sv_vifpointer,
	.size    = sv_vifsize
};
#line 1 "sophia/version/sv_writeiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/






ssiterif sv_writeiter =
{
	.close   = sv_writeiter_close,
	.has     = sv_writeiter_has,
	.of      = sv_writeiter_of,
	.next    = sv_writeiter_next
};
#line 1 "sophia/transaction/sx_v.h"
#ifndef SX_V_H_
#define SX_V_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sxv sxv;

struct sxv {
	uint32_t id, lo;
	void *index;
	svv *v;
	sxv *next;
	sxv *prev;
	ssrbnode node;
} sspacked;

extern svif sx_vif;

static inline sxv*
sx_valloc(ssa *asxv, svv *v)
{
	sxv *vv = ss_malloc(asxv, sizeof(sxv));
	if (ssunlikely(vv == NULL))
		return NULL;
	vv->index = NULL;
	vv->id    = 0;
	vv->lo    = 0;
	vv->v     = v;
	vv->next  = NULL;
	vv->prev  = NULL;
	memset(&vv->node, 0, sizeof(vv->node));
	return vv;
}

static inline void
sx_vfree(ssa *a, ssa *asxv, sxv *v)
{
	ss_free(a, v->v);
	ss_free(asxv, v);
}

static inline sxv*
sx_vmatch(sxv *head, uint32_t id) {
	sxv *c = head;
	while (c) {
		if (c->id == id)
			break;
		c = c->next;
	}
	return c;
}

static inline void
sx_vreplace(sxv *v, sxv *n) {
	if (v->prev)
		v->prev->next = n;
	if (v->next)
		v->next->prev = n;
	n->next = v->next;
	n->prev = v->prev;
}

static inline void
sx_vlink(sxv *head, sxv *v) {
	sxv *c = head;
	while (c->next)
		c = c->next;
	c->next = v;
	v->prev = c;
	v->next = NULL;
}

static inline void
sx_vunlink(sxv *v) {
	if (v->prev)
		v->prev->next = v->next;
	if (v->next)
		v->next->prev = v->prev;
	v->prev = NULL;
	v->next = NULL;
}

static inline void
sx_vabortwaiters(sxv *v) {
	sxv *c = v->next;
	while (c) {
		c->v->flags |= SVABORT;
		c = c->next;
	}
}

#endif
#line 1 "sophia/transaction/sx.h"
#ifndef SX_H_
#define SX_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sxmanager sxmanager;
typedef struct sxindex sxindex;
typedef struct sx sx;

typedef enum {
	SXUNDEF,
	SXREADY,
	SXCOMMIT,
	SXPREPARE,
	SXROLLBACK,
	SXLOCK
} sxstate;

typedef sxstate (*sxpreparef)(sx*, sv*, void*, void*);

struct sxindex {
	ssrb i;
	uint32_t count;
	uint32_t dsn;
	srscheme *scheme;
	void *ptr;
	sr *r;
};

struct sx {
	uint32_t id;
	sxstate s;
	uint64_t vlsn;
	svlog log;
	sslist deadlock;
	sxmanager *manager;
	ssrbnode node;
};

struct sxmanager {
	ssspinlock lock;
	ssrb i;
	uint32_t count;
	ssa *asxv;
	ssa *a;
	srseq *seq;
};

int       sx_managerinit(sxmanager*, srseq*, ssa*, ssa*);
int       sx_managerfree(sxmanager*);
int       sx_indexinit(sxindex*, sr*, void*);
int       sx_indexset(sxindex*, uint32_t, srscheme*);
int       sx_indexfree(sxindex*, sxmanager*);
sx       *sx_find(sxmanager*, uint32_t);
void      sx_init(sxmanager*, sx*);
sxstate   sx_begin(sxmanager*, sx*, uint64_t);
void      sx_gc(sx*, sr*);
sxstate   sx_prepare(sx*, sxpreparef, void*);
sxstate   sx_commit(sx*);
sxstate   sx_rollback(sx*);
int       sx_set(sx*, sxindex*, svv*);
int       sx_get(sx*, sxindex*, sv*, sv*);
uint32_t  sx_min(sxmanager*);
uint32_t  sx_max(sxmanager*);
uint64_t  sx_vlsn(sxmanager*);
sxstate   sx_setstmt(sxmanager*, sxindex*, sv*);
sxstate   sx_getstmt(sxmanager*, sxindex*);

#endif
#line 1 "sophia/transaction/sx_deadlock.h"
#ifndef SX_DEADLOCK_H_
#define SX_DEADLOCK_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int sx_deadlock(sx*);

#endif
#line 1 "sophia/transaction/sx.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







int sx_managerinit(sxmanager *m, srseq *seq, ssa *a, ssa *asxv)
{
	ss_rbinit(&m->i);
	m->count = 0;
	ss_spinlockinit(&m->lock);
	m->seq = seq;
	m->asxv = asxv;
	m->a = a;
	return 0;
}

int sx_managerfree(sxmanager *m)
{
	assert(m->count == 0);
	ss_spinlockfree(&m->lock);
	return 0;
}

int sx_indexinit(sxindex *i, sr *r, void *ptr)
{
	ss_rbinit(&i->i);
	i->count = 0;
	i->scheme = NULL;
	i->ptr = ptr;
	i->r = r;
	return 0;
}

int sx_indexset(sxindex *i, uint32_t dsn, srscheme *scheme)
{
	i->dsn = dsn;
	i->scheme = scheme;
	return 0;
}

ss_rbtruncate(sx_truncate,
              sx_vfree(((ssa**)arg)[0],
                       ((ssa**)arg)[1], sscast(n, sxv, node)))

int sx_indexfree(sxindex *i, sxmanager *m)
{
	ssa *allocators[2] = { m->a, m->asxv };
	if (i->i.root)
		sx_truncate(i->i.root, allocators);
	return 0;
}

uint32_t sx_min(sxmanager *m)
{
	ss_spinlock(&m->lock);
	uint32_t id = 0;
	if (m->count) {
		ssrbnode *node = ss_rbmin(&m->i);
		sx *min = sscast(node, sx, node);
		id = min->id;
	}
	ss_spinunlock(&m->lock);
	return id;
}

uint32_t sx_max(sxmanager *m)
{
	ss_spinlock(&m->lock);
	uint32_t id = 0;
	if (m->count) {
		ssrbnode *node = ss_rbmax(&m->i);
		sx *max = sscast(node, sx, node);
		id = max->id;
	}
	ss_spinunlock(&m->lock);
	return id;
}

uint64_t sx_vlsn(sxmanager *m)
{
	ss_spinlock(&m->lock);
	uint64_t vlsn;
	if (m->count) {
		ssrbnode *node = ss_rbmin(&m->i);
		sx *min = sscast(node, sx, node);
		vlsn = min->vlsn;
	} else {
		vlsn = sr_seq(m->seq, SR_LSN);
	}
	ss_spinunlock(&m->lock);
	return vlsn;
}

ss_rbget(sx_matchtx, ss_cmp((sscast(n, sx, node))->id, *(uint32_t*)key))

sx *sx_find(sxmanager *m, uint32_t id)
{
	ssrbnode *n = NULL;
	int rc = sx_matchtx(&m->i, NULL, (char*)&id, sizeof(id), &n);
	if (rc == 0 && n)
		return  sscast(n, sx, node);
	return NULL;
}

void sx_init(sxmanager *m, sx *t)
{
	t->manager = m;
	sv_loginit(&t->log);
	ss_listinit(&t->deadlock);
}

sxstate sx_begin(sxmanager *m, sx *t, uint64_t vlsn)
{
	t->s = SXREADY; 
	sr_seqlock(m->seq);
	t->id = sr_seqdo(m->seq, SR_TSNNEXT);
	if (sslikely(vlsn == 0))
		t->vlsn = sr_seqdo(m->seq, SR_LSN);
	else
		t->vlsn = vlsn;
	sr_sequnlock(m->seq);
	sx_init(m, t);
	ss_spinlock(&m->lock);
	ssrbnode *n = NULL;
	int rc = sx_matchtx(&m->i, NULL, (char*)&t->id, sizeof(t->id), &n);
	if (rc == 0 && n) {
		assert(0);
	} else {
		ss_rbset(&m->i, n, rc, &t->node);
	}
	m->count++;
	ss_spinunlock(&m->lock);
	return SXREADY;
}

void sx_gc(sx *t, sr *r)
{
	sxmanager *m = t->manager;
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &t->log.buf, sizeof(svlogv));
	if (sslikely(t->s == SXCOMMIT)) {
		for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
		{
			svlogv *lv = ss_iterof(ss_bufiter, &i);
			sxv *v = lv->vgc;
			ss_free(m->asxv, v);
		}
	} else
	if (t->s == SXROLLBACK) {
		int gc = 0;
		for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
		{
			svlogv *lv = ss_iterof(ss_bufiter, &i);
			sxv *v = lv->v.v;
			gc += sv_vsize((svv*)v->v);
			sx_vfree(m->a, m->asxv, v);
		}
		ss_quota(r->quota, SS_QREMOVE, gc);
	}
	sv_logfree(&t->log, m->a);
	t->s = SXUNDEF;
}

static inline void
sx_end(sx *t)
{
	sxmanager *m = t->manager;
	ss_spinlock(&m->lock);
	ss_rbremove(&m->i, &t->node);
	m->count--;
	ss_spinunlock(&m->lock);
}

sxstate sx_prepare(sx *t, sxpreparef prepare, void *arg)
{
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &t->log.buf, sizeof(svlogv));
	sxstate s = SXPREPARE;
	for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
	{
		svlogv *lv = ss_iterof(ss_bufiter, &i);
		sxv *v = lv->v.v;
		/* cancelled by a concurrent commited
		 * transaction */
		if (v->v->flags & SVABORT) {
			s = SXROLLBACK;
			goto done;
		}
		/* concurrent update in progress */
		if (v->prev != NULL) {
			s = SXLOCK;
			goto done;
		}
		/* check that new key has not been committed by
		 * a concurrent transaction */
		if (prepare) {
			sxindex *i = v->index;
			s = prepare(t, &lv->v, arg, i->ptr);
			if (ssunlikely(s != SXPREPARE))
				goto done;
		}
	}
done:
	t->s = s;
	return s;
}

sxstate sx_commit(sx *t)
{
	assert(t->s == SXPREPARE);
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &t->log.buf, sizeof(svlogv));
	for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
	{
		svlogv *lv = ss_iterof(ss_bufiter, &i);
		sxv *v = lv->v.v;
		/* mark waiters as aborted */
		sx_vabortwaiters(v);
		/* remove from concurrent index and replace
		 * head with a first waiter */
		sxindex *i = v->index;
		if (v->next == NULL)
			ss_rbremove(&i->i, &v->node);
		else
			ss_rbreplace(&i->i, &v->node, &v->next->node);
		/* unlink version */
		sx_vunlink(v);
		/* translate log version from sxv to svv */
		sv_init(&lv->v, &sv_vif, v->v, NULL);
		lv->vgc = v;
	}
	t->s = SXCOMMIT;
	sx_end(t);
	return SXCOMMIT;
}

sxstate sx_rollback(sx *t)
{
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &t->log.buf, sizeof(svlogv));
	for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
	{
		svlogv *lv = ss_iterof(ss_bufiter, &i);
		sxv *v = lv->v.v;
		/* remove from index and replace head with
		 * a first waiter */
		if (v->prev)
			goto unlink;
		sxindex *i = v->index;
		if (v->next == NULL)
			ss_rbremove(&i->i, &v->node);
		else
			ss_rbreplace(&i->i, &v->node, &v->next->node);
unlink:
		sx_vunlink(v);
	}
	t->s = SXROLLBACK;
	sx_end(t);
	return SXROLLBACK;
}

ss_rbget(sx_match,
         sr_compare(scheme, sv_vpointer((sscast(n, sxv, node))->v),
                    (sscast(n, sxv, node))->v->size,
                    key, keysize))

int sx_set(sx *t, sxindex *index, svv *version)
{
	sxmanager *m = t->manager;
	/* allocate mvcc container */
	sxv *v = sx_valloc(m->asxv, version);
	if (ssunlikely(v == NULL)) {
		sv_vfree(m->a, version);
		return -1;
	}
	v->id = t->id;
	v->index = index;
	svlogv lv;
	lv.id   = index->dsn;
	lv.vgc  = NULL;
	lv.next = UINT32_MAX;
	sv_init(&lv.v, &sx_vif, v, NULL);
	/* update concurrent index */
	ssrbnode *n = NULL;
	int rc = sx_match(&index->i, index->scheme, sv_vpointer(version),
	                  version->size, &n);
	if (ssunlikely(rc == 0 && n)) {
		/* exists */
	} else {
		/* unique */
		v->lo = sv_logcount(&t->log);
		if (ssunlikely((sv_logadd(&t->log, m->a, &lv, index->ptr)) == -1)) {
			rc = sr_oom(index->r->e);
		} else {
			ss_rbset(&index->i, n, rc, &v->node);
			rc = 0;
		}
		return rc;
	}
	sxv *head = sscast(n, sxv, node);
	/* match previous update made by current
	 * transaction */
	sxv *own = sx_vmatch(head, t->id);
	if (ssunlikely(own))
	{
		if (ssunlikely(version->flags & SVUPDATE)) {
			sr_error(index->r->e, "%s", "only one update statement is "
			         "allowed per a transaction key");
			sx_vfree(m->a, m->asxv, v);
			return -1;
		}
		/* replace old object with the new one */
		lv.next = sv_logat(&t->log, own->lo)->next;
		v->lo = own->lo;
		sx_vreplace(own, v);
		if (sslikely(head == own))
			ss_rbreplace(&index->i, &own->node, &v->node);
		/* update log */
		sv_logreplace(&t->log, v->lo, &lv);
		sx_vfree(m->a, m->asxv, own);
		return 0;
	}
	/* update log */
	rc = sv_logadd(&t->log, m->a, &lv, index->ptr);
	if (ssunlikely(rc == -1)) {
		sx_vfree(m->a, m->asxv, v);
		return sr_oom(index->r->e);
	}
	/* add version */
	sx_vlink(head, v);
	return 0;
}

int sx_get(sx *t, sxindex *index, sv *key, sv *result)
{
	sxmanager *m = t->manager;
	ssrbnode *n = NULL;
	int rc = sx_match(&index->i, index->scheme,
	                  sv_pointer(key),
	                  sv_size(key), &n);
	if (! (rc == 0 && n)) {
		rc = 0;
		goto done;
	}
	sxv *head = sscast(n, sxv, node);
	sxv *v = sx_vmatch(head, t->id);
	if (v == NULL) {
		rc = 0;
		goto done;
	}
	if (ssunlikely((v->v->flags & SVDELETE) > 0)) {
		rc = 2;
		goto done;
	}
	sv vv;
	sv_init(&vv, &sv_vif, v->v, NULL);
	svv *ret = sv_vdup(m->a, &vv);
	if (ssunlikely(ret == NULL)) {
		rc = sr_oom(index->r->e);
	} else {
		sv_init(result, &sv_vif, ret, NULL);
		rc = 1;
	}
done:
	return rc;
}

sxstate sx_setstmt(sxmanager *m, sxindex *index, sv *v)
{
	sr_seq(m->seq, SR_TSNNEXT);
	ssrbnode *n = NULL;
	int rc = sx_match(&index->i, index->scheme, sv_pointer(v), sv_size(v), &n);
	if (rc == 0 && n)
		return SXLOCK;
	return SXCOMMIT;
}

sxstate sx_getstmt(sxmanager *m, sxindex *index ssunused)
{
	sr_seq(m->seq, SR_TSNNEXT);
	return SXCOMMIT;
}
#line 1 "sophia/transaction/sx_deadlock.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







static inline int
sx_deadlock_in(sxmanager *m, sslist *mark, sx *t, sx *p)
{
	if (p->deadlock.next != &p->deadlock)
		return 0;
	ss_listappend(mark, &p->deadlock);
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &p->log.buf, sizeof(svlogv));
	for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
	{
		svlogv *lv = ss_iterof(ss_bufiter, &i);
		sxv *v = lv->v.v;
		if (v->prev == NULL)
			continue;
		do {
			sx *n = sx_find(m, v->id);
			assert(n != NULL);
			if (ssunlikely(n == t))
				return 1;
			int rc = sx_deadlock_in(m, mark, t, n);
			if (ssunlikely(rc == 1))
				return 1;
			v = v->prev;
		} while (v);
	}
	return 0;
}

static inline void
sx_deadlock_unmark(sslist *mark)
{
	sslist *i, *n;
	ss_listforeach_safe(mark, i, n) {
		sx *t = sscast(i, sx, deadlock);
		ss_listinit(&t->deadlock);
	}
}

int sx_deadlock(sx *t)
{
	sxmanager *m = t->manager;
	sslist mark;
	ss_listinit(&mark);
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &t->log.buf, sizeof(svlogv));
	while (ss_iterhas(ss_bufiter, &i))
	{
		svlogv *lv = ss_iterof(ss_bufiter, &i);
		sxv *v = lv->v.v;
		if (v->prev == NULL) {
			ss_iternext(ss_bufiter, &i);
			continue;
		}
		sx *p = sx_find(m, v->prev->id);
		assert(p != NULL);
		int rc = sx_deadlock_in(m, &mark, t, p);
		if (ssunlikely(rc)) {
			sx_deadlock_unmark(&mark);
			return 1;
		}
		ss_iternext(ss_bufiter, &i);
	}
	sx_deadlock_unmark(&mark);
	return 0;
}
#line 1 "sophia/transaction/sx_v.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







static uint8_t
sx_vifflags(sv *v) {
	return ((sxv*)v->v)->v->flags;
}

static uint64_t
sx_viflsn(sv *v) {
	return ((sxv*)v->v)->v->lsn;
}

static void
sx_viflsnset(sv *v, uint64_t lsn) {
	((sxv*)v->v)->v->lsn = lsn;
}

static char*
sx_vifpointer(sv *v) {
	return sv_vpointer(((sxv*)v->v)->v);
}

static uint32_t
sx_vifsize(sv *v) {
	return ((sxv*)v->v)->v->size;
}

svif sx_vif =
{
	.flags   = sx_vifflags,
	.lsn     = sx_viflsn,
	.lsnset  = sx_viflsnset,
	.pointer = sx_vifpointer,
	.size    = sx_vifsize
};
#line 1 "sophia/log/sl_conf.h"
#ifndef SL_CONF_H_
#define SL_CONF_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct slconf slconf;

struct slconf {
	int   enable;
	char *path;
	int   sync_on_rotate;
	int   sync_on_write;
	int   rotatewm;
};

#endif
#line 1 "sophia/log/sl_dir.h"
#ifndef SL_DIR_H_
#define SL_DIR_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sldirtype sldirtype;
typedef struct sldirid sldirid;

struct sldirtype {
	char *ext;
	uint32_t mask;
	int count;
};

struct sldirid {
	uint32_t mask;
	uint64_t id;
};

int sl_dirread(ssbuf*, ssa*, sldirtype*, char*);

#endif
#line 1 "sophia/log/sl_v.h"
#ifndef SL_V_H_
#define SL_V_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct slv slv;

struct slv {
	uint32_t crc;
	uint64_t lsn;
	uint32_t dsn;
	uint32_t size;
	uint8_t  flags;
} sspacked;

extern svif sl_vif;

static inline uint32_t
sl_vdsn(sv *v) {
	return ((slv*)v->v)->dsn;
}

#endif
#line 1 "sophia/log/sl.h"
#ifndef SL_H_
#define SL_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sl sl;
typedef struct slpool slpool;
typedef struct sltx sltx;

struct sl {
	uint32_t id;
	ssgc gc;
	ssmutex filelock;
	ssfile file;
	slpool *p;
	sslist link;
	sslist linkcopy;
};

struct slpool {
	ssspinlock lock;
	slconf *conf;
	sslist list;
	int gc;
	int n;
	ssiov iov;
	sr *r;
};

struct sltx {
	slpool *p;
	sl *l;
	uint64_t svp;
};

int sl_poolinit(slpool*, sr*);
int sl_poolopen(slpool*, slconf*);
int sl_poolrotate(slpool*);
int sl_poolrotate_ready(slpool*, int);
int sl_poolshutdown(slpool*);
int sl_poolgc_enable(slpool*, int);
int sl_poolgc(slpool*);
int sl_poolfiles(slpool*);
int sl_poolcopy(slpool*, char*, ssbuf*);

int sl_begin(slpool*, sltx*);
int sl_prepare(slpool*, svlog*, uint64_t);
int sl_commit(sltx*);
int sl_rollback(sltx*);
int sl_write(sltx*, svlog*);

#endif
#line 1 "sophia/log/sl_iter.h"
#ifndef SL_ITER_H_
#define SL_ITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int sl_iter_open(ssiter *i, sr*, ssfile*, int);
int sl_iter_error(ssiter*);
int sl_iter_continue(ssiter*);

extern ssiterif sl_iter;

#endif
#line 1 "sophia/log/sl.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







static inline sl*
sl_alloc(slpool *p, uint32_t id)
{
	sl *l = ss_malloc(p->r->a, sizeof(*l));
	if (ssunlikely(l == NULL)) {
		sr_oom_malfunction(p->r->e);
		return NULL;
	}
	l->id   = id;
	l->p    = NULL;
	ss_gcinit(&l->gc);
	ss_mutexinit(&l->filelock);
	ss_fileinit(&l->file, p->r->a);
	ss_listinit(&l->link);
	ss_listinit(&l->linkcopy);
	return l;
}

static inline int
sl_close(slpool *p, sl *l)
{
	int rc = ss_fileclose(&l->file);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(p->r->e, "log file '%s' close error: %s",
		               l->file.file, strerror(errno));
	}
	ss_mutexfree(&l->filelock);
	ss_gcfree(&l->gc);
	ss_free(p->r->a, l);
	return rc;
}

static inline sl*
sl_open(slpool *p, uint32_t id)
{
	sl *l = sl_alloc(p, id);
	if (ssunlikely(l == NULL))
		return NULL;
	sspath path;
	ss_pathA(&path, p->conf->path, id, ".log");
	int rc = ss_fileopen(&l->file, path.path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(p->r->e, "log file '%s' open error: %s",
		               l->file.file, strerror(errno));
		goto error;
	}
	return l;
error:
	sl_close(p, l);
	return NULL;
}

static inline sl*
sl_new(slpool *p, uint32_t id)
{
	sl *l = sl_alloc(p, id);
	if (ssunlikely(l == NULL))
		return NULL;
	sspath path;
	ss_pathA(&path, p->conf->path, id, ".log");
	int rc = ss_filenew(&l->file, path.path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(p->r->e, "log file '%s' create error: %s",
		               path.path, strerror(errno));
		goto error;
	}
	srversion v;
	sr_version(&v);
	rc = ss_filewrite(&l->file, &v, sizeof(v));
	if (ssunlikely(rc == -1)) {
		sr_malfunction(p->r->e, "log file '%s' header write error: %s",
		               l->file.file, strerror(errno));
		goto error;
	}
	return l;
error:
	sl_close(p, l);
	return NULL;
}

int sl_poolinit(slpool *p, sr *r)
{
	ss_spinlockinit(&p->lock);
	ss_listinit(&p->list);
	p->n    = 0;
	p->r    = r;
	p->gc   = 1;
	p->conf = NULL;
	struct iovec *iov =
		ss_malloc(r->a, sizeof(struct iovec) * 1021);
	if (ssunlikely(iov == NULL))
		return sr_oom_malfunction(r->e);
	ss_iovinit(&p->iov, iov, 1021);
	return 0;
}

static inline int
sl_poolcreate(slpool *p)
{
	int rc;
	rc = ss_filemkdir(p->conf->path);
	if (ssunlikely(rc == -1))
		return sr_malfunction(p->r->e, "log directory '%s' create error: %s",
		                      p->conf->path, strerror(errno));
	return 1;
}

static inline int
sl_poolrecover(slpool *p)
{
	ssbuf list;
	ss_bufinit(&list);
	sldirtype types[] =
	{
		{ "log", 1, 0 },
		{ NULL,  0, 0 }
	};
	int rc = sl_dirread(&list, p->r->a, types, p->conf->path);
	if (ssunlikely(rc == -1))
		return sr_malfunction(p->r->e, "log directory '%s' open error",
		                      p->conf->path);
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &list, sizeof(sldirid));
	while(ss_iterhas(ss_bufiter, &i)) {
		sldirid *id = ss_iterof(ss_bufiter, &i);
		sl *l = sl_open(p, id->id);
		if (ssunlikely(l == NULL)) {
			ss_buffree(&list, p->r->a);
			return -1;
		}
		ss_listappend(&p->list, &l->link);
		p->n++;
		ss_iternext(ss_bufiter, &i);
	}
	ss_buffree(&list, p->r->a);
	if (p->n) {
		sl *last = sscast(p->list.prev, sl, link);
		p->r->seq->lfsn = last->id;
		p->r->seq->lfsn++;
	}
	return 0;
}

int sl_poolopen(slpool *p, slconf *conf)
{
	p->conf = conf;
	if (ssunlikely(! p->conf->enable))
		return 0;
	int exists = ss_fileexists(p->conf->path);
	int rc;
	if (! exists)
		rc = sl_poolcreate(p);
	else
		rc = sl_poolrecover(p);
	if (ssunlikely(rc == -1))
		return -1;
	return 0;
}

int sl_poolrotate(slpool *p)
{
	if (ssunlikely(! p->conf->enable))
		return 0;
	uint32_t lfsn = sr_seq(p->r->seq, SR_LFSNNEXT);
	sl *l = sl_new(p, lfsn);
	if (ssunlikely(l == NULL))
		return -1;
	sl *log = NULL;
	ss_spinlock(&p->lock);
	if (p->n) {
		log = sscast(p->list.prev, sl, link);
		ss_gccomplete(&log->gc);
	}
	ss_listappend(&p->list, &l->link);
	p->n++;
	ss_spinunlock(&p->lock);
	if (log) {
		if (p->conf->sync_on_rotate) {
			int rc = ss_filesync(&log->file);
			if (ssunlikely(rc == -1)) {
				sr_malfunction(p->r->e, "log file '%s' sync error: %s",
				               log->file.file, strerror(errno));
				return -1;
			}
		}
	}
	return 0;
}

int sl_poolrotate_ready(slpool *p, int wm)
{
	if (ssunlikely(! p->conf->enable))
		return 0;
	ss_spinlock(&p->lock);
	assert(p->n > 0);
	sl *l = sscast(p->list.prev, sl, link);
	int ready = ss_gcrotateready(&l->gc, wm);
	ss_spinunlock(&p->lock);
	return ready;
}

int sl_poolshutdown(slpool *p)
{
	int rcret = 0;
	int rc;
	if (p->n) {
		sslist *i, *n;
		ss_listforeach_safe(&p->list, i, n) {
			sl *l = sscast(i, sl, link);
			rc = sl_close(p, l);
			if (ssunlikely(rc == -1))
				rcret = -1;
		}
	}
	if (p->iov.v)
		ss_free(p->r->a, p->iov.v);
	ss_spinlockfree(&p->lock);
	return rcret;
}

static inline int
sl_gc(slpool *p, sl *l)
{
	int rc;
	rc = ss_fileunlink(l->file.file);
	if (ssunlikely(rc == -1)) {
		return sr_malfunction(p->r->e, "log file '%s' unlink error: %s",
		                      l->file.file, strerror(errno));
	}
	rc = sl_close(p, l);
	if (ssunlikely(rc == -1))
		return -1;
	return 1;
}

int sl_poolgc_enable(slpool *p, int enable)
{
	ss_spinlock(&p->lock);
	p->gc = enable;
	ss_spinunlock(&p->lock);
	return 0;
}

int sl_poolgc(slpool *p)
{
	if (ssunlikely(! p->conf->enable))
		return 0;
	for (;;) {
		ss_spinlock(&p->lock);
		if (ssunlikely(! p->gc)) {
			ss_spinunlock(&p->lock);
			return 0;
		}
		sl *current = NULL;
		sslist *i;
		ss_listforeach(&p->list, i) {
			sl *l = sscast(i, sl, link);
			if (sslikely(! ss_gcgarbage(&l->gc)))
				continue;
			ss_listunlink(&l->link);
			p->n--;
			current = l;
			break;
		}
		ss_spinunlock(&p->lock);
		if (current) {
			int rc = sl_gc(p, current);
			if (ssunlikely(rc == -1))
				return -1;
		} else {
			break;
		}
	}
	return 0;
}

int sl_poolfiles(slpool *p)
{
	ss_spinlock(&p->lock);
	int n = p->n;
	ss_spinunlock(&p->lock);
	return n;
}

int sl_poolcopy(slpool *p, char *dest, ssbuf *buf)
{
	sslist list;
	ss_listinit(&list);
	ss_spinlock(&p->lock);
	sslist *i;
	ss_listforeach(&p->list, i) {
		sl *l = sscast(i, sl, link);
		if (ss_gcinprogress(&l->gc))
			break;
		ss_listappend(&list, &l->linkcopy);
	}
	ss_spinunlock(&p->lock);

	ss_bufinit(buf);
	sslist *n;
	ss_listforeach_safe(&list, i, n)
	{
		sl *l = sscast(i, sl, linkcopy);
		ss_listinit(&l->linkcopy);
		sspath path;
		ss_pathA(&path, dest, l->id, ".log");
		ssfile file;
		ss_fileinit(&file, p->r->a);
		int rc = ss_filenew(&file, path.path);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(p->r->e, "log file '%s' create error: %s",
			               path.path, strerror(errno));
			return -1;
		}
		rc = ss_bufensure(buf, p->r->a, l->file.size);
		if (ssunlikely(rc == -1)) {
			sr_oom_malfunction(p->r->e);
			ss_fileclose(&file);
			return -1;
		}
		rc = ss_filepread(&l->file, 0, buf->s, l->file.size);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(p->r->e, "log file '%s' read error: %s",
			               l->file.file, strerror(errno));
			ss_fileclose(&file);
			return -1;
		}
		ss_bufadvance(buf, l->file.size);
		rc = ss_filewrite(&file, buf->s, l->file.size);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(p->r->e, "log file '%s' write error: %s",
			               path.path, strerror(errno));
			ss_fileclose(&file);
			return -1;
		}
		/* sync? */
		rc = ss_fileclose(&file);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(p->r->e, "log file '%s' close error: %s",
			               path.path, strerror(errno));
			return -1;
		}
		ss_bufreset(buf);
	}
	return 0;
}

int sl_begin(slpool *p, sltx *t)
{
	memset(t, 0, sizeof(*t));
	ss_spinlock(&p->lock);
	t->p = p;
	if (! p->conf->enable)
		return 0;
	assert(p->n > 0);
	sl *l = sscast(p->list.prev, sl, link);
	ss_mutexlock(&l->filelock);
	t->svp = ss_filesvp(&l->file);
	t->l = l;
	t->p = p;
	return 0;
}

int sl_commit(sltx *t)
{
	if (t->p->conf->enable)
		ss_mutexunlock(&t->l->filelock);
	ss_spinunlock(&t->p->lock);
	return 0;
}

int sl_rollback(sltx *t)
{
	int rc = 0;
	if (t->p->conf->enable) {
		rc = ss_filerlb(&t->l->file, t->svp);
		if (ssunlikely(rc == -1))
			sr_malfunction(t->p->r->e, "log file '%s' truncate error: %s",
			               t->l->file.file, strerror(errno));
		ss_mutexunlock(&t->l->filelock);
	}
	ss_spinunlock(&t->p->lock);
	return rc;
}

static inline int
sl_follow(slpool *p, uint64_t lsn)
{
	sr_seqlock(p->r->seq);
	if (lsn > p->r->seq->lsn)
		p->r->seq->lsn = lsn;
	sr_sequnlock(p->r->seq);
	return 0;
}

int sl_prepare(slpool *p, svlog *vlog, uint64_t lsn)
{
	if (sslikely(lsn == 0))
		lsn = sr_seq(p->r->seq, SR_LSNNEXT);
	else
		sl_follow(p, lsn);
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &vlog->buf, sizeof(svlogv));
	for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
	{
		svlogv *v = ss_iterof(ss_bufiter, &i);
		sv_lsnset(&v->v, lsn);
	}
	return 0;
}

static inline void
sl_write_prepare(slpool *p, sltx *t, slv *lv, svlogv *logv)
{
	sv *v = &logv->v;
	lv->lsn   = sv_lsn(v);
	lv->dsn   = logv->id;
	lv->flags = sv_flags(v);
	lv->size  = sv_size(v);
	lv->crc   = ss_crcp(p->r->crc, sv_pointer(v), lv->size, 0);
	lv->crc   = ss_crcs(p->r->crc, lv, sizeof(slv), lv->crc);
	ss_iovadd(&p->iov, lv, sizeof(slv));
	ss_iovadd(&p->iov, sv_pointer(v), lv->size);
	((svv*)v->v)->log = t->l;
}

static inline int
sl_write_stmt(sltx *t, svlog *vlog)
{
	slpool *p = t->p;
	slv lv;
	svlogv *logv = (svlogv*)vlog->buf.s;
	sl_write_prepare(t->p, t, &lv, logv);
	int rc = ss_filewritev(&t->l->file, &p->iov);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(p->r->e, "log file '%s' write error: %s",
		               t->l->file.file, strerror(errno));
		return -1;
	}
	ss_gcmark(&t->l->gc, 1);
	ss_iovreset(&p->iov);
	return 0;
}

static int
sl_write_multi_stmt(sltx *t, svlog *vlog, uint64_t lsn)
{
	slpool *p = t->p;
	sl *l = t->l;
	slv lvbuf[510]; /* 1 + 510 per syscall */
	int lvp;
	int rc;
	lvp = 0;
	/* transaction header */
	slv *lv = &lvbuf[0];
	lv->lsn   = lsn;
	lv->dsn   = 0;
	lv->flags = SVBEGIN;
	lv->size  = sv_logcount(vlog);
	lv->crc   = ss_crcs(p->r->crc, lv, sizeof(slv), 0);
	ss_iovadd(&p->iov, lv, sizeof(slv));
	lvp++;
	/* body */
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &vlog->buf, sizeof(svlogv));
	for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
	{
		if (ssunlikely(! ss_iovensure(&p->iov, 2))) {
			rc = ss_filewritev(&l->file, &p->iov);
			if (ssunlikely(rc == -1)) {
				sr_malfunction(p->r->e, "log file '%s' write error: %s",
				               l->file.file, strerror(errno));
				return -1;
			}
			ss_iovreset(&p->iov);
			lvp = 0;
		}
		svlogv *logv = ss_iterof(ss_bufiter, &i);
		assert(logv->v.i == &sv_vif);
		lv = &lvbuf[lvp];
		sl_write_prepare(p, t, lv, logv);
		lvp++;
	}
	if (sslikely(ss_iovhas(&p->iov))) {
		rc = ss_filewritev(&l->file, &p->iov);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(p->r->e, "log file '%s' write error: %s",
			               l->file.file, strerror(errno));
			return -1;
		}
		ss_iovreset(&p->iov);
	}
	ss_gcmark(&l->gc, sv_logcount(vlog));
	return 0;
}

int sl_write(sltx *t, svlog *vlog)
{
	/* assume transaction log is prepared
	 * (lsn set) */
	if (ssunlikely(! t->p->conf->enable))
		return 0;
	int count = sv_logcount(vlog);
	int rc;
	if (sslikely(count == 1)) {
		rc = sl_write_stmt(t, vlog);
	} else {
		svlogv *lv = (svlogv*)vlog->buf.s;
		uint64_t lsn = sv_lsn(&lv->v);
		rc = sl_write_multi_stmt(t, vlog, lsn);
	}
	if (ssunlikely(rc == -1))
		return -1;
	/* sync */
	if (t->p->conf->enable && t->p->conf->sync_on_write) {
		rc = ss_filesync(&t->l->file);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(t->p->r->e, "log file '%s' sync error: %s",
			               t->l->file.file, strerror(errno));
			return -1;
		}
	}
	return 0;
}
#line 1 "sophia/log/sl_dir.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







static inline ssize_t sl_diridof(char *s)
{
	size_t v = 0;
	while (*s && *s != '.') {
		if (ssunlikely(!isdigit(*s)))
			return -1;
		v = (v * 10) + *s - '0';
		s++;
	}
	return v;
}

static inline sldirid*
sl_dirmatch(ssbuf *list, uint64_t id)
{
	if (ssunlikely(ss_bufused(list) == 0))
		return NULL;
	sldirid *n = (sldirid*)list->s;
	while ((char*)n < list->p) {
		if (n->id == id)
			return n;
		n++;
	}
	return NULL;
}

static inline sldirtype*
sl_dirtypeof(sldirtype *types, char *ext)
{
	sldirtype *p = &types[0];
	int n = 0;
	while (p[n].ext != NULL) {
		if (strcmp(p[n].ext, ext) == 0)
			return &p[n];
		n++;
	}
	return NULL;
}

static int
sl_dircmp(const void *p1, const void *p2)
{
	sldirid *a = (sldirid*)p1;
	sldirid *b = (sldirid*)p2;
	assert(a->id != b->id);
	return (a->id > b->id)? 1: -1;
}

int sl_dirread(ssbuf *list, ssa *a, sldirtype *types, char *dir)
{
	DIR *d = opendir(dir);
	if (ssunlikely(d == NULL))
		return -1;

	struct dirent *de;
	while ((de = readdir(d))) {
		if (ssunlikely(de->d_name[0] == '.'))
			continue;
		ssize_t id = sl_diridof(de->d_name);
		if (ssunlikely(id == -1))
			goto error;
		char *ext = strstr(de->d_name, ".");
		if (ssunlikely(ext == NULL))
			goto error;
		ext++;
		sldirtype *type = sl_dirtypeof(types, ext);
		if (ssunlikely(type == NULL))
			continue;
		sldirid *n = sl_dirmatch(list, id);
		if (n) {
			n->mask |= type->mask;
			type->count++;
			continue;
		}
		int rc = ss_bufensure(list, a, sizeof(sldirid));
		if (ssunlikely(rc == -1))
			goto error;
		n = (sldirid*)list->p;
		ss_bufadvance(list, sizeof(sldirid));
		n->id  = id;
		n->mask = type->mask;
		type->count++;
	}
	closedir(d);

	if (ssunlikely(ss_bufused(list) == 0))
		return 0;

	int n = ss_bufused(list) / sizeof(sldirid);
	qsort(list->s, n, sizeof(sldirid), sl_dircmp);
	return n;

error:
	closedir(d);
	return -1;
}
#line 1 "sophia/log/sl_iter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







typedef struct sliter sliter;

struct sliter {
	int validate;
	int error;
	ssfile *log;
	ssmmap map;
	slv *v;
	slv *next;
	uint32_t count;
	uint32_t pos;
	sv current;
	sr *r;
} sspacked;

static void
sl_iterseterror(sliter *i)
{
	i->error = 1;
	i->v     = NULL;
	i->next  = NULL;
}

static int
sl_iternext_of(sliter *i, slv *next, int validate)
{
	if (next == NULL)
		return 0;
	char *eof   = (char*)i->map.p + i->map.size;
	char *start = (char*)next;

	/* eof */
	if (ssunlikely(start == eof)) {
		if (i->count != i->pos) {
			sr_malfunction(i->r->e, "corrupted log file '%s': transaction is incomplete",
			               i->log->file);
			sl_iterseterror(i);
			return -1;
		}
		i->v = NULL;
		i->next = NULL;
		return 0;
	}

	char *end = start + next->size;
	if (ssunlikely((start > eof || (end > eof)))) {
		sr_malfunction(i->r->e, "corrupted log file '%s': bad record size",
		               i->log->file);
		sl_iterseterror(i);
		return -1;
	}
	if (validate && i->validate)
	{
		uint32_t crc = 0;
		if (! (next->flags & SVBEGIN)) {
			crc = ss_crcp(i->r->crc, start + sizeof(slv), next->size, 0);
		}
		crc = ss_crcs(i->r->crc, start, sizeof(slv), crc);
		if (ssunlikely(crc != next->crc)) {
			sr_malfunction(i->r->e, "corrupted log file '%s': bad record crc",
			               i->log->file);
			sl_iterseterror(i);
			return -1;
		}
	}
	i->pos++;
	if (i->pos > i->count) {
		/* next transaction */
		i->v     = NULL;
		i->pos   = 0;
		i->count = 0;
		i->next  = next;
		return 0;
	}
	i->v = next;
	sv_init(&i->current, &sl_vif, i->v, NULL);
	return 1;
}

int sl_itercontinue_of(sliter *i)
{
	if (ssunlikely(i->error))
		return -1;
	if (ssunlikely(i->v))
		return 1;
	if (ssunlikely(i->next == NULL))
		return 0;
	int validate = 0;
	i->pos   = 0;
	i->count = 0;
	slv *v = i->next;
	if (v->flags & SVBEGIN) {
		validate = 1;
		i->count = v->size;
		v = (slv*)((char*)i->next + sizeof(slv));
	} else {
		i->count = 1;
		v = i->next;
	}
	return sl_iternext_of(i, v, validate);
}

static inline int
sl_iterprepare(sliter *i)
{
	srversion *ver = (srversion*)i->map.p;
	if (! sr_versioncheck(ver))
		return sr_malfunction(i->r->e, "bad log file '%s' version",
		                      i->log->file);
	if (ssunlikely(i->log->size < (sizeof(srversion))))
		return sr_malfunction(i->r->e, "corrupted log file '%s': bad size",
		                      i->log->file);
	slv *next = (slv*)((char*)i->map.p + sizeof(srversion));
	int rc = sl_iternext_of(i, next, 1);
	if (ssunlikely(rc == -1))
		return -1;
	if (sslikely(i->next))
		return sl_itercontinue_of(i);
	return 0;
}

int sl_iter_open(ssiter *i, sr *r, ssfile *file, int validate)
{
	sliter *li = (sliter*)i->priv;
	memset(li, 0, sizeof(*li));
	li->r        = r;
	li->log      = file;
	li->validate = validate;
	if (ssunlikely(li->log->size < sizeof(srversion))) {
		sr_malfunction(li->r->e, "corrupted log file '%s': bad size",
		               li->log->file);
		return -1;
	}
	if (ssunlikely(li->log->size == sizeof(srversion)))
		return 0;
	int rc = ss_mmap(&li->map, li->log->fd, li->log->size, 1);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(li->r->e, "failed to mmap log file '%s': %s",
		               li->log->file, strerror(errno));
		return -1;
	}
	rc = sl_iterprepare(li);
	if (ssunlikely(rc == -1))
		ss_munmap(&li->map);
	return 0;
}

static void
sl_iter_close(ssiter *i ssunused)
{
	sliter *li = (sliter*)i->priv;
	ss_munmap(&li->map);
}

static int
sl_iter_has(ssiter *i)
{
	sliter *li = (sliter*)i->priv;
	return li->v != NULL;
}

static void*
sl_iter_of(ssiter *i)
{
	sliter *li = (sliter*)i->priv;
	if (ssunlikely(li->v == NULL))
		return NULL;
	return &li->current;
}

static void
sl_iter_next(ssiter *i)
{
	sliter *li = (sliter*)i->priv;
	if (ssunlikely(li->v == NULL))
		return;
	slv *next =
		(slv*)((char*)li->v + sizeof(slv) + li->v->size);
	sl_iternext_of(li, next, 1);
}

ssiterif sl_iter =
{
	.close   = sl_iter_close,
	.has     = sl_iter_has,
	.of      = sl_iter_of,
	.next    = sl_iter_next
};

int sl_iter_error(ssiter *i)
{
	sliter *li = (sliter*)i->priv;
	return li->error;
}

int sl_iter_continue(ssiter *i)
{
	sliter *li = (sliter*)i->priv;
	return sl_itercontinue_of(li);
}
#line 1 "sophia/log/sl_v.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







static uint8_t
sl_vifflags(sv *v) {
	return ((slv*)v->v)->flags;
}

static uint64_t
sl_viflsn(sv *v) {
	return ((slv*)v->v)->lsn;
}

static char*
sl_vifpointer(sv *v) {
	return (char*)v->v + sizeof(slv);
}

static uint32_t
sl_vifsize(sv *v) {
	return ((slv*)v->v)->size;
}

svif sl_vif =
{
	.flags   = sl_vifflags,
	.lsn     = sl_viflsn,
	.lsnset  = NULL,
	.pointer = sl_vifpointer,
	.size    = sl_vifsize
};
#line 1 "sophia/database/sd_id.h"
#ifndef SD_ID_H_
#define SD_ID_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdid sdid;

#define SD_IDBRANCH 1

struct sdid {
	uint32_t parent;
	uint32_t id;
	uint8_t  flags;
} sspacked;

static inline void
sd_idinit(sdid *i, uint32_t id, uint32_t parent, uint32_t flags)
{
	i->id     = id;
	i->parent = parent;
	i->flags  = flags;
}

#endif
#line 1 "sophia/database/sd_v.h"
#ifndef SD_V_H_
#define SD_V_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdv sdv;

struct sdv {
	uint32_t offset;
	uint8_t  flags;
} sspacked;

extern svif sd_vif;
extern svif sd_vrawif;

#endif
#line 1 "sophia/database/sd_page.h"
#ifndef SD_PAGE_H_
#define SD_PAGE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdpageheader sdpageheader;
typedef struct sdpage sdpage;

struct sdpageheader {
	uint32_t crc;
	uint32_t crcdata;
	uint32_t count;
	uint32_t countdup;
	uint32_t sizeorigin;
	uint32_t sizekeys;
	uint32_t size;
	uint64_t lsnmin;
	uint64_t lsnmindup;
	uint64_t lsnmax;
	char     reserve[8];
} sspacked;

struct sdpage {
	sdpageheader *h;
};

static inline void
sd_pageinit(sdpage *p, sdpageheader *h) {
	p->h = h;
}

static inline sdv*
sd_pagev(sdpage *p, uint32_t pos) {
	assert(pos < p->h->count);
	return (sdv*)((char*)p->h + sizeof(sdpageheader) + sizeof(sdv) * pos);
}

static inline sdv*
sd_pagemin(sdpage *p) {
	return sd_pagev(p, 0);
}

static inline sdv*
sd_pagemax(sdpage *p) {
	return sd_pagev(p, p->h->count - 1);
}

static inline void*
sd_pagepointer(sdpage *p, sdv *v) {
	assert((sizeof(sdv) * p->h->count) + v->offset <= p->h->sizeorigin);
	return ((char*)p->h + sizeof(sdpageheader) +
	         sizeof(sdv) * p->h->count) + v->offset;
}

static inline uint64_t
sd_pagesizeof(sdpage *p, sdv *v)
{
	char *ptr = sd_pagepointer(p, v);
	uint64_t val = 0;
	ss_leb128read(ptr, &val);
	return val;
}

static inline uint64_t
sd_pagelsnof(sdpage *p, sdv *v)
{
	char *ptr = sd_pagepointer(p, v);
	ptr += ss_leb128skip(ptr);
	uint64_t val;
	ss_leb128read(ptr, &val);
	return val;
}

static inline char*
sd_pagemetaof(sdpage *p, sdv *v, uint64_t *size, uint64_t *lsn)
{
	char *ptr = sd_pagepointer(p, v);
	ptr += ss_leb128read(ptr, size);
	ptr += ss_leb128read(ptr, lsn);
	return ptr;
}

#endif
#line 1 "sophia/database/sd_pagekv.h"
#ifndef SD_PAGEKV_H_
#define SD_PAGEKV_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

static inline char*
sd_pagekv_keyread(sdpage *p, uint32_t offset, uint64_t *size)
{
	char *ptr = (char*)p->h + sizeof(sdpageheader) +
		(p->h->sizeorigin - p->h->sizekeys) + offset;
	ptr += ss_leb128read(ptr, size);
	return ptr;
}

static inline char*
sd_pagekv_key(sdpage *p, sdv *v, uint64_t *size, int part)
{
	char *ptr = sd_pagepointer(p, v);
	ptr += ss_leb128skip(ptr);
	ptr += ss_leb128skip(ptr);
	uint64_t offset;
	int current = 0;
	do {
		ptr += ss_leb128read(ptr, &offset);
	} while (current++ != part);

	return sd_pagekv_keyread(p, offset, size);
}

static inline char*
sd_pagekv_value(sdpage *p, sr *r, sdv *v, uint64_t *ret)
{
	uint64_t size;
	uint64_t sizekey = 0;
	char *ptr = sd_pagepointer(p, v);
	ptr += ss_leb128read(ptr, &size);
	ptr += ss_leb128skip(ptr);
	int i = 0;
	while (i < r->scheme->count) {
		uint64_t offset, v;
		ptr += ss_leb128read(ptr, &offset);
		sd_pagekv_keyread(p, offset, &v);
		sizekey += v;
		i++;
	}
	*ret = size - sizekey;
	return ptr;
}

static inline void
sd_pagekv_convert(sdpage *p, sr *r, sdv *v, char *dest)
{
	uint64_t size, lsn;
	sd_pagemetaof(p, v, &size, &lsn);
	size += sizeof(sfref) * r->scheme->count;

	char *ptr = dest;
	memcpy(dest, v, sizeof(sdv));
	ptr += sizeof(sdv);
	ptr += ss_leb128write(ptr, size);
	ptr += ss_leb128write(ptr, lsn);

	assert(r->scheme->count <= 8);
	sfv kv[8];
	int i = 0;
	while (i < r->scheme->count) {
		sfv *k = &kv[i];
		k->key = sd_pagekv_key(p, v, &size, i);
		k->r.size = size;
		k->r.offset = 0;
		i++;
	}
	uint64_t valuesize;
	char *value = sd_pagekv_value(p, r, v, &valuesize);
	sf_write(SF_KV, ptr, kv, r->scheme->count,
	         value, valuesize);
}

#endif
#line 1 "sophia/database/sd_pageiter.h"
#ifndef SD_PAGEITER_H_
#define SD_PAGEITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdpageiter sdpageiter;

struct sdpageiter {
	sdpage *page;
	ssbuf *xfbuf;
	int64_t pos;
	sdv *v;
	sv current;
	ssorder order;
	void *key;
	int keysize;
	sr *r;
} sspacked;

static inline void
sd_pageiter_result(sdpageiter *i)
{
	if (ssunlikely(i->v == NULL))
		return;
	if (sslikely(i->r->fmt_storage == SF_SRAW)) {
		sv_init(&i->current, &sd_vif, i->v, i->page->h);
		return;
	}
	sd_pagekv_convert(i->page, i->r, i->v, i->xfbuf->s);
	sv_init(&i->current, &sd_vrawif, i->xfbuf->s, NULL);
}

static inline void
sd_pageiter_end(sdpageiter *i)
{
	i->pos = i->page->h->count;
	i->v   = NULL;
}

static inline int
sd_pageiter_cmp(sdpageiter *i, sr *r, sdv *v)
{
	uint64_t size, lsn;
	if (sslikely(r->fmt_storage == SF_SRAW)) {
		char *key = sd_pagemetaof(i->page, v, &size, &lsn);
		return sr_compare(r->scheme, key, size, i->key, i->keysize);
	}
	/* key-value */
	srkey *part = r->scheme->parts;
	srkey *last = part + r->scheme->count;
	int rc;
	while (part < last) {
		char *key = sd_pagekv_key(i->page, v, &size, part->pos);
		rc = part->cmpraw(key, size,
		                  sf_key(i->key, part->pos),
		                  sf_keysize(i->key, part->pos),
		                  NULL);
		if (rc != 0)
			return rc;
		part++;
	}
	return 0;
}

static inline int
sd_pageiter_search(sdpageiter *i)
{
	int min = 0;
	int mid = 0;
	int max = i->page->h->count - 1;
	while (max >= min)
	{
		mid = min + (max - min) / 2;
		int rc = sd_pageiter_cmp(i, i->r, sd_pagev(i->page, mid));
		switch (rc) {
		case -1: min = mid + 1;
			continue;
		case  1: max = mid - 1;
			continue;
		default: return mid;
		}
	}
	return min;
}

static inline void
sd_pageiter_chain_head(sdpageiter *i, int64_t pos)
{
	/* find first non-duplicate key */
	while (pos >= 0) {
		sdv *v = sd_pagev(i->page, pos);
		if (sslikely(! (v->flags & SVDUP))) {
			i->pos = pos;
			i->v = v;
			return;
		}
		pos--;
	}
	sd_pageiter_end(i);
}

static inline void
sd_pageiter_chain_next(sdpageiter *i)
{
	/* skip to next duplicate chain */
	int64_t pos = i->pos + 1;
	while (pos < i->page->h->count) {
		sdv *v = sd_pagev(i->page, pos);
		if (sslikely(! (v->flags & SVDUP))) {
			i->pos = pos;
			i->v = v;
			return;
		}
		pos++;
	}
	sd_pageiter_end(i);
}

static inline int
sd_pageiter_gt(sdpageiter *i, int e)
{
	if (i->key == NULL) {
		i->pos = 0;
		i->v = sd_pagev(i->page, i->pos);
		return 0;
	}
	int64_t pos = sd_pageiter_search(i);
	if (ssunlikely(pos >= i->page->h->count))
		pos = i->page->h->count - 1;
	sd_pageiter_chain_head(i, pos);
	if (i->v == NULL)
		return 0;
	int rc = sd_pageiter_cmp(i, i->r, i->v);
	int match = rc == 0;
	switch (rc) {
		case  0:
			if (e) {
				break;
			}
		case -1:
			sd_pageiter_chain_next(i);
			break;
	}
	return match;
}

static inline int
sd_pageiter_lt(sdpageiter *i, int e)
{
	if (i->key == NULL) {
		sd_pageiter_chain_head(i, i->page->h->count - 1);
		return 0;
	}
	int64_t pos = sd_pageiter_search(i);
	if (ssunlikely(pos >= i->page->h->count))
		pos = i->page->h->count - 1;
	sd_pageiter_chain_head(i, pos);
	if (i->v == NULL)
		return 0;
	int rc = sd_pageiter_cmp(i, i->r, i->v);
	int match = rc == 0;
	switch (rc) {
		case 0:
			if (e) {
				break;
			}
		case 1:
			sd_pageiter_chain_head(i, i->pos - 1);
			break;
	}
	return match;
}

static inline int
sd_pageiter_open(ssiter *i, sr *r, ssbuf *xfbuf, sdpage *page, ssorder o,
                 void *key, int keysize)
{
	sdpageiter *pi = (sdpageiter*)i->priv;
	pi->r       = r;
	pi->page    = page;
	pi->xfbuf   = xfbuf;
	pi->order   = o;
	pi->key     = key;
	pi->keysize = keysize;
	pi->v       = NULL;
	pi->pos     = 0;
	if (ssunlikely(pi->page->h->count == 0)) {
		sd_pageiter_end(pi);
		return 0;
	}
	int rc = 0;
	switch (pi->order) {
	case SS_GT:  rc = sd_pageiter_gt(pi, 0);
		break;
	case SS_GTE: rc = sd_pageiter_gt(pi, 1);
		break;
	case SS_LT:  rc = sd_pageiter_lt(pi, 0);
		break;
	case SS_LTE: rc = sd_pageiter_lt(pi, 1);
		break;
	default: assert(0);
	}
	sd_pageiter_result(pi);
	return rc;
}

static inline void
sd_pageiter_close(ssiter *i ssunused)
{ }

static inline int
sd_pageiter_has(ssiter *i)
{
	sdpageiter *pi = (sdpageiter*)i->priv;
	return pi->v != NULL;
}

static inline void*
sd_pageiter_of(ssiter *i)
{
	sdpageiter *pi = (sdpageiter*)i->priv;
	if (ssunlikely(pi->v == NULL))
		return NULL;
	return &pi->current;
}

static inline void
sd_pageiter_next(ssiter *i)
{
	sdpageiter *pi = (sdpageiter*)i->priv;
	if (pi->v == NULL)
		return;
	switch (pi->order) {
	case SS_GTE:
	case SS_GT:
		pi->pos++;
		if (ssunlikely(pi->pos >= pi->page->h->count)) {
			sd_pageiter_end(pi);
			return;
		}
		pi->v = sd_pagev(pi->page, pi->pos);
		break;
	case SS_LT:
	case SS_LTE: {
		/* key (dup) (dup) key (eof) */
		sdv *v;
		int64_t pos = pi->pos + 1;
		if (pos < pi->page->h->count) {
			v = sd_pagev(pi->page, pos);
			if (v->flags & SVDUP) {
				pi->pos = pos;
				pi->v   = v;
				break;
			}
		}
		/* skip current chain and position to
		 * the previous one */
		sd_pageiter_chain_head(pi, pi->pos);
		sd_pageiter_chain_head(pi, pi->pos - 1);
		break;
	}
	default: assert(0);
	}
	sd_pageiter_result(pi);
}

extern ssiterif sd_pageiter;

#endif
#line 1 "sophia/database/sd_build.h"
#ifndef SD_BUILD_H_
#define SD_BUILD_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdbuildref sdbuildref;
typedef struct sdbuild sdbuild;

struct sdbuildref {
	uint32_t m, msize;
	uint32_t v, vsize;
	uint32_t k, ksize;
	uint32_t c, csize;
} sspacked;

struct sdbuild {
	ssbuf list, m, v, k, c;
	int compress_dup;
	int compress;
	int crc;
	uint32_t vmax;
	uint32_t n;
	ssht tracker;
};

void sd_buildinit(sdbuild*);
void sd_buildfree(sdbuild*, sr*);
void sd_buildreset(sdbuild*);

static inline sdbuildref*
sd_buildref(sdbuild *b) {
	return ss_bufat(&b->list, sizeof(sdbuildref), b->n);
}

static inline sdpageheader*
sd_buildheader(sdbuild *b) {
	return (sdpageheader*)(b->m.s + sd_buildref(b)->m);
}

static inline uint64_t
sd_buildoffset(sdbuild *b)
{
	sdbuildref *r = sd_buildref(b);
	if (b->compress)
		return r->c;
	return r->m + (ss_bufused(&b->v) - (ss_bufused(&b->v) - r->v)) +
	              (ss_bufused(&b->k) - (ss_bufused(&b->k) - r->k));
}

static inline sdv*
sd_buildmin(sdbuild *b) {
	return (sdv*)((char*)sd_buildheader(b) + sizeof(sdpageheader));
}

static inline char*
sd_buildminkey(sdbuild *b) {
	sdbuildref *r = sd_buildref(b);
	return b->v.s + r->v + sd_buildmin(b)->offset;
}

static inline sdv*
sd_buildmax(sdbuild *b) {
	sdpageheader *h = sd_buildheader(b);
	return (sdv*)((char*)h + sizeof(sdpageheader) + sizeof(sdv) * (h->count - 1));
}

static inline char*
sd_buildmaxkey(sdbuild *b) {
	sdbuildref *r = sd_buildref(b);
	return b->v.s + r->v + sd_buildmax(b)->offset;
}

int sd_buildbegin(sdbuild*, sr*, int, int, int);
int sd_buildend(sdbuild*, sr*);
int sd_buildcommit(sdbuild*, sr*);
int sd_buildadd(sdbuild*, sr*, sv*, uint32_t);

#endif
#line 1 "sophia/database/sd_index.h"
#ifndef SD_INDEX_H_
#define SD_INDEX_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdindexheader sdindexheader;
typedef struct sdindexpage sdindexpage;
typedef struct sdindex sdindex;

struct sdindexheader {
	uint32_t  crc;
	srversion version;
	sdid      id;
	uint64_t  offset;
	uint32_t  size;
	uint32_t  sizevmax;
	uint32_t  count;
	uint32_t  keys;
	uint64_t  total;
	uint64_t  totalorigin;
	uint32_t  tsmin;
	uint64_t  lsnmin;
	uint64_t  lsnmax;
	uint32_t  dupkeys;
	uint64_t  dupmin;
	uint32_t  extension;
	char      reserve[32];
} sspacked;

struct sdindexpage {
	uint64_t offset;
	uint32_t offsetindex;
	uint32_t size;
	uint32_t sizeorigin;
	uint16_t sizemin;
	uint16_t sizemax;
	uint64_t lsnmin;
	uint64_t lsnmax;
} sspacked;

struct sdindex {
	ssbuf i, v;
	sdindexheader *h;
};

static inline char*
sd_indexpage_min(sdindex *i, sdindexpage *p) {
	return (char*)i->i.s + sizeof(sdindexheader) +
	             (i->h->count * sizeof(sdindexpage)) + p->offsetindex;
}

static inline char*
sd_indexpage_max(sdindex *i, sdindexpage *p) {
	return sd_indexpage_min(i, p) + p->sizemin;
}

static inline void
sd_indexinit(sdindex *i) {
	ss_bufinit(&i->i);
	ss_bufinit(&i->v);
	i->h = NULL;
}

static inline void
sd_indexfree(sdindex *i, sr *r) {
	ss_buffree(&i->i, r->a);
	ss_buffree(&i->v, r->a);
}

static inline sdindexheader*
sd_indexheader(sdindex *i) {
	return (sdindexheader*)(i->i.s);
}

static inline sdindexpage*
sd_indexpage(sdindex *i, uint32_t pos)
{
	assert(pos < i->h->count);
	char *p = (char*)ss_bufat(&i->i, sizeof(sdindexpage), pos);
   	p += sizeof(sdindexheader);
	return (sdindexpage*)p;
}

static inline sdindexpage*
sd_indexmin(sdindex *i) {
	return sd_indexpage(i, 0);
}

static inline sdindexpage*
sd_indexmax(sdindex *i) {
	return sd_indexpage(i, i->h->count - 1);
}

static inline uint32_t
sd_indexkeys(sdindex *i)
{
	if (ssunlikely(i->i.s == NULL))
		return 0;
	return sd_indexheader(i)->keys;
}

static inline uint32_t
sd_indextotal(sdindex *i)
{
	if (ssunlikely(i->i.s == NULL))
		return 0;
	return sd_indexheader(i)->total;
}

static inline uint32_t
sd_indexsize(sdindexheader *h)
{
	return sizeof(sdindexheader) + h->size;
}

int sd_indexbegin(sdindex*, sr*, uint64_t);
int sd_indexcommit(sdindex*, sr*, sdid*);
int sd_indexadd(sdindex*, sr*, sdbuild*);
int sd_indexcopy(sdindex*, sr*, sdindexheader*);

#endif
#line 1 "sophia/database/sd_indexiter.h"
#ifndef SD_INDEXITER_H_
#define SD_INDEXITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdindexiter sdindexiter;

struct sdindexiter {
	sdindex *index;
	sdindexpage *v;
	int pos;
	ssorder cmp;
	void *key;
	int keysize;
	sr *r;
} sspacked;

#if 0
static inline int
sd_indexpage_cmp(sdindex *i, sdindexpage *p, void *key, int size, srscheme *s)
{
	int l = sr_compare(s, sd_indexpage_min(i, p), p->sizemin, key, size);
	int r = sr_compare(s, sd_indexpage_max(i, p), p->sizemax, key, size);
	/* inside page range */
	if (l <= 0 && r >= 0)
		return 0;
	/* key > page */
	if (l == -1)
		return -1;
	/* key < page */
	assert(r == 1);
	return 1;
}

static inline int
sd_indexiter_seek(sdindexiter *i, void *key, int size, int *minp, int *midp, int *maxp)
{
	int match = 0;
	int min = 0;
	int max = i->index->h->count - 1;
	int mid = 0;
	while (max >= min)
	{
		mid = min + ((max - min) >> 1);
		sdindexpage *page = sd_indexpage(i->index, mid);

		int rc = sd_indexpage_cmp(i->index, page, key, size, i->r->scheme);
		switch (rc) {
		case -1: min = mid + 1;
			continue;
		case  1: max = mid - 1;
			continue;
		default: match = 1;
			goto done;
		}
	}
done:
	*minp = min;
	*midp = mid;
	*maxp = max;
	return match;
}

static inline int
sd_indexiter_route(sdindexiter *i)
{
	int mid, min, max;
	int rc = sd_indexiter_seek(i, i->key, i->keysize, &min, &mid, &max);
	if (sslikely(rc))
		return mid;
	if (ssunlikely(min >= (int)i->index->h->count))
		min = i->index->h->count - 1;
	return min;
}
#endif

static inline int
sd_indexiter_route(sdindexiter *i)
{
	int begin = 0;
	int end = i->index->h->count - 1;
	while (begin != end) {
		int mid = begin + (end - begin) / 2;
		sdindexpage *page = sd_indexpage(i->index, mid);
		int rc = sr_compare(i->r->scheme,
		                    sd_indexpage_max(i->index, page),
		                    page->sizemax,
		                    i->key,
		                    i->keysize);
		if (rc < 0) {
			begin = mid + 1;
		} else {
			/* rc >= 0 */
			end = mid;
		}
	}
	if (ssunlikely(end >= (int)i->index->h->count))
		end = i->index->h->count - 1;
	return end;
}

static inline int
sd_indexiter_open(ssiter *i, sr *r, sdindex *index, ssorder o, void *key, int keysize)
{
	sdindexiter *ii = (sdindexiter*)i->priv;
	ii->r       = r;
	ii->index   = index;
	ii->cmp     = o;
	ii->key     = key;
	ii->keysize = keysize;
	ii->v       = NULL;
	ii->pos     = 0;
	if (ssunlikely(ii->index->h->count == 1)) {
		/* skip bootstrap node  */
		if (ii->index->h->lsnmin == UINT64_MAX &&
		    ii->index->h->lsnmax == 0)
			return 0;
	}
	if (ii->key == NULL) {
		switch (ii->cmp) {
		case SS_LT:
		case SS_LTE: ii->pos = ii->index->h->count - 1;
			break;
		case SS_GT:
		case SS_GTE: ii->pos = 0;
			break;
		default:
			assert(0);
		}
		ii->v = sd_indexpage(ii->index, ii->pos);
		return 0;
	}
	if (sslikely(ii->index->h->count > 1))
		ii->pos = sd_indexiter_route(ii);

	sdindexpage *p = sd_indexpage(ii->index, ii->pos);
	int rc;
	switch (ii->cmp) {
	case SS_LTE:
	case SS_LT:
		rc = sr_compare(ii->r->scheme, sd_indexpage_min(ii->index, p),
		                p->sizemin, ii->key, ii->keysize);
		if (rc ==  1 || (rc == 0 && ii->cmp == SS_LT))
			ii->pos--;
		break;
	case SS_GTE:
	case SS_GT:
		rc = sr_compare(ii->r->scheme, sd_indexpage_max(ii->index, p),
		                p->sizemax, ii->key, ii->keysize);
		if (rc == -1 || (rc == 0 && ii->cmp == SS_GT))
			ii->pos++;
		break;
	default: assert(0);
	}
	if (ssunlikely(ii->pos == -1 ||
	               ii->pos >= (int)ii->index->h->count))
		return 0;
	ii->v = sd_indexpage(ii->index, ii->pos);
	return 0;
}

static inline void
sd_indexiter_close(ssiter *i ssunused)
{ }

static inline int
sd_indexiter_has(ssiter *i)
{
	sdindexiter *ii = (sdindexiter*)i->priv;
	return ii->v != NULL;
}

static inline void*
sd_indexiter_of(ssiter *i)
{
	sdindexiter *ii = (sdindexiter*)i->priv;
	return ii->v;
}

static inline void
sd_indexiter_next(ssiter *i)
{
	sdindexiter *ii = (sdindexiter*)i->priv;
	switch (ii->cmp) {
	case SS_LT:
	case SS_LTE: ii->pos--;
		break;
	case SS_GT:
	case SS_GTE: ii->pos++;
		break;
	default:
		assert(0);
		break;
	}
	if (ssunlikely(ii->pos < 0))
		ii->v = NULL;
	else
	if (ssunlikely(ii->pos >= (int)ii->index->h->count))
		ii->v = NULL;
	else
		ii->v = sd_indexpage(ii->index, ii->pos);
}

extern ssiterif sd_indexiter;

#endif
#line 1 "sophia/database/sd_seal.h"
#ifndef SD_SEAL_H_
#define SD_SEAL_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdseal sdseal;

struct sdseal {
	uint32_t crc;
	uint32_t index_crc;
	uint64_t index_offset;
} sspacked;

static inline void
sd_seal(sdseal *s, sr *r, sdindexheader *h)
{
	s->index_crc = h->crc;
	s->index_offset = h->offset;
	s->crc = ss_crcs(r->crc, s, sizeof(sdseal), 0);
}

static inline int
sd_sealvalidate(sdseal *s, sr *r, sdindexheader *h)
{
	uint32_t crc = ss_crcs(r->crc, s, sizeof(sdseal), 0);
	if (ssunlikely(s->crc != crc))
		return -1;
	if (ssunlikely(h->crc != s->index_crc))
		return -1;
	if (ssunlikely(h->offset != s->index_offset))
		return -1;
	return 0;
}

#endif
#line 1 "sophia/database/sd_c.h"
#ifndef SD_C_H_
#define SD_C_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdc sdc;
typedef struct sdcbuf sdcbuf;

struct sdcbuf {
	ssbuf a; /* decompression */
	ssbuf b; /* transformation */
	sdcbuf *next;
};

struct sdc {
	sdbuild build;
	svupdate update;
	ssbuf a;        /* result */
	ssbuf b;        /* redistribute buffer */
	ssbuf c;        /* file buffer */
	sdcbuf *head;   /* compression buffer list */
	int count;
};

static inline void
sd_cinit(sdc *sc)
{
	sv_updateinit(&sc->update);
	sd_buildinit(&sc->build);
	ss_bufinit(&sc->a);
	ss_bufinit(&sc->b);
	ss_bufinit(&sc->c);
	sc->count = 0;
	sc->head = NULL;
}

static inline void
sd_cfree(sdc *sc, sr *r)
{
	sd_buildfree(&sc->build, r);
	sv_updatefree(&sc->update, r);
	ss_buffree(&sc->a, r->a);
	ss_buffree(&sc->b, r->a);
	ss_buffree(&sc->c, r->a);
	sdcbuf *b = sc->head;
	sdcbuf *next;
	while (b) {
		next = b->next;
		ss_buffree(&b->a, r->a);
		ss_buffree(&b->b, r->a);
		ss_free(r->a, b);
		b = next;
	}
}

static inline void
sd_creset(sdc *sc)
{
	sd_buildreset(&sc->build);
	sv_updatereset(&sc->update);
	ss_bufreset(&sc->a);
	ss_bufreset(&sc->b);
	ss_bufreset(&sc->c);
	sdcbuf *b = sc->head;
	while (b) {
		ss_bufreset(&b->a);
		ss_bufreset(&b->b);
		b = b->next;
	}
}

static inline int
sd_censure(sdc *c, sr *r, int count)
{
	if (c->count < count) {
		while (count-- >= 0) {
			sdcbuf *b = ss_malloc(r->a, sizeof(sdcbuf));
			if (ssunlikely(b == NULL))
				return -1;
			ss_bufinit(&b->a);
			ss_bufinit(&b->b);
			b->next = c->head;
			c->head = b;
			c->count++;
		}
	}
	return 0;
}

#endif
#line 1 "sophia/database/sd_merge.h"
#ifndef SD_MERGE_H_
#define SD_MERGE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdmergeconf sdmergeconf;
typedef struct sdmerge sdmerge;

struct sdmergeconf {
	uint32_t size_stream;
	uint64_t size_node;
	uint32_t size_page;
	uint32_t checksum;
	uint32_t compression;
	uint32_t compression_key;
	uint64_t offset;
	uint64_t vlsn;
	uint32_t save_delete;
	uint32_t save_update;
};

struct sdmerge {
	sdindex index;
	ssiter *merge;
	ssiter i;
	uint64_t processed;
	sdmergeconf *conf;
	sr *r;
	sdbuild *build;
};

int sd_mergeinit(sdmerge*, sr*, ssiter*, sdbuild*, svupdate*, sdmergeconf*);
int sd_mergefree(sdmerge*);
int sd_merge(sdmerge*);
int sd_mergecommit(sdmerge*, sdid*);

#endif
#line 1 "sophia/database/sd_commit.h"
#ifndef SD_COMMIT_H_
#define SD_COMMIT_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int sd_commit(sdbuild*, sr*, sdindex*, ssfile*);
int sd_commitpage(sdbuild*, sr*, ssbuf*);

#endif
#line 1 "sophia/database/sd_iter.h"
#ifndef SD_ITER_H_
#define SD_ITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sditer sditer;

struct sditer {
	int validate;
	int compression;
	ssbuf *compression_buf;
	ssbuf *transform_buf;
	sdindex *index;
	char *start, *end;
	char *page;
	char *pagesrc;
	sdpage pagev;
	uint32_t pos;
	sdv *dv;
	sv v;
	sr *r;
} sspacked;

static inline void
sd_iterresult(sditer *i, int pos)
{
	i->dv = sd_pagev(&i->pagev, pos);
	if (sslikely(i->r->fmt_storage == SF_SRAW)) {
		sv_init(&i->v, &sd_vif, i->dv, i->pagev.h);
		return;
	}
	sd_pagekv_convert(&i->pagev, i->r, i->dv, i->transform_buf->s);
	sv_init(&i->v, &sd_vrawif, i->transform_buf->s, NULL);
}

static inline int
sd_iternextpage(sditer *i)
{
	char *page = NULL;
	if (ssunlikely(i->page == NULL))
	{
		sdindexheader *h = i->index->h;
		page = i->start + h->offset + sd_indexsize(i->index->h);
		i->end = page + h->total;
	} else {
		page = i->pagesrc + sizeof(sdpageheader) + i->pagev.h->size;
	}
	if (ssunlikely(page >= i->end)) {
		i->page = NULL;
		return 0;
	}
	i->pagesrc = page;
	i->page = i->pagesrc;

	/* decompression */
	if (i->compression) {
		ss_bufreset(i->compression_buf);

		/* prepare decompression buffer */
		sdpageheader *h = (sdpageheader*)i->page;
		int rc = ss_bufensure(i->compression_buf, i->r->a, h->sizeorigin + sizeof(sdpageheader));
		if (ssunlikely(rc == -1)) {
			i->page = NULL;
			return sr_oom_malfunction(i->r->e);
		}

		/* copy page header */
		memcpy(i->compression_buf->s, i->page, sizeof(sdpageheader));
		ss_bufadvance(i->compression_buf, sizeof(sdpageheader));

		/* decompression */
		ssfilter f;
		rc = ss_filterinit(&f, (ssfilterif*)i->r->compression, i->r->a, SS_FOUTPUT);
		if (ssunlikely(rc == -1)) {
			i->page = NULL;
			sr_malfunction(i->r->e, "%s", "page decompression error");
			return -1;
		}
		rc = ss_filternext(&f, i->compression_buf, i->page + sizeof(sdpageheader), h->size);
		if (ssunlikely(rc == -1)) {
			ss_filterfree(&f);
			i->page = NULL;
			sr_malfunction(i->r->e, "%s", "page decompression error");
			return -1;
		}
		ss_filterfree(&f);

		/* switch to decompressed page */
		i->page = i->compression_buf->s;
	}

	/* checksum */
	if (i->validate) {
		sdpageheader *h = (sdpageheader*)i->page;
		uint32_t crc = ss_crcs(i->r->crc, h, sizeof(sdpageheader), 0);
		if (ssunlikely(crc != h->crc)) {
			i->page = NULL;
			sr_malfunction(i->r->e, "%s", "bad page header crc");
			return -1;
		}
	}
	sd_pageinit(&i->pagev, (void*)i->page);
	i->pos = 0;
	if (ssunlikely(i->pagev.h->count == 0)) {
		i->page = NULL;
		i->dv = NULL;
		return 0;
	}
	sd_iterresult(i, 0);
	return 1;
}

static inline int
sd_iter_open(ssiter *i, sr *r, sdindex *index, char *start, int validate,
             int compression,
             ssbuf *compression_buf,
             ssbuf *transform_buf)
{
	sditer *ii = (sditer*)i->priv;
	ii->r           = r;
	ii->index       = index;
	ii->start       = start;
	ii->end         = NULL;
	ii->page        = NULL;
	ii->pagesrc     = NULL;
	ii->pos         = 0;
	ii->dv          = NULL;
	ii->validate    = validate;
	ii->compression = compression;
	ii->compression_buf = compression_buf;
	ii->transform_buf = transform_buf;
	memset(&ii->v, 0, sizeof(ii->v));
	return sd_iternextpage(ii);
}

static inline void
sd_iter_close(ssiter *i ssunused)
{
	sditer *ii = (sditer*)i->priv;
	(void)ii;
}

static inline int
sd_iter_has(ssiter *i)
{
	sditer *ii = (sditer*)i->priv;
	return ii->page != NULL;
}

static inline void*
sd_iter_of(ssiter *i)
{
	sditer *ii = (sditer*)i->priv;
	if (ssunlikely(ii->page == NULL))
		return NULL;
	assert(ii->dv != NULL);
	return &ii->v;
}

static inline void
sd_iter_next(ssiter *i)
{
	sditer *ii = (sditer*)i->priv;
	if (ssunlikely(ii->page == NULL))
		return;
	ii->pos++;
	if (sslikely(ii->pos < ii->pagev.h->count)) {
		sd_iterresult(ii, ii->pos);
	} else {
		ii->dv = NULL;
		sd_iternextpage(ii);
	}
}

extern ssiterif sd_iter;

#endif
#line 1 "sophia/database/sd_recover.h"
#ifndef SD_RECOVER_H_
#define SD_RECOVER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int sd_recover_open(ssiter*, sr*, ssfile*);
int sd_recover_complete(ssiter*);

extern ssiterif sd_recover;

#endif
#line 1 "sophia/database/sd_scheme.h"
#ifndef SD_SCHEME_H_
#define SD_SCHEME_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdschemeheader sdschemeheader;
typedef struct sdschemeopt sdschemeopt;
typedef struct sdscheme sdscheme;

struct sdschemeheader {
	uint32_t crc;
	uint32_t size;
	uint32_t count;
} sspacked;

struct sdschemeopt {
	uint8_t  type;
	uint8_t  id;
	uint32_t size;
} sspacked;

struct sdscheme {
	ssbuf buf;
};

static inline void
sd_schemeinit(sdscheme *c) {
	ss_bufinit(&c->buf);
}

static inline void
sd_schemefree(sdscheme *c, sr *r) {
	ss_buffree(&c->buf, r->a);
}

static inline char*
sd_schemesz(sdschemeopt *o) {
	assert(o->type == SS_STRING);
	return (char*)o + sizeof(sdschemeopt);
}

static inline uint32_t
sd_schemeu32(sdschemeopt *o) {
	assert(o->type == SS_U32);
	return *(uint32_t*)((char*)o + sizeof(sdschemeopt));
}

static inline uint64_t
sd_schemeu64(sdschemeopt *o) {
	assert(o->type == SS_U64);
	return *(uint64_t*)((char*)o + sizeof(sdschemeopt));
}

int sd_schemebegin(sdscheme*, sr*);
int sd_schemeadd(sdscheme*, sr*, uint8_t, sstype, void*, uint32_t);
int sd_schemecommit(sdscheme*, sr*);
int sd_schemewrite(sdscheme*, sr*, char*, int);
int sd_schemerecover(sdscheme*, sr*, char*);

#endif
#line 1 "sophia/database/sd_schemeiter.h"
#ifndef SD_SCHEMEITER_H_
#define SD_SCHEMEITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sdschemeiter sdschemeiter;

struct sdschemeiter {
	sdscheme *c;
	char *p;
} sspacked;

static inline int
sd_schemeiter_open(ssiter *i, sr *r, sdscheme *c, int validate)
{
	sdschemeiter *ci = (sdschemeiter*)i->priv;
	ci->c = c;
	ci->p = NULL;
	if (validate) {
		sdschemeheader *h = (sdschemeheader*)c->buf.s;
		uint32_t crc = ss_crcs(r->crc, h, ss_bufused(&c->buf), 0);
		if (h->crc != crc) {
			sr_malfunction(r->e, "%s", "scheme file corrupted");
			return -1;
		}
	}
	ci->p = c->buf.s + sizeof(sdschemeheader);
	return 0;
}

static inline void
sd_schemeiter_close(ssiter *i ssunused)
{
	sdschemeiter *ci = (sdschemeiter*)i->priv;
	(void)ci;
}

static inline int
sd_schemeiter_has(ssiter *i)
{
	sdschemeiter *ci = (sdschemeiter*)i->priv;
	return ci->p < ci->c->buf.p;
}

static inline void*
sd_schemeiter_of(ssiter *i)
{
	sdschemeiter *ci = (sdschemeiter*)i->priv;
	if (ssunlikely(ci->p >= ci->c->buf.p))
		return NULL;
	return ci->p;
}

static inline void
sd_schemeiter_next(ssiter *i)
{
	sdschemeiter *ci = (sdschemeiter*)i->priv;
	if (ssunlikely(ci->p >= ci->c->buf.p))
		return;
	sdschemeopt *o = (sdschemeopt*)ci->p;
	ci->p = (char*)o + sizeof(sdschemeopt) + o->size;
}

extern ssiterif sd_schemeiter;

#endif
#line 1 "sophia/database/sd_build.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







void sd_buildinit(sdbuild *b)
{
	memset(&b->tracker, 0, sizeof(b->tracker));
	ss_bufinit(&b->list);
	ss_bufinit(&b->m);
	ss_bufinit(&b->v);
	ss_bufinit(&b->c);
	ss_bufinit(&b->k);
	b->n = 0;
	b->compress = 0;
	b->crc = 0;
	b->vmax = 0;
}

static inline void
sd_buildfree_tracker(sdbuild *b, sr *r)
{
	if (b->tracker.count == 0)
		return;
	int i = 0;
	for (; i < b->tracker.size; i++) {
		if (b->tracker.i[i] == NULL)
			continue;
		ss_free(r->a, b->tracker.i[i]);
		b->tracker.i[i] = NULL;
	}
	b->tracker.count = 0;
}

void sd_buildfree(sdbuild *b, sr *r)
{
	sd_buildfree_tracker(b, r);
	ss_htfree(&b->tracker, r->a);
	ss_buffree(&b->list, r->a);
	ss_buffree(&b->m, r->a);
	ss_buffree(&b->v, r->a);
	ss_buffree(&b->c, r->a);
	ss_buffree(&b->k, r->a);
}

void sd_buildreset(sdbuild *b)
{
	ss_htreset(&b->tracker);
	ss_bufreset(&b->list);
	ss_bufreset(&b->m);
	ss_bufreset(&b->v);
	ss_bufreset(&b->c);
	ss_bufreset(&b->k);
	b->n = 0;
	b->vmax = 0;
}

int sd_buildbegin(sdbuild *b, sr *r, int crc, int compress, int compress_dup)
{
	b->crc = crc;
	b->compress = compress;
	b->compress_dup = compress_dup;
	int rc;
	if (compress_dup && b->tracker.size == 0) {
		rc = ss_htinit(&b->tracker, r->a, 32768);
		if (ssunlikely(rc == -1))
			return sr_oom(r->e);
	}
	rc = ss_bufensure(&b->list, r->a, sizeof(sdbuildref));
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sdbuildref *ref =
		(sdbuildref*)ss_bufat(&b->list, sizeof(sdbuildref), b->n);
	ref->m     = ss_bufused(&b->m);
	ref->msize = 0;
	ref->v     = ss_bufused(&b->v);
	ref->vsize = 0;
	ref->k     = ss_bufused(&b->k);
	ref->ksize = 0;
	ref->c     = ss_bufused(&b->c);
	ref->csize = 0;
	rc = ss_bufensure(&b->m, r->a, sizeof(sdpageheader));
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sdpageheader *h = sd_buildheader(b);
	memset(h, 0, sizeof(*h));
	h->lsnmin    = UINT64_MAX;
	h->lsnmindup = UINT64_MAX;
	memset(h->reserve, 0, sizeof(h->reserve));
	ss_bufadvance(&b->list, sizeof(sdbuildref));
	ss_bufadvance(&b->m, sizeof(sdpageheader));
	return 0;
}

typedef struct {
	sshtnode node;
	uint32_t offset;
	uint32_t offsetstart;
	uint32_t size;
} sdbuildkey;

ss_htsearch(sd_buildsearch,
            (sscast(t->i[pos], sdbuildkey, node)->node.hash == hash) &&
            (sscast(t->i[pos], sdbuildkey, node)->size == size) &&
            (memcmp(((sdbuild*)ptr)->k.s +
                    sscast(t->i[pos], sdbuildkey, node)->offsetstart, key, size) == 0))

static inline int
sd_buildadd_keyvalue(sdbuild *b, sr *r, sv *v)
{
	/* calculate key size */
	uint32_t keysize = 0;
	int i = 0;
	while (i < r->scheme->count) {
		keysize += sv_keysize(v, r, i);
		i++;
	}
	uint32_t valuesize = sv_valuesize(v, r);
	uint32_t size = keysize + valuesize;

	/* prepare buffer */
	uint64_t lsn = sv_lsn(v);
	uint32_t sizemeta = ss_leb128size(size) + ss_leb128size(lsn);
	int rc = ss_bufensure(&b->v, r->a, sizemeta);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);

	/* write meta */
	ss_bufadvance(&b->v, ss_leb128write(b->v.p, size));
	ss_bufadvance(&b->v, ss_leb128write(b->v.p, lsn));

	/* write key-parts */
	i = 0;
	for (; i < r->scheme->count; i++)
	{
		uint32_t partsize = sv_keysize(v, r, i);
		char *part = sv_key(v, r, i);

		int offsetstart = ss_bufused(&b->k);
		int offset = (offsetstart - sd_buildref(b)->k);

		/* match a key copy */
		int is_duplicate = 0;
		uint32_t hash = 0;
		int pos = 0;
		if (b->compress_dup) {
			hash = ss_fnv(part, partsize);
			pos = sd_buildsearch(&b->tracker, hash, part, partsize, b);
			if (b->tracker.i[pos]) {
				is_duplicate = 1;
				sdbuildkey *ref = sscast(b->tracker.i[pos], sdbuildkey, node);
				offset = ref->offset;
			}
		}

		/* offset */
		rc = ss_bufensure(&b->v, r->a, ss_leb128size(offset));
		if (ssunlikely(rc == -1))
			return sr_oom(r->e);
		ss_bufadvance(&b->v, ss_leb128write(b->v.p, offset));
		if (is_duplicate)
			continue;

		/* copy key */
		int partsize_meta = ss_leb128size(partsize);
		rc = ss_bufensure(&b->k, r->a, partsize_meta + partsize);
		if (ssunlikely(rc == -1))
			return sr_oom(r->e);
		ss_bufadvance(&b->k, ss_leb128write(b->k.p, partsize));
		memcpy(b->k.p, part, partsize);
		ss_bufadvance(&b->k, partsize);

		/* add key reference */
		if (b->compress_dup) {
			if (ssunlikely(ss_htisfull(&b->tracker))) {
				rc = ss_htresize(&b->tracker, r->a);
				if (ssunlikely(rc == -1))
					return sr_oom(r->e);
			}
			sdbuildkey *ref = ss_malloc(r->a, sizeof(sdbuildkey));
			if (ssunlikely(rc == -1))
				return sr_oom(r->e);
			ref->node.hash = hash;
			ref->offset = offset;
			ref->offsetstart = offsetstart + partsize_meta;
			ref->size = partsize;
			ss_htset(&b->tracker, pos, &ref->node);
		}
	}

	/* write value */
	rc = ss_bufensure(&b->v, r->a, valuesize);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	memcpy(b->v.p, sv_value(v, r), valuesize);
	ss_bufadvance(&b->v, valuesize);
	return 0;
}

static inline int
sd_buildadd_raw(sdbuild *b, sr *r, sv *v)
{
	uint64_t lsn = sv_lsn(v);
	uint32_t size = sv_size(v);
	uint32_t sizemeta = ss_leb128size(size) + ss_leb128size(lsn);
	int rc = ss_bufensure(&b->v, r->a, sizemeta + size);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	ss_bufadvance(&b->v, ss_leb128write(b->v.p, size));
	ss_bufadvance(&b->v, ss_leb128write(b->v.p, lsn));
	memcpy(b->v.p, sv_pointer(v), size);
	ss_bufadvance(&b->v, size);
	return 0;
}

int sd_buildadd(sdbuild *b, sr *r, sv *v, uint32_t flags)
{
	/* prepare object metadata */
	int rc = ss_bufensure(&b->m, r->a, sizeof(sdv));
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sdpageheader *h = sd_buildheader(b);
	sdv *sv = (sdv*)b->m.p;
	sv->flags  = sv_flags(v) | flags;
	sv->offset = ss_bufused(&b->v) - sd_buildref(b)->v;
	ss_bufadvance(&b->m, sizeof(sdv));
	/* copy object */
	switch (r->fmt_storage) {
	case SF_SKEYVALUE:
		rc = sd_buildadd_keyvalue(b, r, v);
		break;
	case SF_SRAW:
		rc = sd_buildadd_raw(b, r, v);
		break;
	}
	if (ssunlikely(rc == -1))
		return -1;
	/* update page header */
	h->count++;
	uint32_t size = sizeof(sdv) + sv_size(v) +
		sizeof(sfref) * r->scheme->count;
	if (size > b->vmax)
		b->vmax = size;
	uint64_t lsn = sv_lsn(v);
	if (lsn > h->lsnmax)
		h->lsnmax = lsn;
	if (lsn < h->lsnmin)
		h->lsnmin = lsn;
	if (sv->flags & SVDUP) {
		h->countdup++;
		if (lsn < h->lsnmindup)
			h->lsnmindup = lsn;
	}
	return 0;
}

static inline int
sd_buildcompress(sdbuild *b, sr *r)
{
	/* reserve header */
	int rc = ss_bufensure(&b->c, r->a, sizeof(sdpageheader));
	if (ssunlikely(rc == -1))
		return -1;
	ss_bufadvance(&b->c, sizeof(sdpageheader));
	/* compression (including meta-data) */
	sdbuildref *ref = sd_buildref(b);
	ssfilter f;
	rc = ss_filterinit(&f, (ssfilterif*)r->compression, r->a, SS_FINPUT);
	if (ssunlikely(rc == -1))
		return -1;
	rc = ss_filterstart(&f, &b->c);
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_filternext(&f, &b->c, b->m.s + ref->m + sizeof(sdpageheader),
	                   ref->msize - sizeof(sdpageheader));
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_filternext(&f, &b->c, b->v.s + ref->v, ref->vsize);
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_filternext(&f, &b->c, b->k.s + ref->k, ref->ksize);
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_filtercomplete(&f, &b->c);
	if (ssunlikely(rc == -1))
		goto error;
	ss_filterfree(&f);
	return 0;
error:
	ss_filterfree(&f);
	return -1;
}

int sd_buildend(sdbuild *b, sr *r)
{
	/* update sizes */
	sdbuildref *ref = sd_buildref(b);
	ref->msize = ss_bufused(&b->m) - ref->m;
	ref->vsize = ss_bufused(&b->v) - ref->v;
	ref->ksize = ss_bufused(&b->k) - ref->k;
	ref->csize = 0;
	/* calculate data crc (non-compressed) */
	sdpageheader *h = sd_buildheader(b);
	uint32_t crc = 0;
	if (sslikely(b->crc)) {
		crc = ss_crcp(r->crc, b->m.s + ref->m, ref->msize, 0);
		crc = ss_crcp(r->crc, b->v.s + ref->v, ref->vsize, crc);
		crc = ss_crcp(r->crc, b->k.s + ref->k, ref->ksize, crc);
	}
	h->crcdata = crc;
	/* compression */
	if (b->compress) {
		int rc = sd_buildcompress(b, r);
		if (ssunlikely(rc == -1))
			return -1;
		ref->csize = ss_bufused(&b->c) - ref->c;
	}
	/* update page header */
	int total = ref->msize + ref->vsize + ref->ksize;
	h->sizekeys = ref->ksize;
	h->sizeorigin = total - sizeof(sdpageheader);
	h->size = h->sizeorigin;
	if (b->compress)
		h->size = ref->csize - sizeof(sdpageheader);
	else
		h->size = h->sizeorigin;
	h->crc = ss_crcs(r->crc, h, sizeof(sdpageheader), 0);
	if (b->compress)
		memcpy(b->c.s + ref->c, h, sizeof(sdpageheader));
	return 0;
}

int sd_buildcommit(sdbuild *b, sr *r)
{
	if (b->compress_dup)
		sd_buildfree_tracker(b, r);
	if (b->compress) {
		ss_bufreset(&b->m);
		ss_bufreset(&b->v);
		ss_bufreset(&b->k);
	}
	b->n++;
	return 0;
}
#line 1 "sophia/database/sd_commit.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







int sd_commitpage(sdbuild *b, sr *r, ssbuf *buf)
{
	sdbuildref *ref = sd_buildref(b);
	/* compressed */
	uint32_t size = ss_bufused(&b->c);
	int rc;
	if (size > 0) {
		rc = ss_bufensure(buf, r->a, ref->csize);
		if (ssunlikely(rc == -1))
			return -1;
		memcpy(buf->p, b->c.s, ref->csize);
		ss_bufadvance(buf, ref->csize);
		return 0;
	}
	/* not compressed */
	assert(ref->msize != 0);
	int total = ref->msize + ref->vsize + ref->ksize;
	rc = ss_bufensure(buf, r->a, total);
	if (ssunlikely(rc == -1))
		return -1;
	memcpy(buf->p, b->m.s + ref->m, ref->msize);
	ss_bufadvance(buf, ref->msize);
	memcpy(buf->p, b->v.s + ref->v, ref->vsize);
	ss_bufadvance(buf, ref->vsize);
	memcpy(buf->p, b->k.s + ref->k, ref->ksize);
	ss_bufadvance(buf, ref->ksize);
	return 0;
}

typedef struct {
	sdbuild *b;
	uint32_t i;
	uint32_t iovmax;
} sdcommitiov;

static inline void
sd_commitiov_init(sdcommitiov *i, sdbuild *b, int iovmax)
{
	i->b = b;
	i->iovmax = iovmax;
	i->i = 0;
}

static inline int
sd_commitiov(sdcommitiov *i, ssiov *iov)
{
	uint32_t n = 0;
	while (i->i < i->b->n && n < (i->iovmax - 3)) {
		sdbuildref *ref =
			(sdbuildref*)ss_bufat(&i->b->list, sizeof(sdbuildref), i->i);
		ss_iovadd(iov, i->b->m.s + ref->m, ref->msize);
		ss_iovadd(iov, i->b->v.s + ref->v, ref->vsize);
		ss_iovadd(iov, i->b->k.s + ref->k, ref->ksize);
		i->i++;
		n += 3;
	}
	return i->i < i->b->n;
}

int sd_commit(sdbuild *b, sr *r, sdindex *index, ssfile *file)
{
	sdseal seal;
	sd_seal(&seal, r, index->h);
	struct iovec iovv[1024];
	ssiov iov;
	ss_iovinit(&iov, iovv, 1024);
	ss_iovadd(&iov, index->i.s, ss_bufused(&index->i));

	SS_INJECTION(r->i, SS_INJECTION_SD_BUILD_0,
	             sr_malfunction(r->e, "%s", "error injection");
	             assert( ss_filewritev(file, &iov) == 0 );
	             return -1);

	/* compression enabled */
	uint32_t size = ss_bufused(&b->c);
	int rc;
	if (size > 0) {
		ss_iovadd(&iov, b->c.s, size);
		ss_iovadd(&iov, &seal, sizeof(seal));
		rc = ss_filewritev(file, &iov);
		if (ssunlikely(rc == -1))
			sr_malfunction(r->e, "file '%s' write error: %s",
			               file->file, strerror(errno));
		return rc;
	}

	/* uncompressed */
	sdcommitiov iter;
	sd_commitiov_init(&iter, b, 1020);
	int more = 1;
	while (more) {
		more = sd_commitiov(&iter, &iov);
		if (sslikely(! more)) {
			SS_INJECTION(r->i, SS_INJECTION_SD_BUILD_1,
			             seal.crc++); /* corrupt seal */
			ss_iovadd(&iov, &seal, sizeof(seal));
		}
		rc = ss_filewritev(file, &iov);
		if (ssunlikely(rc == -1)) {
			return sr_malfunction(r->e, "file '%s' write error: %s",
			                      file->file, strerror(errno));
		}
		ss_iovreset(&iov);
	}
	return 0;
}
#line 1 "sophia/database/sd_index.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







int sd_indexbegin(sdindex *i, sr *r, uint64_t offset)
{
	int rc = ss_bufensure(&i->i, r->a, sizeof(sdindexheader));
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sdindexheader *h = sd_indexheader(i);
	sr_version(&h->version);
	h->crc         = 0;
	h->size        = 0;
	h->sizevmax    = 0;
	h->count       = 0;
	h->keys        = 0;
	h->total       = 0;
	h->totalorigin = 0;
	h->extension   = 0;
	h->lsnmin      = UINT64_MAX;
	h->lsnmax      = 0;
	h->tsmin       = 0;
	h->offset      = offset;
	h->dupkeys     = 0;
	h->dupmin      = UINT64_MAX;
	memset(h->reserve, 0, sizeof(h->reserve));
	sd_idinit(&h->id, 0, 0, 0);
	i->h = NULL;
	ss_bufadvance(&i->i, sizeof(sdindexheader));
	return 0;
}

int sd_indexcommit(sdindex *i, sr *r, sdid *id)
{
	int size = ss_bufused(&i->v);
	int rc = ss_bufensure(&i->i, r->a, size);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	memcpy(i->i.p, i->v.s, size);
	ss_bufadvance(&i->i, size);
	ss_buffree(&i->v, r->a);
	i->h      = sd_indexheader(i);
	i->h->id  = *id;
	i->h->crc = ss_crcs(r->crc, i->h, sizeof(sdindexheader), 0);
	return 0;
}

static inline int
sd_indexadd_raw(sdindex *i, sr *r, sdindexpage *p, char *min, char *max)
{
	/* calculate sizes */
	p->sizemin = sf_keytotal(min, r->scheme->count);
	p->sizemax = sf_keytotal(max, r->scheme->count);
	/* prepare buffer */
	int rc = ss_bufensure(&i->v, r->a, p->sizemin + p->sizemax);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	/* reformat key object to exclude value */
	rc = sf_keycopy(i->v.p, min, r->scheme->count);
	assert(rc == p->sizemin);
	(void)rc;
	ss_bufadvance(&i->v, p->sizemin);
	rc = sf_keycopy(i->v.p, max, r->scheme->count);
	assert(rc == p->sizemax);
	(void)rc;
	ss_bufadvance(&i->v, p->sizemax);
	return 0;
}

static inline int
sd_indexadd_keyvalue(sdindex *i, sr *r, sdbuild *build, sdindexpage *p, char *min, char *max)
{
	assert(r->scheme->count <= 8);

	/* min */
	sfv kv[8];
	uint64_t offset;
	int total = 0;
	int part = 0;
	while (part < r->scheme->count) {
		/* read keytab offset */
		min += ss_leb128read(min, &offset);
		/* read key */
		sfv *k = &kv[part];
		char *key = build->k.s + sd_buildref(build)->k + offset;
		uint64_t keysize;
		key += ss_leb128read(key, &keysize);
		k->key = key;
		k->r.size = keysize;
		k->r.offset = 0;
		total += keysize;
		part++;
	}
	p->sizemin = total + (r->scheme->count * sizeof(sfref));
	int rc = ss_bufensure(&i->v, r->a, p->sizemin);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sf_write(SF_KV, i->v.p, kv, r->scheme->count, NULL, 0);
	ss_bufadvance(&i->v, p->sizemin);

	/* max */
	total = 0;
	part = 0;
	while (part < r->scheme->count) {
		/* read keytab offset */
		max += ss_leb128read(max, &offset);
		/* read key */
		sfv *k = &kv[part];
		char *key = build->k.s + sd_buildref(build)->k + offset;
		uint64_t keysize;
		key += ss_leb128read(key, &keysize);
		k->key = key;
		k->r.size = keysize;
		k->r.offset = 0;
		total += keysize;
		part++;
	}
	p->sizemax = total + (r->scheme->count * sizeof(sfref));
	rc = ss_bufensure(&i->v, r->a, p->sizemax);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sf_write(SF_KV, i->v.p, kv, r->scheme->count, NULL, 0);
	ss_bufadvance(&i->v, p->sizemax);
	return 0;
}

int sd_indexadd(sdindex *i, sr *r, sdbuild *build)
{
	int rc = ss_bufensure(&i->i, r->a, sizeof(sdindexpage));
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sdpageheader *ph = sd_buildheader(build);

	int size = ph->size + sizeof(sdpageheader);
	int sizeorigin = ph->sizeorigin + sizeof(sdpageheader);

	/* prepare page header.
	 *
	 * offset is relative to index:
	 * m->offset + (index_size) + page->offset
	*/
	sdindexpage *p = (sdindexpage*)i->i.p;
	p->offset      = sd_buildoffset(build);
	p->offsetindex = ss_bufused(&i->v);
	p->lsnmin      = ph->lsnmin;
	p->lsnmax      = ph->lsnmax;
	p->size        = size;
	p->sizeorigin  = sizeorigin;
	p->sizemin     = 0;
	p->sizemax     = 0;

	/* copy keys */
	if (ssunlikely(ph->count > 0))
	{
		char *min;
		char *max;
		min  = sd_buildminkey(build);
		min += ss_leb128skip(min);
		min += ss_leb128skip(min);
		max  = sd_buildmaxkey(build);
		max += ss_leb128skip(max);
		max += ss_leb128skip(max);
		switch (r->fmt_storage) {
		case SF_SRAW:
			rc = sd_indexadd_raw(i, r, p, min, max);
			break;
		case SF_SKEYVALUE:
			rc = sd_indexadd_keyvalue(i, r, build, p, min, max);
			break;
		}
		if (ssunlikely(rc == -1))
			return -1;
	}

	/* update index info */
	sdindexheader *h = sd_indexheader(i);
	h->count++;
	h->size  += sizeof(sdindexpage) + p->sizemin + p->sizemax;
	h->keys  += ph->count;
	h->total += size;
	h->totalorigin += sizeorigin;
	if (build->vmax > h->sizevmax)
		h->sizevmax = build->vmax;
	if (ph->lsnmin < h->lsnmin)
		h->lsnmin = ph->lsnmin;
	if (ph->lsnmax > h->lsnmax)
		h->lsnmax = ph->lsnmax;
	h->dupkeys += ph->countdup;
	if (ph->lsnmindup < h->dupmin)
		h->dupmin = ph->lsnmindup;
	ss_bufadvance(&i->i, sizeof(sdindexpage));
	return 0;
}

int sd_indexcopy(sdindex *i, sr *r, sdindexheader *h)
{
	int size = sd_indexsize(h);
	int rc = ss_bufensure(&i->i, r->a, size);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	memcpy(i->i.s, (char*)h, size);
	ss_bufadvance(&i->i, size);
	i->h = sd_indexheader(i);
	return 0;
}
#line 1 "sophia/database/sd_indexiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







ssiterif sd_indexiter =
{
	.close = sd_indexiter_close,
	.has   = sd_indexiter_has,
	.of    = sd_indexiter_of,
	.next  = sd_indexiter_next
};
#line 1 "sophia/database/sd_iter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







ssiterif sd_iter =
{
	.close   = sd_iter_close,
	.has     = sd_iter_has,
	.of      = sd_iter_of,
	.next    = sd_iter_next
};
#line 1 "sophia/database/sd_merge.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







int sd_mergeinit(sdmerge *m, sr *r, ssiter *i, sdbuild *build,
                 svupdate *update,
                 sdmergeconf *conf)
{
	m->conf      = conf;
	m->build     = build;
	m->r         = r;
	m->merge     = i;
	m->processed = 0;
	sd_indexinit(&m->index);
	ss_iterinit(sv_writeiter, &m->i);
	ss_iteropen(sv_writeiter, &m->i, r, i, update,
	            (uint64_t)conf->size_page, sizeof(sdv),
	            conf->vlsn,
	            conf->save_delete,
	            conf->save_update);
	return 0;
}

int sd_mergefree(sdmerge *m)
{
	sd_indexfree(&m->index, m->r);
	return 0;
}

int sd_merge(sdmerge *m)
{
	if (ssunlikely(! ss_iterhas(sv_writeiter, &m->i)))
		return 0;
	sdmergeconf *conf = m->conf;
	sd_buildreset(m->build);

	sd_indexinit(&m->index);
	int rc = sd_indexbegin(&m->index, m->r, conf->offset);
	if (ssunlikely(rc == -1))
		return -1;

	uint64_t processed = m->processed;
	uint64_t current = 0;
	uint64_t left = (conf->size_stream - processed);
	uint64_t limit;
	if (left >= (conf->size_node * 2)) {
		limit = conf->size_node;
	} else
	if (left > (conf->size_node)) {
		limit = conf->size_node * 2;
	} else {
		limit = UINT64_MAX;
	}

	while (ss_iterhas(sv_writeiter, &m->i) && (current <= limit))
	{
		rc = sd_buildbegin(m->build, m->r, conf->checksum,
		                   conf->compression,
		                   conf->compression_key);
		if (ssunlikely(rc == -1))
			return -1;
		while (ss_iterhas(sv_writeiter, &m->i))
		{
			sv *v = ss_iterof(sv_writeiter, &m->i);
			uint8_t flags = 0;
			if (sv_writeiter_is_duplicate(&m->i))
				flags = SVDUP;
			rc = sd_buildadd(m->build, m->r, v, flags);
			if (ssunlikely(rc == -1))
				return -1;
			ss_iternext(sv_writeiter, &m->i);
		}
		rc = sd_buildend(m->build, m->r);
		if (ssunlikely(rc == -1))
			return -1;
		rc = sd_indexadd(&m->index, m->r, m->build);
		if (ssunlikely(rc == -1))
			return -1;
		sd_buildcommit(m->build, m->r);

		current = sd_indextotal(&m->index);
		if (ssunlikely(! sv_writeiter_resume(&m->i)))
			break;
	}

	m->processed += sd_indextotal(&m->index);
	return 1;
}

int sd_mergecommit(sdmerge *m, sdid *id)
{
	return sd_indexcommit(&m->index, m->r, id);
}
#line 1 "sophia/database/sd_pageiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







ssiterif sd_pageiter =
{
	.close   = sd_pageiter_close,
	.has     = sd_pageiter_has,
	.of      = sd_pageiter_of,
	.next    = sd_pageiter_next
};
#line 1 "sophia/database/sd_recover.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







typedef struct sdrecover sdrecover;

struct sdrecover {
	ssfile *file;
	int corrupt;
	sdindexheader *actual;
	sdindexheader *v;
	ssmmap map;
	sr *r;
} sspacked;

static int
sd_recovernext_of(sdrecover *i, sdindexheader *next)
{
	if (next == NULL)
		return 0;
	char *eof   = (char*)i->map.p + i->map.size;
	char *start = (char*)next;
	/* eof */
	if (ssunlikely(start == eof)) {
		i->v = NULL;
		return 0;
	}
	/* validate crc */
	uint32_t crc = ss_crcs(i->r->crc, next, sizeof(sdindexheader), 0);
	if (next->crc != crc) {
		sr_malfunction(i->r->e, "corrupted db file '%s': bad index crc",
		               i->file->file);
		i->corrupt = 1;
		i->v = NULL;
		return -1;
	}
	/* check version */
	if (! sr_versioncheck(&next->version))
		return sr_malfunction(i->r->e, "bad db file '%s' version",
		                      i->file->file);
	char *end = start + sizeof(sdindexheader) + next->size +
	            next->total +
	            next->extension + sizeof(sdseal);
	if (ssunlikely((start > eof || (end > eof)))) {
		sr_malfunction(i->r->e, "corrupted db file '%s': bad record size",
		               i->file->file);
		i->corrupt = 1;
		i->v = NULL;
		return -1;
	}
	/* check seal */
	sdseal *s = (sdseal*)(end - sizeof(sdseal));
	int rc = sd_sealvalidate(s, i->r, next);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(i->r->e, "corrupted db file '%s': bad seal",
		               i->file->file);
		i->corrupt = 1;
		i->v = NULL;
		return -1;
	}
	i->actual = next;
	i->v = next;
	return 1;
}

int sd_recover_open(ssiter *i, sr *r, ssfile *file)
{
	sdrecover *ri = (sdrecover*)i->priv;
	memset(ri, 0, sizeof(*ri));
	ri->r = r;
	ri->file = file;
	if (ssunlikely(ri->file->size < (sizeof(sdindexheader) + sizeof(sdseal)))) {
		sr_malfunction(ri->r->e, "corrupted db file '%s': bad size",
		               ri->file->file);
		ri->corrupt = 1;
		return -1;
	}
	int rc = ss_mmap(&ri->map, ri->file->fd, ri->file->size, 1);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(ri->r->e, "failed to mmap db file '%s': %s",
		               ri->file->file, strerror(errno));
		return -1;
	}
	sdindexheader *next = (sdindexheader*)((char*)ri->map.p);
	rc = sd_recovernext_of(ri, next);
	if (ssunlikely(rc == -1))
		ss_munmap(&ri->map);
	return rc;
}

static void
sd_recoverclose(ssiter *i ssunused)
{
	sdrecover *ri = (sdrecover*)i->priv;
	ss_munmap(&ri->map);
}

static int
sd_recoverhas(ssiter *i)
{
	sdrecover *ri = (sdrecover*)i->priv;
	return ri->v != NULL;
}

static void*
sd_recoverof(ssiter *i)
{
	sdrecover *ri = (sdrecover*)i->priv;
	return ri->v;
}

static void
sd_recovernext(ssiter *i)
{
	sdrecover *ri = (sdrecover*)i->priv;
	if (ssunlikely(ri->v == NULL))
		return;
	sdindexheader *next =
		(sdindexheader*)((char*)ri->v +
		    (sizeof(sdindexheader) + ri->v->size) +
		     ri->v->total +
		     ri->v->extension + sizeof(sdseal));
	sd_recovernext_of(ri, next);
}

ssiterif sd_recover =
{
	.close   = sd_recoverclose,
	.has     = sd_recoverhas,
	.of      = sd_recoverof,
	.next    = sd_recovernext
};

int sd_recover_complete(ssiter *i)
{
	sdrecover *ri = (sdrecover*)i->priv;
	if (ssunlikely(ri->actual == NULL))
		return -1;
	if (sslikely(ri->corrupt == 0))
		return  0;
	/* truncate file to the latest actual index */
	char *eof =
		(char*)ri->actual + sizeof(sdindexheader) +
		       ri->actual->size +
		       ri->actual->total +
		       ri->actual->extension + sizeof(sdseal);
	uint64_t file_size = eof - ri->map.p;
	int rc = ss_fileresize(ri->file, file_size);
	if (ssunlikely(rc == -1))
		return -1;
	sr_errorreset(ri->r->e);
	return 0;
}
#line 1 "sophia/database/sd_scheme.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







int sd_schemebegin(sdscheme *c, sr *r)
{
	int rc = ss_bufensure(&c->buf, r->a, sizeof(sdschemeheader));
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	sdschemeheader *h = (sdschemeheader*)c->buf.s;
	memset(h, 0, sizeof(sdschemeheader));
	ss_bufadvance(&c->buf, sizeof(sdschemeheader));
	return 0;
}

int sd_schemeadd(sdscheme *c, sr *r, uint8_t id, sstype type,
                 void *value, uint32_t size)
{
	sdschemeopt opt = {
		.type = type,
		.id   = id,
		.size = size
	};
	int rc = ss_bufadd(&c->buf, r->a, &opt, sizeof(opt));
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_bufadd(&c->buf, r->a, value, size);
	if (ssunlikely(rc == -1))
		goto error;
	sdschemeheader *h = (sdschemeheader*)c->buf.s;
	h->count++;
	return 0;
error:
	return sr_oom(r->e);
}

int sd_schemecommit(sdscheme *c, sr *r)
{
	if (ssunlikely(ss_bufused(&c->buf) == 0))
		return 0;
	sdschemeheader *h = (sdschemeheader*)c->buf.s;
	h->size = ss_bufused(&c->buf) - sizeof(sdschemeheader);
	h->crc  = ss_crcs(r->crc, (char*)h, ss_bufused(&c->buf), 0);
	return 0;
}

int sd_schemewrite(sdscheme *c, sr *r, char *path, int sync)
{
	ssfile meta;
	ss_fileinit(&meta, r->a);
	int rc = ss_filenew(&meta, path);
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_filewrite(&meta, c->buf.s, ss_bufused(&c->buf));
	if (ssunlikely(rc == -1))
		goto error;
	if (sync) {
		rc = ss_filesync(&meta);
		if (ssunlikely(rc == -1))
			goto error;
	}
	rc = ss_fileclose(&meta);
	if (ssunlikely(rc == -1))
		goto error;
	return 0;
error:
	sr_error(r->e, "scheme file '%s' error: %s",
	         path, strerror(errno));
	ss_fileclose(&meta);
	return -1;
}

int sd_schemerecover(sdscheme *c, sr *r, char *path)
{
	ssize_t size = ss_filesize(path);
	if (ssunlikely(size == -1))
		goto error;
	if (ssunlikely((unsigned int)size < sizeof(sdschemeheader))) {
		sr_error(r->e, "scheme file '%s' is corrupted", path);
		return -1;
	}
	int rc = ss_bufensure(&c->buf, r->a, size);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	ssfile meta;
	ss_fileinit(&meta, r->a);
	rc = ss_fileopen(&meta, path);
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_filepread(&meta, 0, c->buf.s, size);
	if (ssunlikely(rc == -1))
		goto error;
	rc = ss_fileclose(&meta);
	if (ssunlikely(rc == -1))
		goto error;
	ss_bufadvance(&c->buf, size);
	return 0;
error:
	sr_error(r->e, "scheme file '%s' error: %s",
	         path, strerror(errno));
	return -1;
}
#line 1 "sophia/database/sd_schemeiter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







ssiterif sd_schemeiter =
{
	.close   = sd_schemeiter_close,
	.has     = sd_schemeiter_has,
	.of      = sd_schemeiter_of,
	.next    = sd_schemeiter_next
};
#line 1 "sophia/database/sd_v.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/







static uint8_t
sd_vifflags(sv *v) {
	return ((sdv*)v->v)->flags;
}

static uint64_t
sd_viflsn(sv *v) {
	sdpage p = {
		.h = (sdpageheader*)v->arg
	};
	return sd_pagelsnof(&p, (sdv*)v->v);
}

static char*
sd_vifpointer(sv *v)
{
	sdpage p = {
		.h = (sdpageheader*)v->arg
	};
	char *ptr = sd_pagepointer(&p, (sdv*)v->v);
	ptr += ss_leb128skip(ptr);
	ptr += ss_leb128skip(ptr);
	return ptr;
}

static uint32_t
sd_vifsize(sv *v) {
	sdpage p = {
		.h = (sdpageheader*)v->arg
	};
	return sd_pagesizeof(&p, (sdv*)v->v);
}

svif sd_vif =
{
	.flags   = sd_vifflags,
	.lsn     = sd_viflsn,
	.lsnset  = NULL,
	.pointer = sd_vifpointer,
	.size    = sd_vifsize
};

static uint64_t
sd_vrawiflsn(sv *v) {
	sdv *dv = v->v;
	char *ptr = (char*)dv + sizeof(sdv);
	ptr += ss_leb128skip(ptr);
	uint64_t val;
	ss_leb128read(ptr, &val);
	return val;
}

static char*
sd_vrawifpointer(sv *v)
{
	sdv *dv = v->v;
	char *ptr = (char*)dv + sizeof(sdv);
	ptr += ss_leb128skip(ptr);
	ptr += ss_leb128skip(ptr);
	return ptr;
}

static uint32_t
sd_vrawifsize(sv *v) {
	sdv *dv = v->v;
	uint64_t val;
	ss_leb128read((char*)dv + sizeof(sdv), &val);
	return val;
}

svif sd_vrawif =
{
	.flags   = sd_vifflags,
	.lsn     = sd_vrawiflsn,
	.lsnset  = NULL,
	.pointer = sd_vrawifpointer,
	.size    = sd_vrawifsize
};
#line 1 "sophia/index/si_scheme.h"
#ifndef SI_SCHEME_H_
#define SI_SCHEME_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sischeme sischeme;

struct sischeme {
	uint32_t    id;
	char       *name;
	char       *path;
	uint32_t    path_fail_on_exists;
	uint32_t    path_fail_on_drop;
	char       *path_backup;
	uint32_t    mmap;
	uint32_t    sync;
	uint64_t    node_size;
	uint32_t    node_page_size;
	uint32_t    node_page_checksum;
	uint32_t    compression;
	char       *compression_sz;
	ssfilterif *compression_if;
	uint32_t    compression_key;
	char       *fmt_sz;
	sf          fmt;
	sfstorage   fmt_storage;
	sfupdate    fmt_update;
	srscheme    scheme;
};

void si_schemeinit(sischeme*);
void si_schemefree(sischeme*, sr*);
int  si_schemedeploy(sischeme*, sr*);
int  si_schemerecover(sischeme*, sr*);

#endif
#line 1 "sophia/index/si_branch.h"
#ifndef SI_BRANCH_H_
#define SI_BRANCH_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sibranch sibranch;

struct sibranch {
	sdid id;
	sdindex index;
	sibranch *next;
};

static inline void
si_branchinit(sibranch *b) {
	memset(&b->id, 0, sizeof(b->id));
	sd_indexinit(&b->index);
	b->next = NULL;
}

static inline sibranch*
si_branchnew(sr *r)
{
	sibranch *b = (sibranch*)ss_malloc(r->a, sizeof(sibranch));
	if (ssunlikely(b == NULL)) {
		sr_oom_malfunction(r->e);
		return NULL;
	}
	si_branchinit(b);
	return b;
}

static inline void
si_branchset(sibranch *b, sdindex *i)
{
	b->id = i->h->id;
	b->index = *i;
}

static inline void
si_branchfree(sibranch *b, sr *r)
{
	sd_indexfree(&b->index, r);
	ss_free(r->a, b);
}

#endif
#line 1 "sophia/index/si_node.h"
#ifndef SI_NODE_H_
#define SI_NODE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sinode sinode;

#define SI_NONE       0
#define SI_LOCK       1
#define SI_ROTATE     2

#define SI_RDB        16
#define SI_RDB_DBI    32
#define SI_RDB_DBSEAL 64
#define SI_RDB_UNDEF  128
#define SI_RDB_REMOVE 256

struct sinode {
	uint32_t  recover;
	uint8_t   flags;
	uint64_t  update_time;
	uint32_t  used;
	uint32_t  backup;
	sibranch  self;
	sibranch *branch;
	uint32_t  branch_count;
	svindex   i0, i1;
	ssfile    file;
	ssmmap    map, map_swap;
	ssrbnode  node;
	ssrqnode  nodecompact;
	ssrqnode  nodebranch;
	sslist    commit;
} sspacked;

sinode *si_nodenew(sr*);
int si_nodeopen(sinode*, sr*, sischeme*, sspath*);
int si_nodecreate(sinode*, sr*, sischeme*, sdid*, sdindex*, sdbuild*);
int si_nodefree(sinode*, sr*, int);
int si_nodegc_index(sr*, svindex*);
int si_nodemap(sinode*, sr*);
int si_nodesync(sinode*, sr*);
int si_nodeseal(sinode*, sr*, sischeme*);
int si_nodecomplete(sinode*, sr*, sischeme*);

static inline void
si_nodelock(sinode *node) {
	assert(! (node->flags & SI_LOCK));
	node->flags |= SI_LOCK;
}

static inline void
si_nodeunlock(sinode *node) {
	assert((node->flags & SI_LOCK) > 0);
	node->flags &= ~SI_LOCK;
}

static inline svindex*
si_noderotate(sinode *node) {
	node->flags |= SI_ROTATE;
	return &node->i0;
}

static inline void
si_nodeunrotate(sinode *node) {
	assert((node->flags & SI_ROTATE) > 0);
	node->flags &= ~SI_ROTATE;
	node->i0 = node->i1;
	sv_indexinit(&node->i1);
}

static inline svindex*
si_nodeindex(sinode *node) {
	if (node->flags & SI_ROTATE)
		return &node->i1;
	return &node->i0;
}

static inline svindex*
si_nodeindex_priority(sinode *node, svindex **second)
{
	if (ssunlikely(node->flags & SI_ROTATE)) {
		*second = &node->i0;
		return &node->i1;
	}
	*second = NULL;
	return &node->i0;
}

static inline sinode*
si_nodeof(ssrbnode *node) {
	return sscast(node, sinode, node);
}

static inline int
si_nodecmp(sinode *n, void *key, int size, srscheme *s)
{
	sdindexpage *min = sd_indexmin(&n->self.index);
	sdindexpage *max = sd_indexmax(&n->self.index);
	int l = sr_compare(s, sd_indexpage_min(&n->self.index, min),
	                   min->sizemin, key, size);
	int r = sr_compare(s, sd_indexpage_max(&n->self.index, max),
	                   max->sizemax, key, size);
	/* inside range */
	if (l <= 0 && r >= 0)
		return 0;
	/* key > range */
	if (l == -1)
		return -1;
	/* key < range */
	assert(r == 1);
	return 1;
}

#endif
#line 1 "sophia/index/si_planner.h"
#ifndef SI_PLANNER_H_
#define SI_PLANNER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct siplanner siplanner;
typedef struct siplan siplan;

struct siplanner {
	void *i;
	ssrq branch;
	ssrq compact;
};

/* plan */
#define SI_BRANCH        1
#define SI_AGE           2
#define SI_COMPACT       4
#define SI_CHECKPOINT    8
#define SI_GC            16
#define SI_BACKUP        32
#define SI_BACKUPEND     64
#define SI_SHUTDOWN      128
#define SI_DROP          256

/* explain */
#define SI_ENONE         0
#define SI_ERETRY        1
#define SI_EINDEX_SIZE   2
#define SI_EINDEX_AGE    4
#define SI_EBRANCH_COUNT 3

struct siplan {
	int explain;
	int plan;
	/* branch:
	 *   a: index_size
	 *   b: ttl
	 *   c: ttl_wm
	 * age:
	 *   a: ttl
	 *   b: ttl_wm
	 *   c:
	 * compact:
	 *   a: branches
	 *   b:
	 *   c:
	 * checkpoint:
	 *   a: lsn
	 *   b:
	 *   c:
	 * gc:
	 *   a: lsn
	 *   b: percent
	 *   c:
	 * backup:
	 *   a: bsn
	 *   b:
	 *   c:
	 * shutdown:
	 * drop:
	 */
	uint64_t a, b, c;
	sinode *node;
};

int si_planinit(siplan*);
int si_plannerinit(siplanner*, ssa*, void*);
int si_plannerfree(siplanner*, ssa*);
int si_plannertrace(siplan*, sstrace*);
int si_plannerupdate(siplanner*, int, sinode*);
int si_plannerremove(siplanner*, int, sinode*);
int si_planner(siplanner*, siplan*);

#endif
#line 1 "sophia/index/si.h"
#ifndef SI_H_
#define SI_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct si si;

struct si {
	ssmutex lock;
	siplanner p;
	ssrb i;
	int n;
	int destroyed;
	uint32_t backup;
	uint64_t update_time;
	uint64_t read_disk;
	uint64_t read_cache;
	ssbuf readbuf;
	svupdate u;
	sischeme *scheme;
	sr *r;
};

static inline void
si_lock(si *i) {
	ss_mutexlock(&i->lock);
}

static inline void
si_unlock(si *i) {
	ss_mutexunlock(&i->lock);
}

int si_init(si*, sr*);
int si_open(si*, sischeme*);
int si_close(si*);
int si_insert(si*, sinode*);
int si_remove(si*, sinode*);
int si_replace(si*, sinode*, sinode*);
int si_plan(si*, siplan*);
int si_execute(si*, sdc*, siplan*, uint64_t);

#endif
#line 1 "sophia/index/si_gc.h"
#ifndef SI_GC_H_
#define SI_GC_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

uint32_t si_gcv(ssa*, svv*);

#endif
#line 1 "sophia/index/si_read.h"
#ifndef SI_READ_H_
#define SI_READ_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct siread siread;
typedef struct sireadarg sireadarg;

struct sireadarg {
	sischeme *scheme;
	si       *index;
	sinode   *n;
	sibranch *b;
	ssbuf    *buf;
	ssbuf    *buf_xf;
	ssbuf    *buf_read;
	ssiter   *index_iter;
	ssiter   *page_iter;
	uint64_t  vlsn;
	int       has;
	int       mmap_copy;
	ssorder   o;
	sr       *r;
};

struct siread {
	sireadarg ra;
	sdindexpage *ref;
	sdpage page;
} sspacked;

static inline int
si_read_page(siread *i, sdindexpage *ref)
{
	sireadarg *arg = &i->ra;
	sr *r = arg->r;

	arg->index->read_disk++;

	ss_bufreset(arg->buf_xf);
	int rc = ss_bufensure(arg->buf_xf, r->a, arg->b->index.h->sizevmax);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	ss_bufreset(arg->buf);
	rc = ss_bufensure(arg->buf, r->a, ref->sizeorigin);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);

	uint64_t offset =
		arg->b->index.h->offset + sd_indexsize(arg->b->index.h) +
		ref->offset;

	/* compression */
	if (i->ra.scheme->compression)
	{
		char *page_pointer;
		if (! i->ra.scheme->mmap) {
			ss_bufreset(arg->buf_read);
			rc = ss_bufensure(arg->buf_read, r->a, ref->size);
			if (ssunlikely(rc == -1))
				return sr_oom(r->e);
			rc = ss_filepread(&arg->n->file, offset, arg->buf_read->s, ref->size);
			if (ssunlikely(rc == -1)) {
				sr_error(r->e, "db file '%s' read error: %s",
				         arg->n->file.file, strerror(errno));
				return -1;
			}
			ss_bufadvance(arg->buf_read, ref->size);
			page_pointer = arg->buf_read->s;
		} else {
			page_pointer = arg->n->map.p + offset;
		}

		/* copy header */
		memcpy(arg->buf->p, page_pointer, sizeof(sdpageheader));
		ss_bufadvance(arg->buf, sizeof(sdpageheader));

		/* decompression */
		ssfilter f;
		rc = ss_filterinit(&f, (ssfilterif*)r->compression, r->a, SS_FOUTPUT);
		if (ssunlikely(rc == -1)) {
			sr_error(r->e, "db file '%s' decompression error", arg->n->file.file);
			return -1;
		}
		int size = ref->size - sizeof(sdpageheader);
		rc = ss_filternext(&f, arg->buf, page_pointer + sizeof(sdpageheader), size);
		if (ssunlikely(rc == -1)) {
			sr_error(r->e, "db file '%s' decompression error", arg->n->file.file);
			return -1;
		}
		ss_filterfree(&f);
		sd_pageinit(&i->page, (sdpageheader*)arg->buf->s);
		return 0;
	}

	/* mmap */
	if (i->ra.scheme->mmap) {
		if (i->ra.mmap_copy) {
			memcpy(arg->buf->s, arg->n->map.p + offset, ref->sizeorigin);
			sd_pageinit(&i->page, (sdpageheader*)(arg->buf->s));
		} else {
			sd_pageinit(&i->page, (sdpageheader*)(arg->n->map.p + offset));
		}
		return 0;
	}

	/* default */
	rc = ss_filepread(&arg->n->file, offset, arg->buf->s, ref->sizeorigin);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "db file '%s' read error: %s",
		         arg->n->file.file, strerror(errno));
		return -1;
	}
	ss_bufadvance(arg->buf, ref->sizeorigin);
	sd_pageinit(&i->page, (sdpageheader*)(arg->buf->s));
	return 0;
}

static inline int
si_read_openpage(siread *i, void *key, int keysize)
{
	sireadarg *arg = &i->ra;
	assert(i->ref != NULL);
	int rc = si_read_page(i, i->ref);
	if (ssunlikely(rc == -1))
		return -1;
	ss_iterinit(sd_pageiter, arg->page_iter);
	return ss_iteropen(sd_pageiter, arg->page_iter, arg->r,
	                   arg->buf_xf,
	                   &i->page, arg->o, key, keysize);
}

static inline void
si_read_next(ssiter*);

static inline int
si_read_open(ssiter *iptr, sireadarg *arg, void *key, int keysize)
{
	siread *i = (siread*)iptr->priv;
	i->ra = *arg;
	ss_iterinit(sd_indexiter, arg->index_iter);
	ss_iteropen(sd_indexiter, arg->index_iter, arg->r, &arg->b->index,
	            arg->o, key, keysize);
	i->ref = ss_iterof(sd_indexiter, arg->index_iter);
	if (i->ref == NULL)
		return 0;
	if (ssunlikely(arg->has)) {
		assert(arg->o == SS_GTE);
		if (i->ref->lsnmax <= arg->vlsn) {
			i->ref = NULL;
			return 0;
		}
	}
	int rc = si_read_openpage(i, key, keysize);
	if (ssunlikely(rc == -1)) {
		i->ref = NULL;
		return -1;
	}
	if (ssunlikely(! ss_iterhas(sd_pageiter, i->ra.page_iter))) {
		si_read_next(iptr);
		rc = 0;
	}
	return rc;
}

static inline void
si_read_close(ssiter *iptr)
{
	siread *i = (siread*)iptr->priv;
	i->ref = NULL;
}

static inline int
si_read_has(ssiter *iptr)
{
	siread *i = (siread*)iptr->priv;
	if (ssunlikely(i->ref == NULL))
		return 0;
	return ss_iterhas(sd_pageiter, i->ra.page_iter);
}

static inline void*
si_read_of(ssiter *iptr)
{
	siread *i = (siread*)iptr->priv;
	if (ssunlikely(i->ref == NULL))
		return NULL;
	return ss_iterof(sd_pageiter, i->ra.page_iter);
}

static inline void
si_read_next(ssiter *iptr)
{
	siread *i = (siread*)iptr->priv;
	if (ssunlikely(i->ref == NULL))
		return;
	ss_iternext(sd_pageiter, i->ra.page_iter);
retry:
	if (sslikely(ss_iterhas(sd_pageiter, i->ra.page_iter)))
		return;
	ss_iternext(sd_indexiter, i->ra.index_iter);
	i->ref = ss_iterof(sd_indexiter, i->ra.index_iter);
	if (i->ref == NULL)
		return;
	int rc = si_read_openpage(i, NULL, 0);
	if (ssunlikely(rc == -1)) {
		i->ref = NULL;
		return;
	}
	goto retry;
}

extern ssiterif si_read;

#endif
#line 1 "sophia/index/si_write.h"
#ifndef SI_WRITE_H_
#define SI_WRITE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sitx sitx;

struct sitx {
	uint64_t time;
	uint64_t vlsn;
	sslist nodelist;
	svlog *l;
	svlogindex *li;
	si *index;
};

void si_begin(sitx*, si*, uint64_t, uint64_t,
              svlog*, svlogindex*);
void si_commit(sitx*);
void si_write(sitx*, int);

#endif
#line 1 "sophia/index/si_cache.h"
#ifndef SI_CACHE_H_
#define SI_CACHE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sicachebranch sicachebranch;
typedef struct sicache sicache;
typedef struct sicachepool sicachepool;

struct sicachebranch {
	sibranch *branch;
	sdindexpage *ref;
	sdpage page;
	ssiter i;
	ssiter page_iter;
	ssiter index_iter;
	ssbuf buf_a;
	ssbuf buf_b;
	int open;
	sicachebranch *next;
} sspacked;

struct sicache {
	sicachebranch *path;
	sicachebranch *branch;
	uint32_t count;
	uint32_t nodeid;
	sinode *node;
	sicache *next;
	sicachepool *pool;
};

struct sicachepool {
	sicache *head;
	int n;
	ssa *ac;
	ssa *acb;
};

static inline void
si_cacheinit(sicache *c, sicachepool *pool)
{
	c->path   = NULL;
	c->branch = NULL;
	c->count  = 0;
	c->node   = NULL;
	c->nodeid = 0;
	c->next   = NULL;
	c->pool   = pool;
}

static inline void
si_cachefree(sicache *c, sr *r)
{
	sicachebranch *next;
	sicachebranch *cb = c->path;
	while (cb) {
		next = cb->next;
		ss_buffree(&cb->buf_a, r->a);
		ss_buffree(&cb->buf_b, r->a);
		ss_free(c->pool->acb, cb);
		cb = next;
	}
}

static inline void
si_cachereset(sicache *c)
{
	sicachebranch *cb = c->path;
	while (cb) {
		ss_bufreset(&cb->buf_a);
		ss_bufreset(&cb->buf_b);
		cb->branch = NULL;
		cb->ref = NULL;
		ss_iterclose(si_read, &cb->i);
		cb->open = 0;
		cb = cb->next;
	}
	c->branch = NULL;
	c->node   = NULL;
	c->nodeid = 0;
	c->count  = 0;
}

static inline sicachebranch*
si_cacheadd(sicache *c, sibranch *b)
{
	sicachebranch *nb = ss_malloc(c->pool->acb, sizeof(sicachebranch));
	if (ssunlikely(nb == NULL))
		return NULL;
	nb->branch  = b;
	nb->ref     = NULL;
	memset(&nb->i, 0, sizeof(nb->i));
	ss_iterinit(si_read, &nb->i);
	nb->open    = 0;
	nb->next    = NULL;
	ss_bufinit(&nb->buf_a);
	ss_bufinit(&nb->buf_b);
	return nb;
}

static inline int
si_cachevalidate(sicache *c, sinode *n)
{
	if (sslikely(c->node == n && c->nodeid == n->self.id.id))
	{
		if (sslikely(n->branch_count == c->count)) {
			c->branch = c->path;
			return 0;
		}
		assert(n->branch_count > c->count);
		/* c b a */
		/* e d c b a */
		sicachebranch *head = NULL;
		sicachebranch *last = NULL;
		sicachebranch *cb = c->path;
		sibranch *b = n->branch;
		while (b) {
			if (cb->branch == b) {
				assert(last != NULL);
				last->next = cb;
				break;
			}
			sicachebranch *nb = si_cacheadd(c, b);
			if (ssunlikely(nb == NULL))
				return -1;
			if (! head)
				head = nb;
			if (last)
				last->next = nb;
			last = nb;
			b = b->next;
		}
		c->path   = head;
		c->count  = n->branch_count;
		c->branch = c->path;
		return 0;
	}
	sicachebranch *last = c->path;
	sicachebranch *cb = last;
	sibranch *b = n->branch;
	while (cb && b) {
		cb->branch = b;
		cb->ref = NULL;
		cb->open = 0;
		ss_iterclose(si_read, &cb->i);
		ss_bufreset(&cb->buf_a);
		ss_bufreset(&cb->buf_b);
		last = cb;
		cb = cb->next;
		b  = b->next;
	}
	while (b) {
		cb = si_cacheadd(c, b);
		if (ssunlikely(cb == NULL))
			return -1;
		if (last)
			last->next = cb;
		last = cb;
		if (c->path == NULL)
			c->path = cb;
		b = b->next;
	}
	c->count  = n->branch_count;
	c->node   = n;
	c->nodeid = n->self.id.id;
	c->branch = c->path;
	return 0;
}

static inline sicachebranch*
si_cachefollow(sicache *c)
{
	sicachebranch *b = c->branch;
	c->branch = c->branch->next;
	return b;
}

static inline void
si_cachepool_init(sicachepool *p, ssa *ac, ssa *acb)
{
	p->head = NULL;
	p->n    = 0;
	p->ac   = ac;
	p->acb  = acb;
}

static inline void
si_cachepool_free(sicachepool *p, sr *r)
{
	sicache *next;
	sicache *c = p->head;
	while (c) {
		next = c->next;
		si_cachefree(c, r);
		c = next;
	}
}

static inline sicache*
si_cachepool_pop(sicachepool *p)
{
	sicache *c;
	if (sslikely(p->n > 0)) {
		c = p->head;
		p->head = c->next;
		p->n--;
		si_cachereset(c);
		c->pool = p;
		return c;
	}
	c = ss_malloc(p->ac, sizeof(sicache));
	if (ssunlikely(c == NULL))
		return NULL;
	si_cacheinit(c, p);
	return c;
}

static inline void
si_cachepool_push(sicache *c)
{
	sicachepool *p = c->pool;
	c->next = p->head;
	p->head = c;
	p->n++;
}

#endif
#line 1 "sophia/index/si_query.h"
#ifndef SI_QUERY_H_
#define SI_QUERY_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct siquery siquery;

struct siquery {
	ssorder order;
	void *prefix;
	void *key;
	uint32_t keysize;
	uint32_t prefixsize;
	uint64_t vlsn;
	svmerge merge;
	int has;
	sv *update_v;
	int update_eq;
	sv result;
	sicache *cache;
	sr *r;
	si *index;
};

int  si_queryopen(siquery*, sicache*, si*, ssorder,
                  uint64_t,
                  void*, uint32_t,
                  void*, uint32_t);
int  si_queryclose(siquery*);
void si_queryhas(siquery*);
void si_queryupdate(siquery*, sv*, int);
int  si_query(siquery*);
int  si_querycommited(si*, sr*, sv*);

#endif
#line 1 "sophia/index/si_iter.h"
#ifndef SI_ITER_H_
#define SI_ITER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct siiter siiter;

struct siiter {
	si *index;
	ssrbnode *v;
	ssorder order;
	void *key;
	int keysize;
} sspacked;

ss_rbget(si_itermatch,
         si_nodecmp(sscast(n, sinode, node), key, keysize, scheme))

static inline int
si_iter_open(ssiter *i, sr *r, si *index, ssorder o, void *key, int keysize)
{
	siiter *ii = (siiter*)i->priv;
	ii->index   = index;
	ii->order   = o;
	ii->key     = key;
	ii->keysize = keysize;
	ii->v       = NULL;
	int eq = 0;
	if (ssunlikely(ii->index->n == 1)) {
		ii->v = ss_rbmin(&ii->index->i);
		return 1;
	}
	if (ssunlikely(ii->key == NULL)) {
		switch (ii->order) {
		case SS_LT:
		case SS_LTE:
			ii->v = ss_rbmax(&ii->index->i);
			break;
		case SS_GT:
		case SS_GTE:
			ii->v = ss_rbmin(&ii->index->i);
			break;
		default:
			assert(0);
			break;
		}
		return 0;
	}
	/* route */
	assert(ii->key != NULL);
	int rc;
	rc = si_itermatch(&ii->index->i, r->scheme, ii->key, ii->keysize, &ii->v);
	if (ssunlikely(ii->v == NULL)) {
		assert(rc != 0);
		if (rc == 1)
			ii->v = ss_rbmin(&ii->index->i);
		else
			ii->v = ss_rbmax(&ii->index->i);
	} else {
		eq = rc == 0 && ii->v;
		if (rc == 1) {
			ii->v = ss_rbprev(&ii->index->i, ii->v);
			if (ssunlikely(ii->v == NULL))
				ii->v = ss_rbmin(&ii->index->i);
		}
	}
	assert(ii->v != NULL);
	return eq;
}

static inline void
si_iter_close(ssiter *i ssunused)
{ }

static inline int
si_iter_has(ssiter *i)
{
	siiter *ii = (siiter*)i->priv;
	return ii->v != NULL;
}

static inline void*
si_iter_of(ssiter *i)
{
	siiter *ii = (siiter*)i->priv;
	if (ssunlikely(ii->v == NULL))
		return NULL;
	sinode *n = si_nodeof(ii->v);
	return n;
}

static inline void
si_iter_next(ssiter *i)
{
	siiter *ii = (siiter*)i->priv;
	switch (ii->order) {
	case SS_LT:
	case SS_LTE:
		ii->v = ss_rbprev(&ii->index->i, ii->v);
		break;
	case SS_GT:
	case SS_GTE:
		ii->v = ss_rbnext(&ii->index->i, ii->v);
		break;
	default: assert(0);
	}
}

extern ssiterif si_iter;

#endif
#line 1 "sophia/index/si_drop.h"
#ifndef SI_DROP_H_
#define SI_DROP_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int si_drop(si*);
int si_dropmark(si*);
int si_droprepository(sischeme*, sr*, int);

#endif
#line 1 "sophia/index/si_backup.h"
#ifndef SI_BACKUP_H_
#define SI_BACKUP_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int si_backup(si*, sdc*, siplan*);

#endif
#line 1 "sophia/index/si_balance.h"
#ifndef SI_BALANCE_H_
#define SI_BALANCE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int si_branch(si*, sdc*, siplan*, uint64_t);
int si_compact(si*, sdc*, siplan*, uint64_t);

#endif
#line 1 "sophia/index/si_compaction.h"
#ifndef SI_COMPACTION_H_
#define SI_COMPACTION_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int si_compaction(si*, sdc*, uint64_t, sinode*, ssiter*, uint32_t);

#endif
#line 1 "sophia/index/si_track.h"
#ifndef SI_TRACK_H_
#define SI_TRACK_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sitrack sitrack;

struct sitrack {
	ssrb i;
	int count;
	uint32_t nsn;
	uint64_t lsn;
};

static inline void
si_trackinit(sitrack *t) {
	ss_rbinit(&t->i);
	t->count = 0;
	t->nsn = 0;
	t->lsn = 0;
}

ss_rbtruncate(si_tracktruncate,
              si_nodefree(sscast(n, sinode, node), (sr*)arg, 0))

static inline void
si_trackfree(sitrack *t, sr *r) {
	if (t->i.root)
		si_tracktruncate(t->i.root, r);
}

static inline void
si_trackmetrics(sitrack *t, sinode *n)
{
	sibranch *b = n->branch;
	while (b) {
		sdindexheader *h = b->index.h;
		if (b->id.parent > t->nsn)
			t->nsn = b->id.parent;
		if (b->id.id > t->nsn)
			t->nsn = b->id.id;
		if (h->lsnmin != UINT64_MAX && h->lsnmin > t->lsn)
			t->lsn = h->lsnmin;
		if (h->lsnmax > t->lsn)
			t->lsn = h->lsnmax;
		b = b->next;
	}
}

static inline void
si_tracknsn(sitrack *t, uint32_t nsn)
{
	if (t->nsn < nsn)
		t->nsn = nsn;
}

ss_rbget(si_trackmatch, ss_cmp((sscast(n, sinode, node))->self.id.id, *(uint32_t*)key))

static inline void
si_trackset(sitrack *t, sinode *n)
{
	ssrbnode *p = NULL;
	int rc = si_trackmatch(&t->i, NULL, (char*)&n->self.id.id,
	                       sizeof(n->self.id.id), &p);
	assert(! (rc == 0 && p));
	ss_rbset(&t->i, p, rc, &n->node);
	t->count++;
}

static inline sinode*
si_trackget(sitrack *t, uint32_t id)
{
	ssrbnode *p = NULL;
	int rc = si_trackmatch(&t->i, NULL, (char*)&id, sizeof(id), &p);
	if (rc == 0 && p)
		return sscast(p, sinode, node);
	return NULL;
}

static inline void
si_trackreplace(sitrack *t, sinode *o, sinode *n)
{
	ss_rbreplace(&t->i, &o->node, &n->node);
}

static inline void
si_trackremove(sitrack *t, sinode *n)
{
	ss_rbremove(&t->i, &n->node);
	t->count--;
}

#endif
#line 1 "sophia/index/si_recover.h"
#ifndef SI_RECOVER_H_
#define SI_RECOVER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

sinode *si_bootstrap(si*, uint32_t);
int si_recover(si*);

#endif
#line 1 "sophia/index/si_profiler.h"
#ifndef SI_PROFILER_H_
#define SI_PROFILER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct siprofiler siprofiler;

struct siprofiler {
	si *i;
	uint32_t  total_node_count;
	uint64_t  total_node_size;
	uint64_t  total_node_origin_size;
	uint32_t  total_branch_count;
	uint32_t  total_branch_avg;
	uint32_t  total_branch_max;
	uint32_t  total_page_count;
	uint64_t  memory_used;
	uint64_t  count;
	uint64_t  count_dup;
	uint64_t  read_disk;
	uint64_t  read_cache;
	int       histogram_branch[20];
	int       histogram_branch_20plus;
	char      histogram_branch_sz[512];
	char     *histogram_branch_ptr;
} sspacked;

int si_profilerbegin(siprofiler*, si*);
int si_profilerend(siprofiler*);
int si_profiler(siprofiler*);

#endif
#line 1 "sophia/index/si_backup.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








static inline int
si_backupend(si *index, sdc *c, siplan *plan)
{
	sr *r = index->r;
	/* copy index scheme file */
	char src[PATH_MAX];
	snprintf(src, sizeof(src), "%s/scheme", index->scheme->path);

	char dst[PATH_MAX];
	snprintf(dst, sizeof(dst), "%s/%" PRIu32 ".incomplete/%s/scheme",
	         index->scheme->path_backup,
	         (uint32_t)plan->a,
	         index->scheme->name);

	/* prepare buffer */
	ssize_t size = ss_filesize(src);
	if (ssunlikely(size == -1)) {
		sr_error(r->e, "backup db file '%s' read error: %s",
		         src, strerror(errno));
		return -1;
	}
	int rc = ss_bufensure(&c->c, r->a, size);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);

	/* read scheme file */
	ssfile file;
	ss_fileinit(&file, r->a);
	rc = ss_fileopen(&file, src);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' open error: %s",
		         src, strerror(errno));
		return -1;
	}
	rc = ss_filepread(&file, 0, c->c.s, size);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' read error: %s",
		         src, strerror(errno));
		ss_fileclose(&file);
		return -1;
	}
	ss_fileclose(&file);

	/* write scheme file */
	rc = ss_filenew(&file, dst);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' create error: %s",
		         dst, strerror(errno));
		return -1;
	}
	rc = ss_filewrite(&file, c->c.s, size);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' write error: %s",
		         dst, strerror(errno));
		ss_fileclose(&file);
		return -1;
	}
	/* sync? */
	rc = ss_fileclose(&file);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' close error: %s",
		         dst, strerror(errno));
		return -1;
	}

	/* finish index backup */
	si_lock(index);
	index->backup = plan->a;
	si_unlock(index);
	return 0;
}

int si_backup(si *index, sdc *c, siplan *plan)
{
	sr *r = index->r;
	sd_creset(c);
	if (ssunlikely(plan->plan == SI_BACKUPEND))
		return si_backupend(index, c, plan);

	sinode *node = plan->node;
	char dst[PATH_MAX];
	snprintf(dst, sizeof(dst), "%s/%" PRIu32 ".incomplete/%s",
	         index->scheme->path_backup,
	         (uint32_t)plan->a,
	         index->scheme->name);

	/* read origin file */
	int rc = ss_bufensure(&c->c, r->a, node->file.size);
	if (ssunlikely(rc == -1))
		return sr_oom(r->e);
	rc = ss_filepread(&node->file, 0, c->c.s, node->file.size);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "db file '%s' read error: %s",
		         node->file.file, strerror(errno));
		return -1;
	}
	ss_bufadvance(&c->c, node->file.size);

	/* copy */
	sspath path;
	ss_pathA(&path, dst, node->self.id.id, ".db");
	ssfile file;
	ss_fileinit(&file, r->a);
	rc = ss_filenew(&file, path.path);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' create error: %s",
		         path.path, strerror(errno));
		return -1;
	}
	rc = ss_filewrite(&file, c->c.s, node->file.size);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' write error: %s",
				 path.path, strerror(errno));
		ss_fileclose(&file);
		return -1;
	}
	/* sync? */
	rc = ss_fileclose(&file);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "backup db file '%s' close error: %s",
				 path.path, strerror(errno));
		return -1;
	}

	si_lock(index);
	node->backup = plan->a;
	si_nodeunlock(node);
	si_unlock(index);
	return 0;
}
#line 1 "sophia/index/si_balance.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








static inline sibranch*
si_branchcreate(si *index, sdc *c, sinode *parent, svindex *vindex, uint64_t vlsn)
{
	sr *r = index->r;
	svmerge vmerge;
	sv_mergeinit(&vmerge);
	int rc = sv_mergeprepare(&vmerge, r, 1);
	if (ssunlikely(rc == -1))
		return NULL;
	svmergesrc *s = sv_mergeadd(&vmerge, NULL);
	ss_iterinit(sv_indexiter, &s->src);
	ss_iteropen(sv_indexiter, &s->src, r, vindex, SS_GTE, NULL, 0);
	ssiter i;
	ss_iterinit(sv_mergeiter, &i);
	ss_iteropen(sv_mergeiter, &i, r, &vmerge, SS_GTE);

	/* merge iter is not used */
	sdmergeconf mergeconf = {
		.size_stream     = UINT32_MAX,
		.size_node       = UINT64_MAX,
		.size_page       = index->scheme->node_page_size,
		.checksum        = index->scheme->node_page_checksum,
		.compression     = index->scheme->compression,
		.compression_key = index->scheme->compression_key,
		.offset          = parent->file.size,
		.vlsn            = vlsn,
		.save_delete     = 1,
		.save_update     = 1
	};
	sdmerge merge;
	sd_mergeinit(&merge, r, &i, &c->build, &c->update, &mergeconf);
	rc = sd_merge(&merge);
	if (ssunlikely(rc == -1)) {
		sv_mergefree(&vmerge, r->a);
		sr_oom_malfunction(r->e);
		goto error;
	}
	assert(rc == 1);
	sv_mergefree(&vmerge, r->a);

	sibranch *branch = si_branchnew(r);
	if (ssunlikely(branch == NULL))
		goto error;
	sdid id = {
		.parent = parent->self.id.id,
		.flags  = SD_IDBRANCH,
		.id     = sr_seq(r->seq, SR_NSNNEXT)
	};
	rc = sd_mergecommit(&merge, &id);
	if (ssunlikely(rc == -1))
		goto error;

	si_branchset(branch, &merge.index);
	rc = sd_commit(&c->build, r, &branch->index, &parent->file);
	if (ssunlikely(rc == -1)) {
		si_branchfree(branch, r);
		return NULL;
	}

	SS_INJECTION(r->i, SS_INJECTION_SI_BRANCH_0,
	             sr_malfunction(r->e, "%s", "error injection");
	             si_branchfree(branch, r);
	             return NULL);

	if (index->scheme->sync) {
		rc = si_nodesync(parent, r);
		if (ssunlikely(rc == -1)) {
			si_branchfree(branch, r);
			return NULL;
		}
	}
	if (index->scheme->mmap) {
		ss_mmapinit(&parent->map_swap);
		rc = ss_mmap(&parent->map_swap, parent->file.fd,
		              parent->file.size, 1);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(r->e, "db file '%s' mmap error: %s",
			               parent->file.file, strerror(errno));
			return NULL;
		}
	}
	return branch;
error:
	sd_mergefree(&merge);
	return NULL;
}

int si_branch(si *index, sdc *c, siplan *plan, uint64_t vlsn)
{
	sr *r = index->r;
	sinode *n = plan->node;
	assert(n->flags & SI_LOCK);

	si_lock(index);
	if (ssunlikely(n->used == 0)) {
		si_nodeunlock(n);
		si_unlock(index);
		return 0;
	}
	svindex *i;
	i = si_noderotate(n);
	si_unlock(index);

	sd_creset(c);
	sibranch *branch = si_branchcreate(index, c, n, i, vlsn);
	if (ssunlikely(branch == NULL))
		return -1;

	/* commit */
	si_lock(index);
	branch->next = n->branch;
	n->branch = branch;
	n->branch_count++;
	uint32_t used = sv_indexused(i);
	n->used -= used;
	ss_quota(r->quota, SS_QREMOVE, used);
	svindex swap = *i;
	si_nodeunrotate(n);
	si_nodeunlock(n);
	si_plannerupdate(&index->p, SI_BRANCH|SI_COMPACT, n);
	ssmmap swap_map = n->map;
	n->map = n->map_swap;
	memset(&n->map_swap, 0, sizeof(n->map_swap));
	si_unlock(index);

	/* gc */
	if (index->scheme->mmap) {
		int rc = ss_munmap(&swap_map);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(r->e, "db file '%s' munmap error: %s",
			               n->file.file, strerror(errno));
			return -1;
		}
	}
	si_nodegc_index(r, &swap);
	return 1;
}

static inline char*
si_noderead(si *index, ssbuf *dest, sinode *node)
{
	sr *r = index->r;
	if (index->scheme->mmap) {
		return node->map.p;
	}
	int rc = ss_bufensure(dest, r->a, node->file.size);
	if (ssunlikely(rc == -1)) {
		sr_oom_malfunction(r->e);
		return NULL;
	}
	rc = ss_filepread(&node->file, 0, dest->s, node->file.size);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' read error: %s",
		               node->file.file, strerror(errno));
		return NULL;
	}
	ss_bufadvance(dest, node->file.size);
	return dest->s;
}

int si_compact(si *index, sdc *c, siplan *plan, uint64_t vlsn)
{
	sr *r = index->r;
	sinode *node = plan->node;
	assert(node->flags & SI_LOCK);

	/* read node file */
	sd_creset(c);
	char *node_file = si_noderead(index, &c->c, node);
	if (ssunlikely(node_file == NULL))
		return -1;

	/* prepare for compaction */
	int rc;
	rc = sd_censure(c, r, node->branch_count);
	if (ssunlikely(rc == -1))
		return sr_oom_malfunction(r->e);
	svmerge merge;
	sv_mergeinit(&merge);
	rc = sv_mergeprepare(&merge, r, node->branch_count);
	if (ssunlikely(rc == -1))
		return -1;
	uint32_t size_stream = 0;
	sdcbuf *cbuf = c->head;
	sibranch *b = node->branch;
	while (b) {
		svmergesrc *s = sv_mergeadd(&merge, NULL);
		rc = ss_bufensure(&cbuf->b, r->a, b->index.h->sizevmax);
		if (ssunlikely(rc == -1))
			return sr_oom_malfunction(r->e);
		size_stream += sd_indextotal(&b->index);
		ss_iterinit(sd_iter, &s->src);
		ss_iteropen(sd_iter, &s->src, r, &b->index, node_file, 0,
		            index->scheme->compression, &cbuf->a, &cbuf->b);
		cbuf = cbuf->next;
		b = b->next;
	}
	ssiter i;
	ss_iterinit(sv_mergeiter, &i);
	ss_iteropen(sv_mergeiter, &i, r, &merge, SS_GTE);
	rc = si_compaction(index, c, vlsn, node, &i, size_stream);
	sv_mergefree(&merge, r->a);
	return rc;
}
#line 1 "sophia/index/si.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








int si_init(si *i, sr *r)
{
	int rc = si_plannerinit(&i->p, r->a, i);
	if (ssunlikely(rc == -1))
		return -1;
	ss_bufinit(&i->readbuf);
	sv_updateinit(&i->u);
	ss_rbinit(&i->i);
	ss_mutexinit(&i->lock);
	i->scheme      = NULL;
	i->update_time = 0;
	i->read_disk   = 0;
	i->read_cache  = 0;
	i->backup      = 0;
	i->destroyed   = 0;
	i->r           = r;
	return 0;
}

int si_open(si *i, sischeme *scheme)
{
	i->scheme = scheme;
	return si_recover(i);
}

ss_rbtruncate(si_truncate,
              si_nodefree(sscast(n, sinode, node), (sr*)arg, 0))

int si_close(si *i)
{
	if (i->destroyed)
		return 0;
	int rcret = 0;
	if (i->i.root)
		si_truncate(i->i.root, i->r);
	i->i.root = NULL;
	sv_updatefree(&i->u, i->r);
	ss_buffree(&i->readbuf, i->r->a);
	si_plannerfree(&i->p, i->r->a);
	ss_mutexfree(&i->lock);
	i->destroyed = 1;
	return rcret;
}

ss_rbget(si_match,
         sr_compare(scheme,
                    sd_indexpage_min(&(sscast(n, sinode, node))->self.index,
                                     sd_indexmin(&(sscast(n, sinode, node))->self.index)),
                    sd_indexmin(&(sscast(n, sinode, node))->self.index)->sizemin,
                                key, keysize))

int si_insert(si *i, sinode *n)
{
	sdindexpage *min = sd_indexmin(&n->self.index);
	ssrbnode *p = NULL;
	int rc = si_match(&i->i, i->r->scheme,
	                  sd_indexpage_min(&n->self.index, min),
	                  min->sizemin, &p);
	assert(! (rc == 0 && p));
	ss_rbset(&i->i, p, rc, &n->node);
	i->n++;
	return 0;
}

int si_remove(si *i, sinode *n)
{
	ss_rbremove(&i->i, &n->node);
	i->n--;
	return 0;
}

int si_replace(si *i, sinode *o, sinode *n)
{
	ss_rbreplace(&i->i, &o->node, &n->node);
	return 0;
}

int si_plan(si *i, siplan *plan)
{
	si_lock(i);
	int rc = si_planner(&i->p, plan);
	si_unlock(i);
	return rc;
}

int si_execute(si *i, sdc *c, siplan *plan, uint64_t vlsn)
{
	int rc = -1;
	switch (plan->plan) {
	case SI_CHECKPOINT:
	case SI_BRANCH:
	case SI_AGE:
		rc = si_branch(i, c, plan, vlsn);
		break;
	case SI_GC:
	case SI_COMPACT:
		rc = si_compact(i, c, plan, vlsn);
		break;
	case SI_BACKUP:
	case SI_BACKUPEND:
		rc = si_backup(i, c, plan);
		break;
	case SI_SHUTDOWN:
		rc = si_close(i);
		break;
	case SI_DROP:
		rc = si_drop(i);
		break;
	}
	return rc;
}
#line 1 "sophia/index/si_compaction.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








static int
si_redistribute(si *index, sr *r, sdc *c, sinode *node, ssbuf *result)
{
	(void)index;
	svindex *vindex = si_nodeindex(node);
	ssiter i;
	ss_iterinit(sv_indexiter, &i);
	ss_iteropen(sv_indexiter, &i, r, vindex, SS_GTE, NULL, 0);
	while (ss_iterhas(sv_indexiter, &i))
	{
		sv *v = ss_iterof(sv_indexiter, &i);
		int rc = ss_bufadd(&c->b, r->a, &v->v, sizeof(svv**));
		if (ssunlikely(rc == -1))
			return sr_oom_malfunction(r->e);
		ss_iternext(sv_indexiter, &i);
	}
	if (ssunlikely(ss_bufused(&c->b) == 0))
		return 0;
	ss_iterinit(ss_bufiterref, &i);
	ss_iteropen(ss_bufiterref, &i, &c->b, sizeof(svv*));
	ssiter j;
	ss_iterinit(ss_bufiterref, &j);
	ss_iteropen(ss_bufiterref, &j, result, sizeof(sinode*));
	sinode *prev = ss_iterof(ss_bufiterref, &j);
	ss_iternext(ss_bufiterref, &j);
	while (1)
	{
		sinode *p = ss_iterof(ss_bufiterref, &j);
		if (p == NULL) {
			assert(prev != NULL);
			while (ss_iterhas(ss_bufiterref, &i)) {
				svv *v = ss_iterof(ss_bufiterref, &i);
				v->next = NULL;
				sv_indexset(&prev->i0, r, v);
				ss_iternext(ss_bufiterref, &i);
			}
			break;
		}
		while (ss_iterhas(ss_bufiterref, &i))
		{
			svv *v = ss_iterof(ss_bufiterref, &i);
			v->next = NULL;
			sdindexpage *page = sd_indexmin(&p->self.index);
			int rc = sr_compare(r->scheme, sv_vpointer(v), v->size,
			                    sd_indexpage_min(&p->self.index, page),
			                    page->sizemin);
			if (ssunlikely(rc >= 0))
				break;
			sv_indexset(&prev->i0, r, v);
			ss_iternext(ss_bufiterref, &i);
		}
		if (ssunlikely(! ss_iterhas(ss_bufiterref, &i)))
			break;
		prev = p;
		ss_iternext(ss_bufiterref, &j);
	}
	assert(ss_iterof(ss_bufiterref, &i) == NULL);
	return 0;
}

static inline void
si_redistribute_set(si *index, sr *r, uint64_t now, svv *v)
{
	index->update_time = now;
	/* match node */
	ssiter i;
	ss_iterinit(si_iter, &i);
	ss_iteropen(si_iter, &i, r, index, SS_GTE, sv_vpointer(v), v->size);
	sinode *node = ss_iterof(si_iter, &i);
	assert(node != NULL);
	/* update node */
	svindex *vindex = si_nodeindex(node);
	sv_indexset(vindex, r, v);
	node->update_time = index->update_time;
	node->used += sv_vsize(v);
	/* schedule node */
	si_plannerupdate(&index->p, SI_BRANCH, node);
}

static int
si_redistribute_index(si *index, sr *r, sdc *c, sinode *node)
{
	svindex *vindex = si_nodeindex(node);
	ssiter i;
	ss_iterinit(sv_indexiter, &i);
	ss_iteropen(sv_indexiter, &i, r, vindex, SS_GTE, NULL, 0);
	while (ss_iterhas(sv_indexiter, &i)) {
		sv *v = ss_iterof(sv_indexiter, &i);
		int rc = ss_bufadd(&c->b, r->a, &v->v, sizeof(svv**));
		if (ssunlikely(rc == -1))
			return sr_oom_malfunction(r->e);
		ss_iternext(sv_indexiter, &i);
	}
	if (ssunlikely(ss_bufused(&c->b) == 0))
		return 0;
	uint64_t now = ss_utime();
	ss_iterinit(ss_bufiterref, &i);
	ss_iteropen(ss_bufiterref, &i, &c->b, sizeof(svv*));
	while (ss_iterhas(ss_bufiterref, &i)) {
		svv *v = ss_iterof(ss_bufiterref, &i);
		si_redistribute_set(index, r, now, v);
		ss_iternext(ss_bufiterref, &i);
	}
	return 0;
}

static int
si_splitfree(ssbuf *result, sr *r)
{
	ssiter i;
	ss_iterinit(ss_bufiterref, &i);
	ss_iteropen(ss_bufiterref, &i, result, sizeof(sinode*));
	while (ss_iterhas(ss_bufiterref, &i))
	{
		sinode *p = ss_iterof(ss_bufiterref, &i);
		si_nodefree(p, r, 0);
		ss_iternext(ss_bufiterref, &i);
	}
	return 0;
}

static inline int
si_split(si *index, sdc *c, ssbuf *result,
         sinode   *parent,
         ssiter   *i,
         uint64_t  size_node,
         uint32_t  size_stream,
         uint64_t  vlsn)
{
	sr *r = index->r;
	int count = 0;
	int rc;
	sdmergeconf mergeconf = {
		.size_stream     = size_stream,
		.size_node       = size_node,
		.size_page       = index->scheme->node_page_size,
		.checksum        = index->scheme->node_page_checksum,
		.compression     = index->scheme->compression,
		.compression_key = index->scheme->compression_key,
		.offset          = 0,
		.vlsn            = vlsn,
		.save_delete     = 0,
		.save_update     = 0
	};
	sdmerge merge;
	sd_mergeinit(&merge, r, i, &c->build, &c->update, &mergeconf);
	while ((rc = sd_merge(&merge)) > 0)
	{
		sinode *n = si_nodenew(r);
		if (ssunlikely(n == NULL))
			goto error;
		sdid id = {
			.parent = parent->self.id.id,
			.flags  = 0,
			.id     = sr_seq(index->r->seq, SR_NSNNEXT)
		};
		rc = sd_mergecommit(&merge, &id);
		if (ssunlikely(rc == -1))
			goto error;
		rc = si_nodecreate(n, r, index->scheme, &id, &merge.index, &c->build);
		if (ssunlikely(rc == -1))
			goto error;
		rc = ss_bufadd(result, index->r->a, &n, sizeof(sinode*));
		if (ssunlikely(rc == -1)) {
			sr_oom_malfunction(index->r->e);
			si_nodefree(n, r, 1);
			goto error;
		}
		sd_buildreset(&c->build);
		count++;
	}
	if (ssunlikely(rc == -1))
		goto error;
	return 0;
error:
	si_splitfree(result, r);
	sd_mergefree(&merge);
	return -1;
}

int si_compaction(si *index, sdc *c, uint64_t vlsn,
                  sinode *node,
                  ssiter *stream, uint32_t size_stream)
{
	sr *r = index->r;
	ssbuf *result = &c->a;
	ssiter i;

	/* begin compaction.
	 *
	 * split merge stream into a number
	 * of a new nodes.
	 */
	int rc;
	rc = si_split(index, c, result,
	              node, stream,
	              index->scheme->node_size,
	              size_stream,
	              vlsn);
	if (ssunlikely(rc == -1))
		return -1;

	SS_INJECTION(r->i, SS_INJECTION_SI_COMPACTION_0,
	             si_splitfree(result, r);
	             sr_malfunction(r->e, "%s", "error injection");
	             return -1);

	/* mask removal of a single node as a
	 * single node update */
	int count = ss_bufused(result) / sizeof(sinode*);
	int count_index;

	si_lock(index);
	count_index = index->n;
	si_unlock(index);

	sinode *n;
	if (ssunlikely(count == 0 && count_index == 1))
	{
		n = si_bootstrap(index, node->self.id.id);
		if (ssunlikely(n == NULL))
			return -1;
		rc = ss_bufadd(result, r->a, &n, sizeof(sinode*));
		if (ssunlikely(rc == -1)) {
			sr_oom_malfunction(r->e);
			si_nodefree(n, r, 1);
			return -1;
		}
		count++;
	}

	/* commit compaction changes */
	si_lock(index);
	svindex *j = si_nodeindex(node);
	si_plannerremove(&index->p, SI_COMPACT|SI_BRANCH, node);
	switch (count) {
	case 0: /* delete */
		si_remove(index, node);
		si_redistribute_index(index, r, c, node);
		uint32_t used = sv_indexused(j);
		if (used) {
			ss_quota(r->quota, SS_QREMOVE, used);
		}
		break;
	case 1: /* self update */
		n = *(sinode**)result->s;
		n->i0   = *j;
		n->used = sv_indexused(j);
		si_nodelock(n);
		si_replace(index, node, n);
		si_plannerupdate(&index->p, SI_COMPACT|SI_BRANCH, n);
		break;
	default: /* split */
		rc = si_redistribute(index, r, c, node, result);
		if (ssunlikely(rc == -1)) {
			si_unlock(index);
			si_splitfree(result, r);
			return -1;
		}
		ss_iterinit(ss_bufiterref, &i);
		ss_iteropen(ss_bufiterref, &i, result, sizeof(sinode*));
		n = ss_iterof(ss_bufiterref, &i);
		n->used = sv_indexused(&n->i0);
		si_nodelock(n);
		si_replace(index, node, n);
		si_plannerupdate(&index->p, SI_COMPACT|SI_BRANCH, n);
		for (ss_iternext(ss_bufiterref, &i); ss_iterhas(ss_bufiterref, &i);
		     ss_iternext(ss_bufiterref, &i)) {
			n = ss_iterof(ss_bufiterref, &i);
			n->used = sv_indexused(&n->i0);
			si_nodelock(n);
			si_insert(index, n);
			si_plannerupdate(&index->p, SI_COMPACT|SI_BRANCH, n);
		}
		break;
	}
	sv_indexinit(j);
	si_unlock(index);

	/* compaction completion */

	/* seal nodes */
	ss_iterinit(ss_bufiterref, &i);
	ss_iteropen(ss_bufiterref, &i, result, sizeof(sinode*));
	while (ss_iterhas(ss_bufiterref, &i))
	{
		n = ss_iterof(ss_bufiterref, &i);
		if (index->scheme->sync) {
			rc = si_nodesync(n, r);
			if (ssunlikely(rc == -1))
				return -1;
		}
		rc = si_nodeseal(n, r, index->scheme);
		if (ssunlikely(rc == -1))
			return -1;
		SS_INJECTION(r->i, SS_INJECTION_SI_COMPACTION_3,
		             si_nodefree(node, r, 0);
		             sr_malfunction(r->e, "%s", "error injection");
		             return -1);
		ss_iternext(ss_bufiterref, &i);
	}

	SS_INJECTION(r->i, SS_INJECTION_SI_COMPACTION_1,
	             si_nodefree(node, r, 0);
	             sr_malfunction(r->e, "%s", "error injection");
	             return -1);

	/* gc old node */
	rc = si_nodefree(node, r, 1);
	if (ssunlikely(rc == -1))
		return -1;

	SS_INJECTION(r->i, SS_INJECTION_SI_COMPACTION_2,
	             sr_malfunction(r->e, "%s", "error injection");
	             return -1);

	/* complete new nodes */
	ss_iterinit(ss_bufiterref, &i);
	ss_iteropen(ss_bufiterref, &i, result, sizeof(sinode*));
	while (ss_iterhas(ss_bufiterref, &i))
	{
		n = ss_iterof(ss_bufiterref, &i);
		rc = si_nodecomplete(n, r, index->scheme);
		if (ssunlikely(rc == -1))
			return -1;
		SS_INJECTION(r->i, SS_INJECTION_SI_COMPACTION_4,
		             sr_malfunction(r->e, "%s", "error injection");
		             return -1);
		ss_iternext(ss_bufiterref, &i);
	}

	/* unlock */
	si_lock(index);
	ss_iterinit(ss_bufiterref, &i);
	ss_iteropen(ss_bufiterref, &i, result, sizeof(sinode*));
	while (ss_iterhas(ss_bufiterref, &i))
	{
		n = ss_iterof(ss_bufiterref, &i);
		si_nodeunlock(n);
		ss_iternext(ss_bufiterref, &i);
	}
	si_unlock(index);
	return 0;
}
#line 1 "sophia/index/si_drop.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








int si_droprepository(sischeme *scheme, sr *r, int drop_directory)
{
	DIR *dir = opendir(scheme->path);
	if (dir == NULL) {
		sr_malfunction(r->e, "directory '%s' open error: %s",
		               scheme->path, strerror(errno));
		return -1;
	}
	char path[1024];
	int rc;
	struct dirent *de;
	while ((de = readdir(dir))) {
		if (de->d_name[0] == '.')
			continue;
		/* skip drop file */
		if (ssunlikely(strcmp(de->d_name, "drop") == 0))
			continue;
		snprintf(path, sizeof(path), "%s/%s", scheme->path, de->d_name);
		rc = ss_fileunlink(path);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(r->e, "db file '%s' unlink error: %s",
			               path, strerror(errno));
			closedir(dir);
			return -1;
		}
	}
	closedir(dir);

	snprintf(path, sizeof(path), "%s/drop", scheme->path);
	rc = ss_fileunlink(path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' unlink error: %s",
		               path, strerror(errno));
		return -1;
	}
	if (drop_directory) {
		rc = rmdir(scheme->path);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(r->e, "directory '%s' unlink error: %s",
			               scheme->path, strerror(errno));
			return -1;
		}
	}
	return 0;
}

int si_dropmark(si *i)
{
	/* create drop file */
	char path[1024];
	snprintf(path, sizeof(path), "%s/drop", i->scheme->path);
	ssfile drop;
	ss_fileinit(&drop, i->r->a);
	int rc = ss_filenew(&drop, path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(i->r->e, "drop file '%s' create error: %s",
		               path, strerror(errno));
		return -1;
	}
	ss_fileclose(&drop);
	return 0;
}

int si_drop(si *i)
{
	sr *r = i->r;
	sischeme *scheme = i->scheme;
	/* drop file must exists at this point */
	/* shutdown */
	int rc = si_close(i);
	if (ssunlikely(rc == -1))
		return -1;
	/* remove directory */
	rc = si_droprepository(scheme, r, 1);
	return rc;
}
#line 1 "sophia/index/si_gc.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/









uint32_t si_gcv(ssa *a, svv *gc)
{
	uint32_t used = 0;
	svv *v = gc;
	while (v) {
		used += sv_vsize(v);
		svv *n = v->next;
		sl *log = (sl*)v->log;
		if (log)
			ss_gcsweep(&log->gc, 1);
		ss_free(a, v);
		v = n;
	}
	return used;
}
#line 1 "sophia/index/si_iter.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








ssiterif si_iter =
{
	.close = si_iter_close,
	.has   = si_iter_has,
	.of    = si_iter_of,
	.next  = si_iter_next
};
#line 1 "sophia/index/si_node.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/









sinode *si_nodenew(sr *r)
{
	sinode *n = (sinode*)ss_malloc(r->a, sizeof(sinode));
	if (ssunlikely(n == NULL)) {
		sr_oom_malfunction(r->e);
		return NULL;
	}
	n->recover = 0;
	n->backup = 0;
	n->flags = 0;
	n->update_time = 0;
	n->used = 0;
	si_branchinit(&n->self);
	n->branch = NULL;
	n->branch_count = 0;
	ss_fileinit(&n->file, r->a);
	ss_mmapinit(&n->map);
	ss_mmapinit(&n->map_swap);
	sv_indexinit(&n->i0);
	sv_indexinit(&n->i1);
	ss_rbinitnode(&n->node);
	ss_rqinitnode(&n->nodecompact);
	ss_rqinitnode(&n->nodebranch);
	ss_listinit(&n->commit);
	return n;
}

static inline int
si_nodeclose(sinode *n, sr *r)
{
	int rcret = 0;
	int rc = ss_munmap(&n->map);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' munmap error: %s",
		               n->file.file, strerror(errno));
		rcret = -1;
	}
	rc = ss_fileclose(&n->file);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' close error: %s",
		               n->file.file, strerror(errno));
		rcret = -1;
	}
	sv_indexfree(&n->i0, r);
	sv_indexfree(&n->i1, r);
	return rcret;
}

static inline int
si_noderecover(sinode *n, sr *r)
{
	/* recover branches */
	ssiter i;
	ss_iterinit(sd_recover, &i);
	ss_iteropen(sd_recover, &i, r, &n->file);
	int first = 1;
	int rc;
	while (ss_iteratorhas(&i))
	{
		sdindexheader *h = ss_iteratorof(&i);
		sibranch *b;
		if (first) {
			b =  &n->self;
		} else {
			b = si_branchnew(r);
			if (ssunlikely(b == NULL))
				goto error;
		}
		sdindex index;
		sd_indexinit(&index);
		rc = sd_indexcopy(&index, r, h);
		if (ssunlikely(rc == -1))
			goto error;
		si_branchset(b, &index);

		b->next   = n->branch;
		n->branch = b;
		n->branch_count++;

		first = 0;
		ss_iteratornext(&i);
	}
	rc = sd_recover_complete(&i);
	if (ssunlikely(rc == -1))
		goto error;
	ss_iteratorclose(&i);
	return 0;
error:
	ss_iteratorclose(&i);
	return -1;
}

int si_nodeopen(sinode *n, sr *r, sischeme *scheme, sspath *path)
{
	int rc = ss_fileopen(&n->file, path->path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' open error: %s",
		               n->file.file, strerror(errno));
		return -1;
	}
	rc = ss_fileseek(&n->file, n->file.size);
	if (ssunlikely(rc == -1)) {
		si_nodeclose(n, r);
		sr_malfunction(r->e, "db file '%s' seek error: %s",
		               n->file.file, strerror(errno));
		return -1;
	}
	rc = si_noderecover(n, r);
	if (ssunlikely(rc == -1))
		si_nodeclose(n, r);
	if (scheme->mmap) {
		rc = si_nodemap(n, r);
		if (ssunlikely(rc == -1))
			si_nodeclose(n, r);
	}
	return rc;
}

int si_nodecreate(sinode *n, sr *r, sischeme *scheme, sdid *id,
                  sdindex *i,
                  sdbuild *build)
{
	si_branchset(&n->self, i);
	sspath path;
	ss_pathAB(&path, scheme->path, id->parent, id->id, ".db.incomplete");
	int rc = ss_filenew(&n->file, path.path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' create error: %s",
		               path.path, strerror(errno));
		return -1;
	}
	rc = sd_commit(build, r, &n->self.index, &n->file);
	if (ssunlikely(rc == -1))
		return -1;
	if (scheme->mmap) {
		rc = si_nodemap(n, r);
		if (ssunlikely(rc == -1))
			return -1;
	}
	n->branch = &n->self;
	n->branch_count++;
	return 0;
}

int si_nodemap(sinode *n, sr *r)
{
	int rc = ss_mmap(&n->map, n->file.fd, n->file.size, 1);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' mmap error: %s",
		               n->file.file, strerror(errno));
		return -1;
	}
	return 0;
}

int si_nodesync(sinode *n, sr *r)
{
	int rc = ss_filesync(&n->file);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' sync error: %s",
		               n->file.file, strerror(errno));
		return -1;
	}
	return 0;
}

static inline void
si_nodefree_branches(sinode *n, sr *r)
{
	sibranch *p = n->branch;
	sibranch *next = NULL;
	while (p && p != &n->self) {
		next = p->next;
		si_branchfree(p, r);
		p = next;
	}
	sd_indexfree(&n->self.index, r);
}

int si_nodefree(sinode *n, sr *r, int gc)
{
	int rcret = 0;
	int rc;
	if (gc && n->file.file) {
		rc = ss_fileunlink(n->file.file);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(r->e, "db file '%s' unlink error: %s",
			               n->file.file, strerror(errno));
			rcret = -1;
		}
	}
	si_nodefree_branches(n, r);
	rc = si_nodeclose(n, r);
	if (ssunlikely(rc == -1))
		rcret = -1;
	ss_free(r->a, n);
	return rcret;
}

ss_rbtruncate(si_nodegc_indexgc,
              si_gcv((ssa*)arg, sscast(n, svv, node)))

int si_nodegc_index(sr *r, svindex *i)
{
	if (i->i.root)
		si_nodegc_indexgc(i->i.root, r->a);
	sv_indexinit(i);
	return 0;
}

int si_nodeseal(sinode *n, sr *r, sischeme *scheme)
{
	sspath path;
	ss_pathAB(&path, scheme->path, n->self.id.parent,
	          n->self.id.id, ".db.seal");
	int rc = ss_filerename(&n->file, path.path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' rename error: %s",
		               n->file.file, strerror(errno));
	}
	return rc;
}

int si_nodecomplete(sinode *n, sr *r, sischeme *scheme)
{
	sspath path;
	ss_pathA(&path, scheme->path, n->self.id.id, ".db");
	int rc = ss_filerename(&n->file, path.path);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "db file '%s' rename error: %s",
		               n->file.file, strerror(errno));
	}
	return rc;
}
#line 1 "sophia/index/si_planner.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








int si_planinit(siplan *p)
{
	p->plan    = SI_NONE;
	p->explain = SI_ENONE;
	p->a       = 0;
	p->b       = 0;
	p->c       = 0;
	p->node    = NULL;
	return 0;
}

int si_plannerinit(siplanner *p, ssa *a, void *i)
{
	int rc = ss_rqinit(&p->compact, a, 1, 20);
	if (ssunlikely(rc == -1))
		return -1;
	/* 1Mb step up to 4Gb */
	rc = ss_rqinit(&p->branch, a, 1024 * 1024, 4000);
	if (ssunlikely(rc == -1)) {
		ss_rqfree(&p->compact, a);
		return -1;
	}
	p->i = i;
	return 0;
}

int si_plannerfree(siplanner *p, ssa *a)
{
	ss_rqfree(&p->compact, a);
	ss_rqfree(&p->branch, a);
	return 0;
}

int si_plannertrace(siplan *p, sstrace *t)
{
	char *plan = NULL;
	switch (p->plan) {
	case SI_BRANCH: plan = "branch";
		break;
	case SI_AGE: plan = "age";
		break;
	case SI_COMPACT: plan = "compact";
		break;
	case SI_CHECKPOINT: plan = "checkpoint";
		break;
	case SI_GC: plan = "gc";
		break;
	case SI_BACKUP:
	case SI_BACKUPEND: plan = "backup";
		break;
	case SI_SHUTDOWN: plan = "database shutdown";
		break;
	case SI_DROP: plan = "database drop";
		break;
	}
	char *explain = NULL;
	switch (p->explain) {
	case SI_ENONE:
		explain = "none";
		break;
	case SI_ERETRY:
		explain = "retry expected";
		break;
	case SI_EINDEX_SIZE:
		explain = "index size";
		break;
	case SI_EINDEX_AGE:
		explain = "index age";
		break;
	case SI_EBRANCH_COUNT:
		explain = "branch count";
		break;
	}
	if (p->node) {
		ss_trace(t, "%s <#%" PRIu32 " explain: %s>",
		         plan,
		         p->node->self.id.id, explain);
	} else {
		ss_trace(t, "%s <explain: %s>", plan, explain);
	}
	return 0;
}

int si_plannerupdate(siplanner *p, int mask, sinode *n)
{
	if (mask & SI_BRANCH)
		ss_rqupdate(&p->branch, &n->nodebranch, n->used);
	if (mask & SI_COMPACT)
		ss_rqupdate(&p->compact, &n->nodecompact, n->branch_count);
	return 0;
}

int si_plannerremove(siplanner *p, int mask, sinode *n)
{
	if (mask & SI_BRANCH)
		ss_rqdelete(&p->branch, &n->nodebranch);
	if (mask & SI_COMPACT)
		ss_rqdelete(&p->compact, &n->nodecompact);
	return 0;
}

static inline int
si_plannerpeek_backup(siplanner *p, siplan *plan)
{
	/* try to peek a node which has
	 * bsn <= required value
	*/
	int rc_inprogress = 0;
	sinode *n;
	ssrqnode *pn = NULL;
	while ((pn = ss_rqprev(&p->branch, pn))) {
		n = sscast(pn, sinode, nodebranch);
		if (n->backup < plan->a) {
			if (n->flags & SI_LOCK) {
				rc_inprogress = 2;
				continue;
			}
			goto match;
		}
	}
	if (rc_inprogress) {
		plan->explain = SI_ERETRY;
		return 2;
	}
	si *index = p->i;
	if (index->backup < plan->a) {
		plan->plan = SI_BACKUPEND;
		plan->node = 0;
		return 1;
	}
	return 0;
match:
	si_nodelock(n);
	plan->explain = SI_ENONE;
	plan->node = n;
	return 1;
}

static inline int
si_plannerpeek_checkpoint(siplanner *p, siplan *plan)
{
	/* try to peek a node which has min
	 * lsn <= required value
	*/
	int rc_inprogress = 0;
	sinode *n;
	ssrqnode *pn = NULL;
	while ((pn = ss_rqprev(&p->branch, pn))) {
		n = sscast(pn, sinode, nodebranch);
		if (n->i0.lsnmin <= plan->a) {
			if (n->flags & SI_LOCK) {
				rc_inprogress = 2;
				continue;
			}
			goto match;
		}
	}
	if (rc_inprogress)
		plan->explain = SI_ERETRY;
	return rc_inprogress;
match:
	si_nodelock(n);
	plan->explain = SI_ENONE;
	plan->node = n;
	return 1;
}

static inline int
si_plannerpeek_branch(siplanner *p, siplan *plan)
{
	/* try to peek a node with a biggest in-memory index */
	sinode *n;
	ssrqnode *pn = NULL;
	while ((pn = ss_rqprev(&p->branch, pn))) {
		n = sscast(pn, sinode, nodebranch);
		if (n->flags & SI_LOCK)
			continue;
		if (n->used >= plan->a)
			goto match;
		return 0;
	}
	return 0;
match:
	si_nodelock(n);
	plan->explain = SI_EINDEX_SIZE;
	plan->node = n;
	return 1;
}

static inline int
si_plannerpeek_age(siplanner *p, siplan *plan)
{
	/* try to peek a node with update >= a and in-memory
	 * index size >= b */

	/* full scan */
	uint64_t now = ss_utime();
	sinode *n = NULL;
	ssrqnode *pn = NULL;
	while ((pn = ss_rqprev(&p->branch, pn))) {
		n = sscast(pn, sinode, nodebranch);
		if (n->flags & SI_LOCK)
			continue;
		if (n->used >= plan->b && ((now - n->update_time) >= plan->a))
			goto match;
	}
	return 0;
match:
	si_nodelock(n);
	plan->explain = SI_EINDEX_AGE;
	plan->node = n;
	return 1;
}

static inline int
si_plannerpeek_compact(siplanner *p, siplan *plan)
{
	/* try to peek a node with a biggest number
	 * of branches */
	sinode *n;
	ssrqnode *pn = NULL;
	while ((pn = ss_rqprev(&p->compact, pn))) {
		n = sscast(pn, sinode, nodecompact);
		if (n->flags & SI_LOCK)
			continue;
		if (n->branch_count >= plan->a)
			goto match;
		return 0;
	}
	return 0;
match:
	si_nodelock(n);
	plan->explain = SI_EBRANCH_COUNT;
	plan->node = n;
	return 1;
}

static inline int
si_plannerpeek_gc(siplanner *p, siplan *plan)
{
	/* try to peek a node with a biggest number
	 * of branches which is ready for gc */
	int rc_inprogress = 0;
	sinode *n;
	ssrqnode *pn = NULL;
	while ((pn = ss_rqprev(&p->compact, pn))) {
		n = sscast(pn, sinode, nodecompact);
		sdindexheader *h = n->self.index.h;
		if (sslikely(h->dupkeys == 0) || (h->dupmin >= plan->a))
			continue;
		uint32_t used = (h->dupkeys * 100) / h->keys;
		if (used >= plan->b) {
			if (n->flags & SI_LOCK) {
				rc_inprogress = 2;
				continue;
			}
			goto match;
		}
	}
	if (rc_inprogress)
		plan->explain = SI_ERETRY;
	return rc_inprogress;
match:
	si_nodelock(n);
	plan->explain = SI_ENONE;
	plan->node = n;
	return 1;
}

int si_planner(siplanner *p, siplan *plan)
{
	switch (plan->plan) {
	case SI_BRANCH:
		return si_plannerpeek_branch(p, plan);
	case SI_AGE:
		return si_plannerpeek_age(p, plan);
	case SI_CHECKPOINT:
		return si_plannerpeek_checkpoint(p, plan);
	case SI_COMPACT:
		return si_plannerpeek_compact(p, plan);
	case SI_GC:
		return si_plannerpeek_gc(p, plan);
	case SI_BACKUP:
		return si_plannerpeek_backup(p, plan);
	}
	return -1;
}
#line 1 "sophia/index/si_profiler.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








int si_profilerbegin(siprofiler *p, si *i)
{
	memset(p, 0, sizeof(*p));
	p->i = i;
	si_lock(i);
	return 0;
}

int si_profilerend(siprofiler *p)
{
	si_unlock(p->i);
	return 0;
}

static void
si_profiler_histogram_branch(siprofiler *p)
{
	/* prepare histogram string */
	int size = 0;
	int i = 0;
	while (i < 20) {
		if (p->histogram_branch[i] == 0) {
			i++;
			continue;
		}
		size += snprintf(p->histogram_branch_sz + size,
		                 sizeof(p->histogram_branch_sz) - size,
		                 "[%d]:%d ", i,
		                 p->histogram_branch[i]);
		i++;
	}
	if (p->histogram_branch_20plus) {
		size += snprintf(p->histogram_branch_sz + size,
		                 sizeof(p->histogram_branch_sz) - size,
		                 "[20+]:%d ",
		                 p->histogram_branch_20plus);
	}
	if (size == 0)
		p->histogram_branch_ptr = NULL;
	else {
		p->histogram_branch_ptr = p->histogram_branch_sz;
	}
}

int si_profiler(siprofiler *p)
{
	uint64_t memory_used = 0;
	ssrbnode *pn;
	sinode *n;
	pn = ss_rbmin(&p->i->i);
	while (pn) {
		n = sscast(pn, sinode, node);
		p->total_node_count++;
		p->count += n->i0.count;
		p->count += n->i1.count;
		p->total_branch_count += n->branch_count;
		if (p->total_branch_max < n->branch_count)
			p->total_branch_max = n->branch_count;
		if (n->branch_count < 20)
			p->histogram_branch[n->branch_count]++;
		else
			p->histogram_branch_20plus++;
		memory_used += sv_indexused(&n->i0);
		memory_used += sv_indexused(&n->i1);
		sibranch *b = n->branch;
		while (b) {
			p->count += b->index.h->keys;
			p->count_dup += b->index.h->dupkeys;
			int indexsize = sd_indexsize(b->index.h);
			p->total_node_size += indexsize + b->index.h->total;
			p->total_node_origin_size += indexsize + b->index.h->totalorigin;
			p->total_page_count += b->index.h->count;
			b = b->next;
		}
		pn = ss_rbnext(&p->i->i, pn);
	}
	if (p->total_node_count > 0)
		p->total_branch_avg =
			p->total_branch_count / p->total_node_count;
	p->memory_used = memory_used;
	p->read_disk  = p->i->read_disk;
	p->read_cache = p->i->read_cache;
	si_profiler_histogram_branch(p);
	return 0;
}
#line 1 "sophia/index/si_query.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








int si_queryopen(siquery *q, sicache *c, si *i, ssorder o,
                 uint64_t vlsn,
                 void *prefix, uint32_t prefixsize,
                 void *key, uint32_t keysize)
{
	q->order      = o;
	q->key        = key;
	q->keysize    = keysize;
	q->vlsn       = vlsn;
	q->index      = i;
	q->r          = i->r;
	q->cache      = c;
	q->prefix     = prefix;
	q->prefixsize = prefixsize;
	q->has        = 0;
	q->update_v   = NULL;
	q->update_eq  = 0;
	memset(&q->result, 0, sizeof(q->result));
	sv_mergeinit(&q->merge);
	si_lock(q->index);
	return 0;
}

void si_queryhas(siquery *q)
{
	q->has = 1;
}

void si_queryupdate(siquery *q, sv *v, int eq)
{
	q->update_v  = v;
	q->update_eq = eq;
}

int si_queryclose(siquery *q)
{
	si_unlock(q->index);
	sv_mergefree(&q->merge, q->r->a);
	return 0;
}

static inline int
si_querydup(siquery *q, sv *result)
{
	svv *v = sv_vdup(q->r->a, result);
	if (ssunlikely(v == NULL))
		return sr_oom(q->r->e);
	sv_init(&q->result, &sv_vif, v, NULL);
	return 1;
}

static inline int
si_qgetresult(siquery *q, sv *v, int compare)
{
	int rc;
	if (compare) {
		rc = sr_compare(q->r->scheme, sv_pointer(v), sv_size(v),
		                q->key, q->keysize);
		if (ssunlikely(rc != 0))
			return 0;
	}
	if (q->prefix) {
		rc = sr_compareprefix(q->r->scheme,
		                      q->prefix,
		                      q->prefixsize,
		                      sv_pointer(v), sv_size(v));
		if (ssunlikely(! rc))
			return 0;
	}
	if (ssunlikely(q->has)) {
		return sv_lsn(v) > q->vlsn;
	}
	if (ssunlikely(sv_is(v, SVDELETE)))
		return 2;
	rc = si_querydup(q, v);
	if (ssunlikely(rc == -1))
		return -1;
	return 1;
}

static inline int
si_qgetindex(siquery *q, sinode *node)
{
	svindex *second;
	svindex *first = si_nodeindex_priority(node, &second);
	ssiter i;
	ss_iterinit(sv_indexiter, &i);
	int rc;
	if (first->count > 0) {
		rc = ss_iteropen(sv_indexiter, &i, q->r, first,
		                 SS_GTE, q->key, q->keysize);
		if (rc) {
			goto result;
		}
	}
	if (sslikely(second == NULL || !second->count))
		return 0;
	rc = ss_iteropen(sv_indexiter, &i, q->r, second,
	                 SS_GTE, q->key, q->keysize);
	if (! rc) {
		return 0;
	}
result:;
	sv *v = ss_iterof(sv_indexiter, &i);
	assert(v != NULL);
	svv *visible = v->v;
	if (sslikely(! q->has)) {
		visible = sv_visible((svv*)v->v, q->vlsn);
		if (visible == NULL)
			return 0;
	}
	sv vret;
	sv_init(&vret, &sv_vif, visible, NULL);
	return si_qgetresult(q, &vret, 0);
}

static inline int
si_qgetbranch(siquery *q, sinode *n, sibranch *b)
{
	sicachebranch *cb = si_cachefollow(q->cache);
	assert(cb->branch == b);
	sireadarg arg = {
		.scheme     = q->index->scheme,
		.index      = q->index,
		.n          = n,
		.b          = b,
		.buf        = &cb->buf_a,
		.buf_xf     = &cb->buf_b,
		.buf_read   = &q->index->readbuf,
		.index_iter = &cb->index_iter,
		.page_iter  = &cb->page_iter,
		.vlsn       = q->vlsn,
		.has        = q->has,
		.mmap_copy  = 0,
		.o          = SS_GTE,
		.r          = q->r
	};
	ss_iterinit(si_read, &cb->i);
	int rc = ss_iteropen(si_read, &cb->i, &arg, q->key, q->keysize);
	if (ssunlikely(rc <= 0))
		return rc;
	uint64_t vlsn = q->vlsn;
	if (q->has) {
		vlsn = UINT64_MAX;
	}
	/* prepare sources */
	sv_mergereset(&q->merge);
	sv_mergeadd(&q->merge, &cb->i);
	ssiter i;
	ss_iterinit(sv_mergeiter, &i);
	ss_iteropen(sv_mergeiter, &i, q->r, &q->merge, SS_GTE);
	ssiter j;
	ss_iterinit(sv_readiter, &j);
	ss_iteropen(sv_readiter, &j, q->r, &i, &q->index->u, vlsn, 1);
	sv *v = ss_iterof(sv_readiter, &j);
	if (ssunlikely(v == NULL))
		return 0;
	return si_qgetresult(q, v, 1);
}

static inline int
si_qget(siquery *q)
{
	ssiter i;
	ss_iterinit(si_iter, &i);
	ss_iteropen(si_iter, &i, q->r, q->index, SS_GTE, q->key, q->keysize);
	sinode *node;
	node = ss_iterof(si_iter, &i);
	assert(node != NULL);
	/* search in memory */
	int rc;
	rc = si_qgetindex(q, node);
	if (rc != 0)
		return rc;
	/* */
	rc = si_cachevalidate(q->cache, node);
	if (ssunlikely(rc == -1)) {
		sr_oom(q->r->e);
		return -1;
	}
	svmerge *m = &q->merge;
	rc = sv_mergeprepare(m, q->r, 1);
	assert(rc == 0);
	/* search on disk */
	sibranch *b = node->branch;
	while (b) {
		rc = si_qgetbranch(q, node, b);
		if (rc != 0)
			return rc;
		b = b->next;
	}
	return 0;
}

static inline void
si_qrangebranch(siquery *q, sinode *n, sibranch *b, svmerge *m)
{
	sicachebranch *cb = si_cachefollow(q->cache);
	assert(cb->branch == b);
	/* iterate cache */
	if (ss_iterhas(si_read, &cb->i)) {
		svmergesrc *s = sv_mergeadd(m, &cb->i);
		q->index->read_cache++;
		s->ptr = cb;
		return;
	}
	if (cb->open) {
		return;
	}
	cb->open = 1;
	sireadarg arg = {
		.scheme     = q->index->scheme,
		.index      = q->index,
		.n          = n,
		.b          = b,
		.buf        = &cb->buf_a,
		.buf_xf     = &cb->buf_b,
		.buf_read   = &q->index->readbuf,
		.index_iter = &cb->index_iter,
		.page_iter  = &cb->page_iter,
		.vlsn       = q->vlsn,
		.has        = 0,
		.mmap_copy  = 1,
		.o          = q->order,
		.r          = q->r
	};
	ss_iterinit(si_read, &cb->i);
	int rc = ss_iteropen(si_read, &cb->i, &arg, q->key, q->keysize);
	if (ssunlikely(rc == -1))
		return;
	if (ssunlikely(! ss_iterhas(si_read, &cb->i)))
		return;
	svmergesrc *s = sv_mergeadd(m, &cb->i);
	s->ptr = cb;
}

static inline int
si_qrange(siquery *q)
{
	ssiter i;
	ss_iterinit(si_iter, &i);
	ss_iteropen(si_iter, &i, q->r, q->index, q->order, q->key, q->keysize);
	sinode *node;
next_node:
	node = ss_iterof(si_iter, &i);
	if (ssunlikely(node == NULL))
		return 0;

	/* prepare sources */
	svmerge *m = &q->merge;
	int count = node->branch_count + 2 + 1;
	int rc = sv_mergeprepare(m, q->r, count);
	if (ssunlikely(rc == -1)) {
		sr_errorreset(q->r->e);
		return -1;
	}

	/* external source (update) */
	svmergesrc *s;
	sv upbuf_reserve;
	ssbuf upbuf;
	if (ssunlikely(q->update_v && q->update_v->v)) {
		ss_bufinit_reserve(&upbuf, &upbuf_reserve, sizeof(upbuf_reserve));
		ss_bufadd(&upbuf, NULL, (void*)&q->update_v, sizeof(sv*));
		s = sv_mergeadd(m, NULL);
		ss_iterinit(ss_bufiterref, &s->src);
		ss_iteropen(ss_bufiterref, &s->src, &upbuf, sizeof(sv*));
	}

	/* in-memory indexes */
	svindex *second;
	svindex *first = si_nodeindex_priority(node, &second);
	if (first->count) {
		s = sv_mergeadd(m, NULL);
		ss_iterinit(sv_indexiter, &s->src);
		ss_iteropen(sv_indexiter, &s->src, q->r, first, q->order,
		            q->key, q->keysize);
	}
	if (ssunlikely(second && second->count)) {
		s = sv_mergeadd(m, NULL);
		ss_iterinit(sv_indexiter, &s->src);
		ss_iteropen(sv_indexiter, &s->src, q->r, second, q->order,
		            q->key, q->keysize);
	}

	/* cache and branches */
	rc = si_cachevalidate(q->cache, node);
	if (ssunlikely(rc == -1)) {
		sr_oom(q->r->e);
		return -1;
	}
	sibranch *b = node->branch;
	while (b) {
		si_qrangebranch(q, node, b, m);
		b = b->next;
	}

	/* merge and filter data stream */
	ssiter j;
	ss_iterinit(sv_mergeiter, &j);
	ss_iteropen(sv_mergeiter, &j, q->r, m, q->order);
	ssiter k;
	ss_iterinit(sv_readiter, &k);
	ss_iteropen(sv_readiter, &k, q->r, &j, &q->index->u, q->vlsn, 0);
	sv *v = ss_iterof(sv_readiter, &k);
	if (ssunlikely(v == NULL)) {
		sv_mergereset(&q->merge);
		ss_iternext(si_iter, &i);
		goto next_node;
	}

	rc = 1;
	/* convert update search to SS_EQ */
	if (q->update_eq) {
		rc = sr_compare(q->r->scheme, sv_pointer(v), sv_size(v),
		                q->key, q->keysize);
		rc = rc == 0;
	}
	/* do prefix search */
	if (q->prefix && rc) {
		rc = sr_compareprefix(q->r->scheme, q->prefix, q->prefixsize,
		                      sv_pointer(v),
		                      sv_size(v));
	}
	if (sslikely(rc == 1)) {
		if (ssunlikely(si_querydup(q, v) == -1))
			return -1;
	}

	/* skip a possible duplicates from data sources */
	ss_iternext(sv_readiter, &k);
	return rc;
}

int si_query(siquery *q)
{
	switch (q->order) {
	case SS_EQ:
		return si_qget(q);
	case SS_LT:
	case SS_LTE:
	case SS_GT:
	case SS_GTE:
		return si_qrange(q);
	default:
		break;
	}
	return -1;
}

static int
si_querycommited_branch(sr *r, sibranch *b, sv *v)
{
	ssiter i;
	ss_iterinit(sd_indexiter, &i);
	ss_iteropen(sd_indexiter, &i, r, &b->index, SS_GTE,
	            sv_pointer(v), sv_size(v));
	sdindexpage *page = ss_iterof(sd_indexiter, &i);
	if (page == NULL)
		return 0;
	return page->lsnmax >= sv_lsn(v);
}

int si_querycommited(si *index, sr *r, sv *v)
{
	ssiter i;
	ss_iterinit(si_iter, &i);
	ss_iteropen(si_iter, &i, r, index, SS_GTE,
	            sv_pointer(v), sv_size(v));
	sinode *node;
	node = ss_iterof(si_iter, &i);
	assert(node != NULL);
	sibranch *b = node->branch;
	int rc;
	while (b) {
		rc = si_querycommited_branch(r, b, v);
		if (rc)
			return 1;
		b = b->next;
	}
	rc = si_querycommited_branch(r, &node->self, v);
	return rc;
}
#line 1 "sophia/index/si_read.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








ssiterif si_read =
{
	.close = si_read_close,
	.has   = si_read_has,
	.of    = si_read_of,
	.next  = si_read_next
};
#line 1 "sophia/index/si_recover.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

/*
	repository recover states
	-------------------------

	compaction

	000000001.000000002.db.incomplete  (1)
	000000001.000000002.db.seal        (2)
	000000002.db                       (3)
	000000001.000000003.db.incomplete
	000000001.000000003.db.seal
	000000003.db
	(4)

	1. remove incomplete, mark parent as having incomplete
	2. find parent, mark as having seal
	3. add
	4. recover:
		a. if parent has incomplete and seal - remove both
		b. if parent has incomplete - remove incomplete
		c. if parent has seal - remove parent, complete seal

	see: test/recovery_crash.test.c
*/








sinode *si_bootstrap(si *i, uint32_t parent)
{
	sr *r = i->r;
	sinode *n = si_nodenew(r);
	if (ssunlikely(n == NULL))
		return NULL;
	sdid id = {
		.parent = parent,
		.flags  = 0,
		.id     = sr_seq(r->seq, SR_NSNNEXT)
	};
	sdindex index;
	sd_indexinit(&index);
	int rc = sd_indexbegin(&index, r, 0);
	if (ssunlikely(rc == -1)) {
		si_nodefree(n, r, 0);
		return NULL;
	}
	sdbuild build;
	sd_buildinit(&build);
	rc = sd_buildbegin(&build, r,
	                   i->scheme->node_page_checksum,
	                   i->scheme->compression,
	                   i->scheme->compression_key);
	if (ssunlikely(rc == -1)) {
		sd_indexfree(&index, r);
		sd_buildfree(&build, r);
		si_nodefree(n, r, 0);
		return NULL;
	}
	sd_buildend(&build, r);
	rc = sd_indexadd(&index, r, &build);
	if (ssunlikely(rc == -1)) {
		sd_indexfree(&index, r);
		si_nodefree(n, r, 0);
		return NULL;
	}
	sd_buildcommit(&build, r);
	sd_indexcommit(&index, r, &id);
	rc = si_nodecreate(n, r, i->scheme, &id, &index, &build);
	sd_buildfree(&build, r);
	if (ssunlikely(rc == -1)) {
		si_nodefree(n, r, 1);
		return NULL;
	}
	return n;
}

static inline int
si_deploy(si *i, sr *r, int create_directory)
{
	int rc;
	if (sslikely(create_directory)) {
		rc = ss_filemkdir(i->scheme->path);
		if (ssunlikely(rc == -1)) {
			sr_malfunction(r->e, "directory '%s' create error: %s",
			               i->scheme->path, strerror(errno));
			return -1;
		}
	}
	rc = si_schemedeploy(i->scheme, r);
	if (ssunlikely(rc == -1)) {
		sr_malfunction_set(r->e);
		return -1;
	}
	sinode *n = si_bootstrap(i, 0);
	if (ssunlikely(n == NULL))
		return -1;
	SS_INJECTION(r->i, SS_INJECTION_SI_RECOVER_0,
	             si_nodefree(n, r, 0);
	             sr_malfunction(r->e, "%s", "error injection");
	             return -1);
	rc = si_nodecomplete(n, r, i->scheme);
	if (ssunlikely(rc == -1)) {
		si_nodefree(n, r, 1);
		return -1;
	}
	si_insert(i, n);
	si_plannerupdate(&i->p, SI_COMPACT|SI_BRANCH, n);
	return 1;
}

static inline ssize_t
si_processid(char **str) {
	char *s = *str;
	size_t v = 0;
	while (*s && *s != '.') {
		if (ssunlikely(!isdigit(*s)))
			return -1;
		v = (v * 10) + *s - '0';
		s++;
	}
	*str = s;
	return v;
}

static inline int
si_process(char *name, uint32_t *nsn, uint32_t *parent)
{
	/* id.db */
	/* id.id.db.incomplete */
	/* id.id.db.seal */
	char *token = name;
	ssize_t id = si_processid(&token);
	if (ssunlikely(id == -1))
		return -1;
	*parent = id;
	*nsn = id;
	if (strcmp(token, ".db") == 0)
		return SI_RDB;
	if (ssunlikely(*token != '.'))
		return -1;
	token++;
	id = si_processid(&token);
	if (ssunlikely(id == -1))
		return -1;
	*nsn = id;
	if (strcmp(token, ".db.incomplete") == 0)
		return SI_RDB_DBI;
	else
	if (strcmp(token, ".db.seal") == 0)
		return SI_RDB_DBSEAL;
	return -1;
}

static inline int
si_trackdir(sitrack *track, sr *r, si *i)
{
	DIR *dir = opendir(i->scheme->path);
	if (ssunlikely(dir == NULL)) {
		sr_malfunction(r->e, "directory '%s' open error: %s",
		               i->scheme->path, strerror(errno));
		return -1;
	}
	struct dirent *de;
	while ((de = readdir(dir))) {
		if (ssunlikely(de->d_name[0] == '.'))
			continue;
		uint32_t id_parent = 0;
		uint32_t id = 0;
		int rc = si_process(de->d_name, &id, &id_parent);
		if (ssunlikely(rc == -1))
			continue; /* skip unknown file */
		si_tracknsn(track, id_parent);
		si_tracknsn(track, id);

		sinode *head, *node;
		sspath path;
		switch (rc) {
		case SI_RDB_DBI:
		case SI_RDB_DBSEAL: {
			/* find parent node and mark it as having
			 * incomplete compaction process */
			head = si_trackget(track, id_parent);
			if (sslikely(head == NULL)) {
				head = si_nodenew(r);
				if (ssunlikely(head == NULL))
					goto error;
				head->self.id.id = id_parent;
				head->recover = SI_RDB_UNDEF;
				si_trackset(track, head);
			}
			head->recover |= rc;
			/* remove any incomplete file made during compaction */
			if (rc == SI_RDB_DBI) {
				ss_pathAB(&path, i->scheme->path, id_parent, id, ".db.incomplete");
				rc = ss_fileunlink(path.path);
				if (ssunlikely(rc == -1)) {
					sr_malfunction(r->e, "db file '%s' unlink error: %s",
					               path.path, strerror(errno));
					goto error;
				}
				continue;
			}
			assert(rc == SI_RDB_DBSEAL);
			/* recover 'sealed' node */
			node = si_nodenew(r);
			if (ssunlikely(node == NULL))
				goto error;
			node->recover = SI_RDB_DBSEAL;
			ss_pathAB(&path, i->scheme->path, id_parent, id, ".db.seal");
			rc = si_nodeopen(node, r, i->scheme, &path);
			if (ssunlikely(rc == -1)) {
				si_nodefree(node, r, 0);
				goto error;
			}
			si_trackset(track, node);
			si_trackmetrics(track, node);
			continue;
		}
		}
		assert(rc == SI_RDB);

		/* recover node */
		node = si_nodenew(r);
		if (ssunlikely(node == NULL))
			goto error;
		node->recover = SI_RDB;
		ss_pathA(&path, i->scheme->path, id, ".db");
		rc = si_nodeopen(node, r, i->scheme, &path);
		if (ssunlikely(rc == -1)) {
			si_nodefree(node, r, 0);
			goto error;
		}
		si_trackmetrics(track, node);

		/* track node */
		head = si_trackget(track, id);
		if (sslikely(head == NULL)) {
			si_trackset(track, node);
		} else {
			/* replace a node previously created by a
			 * incomplete compaction. */
			if (! (head->recover & SI_RDB_UNDEF)) {
				sr_malfunction(r->e, "corrupted database repository: %s",
				               i->scheme->path);
				goto error;
			}
			si_trackreplace(track, head, node);
			head->recover &= ~SI_RDB_UNDEF;
			node->recover |= head->recover;
			si_nodefree(head, r, 0);
		}
	}
	closedir(dir);
	return 0;
error:
	closedir(dir);
	return -1;
}

static inline int
si_trackvalidate(sitrack *track, ssbuf *buf, sr *r, si *i)
{
	ss_bufreset(buf);
	ssrbnode *p = ss_rbmax(&track->i);
	while (p) {
		sinode *n = sscast(p, sinode, node);
		switch (n->recover) {
		case SI_RDB|SI_RDB_DBI|SI_RDB_DBSEAL|SI_RDB_REMOVE:
		case SI_RDB|SI_RDB_DBSEAL|SI_RDB_REMOVE:
		case SI_RDB|SI_RDB_REMOVE:
		case SI_RDB_UNDEF|SI_RDB_DBSEAL|SI_RDB_REMOVE:
		case SI_RDB|SI_RDB_DBI|SI_RDB_DBSEAL:
		case SI_RDB|SI_RDB_DBI:
		case SI_RDB:
		case SI_RDB|SI_RDB_DBSEAL:
		case SI_RDB_UNDEF|SI_RDB_DBSEAL: {
			/* match and remove any leftover ancestor */
			sinode *ancestor = si_trackget(track, n->self.id.parent);
			if (ancestor && (ancestor != n))
				ancestor->recover |= SI_RDB_REMOVE;
			break;
		}
		case SI_RDB_DBSEAL: {
			/* find parent */
			sinode *parent = si_trackget(track, n->self.id.parent);
			if (parent) {
				/* schedule node for removal, if has incomplete merges */
				if (parent->recover & SI_RDB_DBI)
					n->recover |= SI_RDB_REMOVE;
				else
					parent->recover |= SI_RDB_REMOVE;
			}
			if (! (n->recover & SI_RDB_REMOVE)) {
				/* complete node */
				int rc = si_nodecomplete(n, r, i->scheme);
				if (ssunlikely(rc == -1))
					return -1;
				n->recover = SI_RDB;
			}
			break;
		}
		default:
			/* corrupted states */
			return sr_malfunction(r->e, "corrupted database repository: %s",
			                      i->scheme->path);
		}
		p = ss_rbprev(&track->i, p);
	}
	return 0;
}

static inline int
si_recovercomplete(sitrack *track, sr *r, si *index, ssbuf *buf)
{
	/* prepare and build primary index */
	ss_bufreset(buf);
	ssrbnode *p = ss_rbmin(&track->i);
	while (p) {
		sinode *n = sscast(p, sinode, node);
		int rc = ss_bufadd(buf, r->a, &n, sizeof(sinode*));
		if (ssunlikely(rc == -1))
			return sr_oom_malfunction(r->e);
		p = ss_rbnext(&track->i, p);
	}
	ssiter i;
	ss_iterinit(ss_bufiterref, &i);
	ss_iteropen(ss_bufiterref, &i, buf, sizeof(sinode*));
	while (ss_iterhas(ss_bufiterref, &i))
	{
		sinode *n = ss_iterof(ss_bufiterref, &i);
		if (n->recover & SI_RDB_REMOVE) {
			int rc = si_nodefree(n, r, 1);
			if (ssunlikely(rc == -1))
				return -1;
			ss_iternext(ss_bufiterref, &i);
			continue;
		}
		n->recover = SI_RDB;
		si_insert(index, n);
		si_plannerupdate(&index->p, SI_COMPACT|SI_BRANCH, n);
		ss_iternext(ss_bufiterref, &i);
	}
	return 0;
}

static inline int
si_recoverindex(si *i, sr *r)
{
	sitrack track;
	si_trackinit(&track);
	ssbuf buf;
	ss_bufinit(&buf);
	int rc;
	rc = si_trackdir(&track, r, i);
	if (ssunlikely(rc == -1))
		goto error;
	if (ssunlikely(track.count == 0))
		return 1;
	rc = si_trackvalidate(&track, &buf, r, i);
	if (ssunlikely(rc == -1))
		goto error;
	rc = si_recovercomplete(&track, r, i, &buf);
	if (ssunlikely(rc == -1))
		goto error;
	/* set actual metrics */
	if (track.nsn > r->seq->nsn)
		r->seq->nsn = track.nsn;
	if (track.lsn > r->seq->lsn)
		r->seq->lsn = track.lsn;
	ss_buffree(&buf, r->a);
	return 0;
error:
	ss_buffree(&buf, r->a);
	si_trackfree(&track, r);
	return -1;
}

static inline int
si_recoverdrop(si *i, sr *r)
{
	char path[1024];
	snprintf(path, sizeof(path), "%s/drop", i->scheme->path);
	int rc = ss_fileexists(path);
	if (sslikely(! rc))
		return 0;
	if (i->scheme->path_fail_on_drop) {
		sr_malfunction(r->e, "attempt to recover a dropped database: %s:",
		               i->scheme->path);
		return -1;
	}
	rc = si_droprepository(i->scheme, r, 0);
	if (ssunlikely(rc == -1))
		return -1;
	return 1;
}

int si_recover(si *i)
{
	sr *r = i->r;
	int exist = ss_fileexists(i->scheme->path);
	if (exist == 0)
		goto deploy;
	if (i->scheme->path_fail_on_exists) {
		sr_error(r->e, "directory '%s' already exists", i->scheme->path);
		return -1;
	}
	int rc = si_recoverdrop(i, r);
	switch (rc) {
	case -1: return -1;
	case  1: goto deploy;
	}
	rc = si_schemerecover(i->scheme, r);
	if (ssunlikely(rc == -1))
		return -1;
	r->scheme = &i->scheme->scheme;
	r->compression = i->scheme->compression_if;
	r->fmt = i->scheme->fmt;
	r->fmt_storage = i->scheme->fmt_storage;
	rc = si_recoverindex(i, r);
	if (sslikely(rc <= 0))
		return rc;
deploy:
	return si_deploy(i, r, !exist);
}
#line 1 "sophia/index/si_scheme.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








enum {
	SI_SCHEME_NONE,
	SI_SCHEME_NAME,
	SI_SCHEME_FORMAT,
	SI_SCHEME_FORMAT_STORAGE,
	SI_SCHEME_SCHEME,
	SI_SCHEME_NODE_SIZE,
	SI_SCHEME_NODE_PAGE_SIZE,
	SI_SCHEME_NODE_PAGE_CHECKSUM,
	SI_SCHEME_SYNC,
	SI_SCHEME_COMPRESSION,
	SI_SCHEME_COMPRESSION_KEY
};

void si_schemeinit(sischeme *s)
{
	memset(s, 0, sizeof(*s));
}

void si_schemefree(sischeme *s, sr *r)
{
	if (s->name) {
		ss_free(r->a, s->name);
		s->name = NULL;
	}
	if (s->path) {
		ss_free(r->a, s->path);
		s->path = NULL;
	}
	if (s->path_backup) {
		ss_free(r->a, s->path_backup);
		s->path_backup = NULL;
	}
	if (s->compression_sz) {
		ss_free(r->a, s->compression_sz);
		s->compression_sz = NULL;
	}
	if (s->fmt_sz) {
		ss_free(r->a, s->fmt_sz);
		s->fmt_sz = NULL;
	}
	sr_schemefree(&s->scheme, r->a);
}

int si_schemedeploy(sischeme *s, sr *r)
{
	sdscheme c;
	sd_schemeinit(&c);
	int rc;
	rc = sd_schemebegin(&c, r);
	if (ssunlikely(rc == -1))
		return -1;
	rc = sd_schemeadd(&c, r, SI_SCHEME_NAME, SS_STRING, s->name,
	                  strlen(s->name) + 1);
	if (ssunlikely(rc == -1))
		goto error;
	ssbuf buf;
	ss_bufinit(&buf);
	rc = sr_schemesave(&s->scheme, r->a, &buf);
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemeadd(&c, r, SI_SCHEME_SCHEME, SS_STRING, buf.s,
	                  ss_bufused(&buf));
	if (ssunlikely(rc == -1))
		goto error;
	ss_buffree(&buf, r->a);
	uint32_t v = s->fmt;
	rc = sd_schemeadd(&c, r, SI_SCHEME_FORMAT, SS_U32, &v, sizeof(v));
	if (ssunlikely(rc == -1))
		goto error;
	v = s->fmt_storage;
	rc = sd_schemeadd(&c, r, SI_SCHEME_FORMAT_STORAGE, SS_U32, &v, sizeof(v));
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemeadd(&c, r, SI_SCHEME_NODE_SIZE, SS_U64,
	                  &s->node_size,
	                  sizeof(s->node_size));
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemeadd(&c, r, SI_SCHEME_NODE_PAGE_SIZE, SS_U32,
	                  &s->node_page_size,
	                  sizeof(s->node_page_size));
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemeadd(&c, r, SI_SCHEME_NODE_PAGE_CHECKSUM, SS_U32,
	                  &s->node_page_checksum,
	                  sizeof(s->node_page_checksum));
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemeadd(&c, r, SI_SCHEME_SYNC, SS_U32,
	                  &s->sync,
	                  sizeof(s->sync));
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemeadd(&c, r, SI_SCHEME_COMPRESSION, SS_STRING,
	                  s->compression_if->name,
	                  strlen(s->compression_if->name) + 1);
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemeadd(&c, r, SI_SCHEME_COMPRESSION_KEY, SS_U32,
	                  &s->compression_key,
	                  sizeof(s->compression_key));
	if (ssunlikely(rc == -1))
		goto error;
	rc = sd_schemecommit(&c, r);
	if (ssunlikely(rc == -1))
		return -1;
	char path[PATH_MAX];
	snprintf(path, sizeof(path), "%s/scheme", s->path);
	rc = sd_schemewrite(&c, r, path, s->sync);
	sd_schemefree(&c, r);
	return rc;
error:
	ss_buffree(&buf, r->a);
	sd_schemefree(&c, r);
	return -1;
}

int si_schemerecover(sischeme *s, sr *r)
{
	sdscheme c;
	sd_schemeinit(&c);
	char path[PATH_MAX];
	snprintf(path, sizeof(path), "%s/scheme", s->path);
	int rc;
	rc = sd_schemerecover(&c, r, path);
	if (ssunlikely(rc == -1))
		return -1;
	ssiter i;
	ss_iterinit(sd_schemeiter, &i);
	rc = ss_iteropen(sd_schemeiter, &i, r, &c, 1);
	if (ssunlikely(rc == -1))
		return -1;
	while (ss_iterhas(sd_schemeiter, &i))
	{
		sdschemeopt *opt = ss_iterof(sd_schemeiter, &i);
		switch (opt->id) {
		case SI_SCHEME_FORMAT:
			s->fmt = sd_schemeu32(opt);
			char *name;
			if (s->fmt == SF_KV)
				name = "kv";
			else
			if (s->fmt == SF_DOCUMENT)
				name = "document";
			else
				goto error;
			ss_free(r->a, s->fmt_sz);
			s->fmt_sz = ss_strdup(r->a, name);
			if (ssunlikely(s->fmt_sz == NULL))
				goto error;
			break;
		case SI_SCHEME_FORMAT_STORAGE:
			s->fmt_storage = sd_schemeu32(opt);
			break;
		case SI_SCHEME_SCHEME: {
			sr_schemefree(&s->scheme, r->a);
			sr_schemeinit(&s->scheme);
			ssbuf buf;
			ss_bufinit(&buf);
			rc = sr_schemeload(&s->scheme, r->a, sd_schemesz(opt), opt->size);
			if (ssunlikely(rc == -1))
				goto error;
			ss_buffree(&buf, r->a);
			break;
		}
		case SI_SCHEME_NODE_SIZE:
			s->node_size = sd_schemeu64(opt);
			break;
		case SI_SCHEME_NODE_PAGE_SIZE:
			s->node_page_size = sd_schemeu32(opt);
			break;
		case SI_SCHEME_COMPRESSION_KEY:
			s->compression_key = sd_schemeu32(opt);
			break;
		case SI_SCHEME_COMPRESSION: {
			char *name = sd_schemesz(opt);
			ssfilterif *cif = NULL;
			if (strcmp(name, "none") == 0)
				cif = &ss_nonefilter;
			else
			if (strcmp(name, "lz4") == 0)
				cif = &ss_lz4filter;
			else
			if (strcmp(name, "zstd") == 0)
				cif = &ss_zstdfilter;
			else
				goto error;
			s->compression_if = cif;
			s->compression = s->compression_if != &ss_nonefilter;
			ss_free(r->a, s->compression_sz);
			s->compression_sz = ss_strdup(r->a, cif->name);
			if (ssunlikely(s->compression_sz == NULL))
				goto error;
			break;
		}
		}
		ss_iternext(sd_schemeiter, &i);
	}
	sd_schemefree(&c, r);
	return 0;
error:
	sd_schemefree(&c, r);
	return -1;
}
#line 1 "sophia/index/si_write.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/









void si_begin(sitx *t, si *index, uint64_t vlsn, uint64_t time,
              svlog *l,
              svlogindex *li)
{
	t->index = index;
	t->time  = time;
	t->vlsn  = vlsn;
	t->l     = l;
	t->li    = li;
	ss_listinit(&t->nodelist);
	si_lock(index);
}

void si_commit(sitx *t)
{
	/* reschedule nodes */
	sslist *i, *n;
	ss_listforeach_safe(&t->nodelist, i, n) {
		sinode *node = sscast(i, sinode, commit);
		ss_listinit(&node->commit);
		si_plannerupdate(&t->index->p, SI_BRANCH, node);
	}
	si_unlock(t->index);
}

static inline int
si_set(sitx *t, svv *v)
{
	si *index = t->index;
	t->index->update_time = t->time;
	/* match node */
	ssiter i;
	ss_iterinit(si_iter, &i);
	ss_iteropen(si_iter, &i, t->index->r, index, SS_GTE,
	            sv_vpointer(v), v->size);
	sinode *node = ss_iterof(si_iter, &i);
	assert(node != NULL);
	/* insert into node index */
	svindex *vindex = si_nodeindex(node);
	svindexpos pos;
	sv_indexget(vindex, t->index->r, &pos, v);
	sv_indexupdate(vindex, &pos, v);
	/* update node */
	node->update_time = index->update_time;
	node->used += sv_vsize(v);
	if (ss_listempty(&node->commit))
		ss_listappend(&t->nodelist, &node->commit);
	return 0;
}

void si_write(sitx *t, int check)
{
	svlogv *cv = sv_logat(t->l, t->li->head);
	int c = t->li->count;
	while (c) {
		svv *v = cv->v.v;
		if (check && si_querycommited(t->index, t->index->r, &cv->v)) {
			uint32_t gc = si_gcv(t->index->r->a, v);
			ss_quota(t->index->r->quota, SS_QREMOVE, gc);
			goto next;
		}
		si_set(t, v);
next:
		cv = sv_logat(t->l, cv->next);
		c--;
	}
	return;
}
#line 1 "sophia/repository/sy_conf.h"
#ifndef SY_CONF_H_
#define SY_CONF_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct syconf syconf;

struct syconf {
	char *path;
	int   path_create;
	char *path_backup;
	int   sync;
};

#endif
#line 1 "sophia/repository/sy.h"
#ifndef SY_H_
#define SY_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sy sy;

struct sy {
	syconf *conf;
};

int sy_init(sy*);
int sy_open(sy*, sr*, syconf*);
int sy_close(sy*, sr*);

#endif
#line 1 "sophia/repository/sy.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/








int sy_init(sy *e)
{
	e->conf = NULL;
	return 0;
}

static int
sy_deploy(sy *e, sr *r)
{
	int rc;
	rc = ss_filemkdir(e->conf->path);
	if (ssunlikely(rc == -1)) {
		sr_error(r->e, "directory '%s' create error: %s",
		         e->conf->path, strerror(errno));
		return -1;
	}
	return 0;
}

static inline ssize_t
sy_processid(char **str) {
	char *s = *str;
	size_t v = 0;
	while (*s && *s != '.') {
		if (ssunlikely(!isdigit(*s)))
			return -1;
		v = (v * 10) + *s - '0';
		s++;
	}
	*str = s;
	return v;
}

static inline int
sy_process(char *name, uint32_t *bsn)
{
	/* id */
	/* id.incomplete */
	char *token = name;
	ssize_t id = sy_processid(&token);
	if (ssunlikely(id == -1))
		return -1;
	*bsn = id;
	if (strcmp(token, ".incomplete") == 0)
		return 1;
	return 0;
}

static inline int
sy_recoverbackup(sy *i, sr *r)
{
	if (i->conf->path_backup == NULL)
		return 0;
	int rc;
	int exists = ss_fileexists(i->conf->path_backup);
	if (! exists) {
		rc = ss_filemkdir(i->conf->path_backup);
		if (ssunlikely(rc == -1)) {
			sr_error(r->e, "backup directory '%s' create error: %s",
					 i->conf->path_backup, strerror(errno));
			return -1;
		}
	}
	/* recover backup sequential number */
	DIR *dir = opendir(i->conf->path_backup);
	if (ssunlikely(dir == NULL)) {
		sr_error(r->e, "backup directory '%s' open error: %s",
				 i->conf->path_backup, strerror(errno));
		return -1;
	}
	uint32_t bsn = 0;
	struct dirent *de;
	while ((de = readdir(dir))) {
		if (ssunlikely(de->d_name[0] == '.'))
			continue;
		uint32_t id = 0;
		rc = sy_process(de->d_name, &id);
		switch (rc) {
		case  1:
		case  0:
			if (id > bsn)
				bsn = id;
			break;
		case -1: /* skip unknown file */
			continue;
		}
	}
	closedir(dir);
	r->seq->bsn = bsn;
	return 0;
}

int sy_open(sy *e, sr *r, syconf *conf)
{
	e->conf = conf;
	int rc = sy_recoverbackup(e, r);
	if (ssunlikely(rc == -1))
		return -1;
	int exists = ss_fileexists(conf->path);
	if (exists == 0) {
		if (ssunlikely(! conf->path_create)) {
			sr_error(r->e, "directory '%s' does not exist", conf->path);
			return -1;
		}
		return sy_deploy(e, r);
	}
	return 0;
}

int sy_close(sy *e, sr *r)
{
	(void)e;
	(void)r;
	return 0;
}
#line 1 "sophia/environment/se_o.h"
#ifndef SE_O_H_
#define SE_O_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

enum {
	SEUNDEF,
	SEDESTROYED,
	SE,
	SEMETA,
	SEMETACURSOR,
	SEMETAV,
	SEREQ,
	SEV,
	SEDB,
	SEDBASYNC,
	SEBATCH,
	SETX,
	SECURSOR,
	SESNAPSHOT,
	SESNAPSHOTCURSOR
};

extern sotype se_o[];

#define se_cast(ptr, cast, id) \
	so_cast(ptr, cast, &se_o[id])

static inline so*
se_cast_validate(void *ptr)
{
	so *o = ptr;
	if ((char*)o->type >= (char*)&se_o[0] &&
	    (char*)o->type <= (char*)&se_o[SESNAPSHOTCURSOR])
		return ptr;
	return NULL;
}

static inline void
se_mark_destroyed(so *o)
{
	so_init(o, &se_o[SEDESTROYED], NULL, NULL, NULL);
}

#endif
#line 1 "sophia/environment/se_worker.h"
#ifndef SE_WORKER_H_
#define SE_WORKER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct seworkerpool seworkerpool;
typedef struct seworker seworker;

struct seworker {
	ssthread t;
	char name[16];
	sstrace trace;
	void *arg;
	sdc dc;
	sslist link;
};

struct seworkerpool {
	sslist list;
	int n;
};

int se_workerpool_init(seworkerpool*);
int se_workerpool_shutdown(seworkerpool*, sr*);
int se_workerpool_new(seworkerpool*, sr*, int, ssthreadf, void*);

static inline void
se_workerstub_init(seworker *w)
{
	sd_cinit(&w->dc);
	ss_listinit(&w->link);
	ss_traceinit(&w->trace);
	ss_trace(&w->trace, "%s", "init");
}

static inline void
se_workerstub_free(seworker *w, sr *r)
{
	sd_cfree(&w->dc, r);
	ss_tracefree(&w->trace);
}

#endif
#line 1 "sophia/environment/se_status.h"
#ifndef SE_STATUS_H_
#define SE_STATUS_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

enum {
	SE_OFFLINE,
	SE_ONLINE,
	SE_RECOVER,
	SE_SHUTDOWN,
	SE_MALFUNCTION
};

typedef struct sestatus sestatus;

struct sestatus {
	int status;
	ssspinlock lock;
};

static inline void
se_statusinit(sestatus *s)
{
	s->status = SE_OFFLINE;
	ss_spinlockinit(&s->lock);
}

static inline void
se_statusfree(sestatus *s)
{
	ss_spinlockfree(&s->lock);
}

static inline void
se_statuslock(sestatus *s) {
	ss_spinlock(&s->lock);
}

static inline void
se_statusunlock(sestatus *s) {
	ss_spinunlock(&s->lock);
}

static inline int
se_statusset(sestatus *s, int status)
{
	ss_spinlock(&s->lock);
	int old = s->status;
	if (old == SE_MALFUNCTION) {
		ss_spinunlock(&s->lock);
		return -1;
	}
	s->status = status;
	ss_spinunlock(&s->lock);
	return old;
}

static inline int
se_status(sestatus *s)
{
	ss_spinlock(&s->lock);
	int status = s->status;
	ss_spinunlock(&s->lock);
	return status;
}

static inline char*
se_statusof(sestatus *s)
{
	int status = se_status(s);
	switch (status) {
	case SE_OFFLINE:     return "offline";
	case SE_ONLINE:      return "online";
	case SE_RECOVER:     return "recover";
	case SE_SHUTDOWN:    return "shutdown";
	case SE_MALFUNCTION: return "malfunction";
	}
	assert(0);
	return NULL;
}

static inline int
se_statusactive_is(int status) {
	switch (status) {
	case SE_ONLINE:
	case SE_RECOVER:
		return 1;
	case SE_SHUTDOWN:
	case SE_OFFLINE:
	case SE_MALFUNCTION:
		return 0;
	}
	assert(0);
	return 0;
}

static inline int
se_statusactive(sestatus *s) {
	return se_statusactive_is(se_status(s));
}

static inline int
se_online(sestatus *s) {
	return se_status(s) == SE_ONLINE;
}

#endif
#line 1 "sophia/environment/se_meta.h"
#ifndef SE_META_H_
#define SE_META_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct semetart semetart;
typedef struct semeta semeta;

typedef void (*serecovercbf)(char*, void*);

typedef struct {
	serecovercbf function;
	void *arg;
} serecovercb;

struct semetart {
	/* sophia */
	char      version[16];
	char      build[32];
	/* memory */
	uint64_t  memory_used;
	/* scheduler */
	char      zone[4];
	uint32_t  checkpoint_active;
	uint64_t  checkpoint_lsn;
	uint64_t  checkpoint_lsn_last;
	uint32_t  backup_active;
	uint32_t  backup_last;
	uint32_t  backup_last_complete;
	uint32_t  gc_active;
	uint32_t  reqs;
	/* log */
	uint32_t  log_files;
	/* metric */
	srseq     seq;
};

struct semeta {
	/* sophia */
	char         *path;
	uint32_t      path_create;
	/* backup */
	char         *backup_path;
	/* compaction */
	uint32_t      node_size;
	uint32_t      page_size;
	uint32_t      page_checksum;
	srzonemap     zones;
	/* scheduler */
	uint32_t      threads;
	serecovercb   on_recover;
	sstrigger     on_event;
	uint32_t      event_on_backup;
	/* memory */
	uint64_t      memory_limit;
	/* log */
	uint32_t      log_enable;
	char         *log_path;
	uint32_t      log_sync;
	uint32_t      log_rotate_wm;
	uint32_t      log_rotate_sync;
	uint32_t      two_phase_recover;
	uint32_t      commit_lsn;
	srscheme      scheme;
	so           *env;
};

void     se_metainit(semeta*, so*);
void     se_metafree(semeta*);
int      se_metavalidate(semeta*);
int      se_metaserialize(semeta*, ssbuf*);
int      se_metaset_object(so*, const char*, void*);
int      se_metaset_string(so*, const char*, void*, int);
int      se_metaset_int(so*, const char*, int64_t);
void    *se_metaget_object(so*, const char*);
void    *se_metaget_string(so*, const char*, int*);
int64_t  se_metaget_int(so*, const char*);

#endif
#line 1 "sophia/environment/se_metacursor.h"
#ifndef SE_METACURSOR_H_
#define SE_METACURSOR_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct semetav semetav;
typedef struct semetacursor semetacursor;

struct semetav {
	so o;
	char *key;
	char *value;
	int keysize;
	int valuesize;
};

struct semetacursor {
	so o;
	ssbuf dump;
	int first;
	srmetadump *pos;
};

so *se_metacursor_new(void*);

#endif
#line 1 "sophia/environment/se_scheduler.h"
#ifndef SE_SCHEDULER_H_
#define SE_SCHEDULER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sescheduler sescheduler;
typedef struct setask setask;

struct setask {
	siplan  plan;
	int     rotate;
	int     req;
	int     gc;
	int     checkpoint_complete;
	int     backup_complete;
	void   *db;
};

struct sescheduler {
	ssmutex       lock;
	uint64_t      checkpoint_lsn_last;
	uint64_t      checkpoint_lsn;
	uint32_t      checkpoint;
	uint32_t      age;
	uint32_t      age_last;
	uint32_t      backup_bsn;
	uint32_t      backup_last;
	uint32_t      backup_last_complete;
	uint32_t      backup_events;
	uint32_t      backup;
	uint32_t      gc;
	uint32_t      gc_last;
	uint32_t      workers_backup;
	uint32_t      workers_branch;
	uint32_t      workers_gc;
	uint32_t      workers_gc_db;
	int           rotate;
	int           req;
	int           rr;
	int           count;
	void        **i;
	seworkerpool  workers;
	so           *env;
};

int se_scheduler_init(sescheduler*, so*);
int se_scheduler_run(sescheduler*);
int se_scheduler_shutdown(sescheduler*);
int se_scheduler_add(sescheduler*, void*);
int se_scheduler_del(sescheduler*, void*);
int se_scheduler(sescheduler*, seworker*);
int se_scheduler_branch(void*);
int se_scheduler_compact(void*);
int se_scheduler_checkpoint(void*);
int se_scheduler_gc(void*);
int se_scheduler_backup(void*);
int se_scheduler_call(void*);

#endif
#line 1 "sophia/environment/se.h"
#ifndef SE_H_
#define SE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct se se;

struct se {
	so          o;
	sestatus    status;
	ssmutex     apilock;
	ssmutex     reqlock;
	sscond      reqcond;
	ssspinlock  dblock;
	solist      db;
	solist      db_shutdown;
	solist      cursor;
	solist      tx;
	solist      req;
	solist      reqactive;
	solist      reqready;
	solist      snapshot;
	solist      metacursor;
	srseq       seq;
	semeta      meta;
	ssquota     quota;
	sspager     pagersx;
	sspager     pager;
	ssa         a;
	ssa         a_db;
	ssa         a_v;
	ssa         a_cursor;
	ssa         a_cachebranch;
	ssa         a_cache;
	ssa         a_metacursor;
	ssa         a_metav;
	ssa         a_snapshot;
	ssa         a_snapshotcursor;
	ssa         a_batch;
	ssa         a_tx;
	ssa         a_sxv;
	ssa         a_req;
	sicachepool cachepool;
	syconf      repconf;
	sy          rep;
	slconf      lpconf;
	slpool      lp;
	sxmanager   xm;
	sescheduler sched;
	srerror     error;
	ssinjection ei;
	sr          r;
};

static inline int
se_active(se *e) {
	return se_statusactive(&e->status);
}

static inline void
se_apilock(so *o) {
	ss_mutexlock(&((se*)o)->apilock);
}

static inline void
se_apiunlock(so *o) {
	ss_mutexunlock(&((se*)o)->apilock);
}

static inline se *se_of(so *o) {
	return (se*)o->env;
}

so *se_new(void);

#endif
#line 1 "sophia/environment/se_req.h"
#ifndef SE_REQ_H_
#define SE_REQ_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sereqarg sereqarg;
typedef struct sereq sereq;

typedef enum {
	SE_REQREAD,
	SE_REQWRITE,
	SE_REQON_BACKUP
} sereqop;

struct sereqarg {
	sv        v;
	sv        vprefix;
	sv        vup;
	sicache  *cache;
	int       cachegc;
	ssorder   order;
	int       update;
	int       update_eq;
	int       vlsn_generate;
	uint64_t  vlsn;
	svlog    *log;
	void     *arg;
	int       recover;
	uint64_t  lsn;
};

struct sereq {
	so        o;
	uint64_t  id;
	sereqop   op;
	sereqarg  arg;
	so       *object;
	so       *db;
	void     *v;
	int       rc; 
};

void   se_reqinit(se*, sereq*, sereqop, so*, so*);
char  *se_reqof(sereqop);
void   se_reqend(sereq*);
void   se_reqonbackup(se*);
void   se_reqready(sereq*);
int    se_reqcount(se*);
int    se_reqqueue(se*);
sereq *se_reqnew(se*, sereq*, int);
sereq *se_reqdispatch(se*, int);
sereq *se_reqdispatch_ready(se*);
void   se_reqwakeup(se*);
so    *se_reqresult(sereq*, int);

#endif
#line 1 "sophia/environment/se_v.h"
#ifndef SE_V_H_
#define SE_V_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sev sev;

struct sev {
	so        o;
	sv        v;
	sv        vprefix;
	ssorder   order;
	int       orderset;
	sfv       keyv[8];
	int       keyc;
	void     *prefix;
	uint32_t  prefixsize;
	void     *value;
	uint32_t  valuesize;
	/* recover */
	void     *raw;
	uint32_t  rawsize;
	void     *log;
	/* async */
	int       async;
	int       async_status;
	uint32_t  async_operation;
	uint64_t  async_seq;
	void     *async_arg;
};

so *se_vnew(se*, so*, sv*, int);

#endif
#line 1 "sophia/environment/se_db.h"
#ifndef SE_DB_H_
#define SE_DB_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sedb sedb;

struct sedb {
	so         o;
	so         async;
	sestatus   status;
	uint32_t   created;
	uint32_t   scheduled;
	uint32_t   dropped;
	siprofiler rtp;
	solist     batch;
	sdc        dc;
	sischeme   scheme;
	si         index;
	sxindex    coindex;
	ssspinlock reflock;
	uint32_t   ref;
	uint32_t   ref_be;
	uint32_t   txn_min;
	uint32_t   txn_max;
	sr         r;
};

static inline int
se_dbactive(sedb *o) {
	return se_statusactive(&o->status);
}

so       *se_dbnew(se*, char*);
so       *se_dbmatch(se*, char*);
so       *se_dbmatch_id(se*, uint32_t);
void     *se_dbread(sedb*, sev*, sx*, int, sicache*, ssorder);
void      se_dbref(sedb*, int);
uint32_t  se_dbunref(sedb*, int);
uint32_t  se_dbrefof(sedb*, int);
int       se_dbgarbage(sedb*);
int       se_dbvisible(sedb*, uint32_t);
void      se_dbbind(se*);
void      se_dbunbind(se*, uint32_t);
int       se_dbmalfunction(sedb*);
int       se_dbv(sedb*, sev*, int, svv**);
int       se_dbvprefix(sedb*, sev*, svv**);

#endif
#line 1 "sophia/environment/se_batch.h"
#ifndef SE_BATCH_H_
#define SE_BATCH_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sebatch sebatch;

struct sebatch {
	so o;
	uint64_t lsn;
	svlog log;
};

so *se_batchnew(sedb*);

#endif
#line 1 "sophia/environment/se_tx.h"
#ifndef SE_TX_H_
#define SE_TX_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct setx setx;

struct setx {
	so o;
	uint64_t lsn;
	sx t;
};

so   *se_txnew(se*);
void  se_txend(setx*);

#endif
#line 1 "sophia/environment/se_cursor.h"
#ifndef SE_CURSOR_H_
#define SE_CURSOR_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct secursor secursor;

struct secursor {
	so o;
	sx t;
	sicache *cache;
};

so *se_cursornew(se*, uint64_t);

#endif
#line 1 "sophia/environment/se_snapshot.h"
#ifndef SE_SNAPSHOT_H_
#define SE_SNAPSHOT_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sesnapshot sesnapshot;

struct sesnapshot {
	so o;
	uint64_t vlsn;
	char *name;
	sx t;
	solist cursor;
} sspacked;

so  *se_snapshotnew(se*, uint64_t, char*);
int  se_snapshotupdate(sesnapshot*);

#endif
#line 1 "sophia/environment/se_snapshotcursor.h"
#ifndef SE_SNAPSHOTCURSOR_H_
#define SE_SNAPSHOTCURSOR_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sesnapshotcursor sesnapshotcursor;

struct sesnapshotcursor {
	so o;
	int ready;
	ssbuf list;
	char *pos;
	sedb *v;
	sesnapshot *s;
} sspacked;

so *se_snapshotcursor_new(sesnapshot*);

#endif
#line 1 "sophia/environment/se_execute.h"
#ifndef SE_EXECUTE_H_
#define SE_EXECUTE_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int se_execute(sereq*);

#endif
#line 1 "sophia/environment/se_recover.h"
#ifndef SE_RECOVER_H_
#define SE_RECOVER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

int se_recoverbegin(sedb*);
int se_recoverend(sedb*);
int se_recover(se*);
int se_recover_repository(se*);

#endif
#line 1 "sophia/environment/se_scheduler.h"
#ifndef SE_SCHEDULER_H_
#define SE_SCHEDULER_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct sescheduler sescheduler;
typedef struct setask setask;

struct setask {
	siplan  plan;
	int     rotate;
	int     req;
	int     gc;
	int     checkpoint_complete;
	int     backup_complete;
	void   *db;
};

struct sescheduler {
	ssmutex       lock;
	uint64_t      checkpoint_lsn_last;
	uint64_t      checkpoint_lsn;
	uint32_t      checkpoint;
	uint32_t      age;
	uint32_t      age_last;
	uint32_t      backup_bsn;
	uint32_t      backup_last;
	uint32_t      backup_last_complete;
	uint32_t      backup_events;
	uint32_t      backup;
	uint32_t      gc;
	uint32_t      gc_last;
	uint32_t      workers_backup;
	uint32_t      workers_branch;
	uint32_t      workers_gc;
	uint32_t      workers_gc_db;
	int           rotate;
	int           req;
	int           rr;
	int           count;
	void        **i;
	seworkerpool  workers;
	so           *env;
};

int se_scheduler_init(sescheduler*, so*);
int se_scheduler_run(sescheduler*);
int se_scheduler_shutdown(sescheduler*);
int se_scheduler_add(sescheduler*, void*);
int se_scheduler_del(sescheduler*, void*);
int se_scheduler(sescheduler*, seworker*);
int se_scheduler_branch(void*);
int se_scheduler_compact(void*);
int se_scheduler_checkpoint(void*);
int se_scheduler_gc(void*);
int se_scheduler_backup(void*);
int se_scheduler_call(void*);

#endif
#line 1 "sophia/environment/se_batch.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_batchrollback(so *o)
{
	sebatch *b = se_cast(o, sebatch*, SEBATCH);
	se *e = se_of(o);
	ssiter i;
	ss_iterinit(ss_bufiter, &i);
	ss_iteropen(ss_bufiter, &i, &b->log.buf, sizeof(svlogv));
	int gc = 0;
	for (; ss_iterhas(ss_bufiter, &i); ss_iternext(ss_bufiter, &i))
	{
		svlogv *lv = ss_iterof(ss_bufiter, &i);
		gc += sv_vsize((svv*)lv->v.v);
		ss_free(&e->a, lv->v.v);
	}
	ss_quota(&e->quota, SS_QREMOVE, gc);
	sv_logfree(&b->log, &e->a);
	sedb *db = (sedb*)b->o.parent;
	se_dbunref(db, 0);
	so_listdel(&db->batch, &b->o);
	se_mark_destroyed(&b->o);
	ss_free(&e->a_batch, b);
	return 0;
}

static inline int
se_batchwrite(sebatch *b, sev *o, uint8_t flags)
{
	se *e = se_of(&b->o);
	sedb *db = se_cast(o->o.parent, sedb*, SEDB);

	/* xxx: validate database status */
	if (flags == SVUPDATE && !sf_updatehas(&db->scheme.fmt_update))
		flags = 0;

	/* prepare object */
	svv *v;
	int rc = se_dbv(db, o, 0, &v);
	if (ssunlikely(rc == -1))
		goto error;
	v->flags = flags;
	v->log = o->log;
	sv vp;
	sv_init(&vp, &sv_vif, v, NULL);
	so_destroy(&o->o);

	/* ensure quota */
	int size = sizeof(svv) + sv_size(&vp);
	ss_quota(&e->quota, SS_QADD, size);

	/* log add */
	svlogv lv;
	sv_logvinit(&lv, db->coindex.dsn);
	sv_init(&lv.v, &sv_vif, v, NULL);
	rc = sv_logadd(&b->log, &e->a, &lv, db->coindex.ptr);
	if (ssunlikely(rc == -1)) {
		ss_quota(&e->quota, SS_QREMOVE, size);
		return sr_oom(&e->error);
	}
	return 0;
error:
	so_destroy(&o->o);
	return -1;
}

static int
se_batchset(so *o, so *v)
{
	sebatch *b = se_cast(o, sebatch*, SEBATCH);
	sev *key = se_cast(v, sev*, SEV);
	return se_batchwrite(b, key, 0);
}

static int
se_batchupdate(so *o, so *v)
{
	sebatch *b = se_cast(o, sebatch*, SEBATCH);
	sev *key = se_cast(v, sev*, SEV);
	return se_batchwrite(b, key, SVUPDATE);
}

static int
se_batchdelete(so *o, so *v)
{
	sebatch *b = se_cast(o, sebatch*, SEBATCH);
	sev *key = se_cast(v, sev*, SEV);
	return se_batchwrite(b, key, SVDELETE);
}

static int
se_batchcommit(so *o)
{
	sebatch *b = se_cast(o, sebatch*, SEBATCH);
	se *e = se_of(o);
	uint64_t lsn = sr_seq(&e->seq, SR_LSN);
	if (ssunlikely(lsn != b->lsn)) {
		se_batchrollback(o);
		return 1;
	}
	if (ssunlikely(! sv_logcount(&b->log))) {
		se_batchrollback(o);
		return 0;
	}

	/* log write */
	sl_prepare(&e->lp, &b->log, 0);
	sltx tl;
	sl_begin(&e->lp, &tl);
	int rc = sl_write(&tl, &b->log);
	if (ssunlikely(rc == -1)) {
		sl_rollback(&tl);
		se_batchrollback(o);
		return -1;
	}
	sl_commit(&tl);

	/* commit */
	uint64_t vlsn = sx_vlsn(&e->xm);
	uint64_t now = ss_utime();
	svlogindex *i   = (svlogindex*)b->log.index.s;
	svlogindex *end = (svlogindex*)b->log.index.p;
	while (i < end) {
		sedb *db = i->ptr;
		sitx ti;
		si_begin(&ti, &db->index, vlsn, now, &b->log, i);
		si_write(&ti, 0);
		si_commit(&ti);
		i++;
	}
	sv_logfree(&b->log, &e->a);
	return 0;
}

static soif sebatchif =
{
	.open         = NULL,
	.destroy      = se_batchrollback,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = se_batchset,
	.update       = se_batchupdate,
	.del          = se_batchdelete,
	.get          = NULL,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = se_batchcommit,
	.cursor       = NULL,
};

so *se_batchnew(sedb *db)
{
	se *e = se_of(&db->o);
	sebatch *b = ss_malloc(&e->a_batch, sizeof(sebatch));
	if (ssunlikely(b == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	so_init(&b->o, &se_o[SEBATCH], &sebatchif, &db->o, &e->o);
	b->lsn = sr_seq(&e->seq, SR_LSN);
	sv_loginit(&b->log);
	se_dbref(db, 0);
	so_listadd(&db->batch, &b->o);
	return &b->o;
}
#line 1 "sophia/environment/se.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_open(so *o)
{
	se *e = se_cast(o, se*, SE);
	int status = se_status(&e->status);
	if (status == SE_RECOVER) {
		assert(e->meta.two_phase_recover == 1);
		goto online;
	}
	if (status != SE_OFFLINE)
		return -1;
	int rc;
	rc = se_metavalidate(&e->meta);
	if (ssunlikely(rc == -1))
		return -1;
	se_statusset(&e->status, SE_RECOVER);

	/* set memory quota (disable during recovery) */
	ss_quotaset(&e->quota, e->meta.memory_limit);
	ss_quotaenable(&e->quota, 0);

	/* repository recover */
	rc = se_recover_repository(e);
	if (ssunlikely(rc == -1))
		return -1;
	/* databases recover */
	sslist *i, *n;
	ss_listforeach_safe(&e->db.list, i, n) {
		so *o = sscast(i, so, link);
		rc = so_open(o);
		if (ssunlikely(rc == -1))
			return -1;
	}
	/* recover logpool */
	rc = se_recover(e);
	if (ssunlikely(rc == -1))
		return -1;
	if (e->meta.two_phase_recover)
		return 0;

online:
	/* complete */
	ss_listforeach_safe(&e->db.list, i, n) {
		so *o = sscast(i, so, link);
		rc = so_open(o);
		if (ssunlikely(rc == -1))
			return -1;
	}
	/* enable quota */
	ss_quotaenable(&e->quota, 1);
	se_statusset(&e->status, SE_ONLINE);
	/* run thread-pool and scheduler */
	rc = se_scheduler_run(&e->sched);
	if (ssunlikely(rc == -1))
		return -1;
	return 0;
}

static int
se_destroy(so *o)
{
	se *e = se_cast(o, se*, SE);
	int rcret = 0;
	int rc;
	se_statusset(&e->status, SE_SHUTDOWN);
	rc = se_scheduler_shutdown(&e->sched);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->req);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->reqactive);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->reqready);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->cursor);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->tx);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->snapshot);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->metacursor);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->db);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = so_listdestroy(&e->db_shutdown);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = sl_poolshutdown(&e->lp);
	if (ssunlikely(rc == -1))
		rcret = -1;
	rc = sy_close(&e->rep, &e->r);
	if (ssunlikely(rc == -1))
		rcret = -1;
	sx_managerfree(&e->xm);
	si_cachepool_free(&e->cachepool, &e->r);
	se_metafree(&e->meta);
	ss_quotafree(&e->quota);
	ss_mutexfree(&e->apilock);
	ss_mutexfree(&e->reqlock);
	ss_condfree(&e->reqcond);
	ss_spinlockfree(&e->dblock);
	sr_seqfree(&e->seq);
	ss_pagerfree(&e->pager);
	ss_pagerfree(&e->pagersx);
	se_statusfree(&e->status);
	se_mark_destroyed(&e->o);
	free(e);
	return rcret;
}

static void*
se_begin(so *o)
{
	se *e = se_of(o);
	return se_txnew(e);
}

static void*
se_poll(so *o)
{
	se *e = se_cast(o, se*, SE);
	so *result;
	if (e->meta.event_on_backup) {
		ss_mutexlock(&e->sched.lock);
		if (ssunlikely(e->sched.backup_events > 0)) {
			e->sched.backup_events--;
			sereq r;
			se_reqinit(e, &r, SE_REQON_BACKUP, &e->o, NULL);
			result = se_reqresult(&r, 1);
			ss_mutexunlock(&e->sched.lock);
			return result;
		}
		ss_mutexunlock(&e->sched.lock);
	}
	sereq *req = se_reqdispatch_ready(e);
	if (req == NULL)
		return NULL;
	result = se_reqresult(req, 1);
	so_destroy(&req->o);
	return result;
}

static int
se_error(so *o)
{
	se *e = se_cast(o, se*, SE);
	int status = sr_errorof(&e->error);
	if (status == SR_ERROR_MALFUNCTION)
		return 1;
	status = se_status(&e->status);
	if (status == SE_MALFUNCTION)
		return 1;
	return 0;
}

static void*
se_cursor(so *o)
{
	se *e = se_cast(o, se*, SE);
	return se_cursornew(e, 0);
}

static soif seif =
{
	.open         = se_open,
	.destroy      = se_destroy,
	.error        = se_error,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = se_poll,
	.drop         = NULL,
	.setobject    = se_metaset_object,
	.setstring    = se_metaset_string,
	.setint       = se_metaset_int,
	.getobject    = se_metaget_object,
	.getstring    = se_metaget_string,
	.getint       = se_metaget_int,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = NULL,
	.batch        = NULL,
	.begin        = se_begin,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = se_cursor,
};

so *se_new(void)
{
	se *e = malloc(sizeof(*e));
	if (ssunlikely(e == NULL))
		return NULL;
	memset(e, 0, sizeof(*e));
	so_init(&e->o, &se_o[SE], &seif, &e->o, &e->o /* self */);
	se_statusinit(&e->status);
	se_statusset(&e->status, SE_OFFLINE);
	ss_pagerinit(&e->pager, 10, 4096);
	int rc = ss_pageradd(&e->pager);
	if (ssunlikely(rc == -1)) {
		free(e);
		return NULL;
	}
	ss_pagerinit(&e->pagersx, 10, 4096);
	rc = ss_pageradd(&e->pagersx);
	if (ssunlikely(rc == -1)) {
		ss_pagerfree(&e->pager);
		free(e);
		return NULL;
	}
	ss_aopen(&e->a, &ss_stda);
	ss_aopen(&e->a_db, &ss_slaba, &e->pager, sizeof(sedb));
	ss_aopen(&e->a_v, &ss_slaba, &e->pager, sizeof(sev));
	ss_aopen(&e->a_cursor, &ss_slaba, &e->pager, sizeof(secursor));
	ss_aopen(&e->a_cachebranch, &ss_slaba, &e->pager, sizeof(sicachebranch));
	ss_aopen(&e->a_cache, &ss_slaba, &e->pager, sizeof(sicache));
	ss_aopen(&e->a_metacursor, &ss_slaba, &e->pager, sizeof(semetacursor));
	ss_aopen(&e->a_metav, &ss_slaba, &e->pager, sizeof(semetav));
	ss_aopen(&e->a_snapshot, &ss_slaba, &e->pager, sizeof(sesnapshot));
	ss_aopen(&e->a_snapshotcursor, &ss_slaba, &e->pager, sizeof(sesnapshotcursor));
	ss_aopen(&e->a_batch, &ss_slaba, &e->pager, sizeof(sebatch));
	ss_aopen(&e->a_tx, &ss_slaba, &e->pager, sizeof(setx));
	ss_aopen(&e->a_req, &ss_slaba, &e->pager, sizeof(sereq));
	ss_aopen(&e->a_sxv, &ss_slaba, &e->pagersx, sizeof(sxv));
	se_metainit(&e->meta, &e->o);
	so_listinit(&e->db);
	so_listinit(&e->db_shutdown);
	so_listinit(&e->cursor);
	so_listinit(&e->tx);
	so_listinit(&e->snapshot);
	so_listinit(&e->metacursor);
	so_listinit(&e->req);
	so_listinit(&e->reqready);
	so_listinit(&e->reqactive);
	ss_mutexinit(&e->apilock);
	ss_mutexinit(&e->reqlock);
	ss_condinit(&e->reqcond);
	ss_spinlockinit(&e->dblock);
	ss_quotainit(&e->quota);
	sr_seqinit(&e->seq);
	sr_errorinit(&e->error);
	sscrcf crc = ss_crc32c_function();
	sr_init(&e->r, &e->error, &e->a, &e->quota, &e->seq,
	        SF_KV, SF_SRAW, NULL,
	        &e->meta.scheme, &e->ei, crc, NULL);
	sy_init(&e->rep);
	sl_poolinit(&e->lp, &e->r);
	sx_managerinit(&e->xm, &e->seq, &e->a, &e->a_sxv);
	si_cachepool_init(&e->cachepool, &e->a_cache, &e->a_cachebranch);
	se_scheduler_init(&e->sched, &e->o);
	return &e->o;
}
#line 1 "sophia/environment/se_cursor.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_cursordestroy(so *o)
{
	secursor *c = se_cast(o, secursor*, SECURSOR);
	se *e = se_of(&c->o);
	sx_rollback(&c->t);
	uint32_t id = c->t.id;
	if (c->cache)
		si_cachepool_push(c->cache);
	so_listdel(&e->cursor, &c->o);
	se_dbunbind(e, id);
	se_mark_destroyed(&c->o);
	ss_free(&e->a_cursor, c);
	return 0;
}

static void*
se_cursorget(so *o, so *v)
{
	secursor *c = se_cast(o, secursor*, SECURSOR);
	sev *key = se_cast(v, sev*, SEV);
	sedb *db = se_cast(v->parent, sedb*, SEDB);
	ssorder order = key->order;
	if (ssunlikely(! key->orderset))
		order = SS_GTE;
	return se_dbread(db, key, &c->t, 0, c->cache, order);
}

static soif secursorif =
{
	.open         = NULL,
	.destroy      = se_cursordestroy,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = se_cursorget,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

so *se_cursornew(se *e, uint64_t vlsn)
{
	secursor *c = ss_malloc(&e->a_cursor, sizeof(secursor));
	if (ssunlikely(c == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	so_init(&c->o, &se_o[SECURSOR], &secursorif, &e->o, &e->o);
	sx_init(&e->xm, &c->t);
	c->t.s = SXUNDEF;
	c->cache = si_cachepool_pop(&e->cachepool);
	if (ssunlikely(c->cache == NULL)) {
		ss_free(&e->a_cursor, c);
		return NULL;
	}
	sx_begin(&e->xm, &c->t, vlsn);
	se_dbbind(e);
	so_listadd(&e->cursor, &c->o);
	return &c->o;
}
#line 1 "sophia/environment/se_db.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_dbscheme_init(sedb *db, char *name)
{
	se *e = se_of(&db->o);
	/* prepare index scheme */
	sischeme *scheme = &db->scheme;
	scheme->name = ss_strdup(&e->a, name);
	if (ssunlikely(scheme->name == NULL))
		goto e0;
	scheme->id                  = sr_seq(&e->seq, SR_DSNNEXT);
	scheme->sync                = 1;
	scheme->mmap                = 0;
	scheme->compression         = 0;
	scheme->compression_key     = 0;
	scheme->compression_if      = &ss_nonefilter;
	scheme->fmt                 = SF_KV;
	scheme->fmt_storage         = SF_SRAW;
	scheme->path_fail_on_exists = 0;
	scheme->path_fail_on_drop   = 1;
	sf_updateinit(&scheme->fmt_update);
	scheme->compression_sz = ss_strdup(&e->a, scheme->compression_if->name);
	if (ssunlikely(scheme->compression_sz == NULL))
		goto e1;
	scheme->fmt_sz = ss_strdup(&e->a, "kv");
	if (ssunlikely(scheme->fmt_sz == NULL))
		goto e1;
	/* init single key part as string */
	int rc;
	sr_schemeinit(&scheme->scheme);
	srkey *part = sr_schemeadd(&scheme->scheme, &e->a);
	if (ssunlikely(part == NULL))
		goto e1;
	rc = sr_keysetname(part, &e->a, "key");
	if (ssunlikely(rc == -1))
		goto e1;
	rc = sr_keyset(part, &e->a, "string");
	if (ssunlikely(rc == -1))
		goto e1;
	return 0;
e1:
	si_schemefree(&db->scheme, &db->r);
e0:
	sr_oom(&e->error);
	return -1;
}

static int
se_dbscheme_set(sedb *db)
{
	se *e = se_of(&db->o);
	sischeme *s = &db->scheme;
	/* format */
	if (strcmp(s->fmt_sz, "kv") == 0) {
		s->fmt = SF_KV;
	} else
	if (strcmp(s->fmt_sz, "document") == 0) {
		s->fmt = SF_DOCUMENT;
	} else {
		sr_error(&e->error, "unknown format type '%s'", s->fmt_sz);
		return -1;
	}
	/* compression_key */
	if (s->compression_key) {
		if (s->fmt == SF_DOCUMENT) {
			sr_error(&e->error, "%s", "incompatible options: format=document "
			         "and compression_key=1");
			return -1;
		}
		s->fmt_storage = SF_SKEYVALUE;
	}
	/* compression */
	if (strcmp(s->compression_sz, "none") == 0) {
		s->compression_if = &ss_nonefilter;
	} else
	if (strcmp(s->compression_sz, "zstd") == 0) {
		s->compression_if = &ss_zstdfilter;
	} else
	if (strcmp(s->compression_sz, "lz4") == 0) {
		s->compression_if = &ss_lz4filter;
	} else {
		sr_error(&e->error, "unknown compression type '%s'",
		         s->compression_sz);
		return -1;
	}
	s->compression = s->compression_if != &ss_nonefilter;
	/* path */
	if (s->path == NULL) {
		char path[1024];
		snprintf(path, sizeof(path), "%s/%s", e->meta.path, s->name);
		s->path = ss_strdup(&e->a, path);
		if (ssunlikely(s->path == NULL))
			return sr_oom(&e->error);
	}
	/* backup path */
	s->path_backup = e->meta.backup_path;
	if (e->meta.backup_path) {
		s->path_backup = ss_strdup(&e->a, e->meta.backup_path);
		if (ssunlikely(s->path_backup == NULL))
			return sr_oom(&e->error);
	}
	/* compaction */
	s->node_size          = e->meta.node_size;
	s->node_page_size     = e->meta.page_size;
	s->node_page_checksum = e->meta.page_checksum;

	db->r.scheme = &s->scheme;
	db->r.fmt = s->fmt;
	db->r.fmt_storage = s->fmt_storage;
	db->r.fmt_update  = &s->fmt_update;
	db->r.compression = s->compression_if;
	return 0;
}

static void*
se_dbasync_object(so *o)
{
	(void)se_cast(o, so*, SEDBASYNC);
	sedb *db = se_cast(o->parent, sedb*, SEDB);
	se *e = se_of(&db->o);
	return se_vnew(e, &db->o, NULL, 1);
}

static soif sedbasyncif =
{
	.open         = NULL,
	.destroy      = NULL,
	.error        = NULL,
	.object       = se_dbasync_object,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = NULL,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

static void*
se_dbasync(so *o)
{
	return &se_cast(o, sedb*, SEDB)->async;
}

static int
se_dbopen(so *o)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	se *e = se_of(&db->o);
	int status = se_status(&db->status);
	if (status == SE_RECOVER)
		goto online;
	if (status != SE_OFFLINE)
		return -1;
	int rc = se_dbscheme_set(db);
	if (ssunlikely(rc == -1))
		return -1;
	sx_indexset(&db->coindex, db->scheme.id, db->r.scheme);
	rc = se_recoverbegin(db);
	if (ssunlikely(rc == -1))
		return -1;
	if (se_status(&e->status) == SE_RECOVER)
		return 0;
online:
	se_recoverend(db);
	rc = se_scheduler_add(&e->sched, db);
	if (ssunlikely(rc == -1))
		return -1;
	db->scheduled = 1;
	return 0;
}

static int
se_dbdestroy(so *o)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	se *e = se_of(&db->o);
	int rcret = 0;
	int rc;
	int status = se_status(&e->status);
	if (status == SE_SHUTDOWN)
		goto shutdown;

	uint32_t ref;
	status = se_status(&db->status);
	switch (status) {
	case SE_MALFUNCTION:
	case SE_ONLINE:
	case SE_RECOVER:
		ref = se_dbunref(db, 0);
		if (ref > 0)
			return 0;
		/* set last visible transaction id */
		db->txn_max = sx_max(&e->xm);
		if (db->scheduled) {
			rc = se_scheduler_del(&e->sched, o);
			if (ssunlikely(rc == -1))
				return -1;
		}
		so_listdel(&e->db, &db->o);
		ss_spinlock(&e->dblock);
		so_listadd(&e->db_shutdown, &db->o);
		ss_spinunlock(&e->dblock);
		se_statusset(&db->status, SE_SHUTDOWN);
		return 0;
	case SE_SHUTDOWN:
		/* this intended to be called from a
		 * background gc task */
		assert(se_dbrefof(db, 0) == 0);
		ref = se_dbrefof(db, 1);
		if (ref > 0)
			return 0;
		goto shutdown;
	case SE_OFFLINE:
		so_listdel(&e->db, &db->o);
		goto shutdown;
	default: assert(0);
	}

shutdown:;
	rc = so_listdestroy(&db->batch);
	if (ssunlikely(rc == -1))
		rcret = -1;
	sx_indexfree(&db->coindex, &e->xm);
	rc = si_close(&db->index);
	if (ssunlikely(rc == -1))
		rcret = -1;
	si_schemefree(&db->scheme, &db->r);
	sd_cfree(&db->dc, &db->r);
	se_statusfree(&db->status);
	ss_spinlockfree(&db->reflock);
	se_mark_destroyed(&db->o);
	ss_free(&e->a_db, db);
	return rcret;
}

static int
se_dbdrop(so *o)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	int status = se_status(&db->status);
	if (ssunlikely(! se_statusactive_is(status)))
		return -1;
	if (ssunlikely(db->dropped))
		return 0;
	int rc = si_dropmark(&db->index);
	if (ssunlikely(rc == -1))
		return -1;
	db->dropped = 1;
	return 0;
}

void*
se_dbread(sedb *db, sev *o, sx *x, int x_search,
          sicache *cache, ssorder order)
{
	se *e = se_of(&db->o);
	/* validate req */
	if (ssunlikely(o->o.parent != &db->o)) {
		sr_error(&e->error, "%s", "bad object parent");
		return NULL;
	}
	if (ssunlikely(! se_online(&db->status)))
		goto e0;
	int async = o->async;
	void *async_arg = o->async_arg;

	/* set key */
	svv *v;
	int rc = se_dbv(db, o, 1, &v);
	if (ssunlikely(rc == -1))
		goto e0;
	sv vp;
	sv_init(&vp, &sv_vif, v, NULL);
	/* set prefix */
	svv *vprf;
	rc = se_dbvprefix(db, o, &vprf);
	if (ssunlikely(rc == -1))
		goto e1;
	sv vprefix;
	sv_init(&vprefix, &sv_vif, vprf, NULL);
	/* if key is not set: use prefix */
	if (vprf && v == NULL) {
		v = sv_vdup(db->r.a, &vprefix);
		sv_init(&vp, &sv_vif, v, NULL);
	}
	sv vup;
	memset(&vup, 0, sizeof(vup));
	so_destroy(&o->o);
	o = NULL;

	/* concurrent */
	if (x_search) {
		/* note: prefix is ignored during concurrent
		 * index search */
		assert(v != NULL);
		int rc = sx_get(x, &db->coindex, &vp, &vup);
		if (rc == 2) /* delete */
			goto e2;
		if (rc == 1 && !sv_is(&vup, SVUPDATE)) {
			so *ret = se_vnew(e, &db->o, &vup, async);
			if (ssunlikely(ret == NULL))
				sv_vfree(db->r.a, vup.v);
			if (async) {
				sev *match = (sev*)ret;
				match->async_operation = SE_REQREAD;
				match->async_status    = 1;
				match->async_arg       = async_arg;
				match->async_seq       = 0;
			}
			if (vprf)
				sv_vfree(db->r.a, vprf);
			sv_vfree(db->r.a, v);
			return ret;
		}
	} else {
		sx_getstmt(&e->xm, &db->coindex);
	}

	/* prepare read cache */
	int cachegc = 0;
	if (cache == NULL) {
		cachegc = 1;
		cache = si_cachepool_pop(&e->cachepool);
		if (ssunlikely(cache == NULL)) {
			sr_oom(&e->error);
			goto e2;
		}
	}

	/* prepare request */
	sereq q;
	se_reqinit(e, &q, SE_REQREAD, &db->o, &db->o);
	sereqarg *arg = &q.arg;
	arg->v       = vp;
	arg->vup     = vup;
	arg->vprefix = vprefix;
	arg->cache   = cache;
	arg->cachegc = cachegc;
	arg->order   = order;
	arg->arg     = async_arg;
	if (x) {
		arg->vlsn = x->vlsn;
		arg->vlsn_generate = 0;
	} else {
		arg->vlsn = 0;
		arg->vlsn_generate = 1;
	}
	if (sf_updatehas(&db->scheme.fmt_update)) {
		arg->update = 1;
		if (arg->order == SS_EQ) {
			arg->order = SS_GTE;
			arg->update_eq = 1;
		}
	}

	/* asynchronous */
	if (async) {
		o = (sev*)se_reqresult(&q, 1);
		if (ssunlikely(o == NULL)) {
			se_reqend(&q);
			return NULL;
		}
		sereq *req = se_reqnew(e, &q, 1);
		if (ssunlikely(req == NULL)) {
			so_destroy(&o->o);
			se_reqend(&q);
			return NULL;
		}
		return o;
	}
	/* synchronous */
	rc = se_execute(&q);
	if (rc == 1)
		o = (sev*)se_reqresult(&q, async);
	se_reqend(&q);
	return o;
e2: if (vprf)
		sv_vfree(db->r.a, vprf);
e1: if (v)
		sv_vfree(db->r.a, v);
e0: if (o)
		so_destroy(&o->o);
	return NULL;
}

static inline int
se_dbwrite(sedb *db, sev *o, uint8_t flags)
{
	se *e = se_of(&db->o);
	/* validate req */
	if (ssunlikely(o->o.parent != &db->o)) {
		sr_error(&e->error, "%s", "bad object parent");
		return -1;
	}
	if (ssunlikely(! se_online(&db->status)))
		goto error;
	if (flags == SVUPDATE && !sf_updatehas(&db->scheme.fmt_update))
		flags = 0;

	/* prepare object */
	svv *v;
	int rc = se_dbv(db, o, 0, &v);
	if (ssunlikely(rc == -1))
		goto error;
	so_destroy(&o->o);
	v->flags = flags;
	svlogv lv;
	sv_logvinit(&lv, db->scheme.id);
	sv_init(&lv.v, &sv_vif, v, NULL);
	svlog log;
	sv_loginit(&log);
	sv_logadd(&log, db->r.a, &lv, db);

	/* concurrency */
	sxstate s = sx_setstmt(&e->xm, &db->coindex, &lv.v);
	rc = 1; /* rollback */
	switch (s) {
	case SXLOCK: rc = 2;
	case SXROLLBACK:
		sv_vfree(db->r.a, v);
		return rc;
	default: break;
	}

	/* ensure quota */
	int size = sizeof(svv) + sv_size(&lv.v);
	ss_quota(&e->quota, SS_QADD, size);

	/* execute req */
	sereq q;
	se_reqinit(e, &q, SE_REQWRITE, &db->o, &db->o);
	sereqarg *arg = &q.arg;
	arg->vlsn_generate = 1;
	arg->lsn = 0;
	arg->recover = 0;
	arg->log = &log;
	se_execute(&q);
	if (ssunlikely(q.rc == -1))
		ss_quota(&e->quota, SS_QREMOVE, size);
	se_reqend(&q);
	return q.rc;
error:
	so_destroy(&o->o);
	return -1;
}

static int
se_dbset(so *o, so *v)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	sev *key = se_cast(v, sev*, SEV);
	return se_dbwrite(db, key, 0);
}

static int
se_dbupdate(so *o, so *v)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	sev *key = se_cast(v, sev*, SEV);
	return se_dbwrite(db, key, SVUPDATE);
}

static int
se_dbdel(so *o, so *v)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	sev *key = se_cast(v, sev*, SEV);
	return se_dbwrite(db, key, SVDELETE);
}

static void*
se_dbget(so *o, so *v)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	sev *key = se_cast(v, sev*, SEV);
	return se_dbread(db, key, NULL, 0, NULL, key->order);
}

static void*
se_dbbatch(so *o)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	return se_batchnew(db);
}

static void*
se_dbobject(so *o)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	se *e = se_of(&db->o);
	return se_vnew(e, &db->o, NULL, 0);
}

static void*
se_dbget_string(so *o, const char *path, int *size)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	if (strcmp(path, "name") == 0) {
		int namesize = strlen(db->scheme.name) + 1;
		if (size)
			*size = namesize;
		char *name = malloc(namesize);
		if (name == NULL)
			return NULL;
		memcpy(name, db->scheme.name, namesize);
		return name;
	}
	return NULL;
}

static int64_t
se_dbget_int(so *o, const char *path)
{
	sedb *db = se_cast(o, sedb*, SEDB);
	if (strcmp(path, "id") == 0)
		return db->scheme.id;
	else
	if (strcmp(path, "key-count") == 0)
		return db->scheme.scheme.count;
	return -1;
}

static soif sedbif =
{
	.open         = se_dbopen,
	.destroy      = se_dbdestroy,
	.error        = NULL,
	.object       = se_dbobject,
	.asynchronous = se_dbasync,
	.poll         = NULL,
	.drop         = se_dbdrop,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = se_dbget_string,
	.getint       = se_dbget_int,
	.set          = se_dbset,
	.update       = se_dbupdate,
	.del          = se_dbdel,
	.get          = se_dbget,
	.batch        = se_dbbatch,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

so *se_dbnew(se *e, char *name)
{
	sedb *o = ss_malloc(&e->a_db, sizeof(sedb));
	if (ssunlikely(o == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	memset(o, 0, sizeof(*o));
	so_init(&o->o, &se_o[SEDB], &sedbif, &e->o, &e->o);
	so_init(&o->async, &se_o[SEDBASYNC], &sedbasyncif, &o->o, &e->o);
	so_listinit(&o->batch);
	se_statusinit(&o->status);
	se_statusset(&o->status, SE_OFFLINE);
	o->r         = e->r;
	o->r.scheme  = &o->scheme.scheme;
	o->created   = 0;
	o->scheduled = 0;
	o->dropped   = 0;
	memset(&o->rtp, 0, sizeof(o->rtp));
	int rc = se_dbscheme_init(o, name);
	if (ssunlikely(rc == -1)) {
		ss_free(&e->a_db, o);
		return NULL;
	}
	rc = si_init(&o->index, &o->r);
	if (ssunlikely(rc == -1)) {
		ss_free(&e->a_db, o);
		si_schemefree(&o->scheme, &o->r);
		return NULL;
	}
	sx_indexinit(&o->coindex, &o->r, o);
	ss_spinlockinit(&o->reflock);
	o->ref_be = 0;
	o->ref = 0;
	o->txn_min = sx_min(&e->xm);
	o->txn_max = o->txn_min;
	sd_cinit(&o->dc);
	return &o->o;
}

so *se_dbmatch(se *e, char *name)
{
	sslist *i;
	ss_listforeach(&e->db.list, i) {
		sedb *db = (sedb*)sscast(i, so, link);
		if (strcmp(db->scheme.name, name) == 0)
			return &db->o;
	}
	return NULL;
}

so *se_dbmatch_id(se *e, uint32_t id)
{
	sslist *i;
	ss_listforeach(&e->db.list, i) {
		sedb *db = (sedb*)sscast(i, so, link);
		if (db->scheme.id == id)
			return &db->o;
	}
	return NULL;
}

void se_dbref(sedb *o, int be)
{
	ss_spinlock(&o->reflock);
	if (be)
		o->ref_be++;
	else
		o->ref++;
	ss_spinunlock(&o->reflock);
}

uint32_t se_dbunref(sedb *o, int be)
{
	uint32_t prev_ref = 0;
	ss_spinlock(&o->reflock);
	if (be) {
		prev_ref = o->ref_be;
		if (o->ref_be > 0)
			o->ref_be--;
	} else {
		prev_ref = o->ref;
		if (o->ref > 0)
			o->ref--;
	}
	ss_spinunlock(&o->reflock);
	return prev_ref;
}

uint32_t se_dbrefof(sedb *o, int be)
{
	uint32_t ref = 0;
	ss_spinlock(&o->reflock);
	if (be)
		ref = o->ref_be;
	else
		ref = o->ref;
	ss_spinunlock(&o->reflock);
	return ref;
}

int se_dbgarbage(sedb *o)
{
	ss_spinlock(&o->reflock);
	int v = o->ref_be == 0 && o->ref == 0;
	ss_spinunlock(&o->reflock);
	return v;
}

int se_dbvisible(sedb *db, uint32_t txn)
{
	return db->txn_min < txn && txn <= db->txn_max;
}

void se_dbbind(se *e)
{
	sslist *i;
	ss_listforeach(&e->db.list, i) {
		sedb *db = (sedb*)sscast(i, so, link);
		int status = se_status(&db->status);
		if (se_statusactive_is(status))
			se_dbref(db, 1);
	}
}

void se_dbunbind(se *e, uint32_t txn)
{
	sslist *i;
	ss_listforeach(&e->db.list, i) {
		sedb *db = (sedb*)sscast(i, so, link);
		int status = se_status(&db->status);
		if (status != SE_ONLINE)
			continue;
		if (txn > db->txn_min)
			se_dbunref(db, 1);
	}
	ss_spinlock(&e->dblock);
	ss_listforeach(&e->db_shutdown.list, i) {
		sedb *db = (sedb*)sscast(i, so, link);
		if (se_dbvisible(db, txn))
			se_dbunref(db, 1);
	}
	ss_spinunlock(&e->dblock);
}

int se_dbmalfunction(sedb *o)
{
	se_statusset(&o->status, SE_MALFUNCTION);
	return -1;
}

int se_dbv(sedb *db, sev *o, int search, svv **v)
{
	se *e = se_of(&db->o);
	*v = NULL;
	/* reuse object */
	if (o->v.v) {
		*v = o->v.v;
		o->v.v = NULL;
		return 0;
	}
	/* create object from raw data */
	if (o->raw) {
		*v = sv_vbuildraw(db->r.a, o->raw, o->rawsize);
		goto ret;
	}
	sr *runtime = &db->r;
	sr  runtime_search;
	if (search) {
		if (o->keyc == 0)
			return 0;
		/* switch to key-value format to avoid value
		 * copy during search operations */
		if (db->r.fmt == SF_DOCUMENT) {
			runtime_search = db->r;
			runtime_search.fmt = SF_KV;
			runtime = &runtime_search;
		}
	}

	/* create object using current format, supplied
	 * key-chain and value */
	if (ssunlikely(o->keyc != db->scheme.scheme.count))
		return sr_error(&e->error, "%s", "bad object key");

	*v = sv_vbuild(runtime, o->keyv, o->keyc,
	               o->value,
	               o->valuesize);
ret:
	if (ssunlikely(*v == NULL))
		return sr_oom(&e->error);
	return 0;
}

int se_dbvprefix(sedb *db, sev *o, svv **v)
{
	se *e = se_of(&db->o);
	*v = NULL;
	/* reuse prefix */
	if (o->vprefix.v) {
		*v = o->vprefix.v;
		o->vprefix.v = NULL;
		return 0;
	}
	if (o->prefix == NULL)
		return 0;
	/* validate index type */
	if (sr_schemeof(&db->scheme.scheme, 0)->type != SS_STRING)
		return sr_error(&e->error, "%s", "prefix search is only "
		                "supported for a string key");
	/* create prefix object */
	sfv fv;
	fv.key      = o->prefix;
	fv.r.size   = o->prefixsize;
	fv.r.offset = 0;
	*v = sv_vbuild(&e->r, &fv, 1, NULL, 0);
	if (ssunlikely(*v == NULL))
		return -1;
	return 0;
}
#line 1 "sophia/environment/se_execute.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













int se_reqread(sereq *r)
{
	sereqarg *arg = &r->arg;
	sedb *db = (sedb*)r->db;
	uint32_t keysize;
	void *key;
	if (sslikely(arg->v.v)) {
		keysize = sv_size(&arg->v);
		key = sv_pointer(&arg->v);
	} else {
		keysize = 0;
		key = NULL;
	}
	char *prefix;
	uint32_t prefixsize;
	if (arg->vprefix.v) {
		void *vptr = sv_vpointer(arg->vprefix.v);
		prefix = sf_key(vptr, 0);
		prefixsize = sf_keysize(vptr, 0);
	} else {
		prefix = NULL;
		prefixsize = 0;
	}
	if (sslikely(arg->vlsn_generate))
		arg->vlsn = sr_seq(db->r.seq, SR_LSN);
	siquery q;
	si_queryopen(&q, arg->cache, &db->index,
	             arg->order,
	             arg->vlsn,
	             prefix,
	             prefixsize, key, keysize);
	if (arg->update)
		si_queryupdate(&q, &arg->vup, arg->update_eq);
	r->rc = si_query(&q);
	r->v = q.result.v;
	si_queryclose(&q);
	return r->rc;
}

int se_reqwrite(sereq *r)
{
	sereqarg *arg = &r->arg;
	svlog *log = r->arg.log;
	se *e = se_of(r->object);
	/* set lsn */
	sl_prepare(&e->lp, log, arg->lsn);
	/* log write */
	if (! arg->recover) {
		sltx tl;
		sl_begin(&e->lp, &tl);
		int rc = sl_write(&tl, log);
		if (ssunlikely(rc == -1)) {
			sl_rollback(&tl);
			r->rc = -1;
			return -1;
		}
		sl_commit(&tl);
	}
	/* commit */
	if (sslikely(arg->vlsn_generate))
		arg->vlsn = sx_vlsn(&e->xm);
	uint64_t now = ss_utime();
	svlogindex *i   = (svlogindex*)log->index.s;
	svlogindex *end = (svlogindex*)log->index.p;
	while (i < end) {
		sedb *db = i->ptr;
		sitx ti;
		si_begin(&ti, &db->index, arg->vlsn, now, log, i);
		si_write(&ti, arg->recover);
		si_commit(&ti);
		i++;
	}
	return 0;
}

int se_execute(sereq *r)
{
	switch (r->op) {
	case SE_REQREAD:  return se_reqread(r);
	case SE_REQWRITE: return se_reqwrite(r);
	default: assert(0);
	}
	return 0;
}
#line 1 "sophia/environment/se_meta.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static inline int
se_metav(srmeta *c, srmetastmt *s)
{
	switch (s->op) {
	case SR_SERIALIZE: return sr_meta_serialize(c, s);
	case SR_READ:      return sr_meta_read(c, s);
	case SR_WRITE:     return sr_meta_write(c, s);
	}
	assert(0);
	return -1;
}

static inline int
se_metav_offline(srmeta *c, srmetastmt *s)
{
	se *e = s->ptr;
	if (s->op == SR_WRITE) {
		if (se_status(&e->status)) {
			sr_error(s->r->e, "write to %s is offline-only", s->path);
			return -1;
		}
	}
	return se_metav(c, s);
}

static inline int
se_metasophia_error(srmeta *c, srmetastmt *s)
{
	se *e = s->ptr;
	char *errorp;
	char  error[128];
	error[0] = 0;
	int len = sr_errorcopy(&e->error, error, sizeof(error));
	if (sslikely(len == 0))
		errorp = NULL;
	else
		errorp = error;
	srmeta meta = {
		.key      = c->key,
		.flags    = c->flags,
		.type     = c->type,
		.function = NULL,
		.value    = errorp,
		.ptr      = NULL,
		.next     = NULL
	};
	return se_metav(&meta, s);
}

static inline srmeta*
se_metasophia(se *e, semetart *rt, srmeta **pc)
{
	srmeta *sophia = *pc;
	srmeta *p = NULL;
	sr_M(&p, pc, se_metav, "version", SS_STRING, rt->version, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "build", SS_STRING, rt->build, SR_RO, NULL);
	sr_M(&p, pc, se_metasophia_error, "error", SS_STRING, NULL, SR_RO, NULL);
	sr_m(&p, pc, se_metav_offline, "path", SS_STRINGPTR, &e->meta.path);
	sr_m(&p, pc, se_metav_offline, "path_create", SS_U32, &e->meta.path_create);
	return sr_M(NULL, pc, NULL, "sophia", SS_UNDEF, sophia, SR_NS, NULL);
}

static inline srmeta*
se_metamemory(se *e, semetart *rt, srmeta **pc)
{
	srmeta *memory = *pc;
	srmeta *p = NULL;
	sr_m(&p, pc, se_metav_offline, "limit", SS_U64, &e->meta.memory_limit);
	sr_M(&p, pc, se_metav, "used", SS_U64, &rt->memory_used, SR_RO, NULL);
	return sr_M(NULL, pc, NULL, "memory", SS_UNDEF, memory, SR_NS, NULL);
}

static inline int
se_metacompaction_set(srmeta *c ssunused, srmetastmt *s)
{
	se *e = s->ptr;
	if (s->op != SR_WRITE) {
		sr_error(&e->error, "%s", "bad operation");
		return -1;
	}
	if (ssunlikely(se_statusactive(&e->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	/* validate argument */
	uint32_t percent = *(uint32_t*)s->value;
	if (percent > 100) {
		sr_error(&e->error, "%s", "bad argument");
		return -1;
	}
	srzone z;
	memset(&z, 0, sizeof(z));
	z.enable = 1;
	sr_zonemap_set(&e->meta.zones, percent, &z);
	return 0;
}

static inline srmeta*
se_metacompaction(se *e, semetart *rt ssunused, srmeta **pc)
{
	srmeta *compaction = *pc;
	srmeta *prev;
	srmeta *p = NULL;
	sr_m(&p, pc, se_metav_offline, "node_size", SS_U32, &e->meta.node_size);
	sr_m(&p, pc, se_metav_offline, "page_size", SS_U32, &e->meta.page_size);
	sr_m(&p, pc, se_metav_offline, "page_checksum", SS_U32, &e->meta.page_checksum);
	prev = p;
	int i = 0;
	for (; i < 11; i++) {
		srzone *z = &e->meta.zones.zones[i];
		if (! z->enable)
			continue;
		srmeta *zone = *pc;
		p = NULL;
		sr_m(&p, pc, se_metav_offline, "mode", SS_U32, &z->mode);
		sr_m(&p, pc, se_metav_offline, "compact_wm", SS_U32, &z->compact_wm);
		sr_m(&p, pc, se_metav_offline, "branch_prio", SS_U32, &z->branch_prio);
		sr_m(&p, pc, se_metav_offline, "branch_wm", SS_U32, &z->branch_wm);
		sr_m(&p, pc, se_metav_offline, "branch_age", SS_U32, &z->branch_age);
		sr_m(&p, pc, se_metav_offline, "branch_age_period", SS_U32, &z->branch_age_period);
		sr_m(&p, pc, se_metav_offline, "branch_age_wm", SS_U32, &z->branch_age_wm);
		sr_m(&p, pc, se_metav_offline, "backup_prio", SS_U32, &z->backup_prio);
		sr_m(&p, pc, se_metav_offline, "gc_wm", SS_U32, &z->gc_wm);
		sr_m(&p, pc, se_metav_offline, "gc_db_prio", SS_U32, &z->gc_db_prio);
		sr_m(&p, pc, se_metav_offline, "gc_prio", SS_U32, &z->gc_prio);
		sr_m(&p, pc, se_metav_offline, "gc_period", SS_U32, &z->gc_period);
		sr_m(&p, pc, se_metav_offline, "async", SS_U32, &z->async);
		sr_M(&prev, pc, NULL, z->name, SS_UNDEF, zone, SR_NS, NULL);
	}
	return sr_M(NULL, pc, se_metacompaction_set, "compaction", SS_U32,
	            compaction, SR_NS, NULL);
}

static inline int
se_metascheduler_trace(srmeta *c, srmetastmt *s)
{
	seworker *w = c->value;
	char tracesz[128];
	char *trace;
	int tracelen = ss_tracecopy(&w->trace, tracesz, sizeof(tracesz));
	if (sslikely(tracelen == 0))
		trace = NULL;
	else
		trace = tracesz;
	srmeta meta = {
		.key      = c->key,
		.flags    = c->flags,
		.type     = c->type,
		.function = NULL,
		.value    = trace,
		.ptr      = NULL,
		.next     = NULL
	};
	return se_metav(&meta, s);
}

static inline int
se_metascheduler_checkpoint(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	se *e = s->ptr;
	return se_scheduler_checkpoint(e);
}

static inline int
se_metascheduler_on_recover(srmeta *c, srmetastmt *s)
{
	se *e = s->ptr;
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	if (ssunlikely(se_statusactive(&e->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	e->meta.on_recover.function =
		(serecovercbf)(uintptr_t)s->value;
	return 0;
}

static inline int
se_metascheduler_on_recover_arg(srmeta *c, srmetastmt *s)
{
	se *e = s->ptr;
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	if (ssunlikely(se_statusactive(&e->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	e->meta.on_recover.arg = s->value;
	return 0;
}

static inline int
se_metascheduler_on_event(srmeta *c, srmetastmt *s)
{
	se *e = s->ptr;
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	if (ssunlikely(se_statusactive(&e->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	ss_triggerset(&e->meta.on_event, s->value);
	return 0;
}

static inline int
se_metascheduler_on_event_arg(srmeta *c, srmetastmt *s)
{
	se *e = s->ptr;
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	if (ssunlikely(se_statusactive(&e->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	ss_triggerset_arg(&e->meta.on_event, s->value);
	return 0;
}

static inline int
se_metascheduler_gc(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	se *e = s->ptr;
	return se_scheduler_gc(e);
}

static inline int
se_metascheduler_run(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	se *e = s->ptr;
	return se_scheduler_call(e);
}

static inline srmeta*
se_metascheduler(se *e, semetart *rt, srmeta **pc)
{
	srmeta *scheduler = *pc;
	srmeta *prev;
	srmeta *p = NULL;
	sr_m(&p, pc, se_metav_offline, "threads", SS_U32, &e->meta.threads);
	sr_M(&p, pc, se_metav, "zone", SS_STRING, rt->zone, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "checkpoint_active", SS_U32, &rt->checkpoint_active, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "checkpoint_lsn", SS_U64, &rt->checkpoint_lsn, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "checkpoint_lsn_last", SS_U64, &rt->checkpoint_lsn_last, SR_RO, NULL);
	sr_m(&p, pc, se_metascheduler_checkpoint, "checkpoint",  SS_FUNCTION, NULL);
	sr_m(&p, pc, se_metascheduler_on_recover, "on_recover", SS_STRING, NULL);
	sr_m(&p, pc, se_metascheduler_on_recover_arg, "on_recover_arg", SS_STRING, NULL);
	sr_m(&p, pc, se_metascheduler_on_event, "on_event", SS_STRING, NULL);
	sr_m(&p, pc, se_metascheduler_on_event_arg, "on_event_arg", SS_STRING, NULL);
	sr_m(&p, pc, se_metav_offline, "event_on_backup", SS_U32, &e->meta.event_on_backup);
	sr_M(&p, pc, se_metav, "gc_active", SS_U32, &rt->gc_active, SR_RO, NULL);
	sr_m(&p, pc, se_metascheduler_gc, "gc", SS_FUNCTION, NULL);
	sr_M(&p, pc, se_metav, "reqs", SS_U32, &rt->reqs, SR_RO, NULL);
	sr_m(&p, pc, se_metascheduler_run, "run", SS_FUNCTION, NULL);
	prev = p;
	sslist *i;
	ss_listforeach(&e->sched.workers.list, i) {
		seworker *w = sscast(i, seworker, link);
		srmeta *worker = *pc;
		p = NULL;
		sr_M(&p, pc, se_metascheduler_trace, "trace", SS_STRING, w, SR_RO, NULL);
		sr_M(&prev, pc, NULL, w->name, SS_UNDEF, worker, SR_NS, NULL);
	}
	return sr_M(NULL, pc, NULL, "scheduler", SS_UNDEF, scheduler, SR_NS, NULL);
}

static inline int
se_metalog_rotate(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	se *e = s->ptr;
	return sl_poolrotate(&e->lp);
}

static inline int
se_metalog_gc(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	se *e = s->ptr;
	return sl_poolgc(&e->lp);
}

static inline srmeta*
se_metalog(se *e, semetart *rt, srmeta **pc)
{
	srmeta *log = *pc;
	srmeta *p = NULL;
	sr_m(&p, pc, se_metav_offline, "enable", SS_U32, &e->meta.log_enable);
	sr_m(&p, pc, se_metav_offline, "path", SS_STRINGPTR, &e->meta.log_path);
	sr_m(&p, pc, se_metav_offline, "sync", SS_U32, &e->meta.log_sync);
	sr_m(&p, pc, se_metav_offline, "rotate_wm", SS_U32, &e->meta.log_rotate_wm);
	sr_m(&p, pc, se_metav_offline, "rotate_sync", SS_U32, &e->meta.log_rotate_sync);
	sr_m(&p, pc, se_metalog_rotate, "rotate", SS_FUNCTION, NULL);
	sr_m(&p, pc, se_metalog_gc, "gc", SS_FUNCTION, NULL);
	sr_M(&p, pc, se_metav, "files", SS_U32, &rt->log_files, SR_RO, NULL);
	sr_m(&p, pc, se_metav_offline, "two_phase_recover", SS_U32, &e->meta.two_phase_recover);
	sr_m(&p, pc, se_metav_offline, "commit_lsn", SS_U32, &e->meta.commit_lsn);
	return sr_M(NULL, pc, NULL, "log", SS_UNDEF, log, SR_NS, NULL);
}

static inline srmeta*
se_metametric(se *e ssunused, semetart *rt, srmeta **pc)
{
	srmeta *metric = *pc;
	srmeta *p = NULL;
	sr_M(&p, pc, se_metav, "dsn",  SS_U32, &rt->seq.dsn, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "nsn",  SS_U32, &rt->seq.nsn, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "bsn",  SS_U32, &rt->seq.bsn, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "lsn",  SS_U64, &rt->seq.lsn, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "lfsn", SS_U32, &rt->seq.lfsn, SR_RO, NULL);
	sr_M(&p, pc, se_metav, "tsn",  SS_U32, &rt->seq.tsn, SR_RO, NULL);
	return sr_M(NULL, pc, NULL, "metric", SS_UNDEF, metric, SR_NS, NULL);
}

static inline int
se_metadb_set(srmeta *c ssunused, srmetastmt *s)
{
	/* set(db) */
	se *e = s->ptr;
	if (s->op != SR_WRITE) {
		sr_error(&e->error, "%s", "bad operation");
		return -1;
	}
	char *name = s->value;
	sedb *db = (sedb*)se_dbmatch(e, name);
	if (ssunlikely(db)) {
		sr_error(&e->error, "database '%s' already exists", name);
		return -1;
	}
	db = (sedb*)se_dbnew(e, name);
	if (ssunlikely(db == NULL))
		return -1;
	so_listadd(&e->db, &db->o);
	return 0;
}

static inline int
se_metadb_get(srmeta *c, srmetastmt *s)
{
	/* get(db.name) */
	se *e = s->ptr;
	if (s->op != SR_READ) {
		sr_error(&e->error, "%s", "bad operation");
		return -1;
	}
	assert(c->ptr != NULL);
	sedb *db = c->ptr;
	se_dbref(db, 0);
	*(void**)s->value = db;
	return 0;
}

static inline int
se_metadb_update(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	sedb *db = c->ptr;
	if (ssunlikely(se_statusactive(&db->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	/* set update function */
	sfupdatef update = (sfupdatef)(uintptr_t)s->value;
	sf_updateset(&db->scheme.fmt_update, update);
	return 0;
}

static inline int
se_metadb_updatearg(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	sedb *db = c->ptr;
	if (ssunlikely(se_statusactive(&db->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	sf_updateset_arg(&db->scheme.fmt_update, s->value);
	return 0;
}

static inline int
se_metadb_status(srmeta *c, srmetastmt *s)
{
	sedb *db = c->value;
	char *status = se_statusof(&db->status);
	srmeta meta = {
		.key      = c->key,
		.flags    = c->flags,
		.type     = c->type,
		.function = NULL,
		.value    = status,
		.ptr      = NULL,
		.next     = NULL
	};
	return se_metav(&meta, s);
}

static inline int
se_metadb_branch(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	sedb *db = c->value;
	return se_scheduler_branch(db);
}

static inline int
se_metadb_compact(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	sedb *db = c->value;
	return se_scheduler_compact(db);
}

static inline int
se_metadb_deadlock(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	if (s->valuetype != SS_OBJECT) {
		sr_error(s->r->e, "%s", "deadlock(transaction) expected");
		return -1;
	}
	setx *tx = se_cast(s->value, setx*, SETX);
	int rc = sx_deadlock(&tx->t);
	return rc;
}

static inline int
se_metav_dboffline(srmeta *c, srmetastmt *s)
{
	sedb *db = c->ptr;
	if (s->op == SR_WRITE) {
		if (se_status(&db->status)) {
			sr_error(s->r->e, "write to %s is offline-only", s->path);
			return -1;
		}
	}
	return se_metav(c, s);
}

static inline int
se_metadb_index(srmeta *c ssunused, srmetastmt *s)
{
	/* set(index, key) */
	sedb *db = c->ptr;
	se *e = se_of(&db->o);
	if (s->op != SR_WRITE) {
		sr_error(&e->error, "%s", "bad operation");
		return -1;
	}
	if (ssunlikely(se_statusactive(&db->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	char *name = s->value;
	srkey *part = sr_schemefind(&db->scheme.scheme, name);
	if (ssunlikely(part)) {
		sr_error(&e->error, "keypart '%s' already exists", name);
		return -1;
	}
	/* create new key-part */
	part = sr_schemeadd(&db->scheme.scheme, &e->a);
	if (ssunlikely(part == NULL))
		return -1;
	int rc = sr_keysetname(part, &e->a, name);
	if (ssunlikely(rc == -1))
		goto error;
	rc = sr_keyset(part, &e->a, "string");
	if (ssunlikely(rc == -1))
		goto error;
	return 0;
error:
	sr_schemedelete(&db->scheme.scheme, &e->a, part->pos);
	return -1;
}

static inline int
se_metadb_key(srmeta *c, srmetastmt *s)
{
	sedb *db = c->ptr;
	se *e = se_of(&db->o);
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	if (ssunlikely(se_statusactive(&db->status))) {
		sr_error(s->r->e, "write to %s is offline-only", s->path);
		return -1;
	}
	char *path = s->value;
	/* update key-part path */
	srkey *part = sr_schemefind(&db->scheme.scheme, c->key);
	assert(part != NULL);
	return sr_keyset(part, &e->a, path);
}

static inline srmeta*
se_metadb(se *e, semetart *rt ssunused, srmeta **pc)
{
	srmeta *db = NULL;
	srmeta *prev = NULL;
	srmeta *p;
	sslist *i;
	ss_listforeach(&e->db.list, i)
	{
		sedb *o = (sedb*)sscast(i, so, link);
		si_profilerbegin(&o->rtp, &o->index);
		si_profiler(&o->rtp);
		si_profilerend(&o->rtp);
		/* database index */
		srmeta *index = *pc;
		p = NULL;
		sr_M(&p, pc, se_metav, "memory_used", SS_U64, &o->rtp.memory_used, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "node_count", SS_U32, &o->rtp.total_node_count, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "node_size", SS_U64, &o->rtp.total_node_size, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "node_origin_size", SS_U64, &o->rtp.total_node_origin_size, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "count", SS_U64, &o->rtp.count, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "count_dup", SS_U64, &o->rtp.count_dup, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "read_disk", SS_U64, &o->rtp.read_disk, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "read_cache", SS_U64, &o->rtp.read_cache, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "branch_count", SS_U32, &o->rtp.total_branch_count, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "branch_avg", SS_U32, &o->rtp.total_branch_avg, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "branch_max", SS_U32, &o->rtp.total_branch_max, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "branch_histogram", SS_STRINGPTR, &o->rtp.histogram_branch_ptr, SR_RO, NULL);
		sr_M(&p, pc, se_metav, "page_count", SS_U32, &o->rtp.total_page_count, SR_RO, NULL);
		sr_M(&p, pc, se_metadb_update, "update", SS_STRING, NULL, 0, o);
		sr_M(&p, pc, se_metadb_updatearg, "update_arg", SS_STRING, NULL, 0, o);
		/* index keys */
		int i = 0;
		while (i < o->scheme.scheme.count) {
			srkey *part = sr_schemeof(&o->scheme.scheme, i);
			sr_M(&p, pc, se_metadb_key, part->name, SS_STRING, part->path, 0, o);
			i++;
		}
		/* database */
		srmeta *database = *pc;
		p = NULL;
		sr_M(&p, pc, se_metav, "name", SS_STRINGPTR, &o->scheme.name, SR_RO, NULL);
		sr_M(&p, pc, se_metav_dboffline, "id", SS_U32, &o->scheme.id, 0, o);
		sr_M(&p, pc, se_metadb_status,   "status", SS_STRING, o, SR_RO, NULL);
		sr_M(&p, pc, se_metav_dboffline, "format", SS_STRINGPTR, &o->scheme.fmt_sz, 0, o);
		sr_M(&p, pc, se_metav_dboffline, "path", SS_STRINGPTR, &o->scheme.path, 0, o);
		sr_M(&p, pc, se_metav_dboffline, "path_fail_on_exists", SS_U32, &o->scheme.path_fail_on_exists, 0, o);
		sr_M(&p, pc, se_metav_dboffline, "path_fail_on_drop", SS_U32, &o->scheme.path_fail_on_drop, 0, o);
		sr_M(&p, pc, se_metav_dboffline, "sync", SS_U32, &o->scheme.sync, 0, o);
		sr_M(&p, pc, se_metav_dboffline, "mmap", SS_U32, &o->scheme.mmap, 0, o);
		sr_M(&p, pc, se_metav_dboffline, "compression_key", SS_U32, &o->scheme.compression_key, 0, o);
		sr_M(&p, pc, se_metav_dboffline, "compression", SS_STRINGPTR, &o->scheme.compression_sz, 0, o);
		sr_m(&p, pc, se_metadb_branch, "branch", SS_FUNCTION, o);
		sr_m(&p, pc, se_metadb_compact, "compact", SS_FUNCTION, o);
		sr_m(&p, pc, se_metadb_deadlock, "deadlock", SS_FUNCTION, NULL);
		sr_M(&p, pc, se_metadb_index, "index", SS_UNDEF, index, SR_NS, o);
		sr_M(&prev, pc, se_metadb_get, o->scheme.name, SS_STRING, database, SR_NS, o);
		if (db == NULL)
			db = prev;
	}
	return sr_M(NULL, pc, se_metadb_set, "db", SS_STRING, db, SR_NS, NULL);
}

static inline int
se_metasnapshot_set(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	se *e = s->ptr;
	char *name = s->value;
	uint64_t lsn = sr_seq(&e->seq, SR_LSN);
	/* create snapshot object */
	sesnapshot *snapshot =
		(sesnapshot*)se_snapshotnew(e, lsn, name);
	if (ssunlikely(snapshot == NULL))
		return -1;
	so_listadd(&e->snapshot, &snapshot->o);
	return 0;
}

static inline int
se_metasnapshot_lsn(srmeta *c, srmetastmt *s)
{
	int rc = se_metav(c, s);
	if (ssunlikely(rc == -1))
		return -1;
	if (s->op != SR_WRITE)
		return 0;
	sesnapshot *snapshot = c->ptr;
	se_snapshotupdate(snapshot);
	return 0;
}

static inline int
se_metasnapshot_get(srmeta *c, srmetastmt *s)
{
	/* get(snapshot.name) */
	se *e = s->ptr;
	if (s->op != SR_READ) {
		sr_error(&e->error, "%s", "bad operation");
		return -1;
	}
	assert(c->ptr != NULL);
	*(void**)s->value = c->ptr;
	return 0;
}

static inline srmeta*
se_metasnapshot(se *e, semetart *rt ssunused, srmeta **pc)
{
	srmeta *snapshot = NULL;
	srmeta *prev = NULL;
	sslist *i;
	ss_listforeach(&e->snapshot.list, i)
	{
		sesnapshot *s = (sesnapshot*)sscast(i, so, link);
		srmeta *p = sr_M(NULL, pc, se_metasnapshot_lsn, "lsn", SS_U64, &s->vlsn, 0, s);
		sr_M(&prev, pc, se_metasnapshot_get, s->name, SS_STRING, p, SR_NS, s);
		if (snapshot == NULL)
			snapshot = prev;
	}
	return sr_M(NULL, pc, se_metasnapshot_set, "snapshot", SS_STRING,
	            snapshot, SR_NS, NULL);
}

static inline int
se_metabackup_run(srmeta *c, srmetastmt *s)
{
	if (s->op != SR_WRITE)
		return se_metav(c, s);
	se *e = s->ptr;
	return se_scheduler_backup(e);
}

static inline srmeta*
se_metabackup(se *e, semetart *rt, srmeta **pc)
{
	srmeta *backup = *pc;
	srmeta *p = NULL;
	sr_m(&p, pc, se_metav_offline, "path", SS_STRINGPTR, &e->meta.backup_path);
	sr_m(&p, pc, se_metabackup_run, "run", SS_FUNCTION, NULL);
	sr_M(&p, pc, se_metav, "active", SS_U32, &rt->backup_active, SR_RO, NULL);
	sr_m(&p, pc, se_metav, "last", SS_U32, &rt->backup_last);
	sr_m(&p, pc, se_metav, "last_complete", SS_U32, &rt->backup_last_complete);
	return sr_M(NULL, pc, NULL, "backup", 0, backup, SR_NS, NULL);
}

static inline srmeta*
se_metadebug(se *e, semetart *rt ssunused, srmeta **pc)
{
	srmeta *prev = NULL;
	srmeta *p = NULL;
	prev = p;
	srmeta *ei = *pc;
	sr_m(&p, pc, se_metav, "sd_build_0",      SS_U32, &e->ei.e[0]);
	sr_m(&p, pc, se_metav, "sd_build_1",      SS_U32, &e->ei.e[1]);
	sr_m(&p, pc, se_metav, "si_branch_0",     SS_U32, &e->ei.e[2]);
	sr_m(&p, pc, se_metav, "si_compaction_0", SS_U32, &e->ei.e[3]);
	sr_m(&p, pc, se_metav, "si_compaction_1", SS_U32, &e->ei.e[4]);
	sr_m(&p, pc, se_metav, "si_compaction_2", SS_U32, &e->ei.e[5]);
	sr_m(&p, pc, se_metav, "si_compaction_3", SS_U32, &e->ei.e[6]);
	sr_m(&p, pc, se_metav, "si_compaction_4", SS_U32, &e->ei.e[7]);
	sr_m(&p, pc, se_metav, "si_recover_0",    SS_U32, &e->ei.e[8]);
	sr_M(&prev, pc, se_metadb_set, "error_injection", SS_UNDEF, ei, SR_NS, NULL);
	srmeta *debug = prev;
	return sr_M(NULL, pc, NULL, "debug", SS_UNDEF, debug, SR_NS, NULL);
}

static srmeta*
se_metaprepare(se *e, semetart *rt, srmeta *c, int serialize)
{
	/* sophia */
	srmeta *pc = c;
	srmeta *sophia     = se_metasophia(e, rt, &pc);
	srmeta *memory     = se_metamemory(e, rt, &pc);
	srmeta *compaction = se_metacompaction(e, rt, &pc);
	srmeta *scheduler  = se_metascheduler(e, rt, &pc);
	srmeta *metric     = se_metametric(e, rt, &pc);
	srmeta *log        = se_metalog(e, rt, &pc);
	srmeta *snapshot   = se_metasnapshot(e, rt, &pc);
	srmeta *backup     = se_metabackup(e, rt, &pc);
	srmeta *db         = se_metadb(e, rt, &pc);
	srmeta *debug      = se_metadebug(e, rt, &pc);

	sophia->next     = memory;
	memory->next     = compaction;
	compaction->next = scheduler;
	scheduler->next  = metric;
	metric->next     = log;
	log->next        = snapshot;
	snapshot->next   = backup;
	backup->next     = db;
	if (! serialize)
		db->next = debug;
	return sophia;
}

static int
se_metart(se *e, semetart *rt)
{
	/* sophia */
	snprintf(rt->version, sizeof(rt->version),
	         "%d.%d.%d",
	         SR_VERSION_A - '0',
	         SR_VERSION_B - '0',
	         SR_VERSION_C - '0');
	snprintf(rt->build, sizeof(rt->build), "%s",
	         SR_VERSION_COMMIT);

	/* memory */
	rt->memory_used = ss_quotaused(&e->quota);

	/* scheduler */
	ss_mutexlock(&e->sched.lock);
	rt->checkpoint_active    = e->sched.checkpoint;
	rt->checkpoint_lsn_last  = e->sched.checkpoint_lsn_last;
	rt->checkpoint_lsn       = e->sched.checkpoint_lsn;
	rt->backup_active        = e->sched.backup;
	rt->backup_last          = e->sched.backup_last;
	rt->backup_last_complete = e->sched.backup_last_complete;
	rt->gc_active            = e->sched.gc;
	ss_mutexunlock(&e->sched.lock);

	/* requests */
	rt->reqs = se_reqcount(e);

	ss_mutexlock(&e->reqlock);
	rt->reqs = e->req.n + e->reqactive.n + e->reqready.n;
	ss_mutexunlock(&e->reqlock);

	int v = ss_quotaused_percent(&e->quota);
	srzone *z = sr_zonemap(&e->meta.zones, v);
	memcpy(rt->zone, z->name, sizeof(rt->zone));

	/* log */
	rt->log_files = sl_poolfiles(&e->lp);

	/* metric */
	sr_seqlock(&e->seq);
	rt->seq = e->seq;
	sr_sequnlock(&e->seq);
	return 0;
}

int se_metaserialize(semeta *c, ssbuf *buf)
{
	se *e = (se*)c->env;
	semetart rt;
	se_metart(e, &rt);
	srmeta meta[1024];
	srmeta *root;
	root = se_metaprepare(e, &rt, meta, 1);
	srmetastmt stmt = {
		.op        = SR_SERIALIZE,
		.path      = NULL,
		.value     = NULL,
		.valuesize = 0,
		.valuetype = SS_UNDEF,
		.serialize = buf,
		.ptr       = e,
		.r         = &e->r
	};
	return sr_metaexec(root, &stmt);
}

static int
se_metaquery(se *e, int op, const char *path,
             sstype valuetype, void *value, int valuesize,
             int *size)
{
	semetart rt;
	se_metart(e, &rt);
	srmeta meta[1024];
	srmeta *root;
	root = se_metaprepare(e, &rt, meta, 0);
	srmetastmt stmt = {
		.op        = op,
		.path      = path,
		.value     = value,
		.valuesize = valuesize,
		.valuetype = valuetype,
		.serialize = NULL,
		.ptr       = e,
		.r         = &e->r
	};
	int rc = sr_metaexec(root, &stmt);
	if (size)
		*size = stmt.valuesize;
	return rc;
}

int se_metaset_object(so *o, const char *path, void *object)
{
	se *e = se_of(o);
	return se_metaquery(e, SR_WRITE, path, SS_OBJECT,
	                    object, sizeof(so*), NULL);
}

int se_metaset_string(so *o, const char *path, void *string, int size)
{
	se *e = se_of(o);
	if (string && size == 0)
		size = strlen(string) + 1;
	return se_metaquery(e, SR_WRITE, path, SS_STRING,
	                   string, size, NULL);
}

int se_metaset_int(so *o, const char *path, int64_t v)
{
	se *e = se_of(o);
	return se_metaquery(e, SR_WRITE, path, SS_I64,
	                    &v, sizeof(v), NULL);
}

void *se_metaget_object(so *o, const char *path)
{
	se *e = se_of(o);
	if (path == NULL)
		return se_metacursor_new(o);
	void *result = NULL;
	int rc = se_metaquery(e, SR_READ, path, SS_OBJECT,
	                      &result, sizeof(void*), NULL);
	if (ssunlikely(rc == -1))
		return NULL;
	return result;
}

void *se_metaget_string(so *o, const char *path, int *size)
{
	se *e = se_of(o);
	void *result = NULL;
	int rc = se_metaquery(e, SR_READ, path, SS_STRING,
	                      &result, sizeof(void*), size);
	if (ssunlikely(rc == -1))
		return NULL;
	return result;
}

int64_t se_metaget_int(so *o, const char *path)
{
	se *e = se_of(o);
	int64_t result = 0;
	int rc = se_metaquery(e, SR_READ, path, SS_I64,
	                      &result, sizeof(void*), NULL);
	if (ssunlikely(rc == -1))
		return -1;
	return result;
}

void se_metainit(semeta *c, so *e)
{
	se *o = (se*)e;
	sr_schemeinit(&c->scheme);
	srkey *part = sr_schemeadd(&c->scheme, &o->a);
	sr_keysetname(part, &o->a, "key");
	sr_keyset(part, &o->a, "string");
	c->env                 = e;
	c->path                = NULL;
	c->path_create         = 1;
	c->memory_limit        = 0;
	c->node_size           = 64 * 1024 * 1024;
	c->page_size           = 64 * 1024;
	c->page_checksum       = 1;
	c->threads             = 6;
	c->log_enable          = 1;
	c->log_path            = NULL;
	c->log_rotate_wm       = 500000;
	c->log_sync            = 0;
	c->log_rotate_sync     = 1;
	c->two_phase_recover   = 0;
	c->commit_lsn          = 0;
	c->on_recover.function = NULL;
	c->on_recover.arg      = NULL;
	ss_triggerinit(&c->on_event);
	c->event_on_backup     = 0;
	srzone def = {
		.enable        = 1,
		.mode          = 3, /* branch + compact */
		.compact_wm    = 2,
		.branch_prio   = 1,
		.branch_wm     = 10 * 1024 * 1024,
		.branch_age    = 40,
		.branch_age_period = 40,
		.branch_age_wm = 1 * 1024 * 1024,
		.backup_prio   = 1,
		.gc_db_prio    = 1,
		.gc_prio       = 1,
		.gc_period     = 60,
		.gc_wm         = 30,
		.async         = 2 /* do not own thread */
	};
	srzone redzone = {
		.enable        = 1,
		.mode          = 2, /* checkpoint */
		.compact_wm    = 4,
		.branch_prio   = 0,
		.branch_wm     = 0,
		.branch_age    = 0,
		.branch_age_period = 0,
		.branch_age_wm = 0,
		.backup_prio   = 0,
		.gc_db_prio    = 0,
		.gc_prio       = 0,
		.gc_period     = 0,
		.gc_wm         = 0,
		.async         = 2
	};
	sr_zonemap_set(&o->meta.zones,  0, &def);
	sr_zonemap_set(&o->meta.zones, 80, &redzone);
	c->backup_path = NULL;
}

void se_metafree(semeta *c)
{
	se *e = (se*)c->env;
	if (c->path) {
		ss_free(&e->a, c->path);
		c->path = NULL;
	}
	if (c->log_path) {
		ss_free(&e->a, c->log_path);
		c->log_path = NULL;
	}
	if (c->backup_path) {
		ss_free(&e->a, c->backup_path);
		c->backup_path = NULL;
	}
	sr_schemefree(&c->scheme, &e->a);
}

int se_metavalidate(semeta *c)
{
	se *e = (se*)c->env;
	if (c->path == NULL) {
		sr_error(&e->error, "%s", "repository path is not set");
		return -1;
	}
	char path[1024];
	if (c->log_path == NULL) {
		snprintf(path, sizeof(path), "%s/log", c->path);
		c->log_path = ss_strdup(&e->a, path);
		if (ssunlikely(c->log_path == NULL)) {
			return sr_oom(&e->error);
		}
	}
	int i = 0;
	for (; i < 11; i++) {
		srzone *z = &e->meta.zones.zones[i];
		if (! z->enable)
			continue;
		if (z->compact_wm <= 1) {
			sr_error(&e->error, "bad %d.compact_wm value", i * 10);
			return -1;
		}
	}
	return 0;
}
#line 1 "sophia/environment/se_metacursor.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_metav_destroy(so *o)
{
	semetav *v = se_cast(o, semetav*, SEMETAV);
	se *e = se_of(o);
	ss_free(&e->a, v->key);
	if (v->value)
		ss_free(&e->a, v->value);
	se_mark_destroyed(&v->o);
	ss_free(&e->a_metav, v);
	return 0;
}

void *se_metav_string(so *o, const char *path, int *size)
{
	semetav *v = se_cast(o, semetav*, SEMETAV);
	if (strcmp(path, "key") == 0) {
		if (size)
			*size = v->keysize;
		return v->key;
	} else
	if (strcmp(path, "value") == 0) {
		if (size)
			*size = v->valuesize;
		return v->value;
	}
	return NULL;
}

static soif semetavif =
{
	.open         = NULL,
	.destroy      = se_metav_destroy,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = se_metav_string,
	.getint       = NULL,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = NULL,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

static inline so *se_metav_new(se *e, srmetadump *vp)
{
	semetav *v = ss_malloc(&e->a_metav, sizeof(semetav));
	if (ssunlikely(v == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	so_init(&v->o, &se_o[SEMETAV], &semetavif, &e->o, &e->o);
	v->keysize = vp->keysize;
	v->key = ss_malloc(&e->a, v->keysize);
	if (ssunlikely(v->key == NULL)) {
		se_mark_destroyed(&v->o);
		ss_free(&e->a_metav, v);
		return NULL;
	}
	memcpy(v->key, sr_metakey(vp), v->keysize);
	v->valuesize = vp->valuesize;
	v->value = NULL;
	if (v->valuesize > 0) {
		v->value = ss_malloc(&e->a, v->valuesize);
		if (ssunlikely(v->key == NULL)) {
			ss_free(&e->a, v->key);
			se_mark_destroyed(&v->o);
			ss_free(&e->a_metav, v);
			return NULL;
		}
	}
	memcpy(v->value, sr_metavalue(vp), v->valuesize);
	return &v->o;
}

static int
se_metacursor_destroy(so *o)
{
	semetacursor *c = se_cast(o, semetacursor*, SEMETACURSOR);
	se *e = se_of(o);
	ss_buffree(&c->dump, &e->a);
	so_listdel(&e->metacursor, &c->o);
	se_mark_destroyed(&c->o);
	ss_free(&e->a_metacursor, c);
	return 0;
}

static inline so*
se_metacursor_object(semetacursor *c)
{
	se *e = se_of(&c->o);
	return se_metav_new(e, c->pos);
}

static void*
se_metacursor_get(so *o, so *v)
{
	semetacursor *c = se_cast(o, semetacursor*, SEMETACURSOR);
	if (v) {
		so_destroy(v);
	}
	if (c->first) {
		assert( ss_bufsize(&c->dump) >= (int)sizeof(srmetadump) );
		c->first = 0;
		c->pos = (srmetadump*)c->dump.s;
	} else {
		int size = sizeof(srmetadump) + c->pos->keysize + c->pos->valuesize;
		c->pos = (srmetadump*)((char*)c->pos + size);
		if ((char*)c->pos >= c->dump.p)
			c->pos = NULL;
	}
	if (ssunlikely(c->pos == NULL))
		return NULL;
	return se_metacursor_object(c);
}

static soif semetacursorif =
{
	.open         = NULL,
	.destroy      = se_metacursor_destroy,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = se_metacursor_get,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

so *se_metacursor_new(void *o)
{
	se *e = o;
	semetacursor *c = ss_malloc(&e->a_metacursor, sizeof(semetacursor));
	if (ssunlikely(c == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	so_init(&c->o, &se_o[SEMETACURSOR], &semetacursorif, &e->o, &e->o);
	c->pos = NULL;
	c->first = 1;
	ss_bufinit(&c->dump);
	int rc = se_metaserialize(&e->meta, &c->dump);
	if (ssunlikely(rc == -1)) {
		so_destroy(&c->o);
		return NULL;
	}
	so_listadd(&e->metacursor, &c->o);
	return &c->o;
}
#line 1 "sophia/environment/se_o.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













sotype se_o[] =
{
	{ 0L,          "undefined"       },
	{ 0x9BA14568L, "destroyed"       },
	{ 0x06154834L, "env"             },
	{ 0x20490B34L, "env_meta"        },
	{ 0x6AB65429L, "env_meta_cursor" },
	{ 0x00FCDE12L, "env_meta_object" },
	{ 0x64519F00L, "req"             },
	{ 0x2FABCDE2L, "object"          },
	{ 0x34591111L, "database"        },
	{ 0x24242489L, "database_async"  },
	{ 0x571DEF09L, "batch"           },
	{ 0x13491FABL, "transaction"     },
	{ 0x45ABCDFAL, "cursor"          },
	{ 0x71230BAFL, "snapshot"        },
	{ 0x63102654L, "snapshot_cursor" }
};
#line 1 "sophia/environment/se_recover.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static inline void
se_recoverf(se *e, char *fmt, ...)
{
	if (e->meta.on_recover.function == NULL)
		return;
	char trace[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(trace, sizeof(trace), fmt, args);
	va_end(args);
	e->meta.on_recover.function(trace, e->meta.on_recover.arg);
}

int se_recoverbegin(sedb *db)
{
	/* open and recover repository */
	se_statusset(&db->status, SE_RECOVER);
	se *e = se_of(&db->o);
	/* do not allow to recover existing databases
	 * during online (only create), since logpool
	 * reply is required. */
	if (se_status(&e->status) == SE_ONLINE)
		db->scheme.path_fail_on_exists = 1;
	se_recoverf(e, "loading database '%s'", db->scheme.path);
	int rc = si_open(&db->index, &db->scheme);
	if (ssunlikely(rc == -1))
		goto error;
	db->created = rc;
	return 0;
error:
	se_dbmalfunction(db);
	return -1;
}

int se_recoverend(sedb *db)
{
	se_statusset(&db->status, SE_ONLINE);
	return 0;
}

static int
se_recoverlog(se *e, sl *log)
{
	so *tx = NULL;
	sedb *db = NULL;
	ssiter i;
	ss_iterinit(sl_iter, &i);
	int processed = 0;
	int rc = ss_iteropen(sl_iter, &i, &e->r, &log->file, 1);
	if (ssunlikely(rc == -1))
		return -1;
	for (;;)
	{
		sv *v = ss_iteratorof(&i);
		if (ssunlikely(v == NULL))
			break;

		/* reply transaction */
		uint64_t lsn = sv_lsn(v);
		tx = so_begin(&e->o);
		if (ssunlikely(tx == NULL))
			goto error;

		while (ss_iteratorhas(&i)) {
			v = ss_iteratorof(&i);
			assert(sv_lsn(v) == lsn);
			/* match a database */
			uint32_t dsn = sl_vdsn(v);
			if (db == NULL || db->scheme.id != dsn)
				db = (sedb*)se_dbmatch_id(e, dsn);
			if (ssunlikely(db == NULL)) {
				sr_malfunction(&e->error, "database id %" PRIu32
				               " is not declared", dsn);
				goto rlb;
			}
			so *o = so_object(&db->o);
			if (ssunlikely(o == NULL))
				goto rlb;
			so_setstring(o, "raw", sv_pointer(v), sv_size(v));
			so_setstring(o, "log", log, 0);
			
			int flags = sv_flags(v);
			if (flags == SVDELETE) {
				rc = so_delete(tx, o);
			} else
			if (flags == SVUPDATE) {
				rc = so_update(tx, o);
			} else {
				assert(flags == 0);
				rc = so_set(tx, o);
			}
			if (ssunlikely(rc == -1))
				goto rlb;
			ss_gcmark(&log->gc, 1);
			processed++;
			if ((processed % 100000) == 0)
				se_recoverf(e, " %.1fM processed", processed / 1000000.0);
			ss_iteratornext(&i);
		}
		if (ssunlikely(sl_iter_error(&i)))
			goto rlb;

		so_setint(tx, "lsn", lsn);
		rc = so_commit(tx);
		if (ssunlikely(rc != 0))
			goto error;
		rc = sl_iter_continue(&i);
		if (ssunlikely(rc == -1))
			goto error;
		if (rc == 0)
			break;
	}
	ss_iteratorclose(&i);
	return 0;
rlb:
	so_destroy(tx);
error:
	ss_iteratorclose(&i);
	return -1;
}

static inline int
se_recoverlogpool(se *e)
{
	sslist *i;
	ss_listforeach(&e->lp.list, i) {
		sl *log = sscast(i, sl, link);
		se_recoverf(e, "loading journal '%s'", log->file.file);
		int rc = se_recoverlog(e, log);
		if (ssunlikely(rc == -1))
			return -1;
		ss_gccomplete(&log->gc);
	}
	return 0;
}

int se_recover(se *e)
{
	slconf *lc = &e->lpconf;
	lc->enable         = e->meta.log_enable;
	lc->path           = e->meta.log_path;
	lc->rotatewm       = e->meta.log_rotate_wm;
	lc->sync_on_rotate = e->meta.log_rotate_sync;
	lc->sync_on_write  = e->meta.log_sync;
	int rc = sl_poolopen(&e->lp, lc);
	if (ssunlikely(rc == -1))
		return -1;
	if (e->meta.two_phase_recover)
		return 0;
	/* recover log files */
	rc = se_recoverlogpool(e);
	if (ssunlikely(rc == -1))
		goto error;
	rc = sl_poolrotate(&e->lp);
	if (ssunlikely(rc == -1))
		goto error;
	return 0;
error:
	se_statusset(&e->status, SE_MALFUNCTION);
	return -1;
}

int se_recover_repository(se *e)
{
	syconf *rc = &e->repconf;
	rc->path        = e->meta.path;
	rc->path_create = e->meta.path_create;
	rc->path_backup = e->meta.backup_path;
	rc->sync = 0;
	se_recoverf(e, "recovering repository '%s'", e->meta.path);
	return sy_open(&e->rep, &e->r, rc);
}
#line 1 "sophia/environment/se_req.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













void
se_reqend(sereq *r)
{
	se *e = se_of(r->object);
	/* free key, prefix, update and a pending result */
	if (r->arg.v.v)
		sv_vfree(&e->a, r->arg.v.v);
	if (r->arg.vprefix.v)
		sv_vfree(&e->a, r->arg.vprefix.v);
	if (r->arg.vup.v)
		sv_vfree(&e->a, r->arg.vup.v);
	if (ssunlikely(r->v))
		sv_vfree(&e->a, (svv*)r->v);
	/* free read cache */
	if (sslikely(r->arg.cachegc && r->arg.cache))
		si_cachepool_push(r->arg.cache);
	/* unref db */
	if (sslikely(r->db))
		se_dbunref((sedb*)r->db, 1);
}

static int
se_reqdestroy(so *o)
{
	sereq *r = se_cast(o, sereq*, SEREQ);
	se *e = se_of(r->object);
	se_reqend(r);
	ss_free(&e->a_req, r);
	return 0;
}

static soif sereqif =
{
	.open         = NULL,
	.destroy      = se_reqdestroy,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = NULL,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

void se_reqinit(se *e, sereq *r, sereqop op, so *o, so *db)
{
	so_init(&r->o, &se_o[SEREQ], &sereqif, &e->o, &e->o);
	r->id = sr_seq(&e->seq, SR_RSNNEXT);
	r->op = op;
	r->object = o;
	r->db = db;
	if (db) {
		se_dbref((sedb*)db, 1);
	}
	memset(&r->arg, 0, sizeof(r->arg));
	r->v = NULL;
	r->rc = 0;
}

char *se_reqof(sereqop o)
{
	switch (o) {
	case SE_REQREAD:      return "on_read";
	case SE_REQWRITE:     return "on_write";
	case SE_REQON_BACKUP: return "on_backup";
	default: assert(0);
	}
	return NULL;
}

static void
se_reqadd(se *e, sereq *r)
{
	ss_mutexlock(&e->reqlock);
	so_listadd(&e->req, &r->o);
	ss_condsignal(&e->reqcond);
	ss_mutexunlock(&e->reqlock);
}

void se_reqonbackup(se *e)
{
	if (e->meta.event_on_backup)
		ss_triggerrun(&e->meta.on_event);
}

void se_reqready(sereq *r)
{
	sedb *db = (sedb*)r->object;
	se *e = se_of(&db->o);
	ss_mutexlock(&e->reqlock);
	so_listdel(&e->reqactive, &r->o);
	so_listadd(&e->reqready, &r->o);
	ss_mutexunlock(&e->reqlock);
	ss_triggerrun(&e->meta.on_event);
}

sereq*
se_reqnew(se *e, sereq *src, int add)
{
	sereq *r = ss_malloc(&e->a_req, sizeof(sereq));
	if (ssunlikely(r == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	memcpy(r, src, sizeof(*r));
	if (add) {
		se_reqadd(e, r);
	}
	return r;
}

void se_reqwakeup(se *e)
{
	ss_mutexlock(&e->reqlock);
	ss_condsignal(&e->reqcond);
	ss_mutexunlock(&e->reqlock);
}

sereq*
se_reqdispatch(se *e, int block)
{
	ss_mutexlock(&e->reqlock);
	if (e->req.n == 0) {
		if (! block)
			goto empty;
		ss_condwait(&e->reqcond, &e->reqlock);
		if (e->req.n == 0)
			goto empty;
	}
	sereq *r = (sereq*)so_listfirst(&e->req);
	so_listdel(&e->req, &r->o);
	so_listadd(&e->reqactive, &r->o);
	ss_mutexunlock(&e->reqlock);
	return r;
empty:
	ss_mutexunlock(&e->reqlock);
	return NULL;
}

sereq*
se_reqdispatch_ready(se *e)
{
	ss_mutexlock(&e->reqlock);
	if (e->reqready.n == 0) {
		ss_mutexunlock(&e->reqlock);
		return NULL;
	}
	sereq *r = (sereq*)so_listfirst(&e->reqready);
	so_listdel(&e->reqready, &r->o);
	ss_mutexunlock(&e->reqlock);
	return r;
}

int se_reqqueue(se *e)
{
	ss_mutexlock(&e->reqlock);
	int n = e->req.n;
	ss_mutexunlock(&e->reqlock);
	return n;
}

int se_reqcount(se *e)
{
	ss_mutexlock(&e->reqlock);
	int n = e->req.n + e->reqactive.n + e->reqready.n;
	ss_mutexunlock(&e->reqlock);
	return n;
}

so *se_reqresult(sereq *r, int async)
{
	se *e = se_of(&r->o);
	sv result;
	sv_init(&result, &sv_vif, r->v, NULL);
	sev *v = (sev*)se_vnew(e, r->db, &result, async);
	if (ssunlikely(v == NULL))
		return NULL;
	r->v = NULL;
	v->async_operation = r->op;
	v->async_status    = r->rc;
	v->async_seq       = r->id;
	v->async_arg       = r->arg.arg;
	/* propagate current object settings to
	 * the result one */
	v->orderset = 1;
	v->order = r->arg.order;
	if (v->order == SS_GTE)
		v->order = SS_GT;
	else
	if (v->order == SS_LTE)
		v->order = SS_LT;
	/* reuse prefix object */
	v->vprefix.v = r->arg.vprefix.v;
	if (v->vprefix.v) {
		r->arg.vprefix.v = NULL;
		void *vptr = sv_vpointer(v->vprefix.v);
		v->prefix = sf_key(vptr, 0);
		v->prefixsize = sf_keysize(vptr, 0);
	}
	return &v->o;
}
#line 1 "sophia/environment/se_scheduler.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static inline srzone*
se_zoneof(se *e)
{
	int p = ss_quotaused_percent(&e->quota);
	return sr_zonemap(&e->meta.zones, p);
}

int se_scheduler_branch(void *arg)
{
	sedb *db = arg;
	se *e = se_of(&db->o);
	srzone *z = se_zoneof(e);
	seworker stub;
	se_workerstub_init(&stub);
	int rc;
	while (1) {
		uint64_t vlsn = sx_vlsn(&e->xm);
		siplan plan = {
			.explain   = SI_ENONE,
			.plan      = SI_BRANCH,
			.a         = z->branch_wm,
			.b         = 0,
			.c         = 0,
			.node      = NULL
		};
		rc = si_plan(&db->index, &plan);
		if (rc == 0)
			break;
		rc = si_execute(&db->index, &stub.dc, &plan, vlsn);
		if (ssunlikely(rc == -1))
			break;
	}
	se_workerstub_free(&stub, &db->r);
	return rc;
}

int se_scheduler_compact(void *arg)
{
	sedb *db = arg;
	se *e = se_of(&db->o);
	srzone *z = se_zoneof(e);
	seworker stub;
	se_workerstub_init(&stub);
	int rc;
	while (1) {
		uint64_t vlsn = sx_vlsn(&e->xm);
		siplan plan = {
			.explain   = SI_ENONE,
			.plan      = SI_COMPACT,
			.a         = z->compact_wm,
			.b         = 0,
			.c         = 0,
			.node      = NULL
		};
		rc = si_plan(&db->index, &plan);
		if (rc == 0)
			break;
		rc = si_execute(&db->index, &stub.dc, &plan, vlsn);
		if (ssunlikely(rc == -1))
			break;
	}
	se_workerstub_free(&stub, &db->r);
	return rc;
}

int se_scheduler_checkpoint(void *arg)
{
	se *o = arg;
	sescheduler *s = &o->sched;
	uint64_t lsn = sr_seq(&o->seq, SR_LSN);
	ss_mutexlock(&s->lock);
	s->checkpoint_lsn = lsn;
	s->checkpoint = 1;
	ss_mutexunlock(&s->lock);
	return 0;
}

int se_scheduler_gc(void *arg)
{
	se *o = arg;
	sescheduler *s = &o->sched;
	ss_mutexlock(&s->lock);
	s->gc = 1;
	ss_mutexunlock(&s->lock);
	return 0;
}

int se_scheduler_backup(void *arg)
{
	se *e = arg;
	sescheduler *s = &e->sched;
	if (ssunlikely(e->meta.backup_path == NULL)) {
		sr_error(&e->error, "%s", "backup is not enabled");
		return -1;
	}
	/* begin backup procedure
	 * state 0
	 *
	 * disable log garbage-collection
	*/
	sl_poolgc_enable(&e->lp, 0);
	ss_mutexlock(&s->lock);
	if (ssunlikely(s->backup > 0)) {
		ss_mutexunlock(&s->lock);
		sl_poolgc_enable(&e->lp, 1);
		/* in progress */
		return 0;
	}
	uint64_t bsn = sr_seq(&e->seq, SR_BSNNEXT);
	s->backup = 1;
	s->backup_bsn = bsn;
	ss_mutexunlock(&s->lock);
	return 0;
}

static inline int
se_backupstart(sescheduler *s)
{
	se *e = (se*)s->env;
	/*
	 * a. create backup_path/<bsn.incomplete> directory
	 * b. create database directories
	 * c. create log directory
	*/
	char path[1024];
	snprintf(path, sizeof(path), "%s/%" PRIu32 ".incomplete",
	         e->meta.backup_path, s->backup_bsn);
	int rc = ss_filemkdir(path);
	if (ssunlikely(rc == -1)) {
		sr_error(&e->error, "backup directory '%s' create error: %s",
		         path, strerror(errno));
		return -1;
	}
	int i = 0;
	while (i < s->count) {
		sedb *db = s->i[i];
		snprintf(path, sizeof(path), "%s/%" PRIu32 ".incomplete/%s",
		         e->meta.backup_path, s->backup_bsn,
		         db->scheme.name);
		rc = ss_filemkdir(path);
		if (ssunlikely(rc == -1)) {
			sr_error(&e->error, "backup directory '%s' create error: %s",
			         path, strerror(errno));
			return -1;
		}
		i++;
	}
	snprintf(path, sizeof(path), "%s/%" PRIu32 ".incomplete/log",
	         e->meta.backup_path, s->backup_bsn);
	rc = ss_filemkdir(path);
	if (ssunlikely(rc == -1)) {
		sr_error(&e->error, "backup directory '%s' create error: %s",
		         path, strerror(errno));
		return -1;
	}
	return 0;
}

static inline int
se_backupcomplete(sescheduler *s, seworker *w)
{
	/*
	 * a. rotate log file
	 * b. copy log files
	 * c. enable log gc
	 * d. rename <bsn.incomplete> into <bsn>
	 * e. set last backup, set COMPLETE
	 */
	se *e = (se*)s->env;

	/* force log rotation */
	ss_trace(&w->trace, "%s", "log rotation for backup");
	int rc = sl_poolrotate(&e->lp);
	if (ssunlikely(rc == -1))
		return -1;

	/* copy log files */
	ss_trace(&w->trace, "%s", "log files backup");

	char path[1024];
	snprintf(path, sizeof(path), "%s/%" PRIu32 ".incomplete/log",
	         e->meta.backup_path, s->backup_bsn);
	rc = sl_poolcopy(&e->lp, path, &w->dc.c);
	if (ssunlikely(rc == -1)) {
		sr_errorrecover(&e->error);
		return -1;
	}

	/* enable log gc */
	sl_poolgc_enable(&e->lp, 1);

	/* complete backup */
	snprintf(path, sizeof(path), "%s/%" PRIu32 ".incomplete",
	         e->meta.backup_path, s->backup_bsn);
	char newpath[1024];
	snprintf(newpath, sizeof(newpath), "%s/%" PRIu32,
	         e->meta.backup_path, s->backup_bsn);
	rc = rename(path, newpath);
	if (ssunlikely(rc == -1)) {
		sr_error(&e->error, "backup directory '%s' rename error: %s",
		         path, strerror(errno));
		return -1;
	}

	/* complete */
	s->backup_last = s->backup_bsn;
	s->backup_last_complete = 1;
	s->backup = 0;
	s->backup_bsn = 0;
	return 0;
}

static inline int
se_backuperror(sescheduler *s)
{
	se *e = (se*)s->env;
	sl_poolgc_enable(&e->lp, 1);
	s->backup = 0;
	s->backup_last_complete = 0;
	return 0;
}

int se_scheduler_call(void *arg)
{
	se *e = arg;
	sescheduler *s = &e->sched;
	seworker stub;
	se_workerstub_init(&stub);
	int rc = se_scheduler(s, &stub);
	se_workerstub_free(&stub, &e->r);
	return rc;
}

int se_scheduler_init(sescheduler *s, so *env)
{
	ss_mutexinit(&s->lock);
	s->workers_branch       = 0;
	s->workers_backup       = 0;
	s->workers_gc           = 0;
	s->workers_gc_db        = 0;
	s->rotate               = 0;
	s->req                  = 0;
	s->i                    = NULL;
	s->count                = 0;
	s->rr                   = 0;
	s->env                  = env;
	s->checkpoint_lsn       = 0;
	s->checkpoint_lsn_last  = 0;
	s->checkpoint           = 0;
	s->age                  = 0;
	s->age_last             = 0;
	s->backup_bsn           = 0;
	s->backup_last          = 0;
	s->backup_last_complete = 0;
	s->backup_events        = 0;
	s->backup               = 0;
	s->gc                   = 0;
	s->gc_last              = 0;
	se_workerpool_init(&s->workers);
	return 0;
}

int se_scheduler_shutdown(sescheduler *s)
{
	se *e = (se*)s->env;
	se_reqwakeup(e);
	int rcret = 0;
	int rc = se_workerpool_shutdown(&s->workers, &e->r);
	if (ssunlikely(rc == -1))
		rcret = -1;
	if (s->i) {
		ss_free(&e->a, s->i);
		s->i = NULL;
	}
	ss_mutexfree(&s->lock);
	return rcret;
}

int se_scheduler_add(sescheduler *s , void *db)
{
	ss_mutexlock(&s->lock);
	se *e = (se*)s->env;
	int count = s->count + 1;
	void **i = ss_malloc(&e->a, count * sizeof(void*));
	if (ssunlikely(i == NULL)) {
		ss_mutexunlock(&s->lock);
		return -1;
	}
	memcpy(i, s->i, s->count * sizeof(void*));
	i[s->count] = db;
	void *iprev = s->i;
	s->i = i;
	s->count = count;
	ss_mutexunlock(&s->lock);
	if (iprev)
		ss_free(&e->a, iprev);
	return 0;
}

int se_scheduler_del(sescheduler *s, void *db)
{
	if (ssunlikely(s->i == NULL))
		return 0;
	ss_mutexlock(&s->lock);
	se *e = (se*)s->env;
	int count = s->count - 1;
	if (ssunlikely(count == 0)) {
		s->count = 0;
		ss_free(&e->a, s->i);
		s->i = NULL;
		ss_mutexunlock(&s->lock);
		return 0;
	}
	void **i = ss_malloc(&e->a, count * sizeof(void*));
	if (ssunlikely(i == NULL)) {
		ss_mutexunlock(&s->lock);
		return -1;
	}
	int j = 0;
	int k = 0;
	while (j < s->count) {
		if (s->i[j] == db) {
			j++;
			continue;
		}
		i[k] = s->i[j];
		k++;
		j++;
	}
	void *iprev = s->i;
	s->i = i;
	s->count = count;
	if (ssunlikely(s->rr >= s->count))
		s->rr = 0;
	ss_mutexunlock(&s->lock);
	ss_free(&e->a, iprev);
	return 0;
}

static void *se_worker(void *arg)
{
	seworker *self = arg;
	se *o = self->arg;
	for (;;)
	{
		int rc = se_active(o);
		if (ssunlikely(rc == 0))
			break;
		rc = se_scheduler(&o->sched, self);
		if (ssunlikely(rc == -1))
			break;
		if (ssunlikely(rc == 0))
			ss_sleep(10000000); /* 10ms */
	}
	return NULL;
}

int se_scheduler_run(sescheduler *s)
{
	se *e = (se*)s->env;
	int rc;
	rc = se_workerpool_new(&s->workers, &e->r, e->meta.threads,
	                       se_worker, e);
	if (ssunlikely(rc == -1))
		return -1;
	return 0;
}

static int
se_schedule_plan(sescheduler *s, siplan *plan, sedb **dbret)
{
	int start = s->rr;
	int limit = s->count;
	int i = start;
	int rc_inprogress = 0;
	int rc;
	*dbret = NULL;
first_half:
	while (i < limit) {
		sedb *db = s->i[i];
		if (ssunlikely(! se_dbactive(db))) {
			i++;
			continue;
		}
		rc = si_plan(&db->index, plan);
		switch (rc) {
		case 1:
			s->rr = i;
			*dbret = db;
			return 1;
		case 2: rc_inprogress = rc;
		case 0: break;
		}
		i++;
	}
	if (i > start) {
		i = 0;
		limit = start;
		goto first_half;
	}
	s->rr = 0;
	return rc_inprogress;
}

static int
se_schedule(sescheduler *s, setask *task, seworker *w)
{
	ss_trace(&w->trace, "%s", "schedule");
	si_planinit(&task->plan);

	uint64_t now = ss_utime();
	se *e = (se*)s->env;
	sedb *db;
	srzone *zone = se_zoneof(e);
	assert(zone != NULL);

	task->checkpoint_complete = 0;
	task->backup_complete = 0;
	task->rotate = 0;
	task->req = 0;
	task->gc = 0;
	task->db = NULL;

	ss_mutexlock(&s->lock);

	/* asynchronous reqs dispatcher */
	if (s->req == 0) {
		switch (zone->async) {
		case 2:
			if (se_reqqueue(e) == 0)
				break;
		case 1:
			s->req = 1;
			task->req = zone->async;
			ss_mutexunlock(&s->lock);
			return 0;
		}
	}

	/* log gc and rotation */
	if (s->rotate == 0)
	{
		task->rotate = 1;
		s->rotate = 1;
	}

	/* checkpoint */
	int in_progress = 0;
	int rc;
checkpoint:
	if (s->checkpoint) {
		task->plan.plan = SI_CHECKPOINT;
		task->plan.a = s->checkpoint_lsn;
		rc = se_schedule_plan(s, &task->plan, &db);
		switch (rc) {
		case 1:
			s->workers_branch++;
			se_dbref(db, 1);
			task->db = db;
			task->gc = 1;
			ss_mutexunlock(&s->lock);
			return 1;
		case 2: /* work in progress */
			in_progress = 1;
			break;
		case 0: /* complete checkpoint */
			s->checkpoint = 0;
			s->checkpoint_lsn_last = s->checkpoint_lsn;
			s->checkpoint_lsn = 0;
			task->checkpoint_complete = 1;
			break;
		}
	}

	/* apply zone policy */
	switch (zone->mode) {
	case 0:  /* compact_index */
	case 1:  /* compact_index + branch_count prio */
		assert(0);
		break;
	case 2:  /* checkpoint */
	{
		if (in_progress) {
			ss_mutexunlock(&s->lock);
			return 0;
		}
		uint64_t lsn = sr_seq(&e->seq, SR_LSN);
		s->checkpoint_lsn = lsn;
		s->checkpoint = 1;
		goto checkpoint;
	}
	default: /* branch + compact */
		assert(zone->mode == 3);
	}

	/* database shutdown-drop */
	if (s->workers_gc_db < zone->gc_db_prio) {
		ss_spinlock(&e->dblock);
		db = NULL;
		if (ssunlikely(e->db_shutdown.n > 0)) {
			db = (sedb*)so_listfirst(&e->db_shutdown);
			if (se_dbgarbage(db)) {
				so_listdel(&e->db_shutdown, &db->o);
			} else {
				db = NULL;
			}
		}
		ss_spinunlock(&e->dblock);
		if (ssunlikely(db)) {
			if (db->dropped)
				task->plan.plan = SI_DROP;
			else
				task->plan.plan = SI_SHUTDOWN;
			s->workers_gc_db++;
			se_dbref(db, 1);
			task->db = db;
			ss_mutexunlock(&s->lock);
			return 1;
		}
	}

	/* backup */
	if (s->backup && (s->workers_backup < zone->backup_prio))
	{
		/* backup procedure.
		 *
		 * state 0 (start)
		 * -------
		 *
		 * a. disable log gc
		 * b. mark to start backup (state 1)
		 *
		 * state 1 (background, delayed start)
		 * -------
		 *
		 * a. create backup_path/<bsn.incomplete> directory
		 * b. create database directories
		 * c. create log directory
		 * d. state 2
		 *
		 * state 2 (background, copy)
		 * -------
		 *
		 * a. schedule and execute node backup which bsn < backup_bsn
		 * b. state 3
		 *
		 * state 3 (background, completion)
		 * -------
		 *
		 * a. rotate log file
		 * b. copy log files
		 * c. enable log gc, schedule gc
		 * d. rename <bsn.incomplete> into <bsn>
		 * e. set last backup, set COMPLETE
		 *
		*/
		if (s->backup == 1) {
			/* state 1 */
			rc = se_backupstart(s);
			if (ssunlikely(rc == -1)) {
				se_backuperror(s);
				goto backup_error;
			}
			s->backup = 2;
		}
		/* state 2 */
		task->plan.plan = SI_BACKUP;
		task->plan.a = s->backup_bsn;
		rc = se_schedule_plan(s, &task->plan, &db);
		switch (rc) {
		case 1:
			s->workers_backup++;
			se_dbref(db, 1);
			task->db = db;
			ss_mutexunlock(&s->lock);
			return 1;
		case 2: /* work in progress */
			break;
		case 0: /* state 3 */
			rc = se_backupcomplete(s, w);
			if (ssunlikely(rc == -1)) {
				se_backuperror(s);
				goto backup_error;
			}
			s->backup_events++;
			task->gc = 1;
			task->backup_complete = 1;
			break;
		}
backup_error:;
	}

	/* garbage-collection */
	if (s->gc) {
		if (s->workers_gc < zone->gc_prio) {
			task->plan.plan = SI_GC;
			task->plan.a = sx_vlsn(&e->xm);
			task->plan.b = zone->gc_wm;
			rc = se_schedule_plan(s, &task->plan, &db);
			switch (rc) {
			case 1:
				s->workers_gc++;
				se_dbref(db, 1);
				task->db = db;
				ss_mutexunlock(&s->lock);
				return 1;
			case 2: /* work in progress */
				break;
			case 0: /* state 3 */
				s->gc = 0;
				s->gc_last = now;
				break;
			}
		}
	} else {
		if (zone->gc_prio && zone->gc_period) {
			if ( (now - s->gc_last) >= ((uint64_t)zone->gc_period * 1000000) ) {
				s->gc = 1;
			}
		}
	}

	/* index aging */
	if (s->age) {
		if (s->workers_branch < zone->branch_prio) {
			task->plan.plan = SI_AGE;
			task->plan.a = zone->branch_age * 1000000; /* ms */
			task->plan.b = zone->branch_age_wm;
			rc = se_schedule_plan(s, &task->plan, &db);
			switch (rc) {
			case 1:
				s->workers_branch++;
				se_dbref(db, 1);
				task->db = db;
				ss_mutexunlock(&s->lock);
				return 1;
			case 0:
				s->age = 0;
				s->age_last = now;
				break;
			}
		}
	} else {
		if (zone->branch_prio && zone->branch_age_period) {
			if ( (now - s->age_last) >= ((uint64_t)zone->branch_age_period * 1000000) ) {
				s->age = 1;
			}
		}
	}

	/* branching */
	if (s->workers_branch < zone->branch_prio)
	{
		/* schedule branch task using following
		 * priority:
		 *
		 * a. peek node with the largest in-memory index
		 *    which is equal or greater then branch
		 *    watermark.
		 *    If nothing is found, stick to b.
		 *
		 * b. peek node with the largest in-memory index,
		 *    which has oldest update time.
		 *
		 * c. if no branch work is needed, schedule a
		 *    compaction job
		 *
		 */
		task->plan.plan = SI_BRANCH;
		task->plan.a = zone->branch_wm;
		rc = se_schedule_plan(s, &task->plan, &db);
		if (rc == 1) {
			s->workers_branch++;
			se_dbref(db, 1);
			task->db = db;
			task->gc = 1;
			ss_mutexunlock(&s->lock);
			return 1;
		}
	}

	/* compaction */
	task->plan.plan = SI_COMPACT;
	task->plan.a = zone->compact_wm;
	rc = se_schedule_plan(s, &task->plan, &db);
	if (rc == 1) {
		se_dbref(db, 1);
		task->db = db;
		ss_mutexunlock(&s->lock);
		return 1;
	}

	ss_mutexunlock(&s->lock);
	return 0;
}

static int
se_gc(sescheduler *s, seworker *w)
{
	ss_trace(&w->trace, "%s", "log gc");
	se *e = (se*)s->env;
	int rc = sl_poolgc(&e->lp);
	if (ssunlikely(rc == -1))
		return -1;
	return 0;
}

static int
se_rotate(sescheduler *s, seworker *w)
{
	ss_trace(&w->trace, "%s", "log rotation");
	se *e = (se*)s->env;
	int rc = sl_poolrotate_ready(&e->lp, e->meta.log_rotate_wm);
	if (rc) {
		rc = sl_poolrotate(&e->lp);
		if (ssunlikely(rc == -1))
			return -1;
	}
	return 0;
}

static int
se_run(setask *t, seworker *w)
{
	si_plannertrace(&t->plan, &w->trace);
	sedb *db = t->db;
	se *e = (se*)db->o.env;
	uint64_t vlsn = sx_vlsn(&e->xm);
	return si_execute(&db->index, &w->dc, &t->plan, vlsn);
}

static int
se_dispatch(sescheduler *s, seworker *w, setask *t)
{
	ss_trace(&w->trace, "%s", "dispatcher");
	se *e = (se*)s->env;
	int block = t->req == 1;
	do {
		int rc = se_active(e);
		if (ssunlikely(rc == 0))
			break;
		sereq *req = se_reqdispatch(e, block);
		if (req) {
			se_execute(req);
			se_reqready(req);
		}
	} while (block);
	return 0;
}

static int
se_complete(sescheduler *s, setask *t)
{
	ss_mutexlock(&s->lock);
	sedb *db = t->db;
	if (db)
		se_dbunref(db, 1);
	switch (t->plan.plan) {
	case SI_BRANCH:
	case SI_AGE:
	case SI_CHECKPOINT:
		s->workers_branch--;
		break;
	case SI_BACKUP:
	case SI_BACKUPEND:
		s->workers_backup--;
		break;
	case SI_GC:
		s->workers_gc--;
		break;
	case SI_SHUTDOWN:
	case SI_DROP:
		s->workers_gc_db--;
		so_destroy(&db->o);
		break;
	}
	if (t->rotate == 1)
		s->rotate = 0;
	if (t->req)
		s->req = 0;
	ss_mutexunlock(&s->lock);
	return 0;
}

int se_scheduler(sescheduler *s, seworker *w)
{
	setask task;
	int rc = se_schedule(s, &task, w);
	int job = rc;
	if (task.rotate) {
		rc = se_rotate(s, w);
		if (ssunlikely(rc == -1))
			goto error;
	}
	if (task.req) {
		rc = se_dispatch(s, w, &task);
		if (ssunlikely(rc == -1)) {
			goto error;
		}
	}
	se *e = (se*)s->env;
	if (task.backup_complete)
		se_reqonbackup(e);
	if (job) {
		rc = se_run(&task, w);
		if (ssunlikely(rc == -1)) {
			if (task.plan.plan != SI_BACKUP &&
			    task.plan.plan != SI_BACKUPEND) {
				se_dbmalfunction(task.db);
				goto error;
			}
			ss_mutexlock(&s->lock);
			se_backuperror(s);
			ss_mutexunlock(&s->lock);
		}
	}
	if (task.gc) {
		rc = se_gc(s, w);
		if (ssunlikely(rc == -1))
			goto error;
	}
	se_complete(s, &task);
	ss_trace(&w->trace, "%s", "sleep");
	return job;
error:
	ss_trace(&w->trace, "%s", "malfunction");
	return -1;
}
#line 1 "sophia/environment/se_snapshot.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_snapshotfree(sesnapshot *s)
{
	se *e = se_of(&s->o);
	sx_rollback(&s->t);
	if (sslikely(s->name)) {
		ss_free(&e->a, s->name);
		s->name = NULL;
	}
	se_mark_destroyed(&s->o);
	ss_free(&e->a_snapshot, s);
	return 0;
}

static int
se_snapshotdestroy(so *o)
{
	sesnapshot *s = se_cast(o, sesnapshot*, SESNAPSHOT);
	se *e = se_of(o);
	so_listdestroy(&s->cursor);
	uint32_t id = s->t.id;
	so_listdel(&e->snapshot, &s->o);
	se_dbunbind(e, id);
	se_snapshotfree(s);
	return 0;
}

void *se_snapshotget_object(so *o, const char *path)
{
	sesnapshot *s = se_cast(o, sesnapshot*, SESNAPSHOT);
	if (strcmp(path, "db-cursor") == 0)
		return se_snapshotcursor_new(s);
	return NULL;
}

static void*
se_snapshotget(so *o, so *key)
{
	sesnapshot *s = se_cast(o, sesnapshot*, SESNAPSHOT);
	sev *v = se_cast(key, sev*, SEV);
	sedb *db = se_cast(key->parent, sedb*, SEDB);
	return se_dbread(db, v, &s->t, 0, NULL, SS_EQ);
}

static void*
se_snapshotcursor(so *o)
{
	sesnapshot *s = (sesnapshot*)o;
	se *e = se_of(o);
	return se_cursornew(e, s->vlsn);
}

static soif sesnapshotif =
{
	.open         = NULL,
	.destroy      = se_snapshotdestroy,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = se_snapshotget_object,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = se_snapshotget,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = se_snapshotcursor
};

so *se_snapshotnew(se *e, uint64_t vlsn, char *name)
{
	sslist *i;
	ss_listforeach(&e->snapshot.list, i) {
		sesnapshot *s = (sesnapshot*)sscast(i, so, link);
		if (ssunlikely(strcmp(s->name, name) == 0)) {
			sr_error(&e->error, "snapshot '%s' already exists", name);
			return NULL;
		}
	}
	sesnapshot *s = ss_malloc(&e->a_snapshot, sizeof(sesnapshot));
	if (ssunlikely(s == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	so_init(&s->o, &se_o[SESNAPSHOT], &sesnapshotif, &e->o, &e->o);
	so_listinit(&s->cursor);
	s->vlsn = vlsn;
	s->name = ss_strdup(&e->a, name);
	if (ssunlikely(s->name == NULL)) {
		ss_free(&e->a_snapshot, s);
		sr_oom(&e->error);
		return NULL;
	}
	sx_begin(&e->xm, &s->t, vlsn);
	se_dbbind(e);
	return &s->o;
}

int se_snapshotupdate(sesnapshot *s)
{
	se *e = se_of(&s->o);
	uint32_t id = s->t.id;
	sx_rollback(&s->t);
	sx_begin(&e->xm, &s->t, s->vlsn);
	s->t.id = id;
	return 0;
}
#line 1 "sophia/environment/se_snapshotcursor.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_snapshotcursor_destroy(so *o)
{
	sesnapshotcursor *c =
		se_cast(o, sesnapshotcursor*, SESNAPSHOTCURSOR);
	se *e = se_of(&c->o);
	ss_buffree(&c->list, &e->a);
	so_listdel(&c->s->cursor, &c->o);
	se_mark_destroyed(&c->o);
	ss_free(&e->a_snapshotcursor, c);
	return 0;
}

static void*
se_snapshotcursor_get(so *o, so *v ssunused)
{
	sesnapshotcursor *c =
		se_cast(o, sesnapshotcursor*, SESNAPSHOTCURSOR);
	if (c->ready) {
		c->ready = 0;
		return c->v;
	}
	if (ssunlikely(c->pos == NULL))
		return NULL;
	c->pos += sizeof(sedb**);
	if (c->pos >= c->list.p) {
		c->pos = NULL;
		c->v = NULL;
		return NULL;
	}
	c->v = *(sedb**)c->pos;
	return c->v;
}

static soif sesnapshotcursorif =
{
	.open         = NULL,
	.destroy      = se_snapshotcursor_destroy,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = se_snapshotcursor_get,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

static inline int
se_snapshotcursor_open(sesnapshotcursor *c)
{
	se *e = se_of(&c->o);
	int rc;
	uint32_t txn = c->s->t.id;
	sslist *i;
	ss_listforeach(&e->db.list, i) {
		sedb *db = (sedb*)sscast(i, so, link);
		int status = se_status(&db->status);
		if (status != SE_ONLINE)
			continue;
		if (txn > db->txn_min) {
			rc = ss_bufadd(&c->list, &e->a, &db, sizeof(db));
			if (ssunlikely(rc == -1))
				return -1;
		}
	}
	ss_spinlock(&e->dblock);
	ss_listforeach(&e->db_shutdown.list, i) {
		sedb *db = (sedb*)sscast(i, so, link);
		if (db->txn_min < txn && txn <= db->txn_max) {
			rc = ss_bufadd(&c->list, &e->a, &db, sizeof(db));
			if (ssunlikely(rc == -1))
				return -1;
		}
	}
	ss_spinunlock(&e->dblock);
	if (ss_bufsize(&c->list) == 0)
		return 0;
	c->ready = 1;
	c->pos = c->list.s;
	c->v = *(sedb**)c->list.s;
	return 0;
}

so *se_snapshotcursor_new(sesnapshot *s)
{
	se *e = se_of(&s->o);
	sesnapshotcursor *c =
		ss_malloc(&e->a_snapshotcursor, sizeof(sesnapshotcursor));
	if (ssunlikely(c == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	so_init(&c->o, &se_o[SESNAPSHOTCURSOR],
	        &sesnapshotcursorif, &e->o, &e->o);
	c->s = s;
	c->v = NULL;
	c->pos = NULL;
	c->ready = 0;
	ss_bufinit(&c->list);
	int rc = se_snapshotcursor_open(c);
	if (ssunlikely(rc == -1)) {
		so_destroy(&c->o);
		sr_oom(&e->error);
		return NULL;
	}
	so_listadd(&s->cursor, &c->o);
	return &c->o;
}
#line 1 "sophia/environment/se_tx.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static inline int
se_txwrite(setx *t, sev *o, uint8_t flags)
{
	se *e = se_of(&t->o);
	sedb *db = se_cast(o->o.parent, sedb*, SEDB);
	/* validate req */
	if (ssunlikely(t->t.s == SXPREPARE)) {
		sr_error(&e->error, "%s", "transaction is in 'prepare' state (read-only)");
		goto error;
	}

	/* validate database status */
	int status = se_status(&db->status);
	switch (status) {
	case SE_SHUTDOWN:
		if (ssunlikely(! se_dbvisible(db, t->t.id))) {
			sr_error(&e->error, "%s", "database is invisible for the transaction");
			goto error;
		}
		break;
	case SE_RECOVER:
	case SE_ONLINE: break;
	default: goto error;
	}
	if (flags == SVUPDATE && !sf_updatehas(&db->scheme.fmt_update))
		flags = 0;

	/* prepare object */
	svv *v;
	int rc = se_dbv(db, o, 0, &v);
	if (ssunlikely(rc == -1))
		goto error;
	v->flags = flags;
	v->log = o->log;
	sv vp;
	sv_init(&vp, &sv_vif, v, NULL);
	so_destroy(&o->o);

	/* ensure quota */
	int size = sizeof(svv) + sv_size(&vp);
	ss_quota(&e->quota, SS_QADD, size);

	/* concurrent index only */
	rc = sx_set(&t->t, &db->coindex, v);
	if (ssunlikely(rc == -1)) {
		ss_quota(&e->quota, SS_QREMOVE, size);
		return -1;
	}
	return 0;
error:
	so_destroy(&o->o);
	return -1;
}

static int
se_txset(so *o, so *v)
{
	setx  *t = se_cast(o, setx*, SETX);
	sev *key = se_cast(v, sev*, SEV);
	return se_txwrite(t, key, 0);
}

static int
se_txupdate(so *o, so *v)
{
	setx  *t = se_cast(o, setx*, SETX);
	sev *key = se_cast(v, sev*, SEV);
	return se_txwrite(t, key, SVUPDATE);
}

static int
se_txdelete(so *o, so *v)
{
	setx  *t = se_cast(o, setx*, SETX);
	sev *key = se_cast(v, sev*, SEV);
	return se_txwrite(t, key, SVDELETE);
}

static void*
se_txget(so *o, so *v)
{
	setx  *t = se_cast(o, setx*, SETX);
	sev *key = se_cast(v, sev*, SEV);
	se *e = se_of(&t->o);
	sedb *db = se_cast(key->o.parent, sedb*, SEDB);
	/* validate database */
	int status = se_status(&db->status);
	switch (status) {
	case SE_SHUTDOWN:
		if (ssunlikely(! se_dbvisible(db, t->t.id))) {
			sr_error(&e->error, "%s", "database is invisible for the transaction");
			goto error;
		}
		break;
	case SE_ONLINE:
	case SE_RECOVER:
		break;
	default: goto error;
	}
	return se_dbread(db, key, &t->t, 1, NULL, SS_EQ);
error:
	so_destroy(&key->o);
	return NULL;
}

void se_txend(setx *t)
{
	se *e = se_of(&t->o);
	sx_gc(&t->t, &e->r);
	se_dbunbind(e, t->t.id);
	so_listdel(&e->tx, &t->o);
	se_mark_destroyed(&t->o);
	ss_free(&e->a_tx, t);
}

static int
se_txrollback(so *o)
{
	setx *t = se_cast(o, setx*, SETX);
	sx_rollback(&t->t);
	se_txend(t);
	return 0;
}

static sxstate
se_txprepare_trigger(sx *t, sv *v, void *arg0, void *arg1)
{
	sicache *cache = arg0;
	sedb *db = arg1;
	se *e = se_of(&db->o);
	uint64_t lsn = sr_seq(e->r.seq, SR_LSN);
	if (t->vlsn == lsn)
		return SXPREPARE;
	siquery q;
	si_queryopen(&q, cache, &db->index,
	             SS_EQ, t->vlsn,
	             NULL, 0,
	             sv_pointer(v), sv_size(v));
	si_queryhas(&q);
	int rc;
	rc = si_query(&q);
	assert(q.result.v == NULL);
	si_queryclose(&q);
	if (ssunlikely(rc))
		return SXROLLBACK;
	return SXPREPARE;
}

static int
se_txprepare(so *o)
{
	setx *t = se_cast(o, setx*, SETX);
	se *e = se_of(o);
	int status = se_status(&e->status);
	if (ssunlikely(! se_statusactive_is(status)))
		return -1;
	sicache *cache = si_cachepool_pop(&e->cachepool);
	if (ssunlikely(cache == NULL))
		return sr_oom(&e->error);
	/* resolve conflicts */
	sxpreparef prepare_trigger = se_txprepare_trigger;
	if (status == SE_RECOVER)
		prepare_trigger = NULL;
	sxstate s = sx_prepare(&t->t, prepare_trigger, cache);
	si_cachepool_push(cache);
	if (s == SXLOCK)
		return 2;
	if (s == SXROLLBACK) {
		sx_rollback(&t->t);
		se_txend(t);
		return 1;
	}
	assert(s == SXPREPARE);
	return 0;
}

static int
se_txcommit(so *o)
{
	setx *t = se_cast(o, setx*, SETX);
	se *e = se_of(o);
	int status = se_status(&e->status);
	if (ssunlikely(! se_statusactive_is(status)))
		return -1;
	int recover = (status == SE_RECOVER);

	/* prepare transaction */
	if (ssunlikely(! sv_logcount(&t->t.log))) {
		sx_prepare(&t->t, NULL, NULL);
		sx_commit(&t->t);
		se_txend(t);
		return 0;
	}
	int rc;
	if (sslikely(t->t.s == SXREADY || t->t.s == SXLOCK)) {
		rc = se_txprepare(&t->o);
		if (ssunlikely(rc != 0))
			return rc;
	}
	assert(t->t.s == SXPREPARE);
	sx_commit(&t->t);

	/* prepare for commit */
	sereq q;
	se_reqinit(e, &q, SE_REQWRITE, &t->o, NULL);
	sereqarg *arg = &q.arg;
	arg->log = &t->t.log;
	arg->lsn = 0;
	if (recover || e->meta.commit_lsn)
		arg->lsn = t->lsn;
	if (ssunlikely(recover)) {
		arg->recover = 1;
		arg->vlsn_generate = 0;
		arg->vlsn = sr_seq(e->r.seq, SR_LSN);
	} else {
		arg->vlsn_generate = 1;
		arg->vlsn = 0;
	}
	/* log write and commit */
	se_execute(&q);

	se_txend(t);
	return q.rc;
}

static int
se_txset_int(so *o, const char *path, int64_t v)
{
	setx *t = se_cast(o, setx*, SETX);
	if (strcmp(path, "lsn") == 0) {
		t->lsn = v;
		return 0;
	}
	return -1;
}

static soif setxif =
{
	.open         = NULL,
	.destroy      = se_txrollback,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = NULL,
	.setint       = se_txset_int,
	.getobject    = NULL,
	.getstring    = NULL,
	.getint       = NULL,
	.set          = se_txset,
	.update       = se_txupdate,
	.del          = se_txdelete,
	.get          = se_txget,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = se_txprepare,
	.commit       = se_txcommit,
	.cursor       = NULL,
};

so *se_txnew(se *e)
{
	setx *t = ss_malloc(&e->a_tx, sizeof(setx));
	if (ssunlikely(t == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	memset(t, 0, sizeof(*t));
	so_init(&t->o, &se_o[SETX], &setxif, &e->o, &e->o);
	sx_init(&e->xm, &t->t);
	t->lsn = 0;
	sx_begin(&e->xm, &t->t, 0);
	se_dbbind(e);
	so_listadd(&e->tx, &t->o);
	return &t->o;
}
#line 1 "sophia/environment/se_v.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static int
se_vdestroy(so *o)
{
	sev *v = se_cast(o, sev*, SEV);
	se *e = se_of(o);
	if (v->v.v)
		sv_vfree(&e->a, (svv*)v->v.v);
	v->v.v = NULL;
	se_mark_destroyed(&v->o);
	ss_free(&e->a_v, v);
	return 0;
}

static sfv*
se_vsetpart(sev *v, const char *path, void *pointer, int size)
{
	se *e = se_of(&v->o);
	sedb *db = (sedb*)v->o.parent;
	srkey *part = sr_schemefind(&db->scheme.scheme, (char*)path);
	if (ssunlikely(part == NULL))
		return NULL;
	assert(part->pos < (int)(sizeof(v->keyv) / sizeof(sfv)));
	const int keysize_max = 1 << 15;
	if (size == 0)
		size = strlen(pointer) + 1;
	if (ssunlikely(size > keysize_max)) {
		sr_error(&e->error, "key '%s' is too big (%d limit)",
		         pointer, keysize_max);
		return NULL;
	}
	sfv *fv = &v->keyv[part->pos];
	fv->r.offset = 0;
	fv->key = pointer;
	fv->r.size = size;
	if (fv->part == NULL)
		v->keyc++;
	fv->part = part;
	return fv;
}

static int
se_vsetstring(so *o, const char *path, void *pointer, int size)
{
	sev *v = se_cast(o, sev*, SEV);
	se *e = se_of(o);
	if (ssunlikely(v->v.v))
		return sr_error(&e->error, "%s", "object is read-only");

	if (strcmp(path, "value") == 0) {
		const int valuesize_max = 1 << 21;
		if (ssunlikely(size > valuesize_max)) {
			sr_error(&e->error, "value is too big (%d limit)",
			         valuesize_max);
			return -1;
		}
		v->value = pointer;
		v->valuesize = size;
		return 0;
	}
	if (strcmp(path, "prefix") == 0) {
		v->prefix = pointer;
		v->prefixsize = size;
		return 0;
	}
	if (strcmp(path, "log") == 0) {
		v->log = pointer;
		return 0;
	}
	if (strcmp(path, "order") == 0) {
		if (size == 0)
			size = strlen(pointer);
		ssorder cmp = ss_orderof(pointer, size);
		if (ssunlikely(cmp == SS_STOP)) {
			sr_error(&e->error, "%s", "bad order name");
			return -1;
		}
		v->order = cmp;
		v->orderset = 1;
		return 0;
	}
	if (strcmp(path, "raw") == 0) {
		v->raw = pointer;
		v->rawsize = size;
		return 0;
	}
	if (strcmp(path, "arg") == 0) {
		v->async_arg = pointer;
		return 0;
	}
	/* object keypart */
	sfv *fv = se_vsetpart(v, path, pointer, size);
	if (ssunlikely(fv == NULL))
		return -1;
	return 0;
}

static void*
se_vgetstring(so *o, const char *path, int *size)
{
	sev *v = se_cast(o, sev*, SEV);
	if (strcmp(path, "value") == 0) {
		/* key object */
		if (v->value) {
			if (size)
				*size = v->valuesize;
			if (v->valuesize == 0)
				return NULL;
			return v->value;
		}
		if (v->v.v == NULL) {
			if (size)
				*size = 0;
			return NULL;
		}
		/* result object */
		sedb *db = (sedb*)o->parent;
		int vsize = sv_valuesize(&v->v, &db->r);
		if (size)
			*size = vsize;
		if (vsize == 0)
			return NULL;
		return sv_value(&v->v, &db->r);
	}
	if (strcmp(path, "prefix") == 0) {
		if (v->prefix == NULL)
			return NULL;
		if (size)
			*size = v->prefixsize;
		return v->prefix;
	}
	if (strcmp(path, "order") == 0) {
		char *order = ss_ordername(v->order);
		if (*size)
			*size = strlen(order) + 1;
		return order;
	}
	if (strcmp(path, "type") == 0) {
		char *type = se_reqof(v->async_operation);
		if (size)
			*size = strlen(type);
		return type;
	}
	if (strcmp(path, "arg") == 0) {
		if (size)
			*size = 0;
		return v->async_arg;
	}
	if (strcmp(path, "raw") == 0) {
		if (v->raw) {
			if (size)
				*size = v->rawsize;
			return v->raw;
		}
		if (v->v.v == NULL)
			return NULL;
		if (size)
			*size = sv_size(&v->v);
		return sv_pointer(&v->v);
	}

	/* match key-part */
	sedb *db = (sedb*)o->parent;
	srkey *part = sr_schemefind(&db->scheme.scheme, (char*)path);
	if (ssunlikely(part == NULL))
		return NULL;
	/* database result object */
	if (v->v.v) {
		if (size)
			*size = sv_keysize(&v->v, &db->r, part->pos);
		return sv_key(&v->v, &db->r, part->pos);
	}
	/* database key object */
	assert(part->pos < (int)(sizeof(v->keyv) / sizeof(sfv)));
	sfv *fv = &v->keyv[part->pos];
	if (fv->key == NULL)
		return NULL;
	if (size)
		*size = fv->r.size;
	return fv->key;
}

static int64_t
se_vgetint(so *o, const char *path)
{
	sev *v = se_cast(o, sev*, SEV);
	se *e = se_of(o);
	if (strcmp(path, "lsn") == 0) {
		uint64_t lsn = -1;
		if (v->v.v)
			lsn = ((svv*)(v->v.v))->lsn;
		return lsn;
	} else
	if (strcmp(path, "status") == 0) {
		return v->async_status;
	} else
	if (strcmp(path, "seq") == 0) {
		return v->async_seq;
	} else {
		sr_error(&e->error, "unknown object field '%s'",
		         path);
	}
	return -1;
}

static soif sevif =
{
	.open         = NULL,
	.destroy      = se_vdestroy,
	.error        = NULL,
	.object       = NULL,
	.asynchronous = NULL,
	.poll         = NULL,
	.drop         = NULL,
	.setobject    = NULL,
	.setstring    = se_vsetstring,
	.setint       = NULL,
	.getobject    = NULL,
	.getstring    = se_vgetstring,
	.getint       = se_vgetint,
	.set          = NULL,
	.update       = NULL,
	.del          = NULL,
	.get          = NULL,
	.batch        = NULL,
	.begin        = NULL,
	.prepare      = NULL,
	.commit       = NULL,
	.cursor       = NULL,
};

so *se_vnew(se *e, so *parent, sv *vp, int async)
{
	sev *v = ss_malloc(&e->a_v, sizeof(sev));
	if (ssunlikely(v == NULL)) {
		sr_oom(&e->error);
		return NULL;
	}
	memset(v, 0, sizeof(*v));
	so_init(&v->o, &se_o[SEV], &sevif, parent, &e->o);
	v->order = SS_EQ;
	v->async = async;
	if (vp) {
		v->v = *vp;
	}
	return &v->o;
}
#line 1 "sophia/environment/se_worker.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/













static inline seworker*
se_workernew(sr *r, int id, ssthreadf f, void *arg)
{
	seworker *w = ss_malloc(r->a, sizeof(seworker));
	if (ssunlikely(w == NULL)) {
		sr_oom_malfunction(r->e);
		return NULL;
	}
	snprintf(w->name, sizeof(w->name), "%d", id);
	w->arg = arg;
	sd_cinit(&w->dc);
	ss_listinit(&w->link);
	ss_traceinit(&w->trace);
	ss_trace(&w->trace, "%s", "init");
	int rc = ss_threadnew(&w->t, f, w);
	if (ssunlikely(rc == -1)) {
		sr_malfunction(r->e, "failed to create thread: %s",
		               strerror(errno));
		ss_free(r->a, w);
		return NULL;
	}
	return w;
}

static inline int
se_workershutdown(seworker *w, sr *r)
{
	int rc = ss_threadjoin(&w->t);
	if (ssunlikely(rc == -1))
		sr_malfunction(r->e, "failed to join a thread: %s",
		               strerror(errno));
	sd_cfree(&w->dc, r);
	ss_tracefree(&w->trace);
	ss_free(r->a, w);
	return rc;
}

int se_workerpool_init(seworkerpool *w)
{
	ss_listinit(&w->list);
	w->n = 0;
	return 0;
}

int se_workerpool_shutdown(seworkerpool *p, sr *r)
{
	int rcret = 0;
	int rc;
	sslist *i, *n;
	ss_listforeach_safe(&p->list, i, n) {
		seworker *w = sscast(i, seworker, link);
		rc = se_workershutdown(w, r);
		if (ssunlikely(rc == -1))
			rcret = -1;
	}
	return rcret;
}

int se_workerpool_new(seworkerpool *p, sr *r, int n, ssthreadf f, void *arg)
{
	int i = 0;
	int id = 0;
	while (i < n) {
		seworker *w = se_workernew(r, id, f, arg);
		if (ssunlikely(p == NULL))
			return -1;
		ss_listappend(&p->list, &w->link);
		p->n++;
		i++;
		id++;
	}
	return 0;
}
#line 1 "sophia/sophia/sophia.h"
#ifndef SOPHIA_H_
#define SOPHIA_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

#if __GNUC__ >= 4
#  define SP_API __attribute__((visibility("default")))
#else
#  define SP_API
#endif

SP_API void    *sp_env(void);
SP_API void    *sp_object(void*);
SP_API int      sp_open(void*);
SP_API int      sp_drop(void*);
SP_API int      sp_destroy(void*);
SP_API int      sp_error(void*);
SP_API void    *sp_asynchronous(void*);
SP_API void    *sp_poll(void*);
SP_API int      sp_setobject(void*, const char*, const void*);
SP_API int      sp_setstring(void*, const char*, const void*, int);
SP_API int      sp_setint(void*, const char*, int64_t);
SP_API void    *sp_getobject(void*, const char*);
SP_API void    *sp_getstring(void*, const char*, int*);
SP_API int64_t  sp_getint(void*, const char*);
SP_API int      sp_set(void*, void*);
SP_API int      sp_update(void*, void*);
SP_API int      sp_delete(void*, void*);
SP_API void    *sp_get(void*, void*);
SP_API void    *sp_cursor(void*);
SP_API void    *sp_batch(void*);
SP_API void    *sp_begin(void*);
SP_API int      sp_prepare(void*);
SP_API int      sp_commit(void*);

#ifdef __cplusplus
}
#endif

#endif
#line 1 "sophia/sophia/sophia.c"

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/














static inline void
sp_unsupported(so *o, const char *method)
{
	fprintf(stderr, "\n%s(%s): unsupported operation\n",
	        (char*)method, o->type->name);
	abort();
}

static inline so*
sp_cast(void *ptr, const char *method)
{
	so *o = se_cast_validate(ptr);
	if (ssunlikely(o == NULL)) {
		fprintf(stderr, "\n%s(%p): bad object\n", method, ptr);
		abort();
	}
	if (ssunlikely(o->type == &se_o[SEDESTROYED])) {
		fprintf(stderr, "\n%s(%p): attempt to use destroyed object\n",
		        method, ptr);
		abort();
	}
	return o;
}

SP_API void *sp_env(void)
{
	return se_new();
}

SP_API void *sp_object(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->object == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->object(o);
	se_apiunlock(e);
	return h;
}

SP_API int sp_open(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->open == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->open(o);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_drop(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->drop == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->drop(o);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_destroy(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->destroy == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	int rc;
	if (ssunlikely(e == o)) {
		rc = o->i->destroy(o);
		return rc;
	}
	se_apilock(e);
	rc = o->i->destroy(o);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_error(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->error == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->error(o);
	se_apiunlock(e);
	return rc;
}

SP_API void *sp_asynchronous(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->asynchronous == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->asynchronous(o);
	se_apiunlock(e);
	return h;
}

SP_API void *sp_poll(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->poll == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->poll(o);
	se_apiunlock(e);
	return h;
}

SP_API int sp_setobject(void *ptr, const char *path, const void *object)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->setobject == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->setobject(o, path, (void*)object);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_setstring(void *ptr, const char *path, const void *pointer, int size)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->setstring == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->setstring(o, path, (void*)pointer, size);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_setint(void *ptr, const char *path, int64_t v)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->setint == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->setint(o, path, v);
	se_apiunlock(e);
	return rc;
}

SP_API void *sp_getobject(void *ptr, const char *path)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->getobject == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->getobject(o, path);
	se_apiunlock(e);
	return h;
}

SP_API void *sp_getstring(void *ptr, const char *path, int *size)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->getstring == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->getstring(o, path, size);
	se_apiunlock(e);
	return h;
}

SP_API int64_t sp_getint(void *ptr, const char *path)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->getint == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int64_t rc = o->i->getint(o, path);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_set(void *ptr, void *v)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->set == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->set(o, v);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_update(void *ptr, void *v)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->update == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->update(o, v);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_delete(void *ptr, void *v)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->del == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->del(o, v);
	se_apiunlock(e);
	return rc;
}

SP_API void *sp_get(void *ptr, void *v)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->get == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->get(o, v);
	se_apiunlock(e);
	return h;
}

SP_API void *sp_cursor(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->cursor == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->cursor(o);
	se_apiunlock(e);
	return h;
}

SP_API void *sp_batch(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->batch == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->batch(o);
	se_apiunlock(e);
	return h;
}

SP_API void *sp_begin(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->begin == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return NULL;
	}
	so *e = o->env;
	se_apilock(e);
	void *h = o->i->begin(o);
	se_apiunlock(e);
	return h;
}

SP_API int sp_prepare(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->prepare == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->prepare(o);
	se_apiunlock(e);
	return rc;
}

SP_API int sp_commit(void *ptr)
{
	so *o = sp_cast(ptr, __FUNCTION__);
	if (ssunlikely(o->i->commit == NULL)) {
		sp_unsupported(o, __FUNCTION__);
		return -1;
	}
	so *e = o->env;
	se_apilock(e);
	int rc = o->i->commit(o);
	se_apiunlock(e);
	return rc;
}
/* vim: foldmethod=marker
*/
/* }}} */
