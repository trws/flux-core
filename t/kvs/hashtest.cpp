#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>
#include <stdarg.h>
#include <czmq.h>
#include <stdlib.h>
#if HAVE_LIBJUDY
#include <Judy.h>
#endif
#if HAVE_LIBSQLITE3
#include <sqlite3.h>
#endif

#include "src/common/libutil/xzmalloc.h"
#include "src/common/libutil/cleanup.h"
#include "src/common/libutil/log.h"
#if HAVE_LSD_HASH
extern "C" {
#include "hash.h"
}
#endif
#if HAVE_SOPHIA
#include "src/common/libsophia/sophia.h"
#endif // HAVE_SOPHIA
#if HAVE_HATTRIE
#include "src/common/libhat-trie/hat-trie.h"
#endif

#include <jemalloc.h>

#include <locale.h>

#include <sys/time.h>
#include <sys/types.h>
#include <map>
#include <unordered_map>
#include <array>
#include <string>
#include <google/sparse_hash_map>
#include <google/dense_hash_map>
#include <boost/functional/hash.hpp>
#include "hash-map.h"

#include <city.h>
#include <citycrc.h>

static struct timeval start_time;

void init_time() {
              gettimeofday(&start_time, NULL);
}

u_int64_t get_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (u_int64_t) (t.tv_sec - start_time.tv_sec) * 1000000
        + (t.tv_usec - start_time.tv_usec);
}

//const int num_keys = 1024*1024;
const int num_keys = 1024*1024*10;

struct hash_impl {
    void (*destroy)(struct hash_impl *h);
    void (*insert)(struct hash_impl *h, zlist_t *items);
    void (*lookup)(struct hash_impl *h, zlist_t *items);
    void *h;
};

struct item {
    zdigest_t *zd;
    std::array<char,16> data;
    std::array<uint8_t,20> key;
    std::array<char,41> skey;
};

void rusage (struct rusage *res)
{
    int rc = getrusage (RUSAGE_SELF, res);
    assert (rc == 0);
}

long int rusage_maxrss_since (struct rusage *res)
{
    struct rusage ne;
    int rc = getrusage (RUSAGE_SELF, &ne);
    assert (rc == 0);
    return ne.ru_maxrss - res->ru_maxrss;
}

struct item *item_create (int id)
{
    struct item *item = (struct item *)malloc (sizeof (*item));
    assert (item != NULL);
    snprintf (item->data.data(), sizeof (item->data), "%d", id);

    item->zd = zdigest_new ();
    assert (item->zd != NULL);
    zdigest_update (item->zd, (uint8_t *)item->data.data(), sizeof (item->data));

    assert (zdigest_size (item->zd) == sizeof (item->key));
    memcpy (item->key.data(), zdigest_data (item->zd), zdigest_size (item->zd));

    assert (strlen (zdigest_string (item->zd)) == sizeof (item->skey) - 1);
    strcpy (item->skey.data(), zdigest_string (item->zd));
    zdigest_destroy (&item->zd);

    return item;
}

void item_destroy (void *arg)
{
    struct item *item =(struct item *) arg;
    free (item);
}

zlist_t *create_items (void)
{
    zlist_t *items = zlist_new ();
    struct item *item;
    int i, rc;

    assert (items != NULL);
    for (i = 0; i < num_keys; i++) {
        item = item_create (i);
        rc = zlist_append (items, item);
        assert (rc == 0);
    }
    return items;
}

/* zhash
 */

void insert_zhash (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    int rc;

    item =(struct item *) zlist_first (items);
    while (item != NULL) {
        rc = zhash_insert ((zhash_t*)impl->h, item->skey.data(), item);
        assert (rc == 0);
        item =(struct item *) zlist_next (items);
    }
}

void lookup_zhash (struct hash_impl *impl, zlist_t *items)
{
    struct item *item, *ip;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        ip = (struct item *)zhash_lookup ((zhash_t*)impl->h, item->skey.data());
        assert (ip != NULL);
        assert (ip == item);
        item = (struct item *)zlist_next (items);
    }
}

void destroy_zhash (struct hash_impl *impl)
{
    zhash_t *zh = (zhash_t*)impl->h;
    zhash_destroy (&zh);
    free (impl);
}

