#include <map>
#include <string>

#include <cassert>

#include "storage.h"

struct sv {
    time_t ts;
    blob_type b;
    int locked;
    sv(time_t t, blob_type bl) : ts(t), b(bl), locked(0) {}
};

typedef std::map<std::string, sv> stor_type;
static stor_type stor;
typedef std::multimap<time_t, std::string> lru_type;
static lru_type lru;
static size_t sum_size;
size_t max_stor_size = 10000000;

int stor_store(const char *key, time_t ts, blob_type obj) {
    std::pair<stor_type::iterator, bool> r = stor.insert(stor_type::value_type(key, sv(ts, obj)));
    if (!r.second) {
#if 0
        if (r.first->second.ts < ts)
            r.first->second.ts = ts;
#endif
        blob_destroy(obj);
        return 1;
    } else {
        lru.insert(lru_type::value_type(ts, key));
        sum_size += blob_data_size(obj);
        for (lru_type::iterator i = lru.begin(); sum_size > max_stor_size && i != lru.end();) {
            stor_type::iterator si = stor.find(i->second);
            if (si == stor.end()) {
                assert(0);
                continue;
            }
            if (si->second.locked) { // XXX
                i++;
                continue;
            }
            sum_size -= blob_data_size(si->second.b);
            stor.erase(si);
            lru.erase(i++);
        }
        return 0;
    }
}

int stor_find(const char *key, blob_type *obj, sv** v) {
    stor_type::iterator i = stor.find(key);
    if (i == stor.end())
        return 1;
    *obj = i->second.b;
    i->second.locked = 1;
    *v = &(i->second);
    return 0;
}

int stor_unlock(sv* v) {
    v->locked = 0;
    return 0;
}