struct hash_impl *create_zhash (void)
{
    struct hash_impl *impl = (struct hash_impl *)malloc (sizeof (*impl));
    impl->h = zhash_new ();
    assert (impl->h != NULL);
    impl->insert = insert_zhash;
    impl->lookup = lookup_zhash;
    impl->destroy = destroy_zhash;
    return impl;
}

/* zhashx
 */
#if HAVE_ZHASHX_NEW
void insert_zhashx (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    int rc;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        rc = zhashx_insert ((zhashx_t*)impl->h, item->key.data(), item);
        assert (rc == 0);
        item = (struct item *)zlist_next (items);
    }
}

void lookup_zhashx (struct hash_impl *impl, zlist_t *items)
{
    struct item *item, *ip;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        ip = (struct item *)zhashx_lookup ((zhashx_t*)impl->h, item->key.data());
        assert (ip != NULL);
        assert (ip == item);
        item = (struct item *)zlist_next (items);
    }
}

void destroy_zhashx (struct hash_impl *impl)
{
    zhashx_t *zh = (zhashx_t*)impl->h;
    zhashx_destroy (&zh);
    free (impl);
}


void *duplicator_zhashx (const void *item)
{
    return (void *)item;
}

void destructor_zhashx (void **item)
{
    *item = NULL;
}

int comparator_zhashx (const void *item1, const void *item2)
{
    return memcmp (item1, item2, 20);
}

size_t hasher_zhashx (const void *item)
{
    return *(size_t *)item;
}

struct hash_impl *create_zhashx (void)
{
    struct hash_impl *impl = (struct hash_impl *)malloc (sizeof (*impl));
    impl->h = zhashx_new ();
    assert (impl->h != NULL);

    /* Use 20 byte keys that are not copied.
     */
    zhashx_set_key_duplicator ((zhashx_t*)impl->h, duplicator_zhashx);
    zhashx_set_key_destructor ((zhashx_t*)impl->h, destructor_zhashx);
    zhashx_set_key_comparator ((zhashx_t*)impl->h, comparator_zhashx);
    zhashx_set_key_hasher ((zhashx_t*)impl->h, hasher_zhashx);

    impl->insert = insert_zhashx;
    impl->lookup = lookup_zhashx;
    impl->destroy = destroy_zhashx;
    return impl;
}
#endif /* HAVE_ZHASHX_NEW */

/* lsd-hash
 */

#if HAVE_LSD_HASH
void insert_lsd (struct hash_impl *impl, zlist_t *items)
{
    struct item *item, *ip;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        ip = (struct item *)hash_insert ((hash_t)impl->h, item->key.data(), item);
        assert (ip != NULL);
        item = (struct item *)zlist_next (items);
    }
}

void lookup_lsd (struct hash_impl *impl, zlist_t *items)
{
    struct item *item, *ip;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        ip = (struct item *)hash_find ((hash_t)impl->h, item->key.data());
        assert (ip != NULL);
        assert (ip == item);
        item = (struct item *)zlist_next (items);
    }
}

void destroy_lsd (struct hash_impl *impl)
{
    hash_destroy ((hash_t)impl->h);
    free (impl);
}

unsigned int hash_lsd (const void *key)
{
    return *(unsigned int *)key; /* 1st 4 bytes of sha1 */
}

int cmp_lsd (const void *key1, const void *key2)
{
    return memcmp (key1, key2, 20);
}

struct hash_impl *create_lsd (void)
{
    struct hash_impl *impl = (struct hash_impl *)malloc (sizeof (*impl));
    impl->h = hash_create (1024*1024*8, hash_lsd, cmp_lsd, NULL);
    assert (impl->h != NULL);

    impl->insert = insert_lsd;
    impl->lookup = lookup_lsd;
    impl->destroy = destroy_lsd;

    return impl;
}
#endif

/* judy
 */

#if HAVE_LIBJUDY
void insert_judy (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    //Pvoid_t array = NULL;
    Pvoid_t valp;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        JHSI (valp, impl->h, item->key.data(), sizeof (item->key));
        assert (valp != PJERR);
        assert (*(uintptr_t*)valp == (uintptr_t)0); /* nonzero indicates dup */
        *(uintptr_t*)valp = (uintptr_t)item;
        item = (struct item *)zlist_next (items);
    }
}

void lookup_judy (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    //Pvoid_t array = NULL;
    Pvoid_t valp;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        JHSG (valp, impl->h, item->key.data(), sizeof (item->key));
        assert (valp != NULL);
        assert (*(uintptr_t*)valp == (uintptr_t)item);
        item = (struct item *)zlist_next (items);
    }
}

void destroy_judy (struct hash_impl *impl)
{
    Word_t bytes;
    JHSFA (bytes, impl->h);
    printf ("judy freed %lu Kbytes of memory", bytes / 1024);
    free (impl);
}

struct hash_impl *create_judy (void)
{
    struct hash_impl *impl = (struct hash_impl *)malloc (sizeof (*impl));

    impl->insert = insert_judy;
    impl->lookup = lookup_judy;
    impl->destroy = destroy_judy;

    return impl;
}

void insert_judys (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    //Pvoid_t array = NULL;
    Pvoid_t valp;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        JSLI (valp, impl->h, (const uint8_t *)item->skey.data());
        assert (valp != PJERR);
        assert (*(uintptr_t*)valp == (uintptr_t)0); /* nonzero indicates dup */
        *(uintptr_t*)valp = (uintptr_t)item;
        item = (struct item *)zlist_next (items);
    }
}

void lookup_judys (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    //Pvoid_t array = NULL;
    Pvoid_t valp;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        JSLG (valp, impl->h, (const uint8_t *)item->skey.data());
        assert (valp != NULL);
        assert (*(uintptr_t*)valp == (uintptr_t)item);
        item = (struct item *)zlist_next (items);
    }
}

void destroy_judys (struct hash_impl *impl)
{
    Word_t bytes;
    JSLFA (bytes, impl->h);
    printf ("judys freed %lu Kbytes of memory", bytes / 1024);
    free (impl);
}

struct hash_impl *create_judys (void)
{
    struct hash_impl *impl = (struct hash_impl *)malloc (sizeof (*impl));

    impl->insert = insert_judys;
    impl->lookup = lookup_judys;
    impl->destroy = destroy_judys;

    return impl;
}
#endif

/* sophia
 */
#if HAVE_SOPHIA
void log_sophia_error (void *env, const char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    char *s = xvasprintf (fmt, ap);
    va_end (ap);

    int error_size;
    char *error = NULL;
    if (env)
        error = sp_getstring (env, "sophia.error", &error_size);
    fprintf (stderr, "%s: %s", s, error ? error : "failure");
    if (error)
        free (error);
    free (s);
}

void insert_sophia (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    void *db, *o;
    int rc;

    db = sp_getobject (impl->h, "db.test");
    if (!db)
        log_sophia_error (impl->h, "db.test");
    assert (db != NULL);

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        // existence check slows down by about 23X! */
        //o = sp_object (db);
        //assert (o != NULL);
        //rc = sp_setstring (o, "key", item->key, sizeof (item->key));
        //assert (rc == 0);
        //void *result = sp_get (db, o); /* destroys 'o' */
        //assert (result == NULL);

        o = sp_object (db);
        assert (o != NULL);
        rc = sp_setstring (o, "key", item->key, sizeof (item->key));
        assert (rc == 0);
        rc = sp_setstring (o, "value", item, sizeof (*item));
        assert (rc == 0);
        rc = sp_set (db, o); /* destroys 'o', copies item  */
        assert (rc == 0);
        item = (struct item *)zlist_next (items);
    }

    sp_destroy (db);
}

void lookup_sophia (struct hash_impl *impl, zlist_t *items)
{
    struct item *item, *ip;
    void *db, *o, *result;
    int rc, item_size;
    int count = 0;

    db = sp_getobject (impl->h, "db.test");
    if (!db)
        log_sophia_error (impl->h, "db.test");
    assert (db != NULL);

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        o = sp_object (db);
        assert (o != NULL);
        rc = sp_setstring (o, "key", item->key, sizeof (item->key));
        assert (rc == 0);
        result = sp_get (db, o); /* destroys 'o' */
        assert (result != NULL);
        ip = sp_getstring (result, "value", &item_size);
        assert (ip != NULL);
        assert (item_size == sizeof (*item));
        assert (memcmp (ip, item, item_size) == 0);
        sp_destroy (result);
        item = (struct item *)zlist_next (items);
        count++;
        //if (count % 10000 == 0)
        //    printf ("lookup: %d of %d", count, num_keys);
    }

    sp_destroy (db);
}

void destroy_sophia (struct hash_impl *impl)
{
    sp_destroy (impl->h);
    free (impl);
}

struct hash_impl *create_sophia (void)
{
    struct hash_impl *impl = (struct item *)malloc (sizeof (*impl));
    char template[] = "/tmp/hashtest-sophia.XXXXXX";
    char *path;
    int rc;

    path = mkdtemp (template);
    assert (path != NULL);
    /* cleanup_push_string (cleanup_directory_recursive, path); */
    printf ("sophia.path: %s", path);
    impl->h = sp_env ();
    assert (impl->h != NULL);
    rc = sp_setstring (impl->h, "sophia.path", path, 0);
    assert (rc == 0);
    // 16m limit increases memory used during insert by about 2X
    //rc = sp_setint (impl->h, "memory.limit", 1024*1024*16);
    //assert (rc == 0);
    rc = sp_setstring (impl->h, "db", "test", 0);
    assert (rc == 0);
    rc = sp_setstring (impl->h, "db.test.index.key", "string", 0);
    assert (rc == 0);
    // N.B. lz4 slows down lookups by about 4X
    //rc = sp_setstring (impl->h, "db.test.compression", "lz4", 0);
    //assert (rc == 0);
    rc = sp_open (impl->h);
    assert (rc == 0);

    impl->insert = insert_sophia;
    impl->lookup = lookup_sophia;
    impl->destroy = destroy_sophia;

    return impl;
}
#endif

#if HAVE_HATTRIE
void insert_hat (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    value_t *val;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        val = hattrie_get (impl->h, (char *)item->key, sizeof (item->key));
        assert (val != NULL);
        assert (*val == 0);
        *(void **)val = item;
        item = (struct item *)zlist_next (items);
    }
}

void lookup_hat (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    value_t *val;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        val = hattrie_tryget (impl->h, (char *)item->key, sizeof (item->key));
        assert (*(void **)val == item);
        item = (struct item *)zlist_next (items);
    }
}

void destroy_hat (struct hash_impl *impl)
{
    hattrie_free (impl->h);
    free (impl);
}

struct hash_impl *create_hat (void)
{
    struct hash_impl *impl = (struct item *)malloc (sizeof (*impl));
    impl->h = hattrie_create ();
    assert (impl->h != NULL);
    impl->insert = insert_hat;
    impl->lookup = lookup_hat;
    impl->destroy = destroy_hat;
    return impl;
}
#endif

#if HAVE_LIBSQLITE3
void insert_sqlite (struct hash_impl *impl, zlist_t *items)
{
    const char *sql = "INSERT INTO objects (hash,object) values (?1, ?2)";
    sqlite3_stmt *stmt;
    int rc;
    struct item *item;

    rc = sqlite3_prepare_v2 (impl->h, sql, -1, &stmt, NULL);
    assert (rc == SQLITE_OK);

    rc = sqlite3_exec (impl->h, "BEGIN", 0, 0, 0);
    assert (rc == SQLITE_OK);

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        rc = sqlite3_bind_text (stmt, 1, (char *)item->key,
                                sizeof (item->key), SQLITE_STATIC);
        assert (rc == SQLITE_OK);
        rc = sqlite3_bind_blob (stmt, 2, item, sizeof (*item), SQLITE_STATIC);
        assert (rc == SQLITE_OK);
        rc = sqlite3_step (stmt);
        assert (rc == SQLITE_DONE);
        rc = sqlite3_clear_bindings (stmt);
        assert (rc == SQLITE_OK);
        rc = sqlite3_reset (stmt);
        assert (rc == SQLITE_OK);

        item = (struct item *)zlist_next (items);
    }
    rc = sqlite3_exec (impl->h, "COMMIT", 0, 0, 0);
    assert (rc == SQLITE_OK);

    sqlite3_finalize (stmt);
}

void lookup_sqlite (struct hash_impl *impl, zlist_t *items)
{
    const char *sql = "SELECT object FROM objects WHERE hash = ?1 LIMIT 1";
    sqlite3_stmt *stmt;
    int rc;
    struct item *item;
    const void *val;

    rc = sqlite3_prepare_v2 (impl->h, sql, -1, &stmt, NULL);
    assert (rc == SQLITE_OK);

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        rc = sqlite3_bind_text (stmt, 1, (char *)item->key,
                                sizeof (item->key), SQLITE_STATIC);
        assert (rc == SQLITE_OK);
        rc = sqlite3_step (stmt);
        assert (rc == SQLITE_ROW);
        assert (sqlite3_column_type (stmt, 0) == SQLITE_BLOB);
        assert (sqlite3_column_bytes (stmt, 0) == sizeof (*item));
        val = sqlite3_column_blob (stmt, 0);
        assert (val != NULL);
        assert (memcmp (val, item, sizeof (*item)) == 0);

        rc = sqlite3_step (stmt);
        assert (rc == SQLITE_DONE);
        rc = sqlite3_reset (stmt);
        assert (rc == SQLITE_OK);

        item = (struct item *)zlist_next (items);
    }

    sqlite3_finalize (stmt);
}

void destroy_sqlite (struct hash_impl *impl)
{
    sqlite3_close (impl->h);
}

struct hash_impl *create_sqlite (void)
{
    int rc;
    sqlite3 *db;
    char template[] = "/tmp/hashtest-sqlite.XXXXXX";
    char *path;
    struct hash_impl *impl = (struct item *)malloc (sizeof (*impl));
    char dbpath[PATH_MAX];

    path = mkdtemp (template);
    assert (path != NULL);
    /* cleanup_push_string (cleanup_directory_recursive, path); */
    printf ("sqlite path: %s", path);

    snprintf (dbpath, sizeof (dbpath), "%s/db", path);
    rc = sqlite3_open (dbpath, &db);
    assert (rc == SQLITE_OK);

    // avoid creating journal
    rc = sqlite3_exec (db, "PRAGMA journal_mode=MEMORY", NULL, NULL, NULL);
    assert (rc == SQLITE_OK);

    // avoid fsync
    rc = sqlite3_exec (db, "PRAGMA synchronous=OFF", NULL, NULL, NULL);
    assert (rc == SQLITE_OK);

    // avoid mutex locking
    rc = sqlite3_exec (db, "PRAGMA locking_mode=EXCLUSIVE", NULL, NULL, NULL);
    assert (rc == SQLITE_OK);

    // raise max db pages cached in memory from 2000
    //rc = sqlite3_exec (db, "PRAGMA cache_size=16000", NULL, NULL, NULL);
    //assert (rc == SQLITE_OK);

    // raise db page size from default 1024 bytes
    // N.B. must set before table create
    //rc = sqlite3_exec (db, "PRAGMA page_size=4096", NULL, NULL, NULL);
    //assert (rc == SQLITE_OK);

    rc = sqlite3_exec (db, "CREATE TABLE objects("
            "hash CHAR(20) PRIMARY KEY,"
            "object BLOB"
            ");", NULL, NULL, NULL);
    assert (rc == SQLITE_OK);

    impl->h = db;
    impl->insert = insert_sqlite;
    impl->lookup = lookup_sqlite;
    impl->destroy = destroy_sqlite;
    return impl;
}
#endif

void usage (void)
{
    fprintf (stderr, "Usage: hashtest zhash | zhashx | judy | lsd | hat"
           " | sophia | sqlite\n");
    exit (1);
}

template<typename T>
void insert_map (struct hash_impl *impl, zlist_t *items)
{
    struct item *item;
    T &map = *reinterpret_cast<T*>(impl->h);

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        map[item->key] = *item;
        item = (struct item *)zlist_next (items);
    }
}

template<typename T>
void lookup_map (struct hash_impl *impl, zlist_t *items)
{
    T &map = *reinterpret_cast<T*>(impl->h);
    struct item *item;
    const void *val;

    item = (struct item *)zlist_first (items);
    while (item != NULL) {
        assert (memcmp (&map[item->key], item, sizeof (*item)) == 0);

        item = (struct item *)zlist_next (items);
    }
}

template<typename T>
void destroy_map (struct hash_impl *impl)
{
    delete reinterpret_cast<T*>(impl->h);
}

using array = std::array<uint8_t, 20>;
template<typename T>
struct hash_impl *create_treemap (void)
{
    struct hash_impl *impl = new struct hash_impl();
    impl->h = new T();
    impl->insert = insert_map<T>;
    impl->lookup = lookup_map<T>;
    impl->destroy = destroy_map<T>;
    return impl;
}
template<typename T>
struct hash_impl *create_map (uint64_t in=0)
{
    struct hash_impl *impl = new struct hash_impl();
    if (in) {
        impl->h = new T(in);
    } else {
        impl->h = new T();
    }
    impl->insert = insert_map<T>;
    impl->lookup = lookup_map<T>;
    impl->destroy = destroy_map<T>;
    return impl;
}

template<typename K, typename V, typename H>
class EasyUseDenseHashMap : public google::dense_hash_map<K,V,H> {
    public:
        EasyUseDenseHashMap(uint64_t num) : google::dense_hash_map<K,V,H>(num){
            K v;
            v.fill(-1);
            this->set_empty_key(v);
            v.fill(-2);
            this->set_deleted_key(v);
        }
        EasyUseDenseHashMap() {
            K v;
            v.fill(-1);
            this->set_empty_key(v);
            v.fill(-2);
            this->set_deleted_key(v);
        }
};
template<typename K, typename V, typename H>
class EasyUseSparseHashMap : public google::sparse_hash_map<K,V,H> {
    public:
        EasyUseSparseHashMap(uint64_t num) : google::sparse_hash_map<K,V,H>(num){
            K v;
            v.fill(-2);
            this->set_deleted_key(v);
        }
        EasyUseSparseHashMap() {
            K v;
            v.fill(-2);
            this->set_deleted_key(v);
        }
};

template<typename T>
class CityHash {
 public:
  uint64 operator()(const T& s) const {  
      return CityHash64(reinterpret_cast<const char *>(s.data()), sizeof(T));
    // return CityHash64WithSeed(s.c_str(), s.size(), seed);
  }
};
template<typename T>
class CityHash32_c {
 public:
  uint64 operator()(const T& s) const {  
      return CityHash32(reinterpret_cast<const char *>(s.data()), sizeof(T));
    // return CityHash64WithSeed(s.c_str(), s.size(), seed);
  }
};
template<typename T>
class CityHash128_c {
 public:
  auto operator()(const T& s) const {  
      return CityHashCrc128(reinterpret_cast<const char *>(s.data()), sizeof(T));
    // return CityHash64WithSeed(s.c_str(), s.size(), seed);
  }
};
template<typename T>
class lsd_hash {
 public:
  uint64 operator()(const T& s) const {  
      return *reinterpret_cast<const uint64_t *>(s.data());
    // return CityHash64WithSeed(s.c_str(), s.size(), seed);
  }
};

int main (int argc, char *argv[])
{
    zlist_t *items;
    struct hash_impl *impl = NULL;
    struct timespec t0;
    struct rusage res;
    size_t sz, allocated, previous = 0;
    sz = sizeof(size_t);

    if (argc == 1)
        usage ();

    setlocale(LC_ALL, "");
    uint64_t epoch = 0;
    sz = sizeof(epoch);
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &allocated, &sz, NULL, 0);
    rusage (&res);
    init_time ();
    if (!strcmp (argv[1], "zhash"))
        impl = create_zhash ();
#if HAVE_ZHASHX_NEW
    else if (!strcmp (argv[1], "zhashx"))
        impl = create_zhashx ();
#endif
#if HAVE_LSD_HASH
    else if (!strcmp (argv[1], "lsd"))
        impl = create_lsd ();
#endif
#if HAVE_LIBJUDY
    else if (!strcmp (argv[1], "judy"))
        impl = create_judy ();
    else if (!strcmp (argv[1], "judys"))
        impl = create_judys ();
#endif
#if HAVE_SOPHIA
    else if (!strcmp (argv[1], "sophia"))
        impl = create_sophia ();
#endif
#if HAVE_HATTRIE
    else if (!strcmp (argv[1], "hat"))
        impl = create_hat ();
#endif
#if HAVE_LIBSQLITE3
    else if (!strcmp (argv[1], "sqlite"))
        impl = create_sqlite ();
#endif
    else if (!strcmp (argv[1], "map"))
        impl = create_treemap<std::map<array, item>> ();
    else if (!strcmp (argv[1], "lmap"))
        impl = create_map<hash_map<array, item>> ();
    else if (!strcmp (argv[1], "umap"))
        impl = create_map<std::unordered_map<array, item, boost::hash<array>>> ();
    else if (!strcmp (argv[1], "smap"))
        impl = create_map<EasyUseSparseHashMap<array, item, boost::hash<array>>> ();
    else if (!strcmp (argv[1], "dmap"))
        impl = create_map<EasyUseDenseHashMap<array, item, boost::hash<array>>> ();
    else if (!strcmp (argv[1], "umapc"))
        impl = create_map<std::unordered_map<array, item, CityHash<array>>> ();
    else if (!strcmp (argv[1], "smapc"))
        impl = create_map<EasyUseSparseHashMap<array, item, CityHash<array>>> ();
    else if (!strcmp (argv[1], "dmapc"))
        impl = create_map<EasyUseDenseHashMap<array, item, CityHash<array>>> ();
    else if (!strcmp (argv[1], "umapc32"))
        impl = create_map<std::unordered_map<array, item, CityHash32_c<array>>> ();
    else if (!strcmp (argv[1], "smapc32"))
        impl = create_map<EasyUseSparseHashMap<array, item, CityHash32_c<array>>> ();
    else if (!strcmp (argv[1], "dmapc32"))
        impl = create_map<EasyUseDenseHashMap<array, item, CityHash32_c<array>>> ();
    else if (!strcmp (argv[1], "umaplsd"))
        impl = create_map<std::unordered_map<array, item, lsd_hash<array>>> ();
    else if (!strcmp (argv[1], "smaplsd"))
        impl = create_map<EasyUseSparseHashMap<array, item, lsd_hash<array>>> ();
    else if (!strcmp (argv[1], "dmaplsd"))
        impl = create_map<EasyUseDenseHashMap<array, item, lsd_hash<array>>> ();
    else if (!strcmp (argv[1], "iumap"))
        impl = create_map<std::unordered_map<array, item, boost::hash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "ismap"))
        impl = create_map<EasyUseSparseHashMap<array, item, boost::hash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "idmap"))
        impl = create_map<EasyUseDenseHashMap<array, item, boost::hash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "iumapc"))
        impl = create_map<std::unordered_map<array, item, CityHash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "ismapc"))
        impl = create_map<EasyUseSparseHashMap<array, item, CityHash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "idmapc"))
        impl = create_map<EasyUseDenseHashMap<array, item, CityHash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "iumapc32"))
        impl = create_map<std::unordered_map<array, item, CityHash32_c<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "ismapc32"))
        impl = create_map<EasyUseSparseHashMap<array, item, CityHash32_c<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "idmapc32"))
        impl = create_map<EasyUseDenseHashMap<array, item, CityHash32_c<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "iumaplsd"))
        impl = create_map<std::unordered_map<array, item, lsd_hash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "ismaplsd"))
        impl = create_map<EasyUseSparseHashMap<array, item, lsd_hash<array>>> (1024*1024*8);
    else if (!strcmp (argv[1], "idmaplsd"))
        impl = create_map<EasyUseDenseHashMap<array, item, lsd_hash<array>>> (1024*1024*8);
    if (!impl)
        usage ();
    previous = allocated;
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &allocated, &sz, NULL, 0);
    printf ("create %s: %.2fs %'+ldB (%'+ldB)", argv[1], get_time () * 1E-6,
            allocated, allocated - previous);

    rusage (&res);
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &previous, &sz, NULL, 0);
    init_time();
    items = create_items ();
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &allocated, &sz, NULL, 0);
    printf ("create items: %.2fs %'+ldB (%'+ldB)", get_time() * 1E-6,
            allocated, allocated - previous);

    rusage (&res);
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &previous, &sz, NULL, 0);
    init_time();
    impl->insert (impl, items);
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &allocated, &sz, NULL, 0);
    printf ("insert items: %.2fs %'+ldB (%'+ldB)", get_time () * 1E-6,
            allocated, allocated - previous);

    rusage (&res);
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &previous, &sz, NULL, 0);
    init_time ();
    impl->lookup (impl, items);
    epoch++;
    mallctl("epoch", &epoch, &sz, &epoch, sz);
    mallctl("stats.allocated", &allocated, &sz, NULL, 0);
    printf ("lookup items: %.2fs %'+ldB (%'+ldB)", get_time () * 1E-6,
            allocated, allocated - previous);
    printf("\n");

    impl->destroy (impl);
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
