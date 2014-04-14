#include <map>
#include <string>

#include "storage.h"

struct sv {
    time_t ts;
    blob_type b;
    int locked;
    sv(time_t t, blob_type bl) : ts(t), b(bl), locked(0) {}
};

typedef std::map<std::string, sv> stor_type;
static stor_type stor;

int stor_store(const char *key, time_t ts, blob_type obj) {
    std::pair<stor_type::iterator, bool> r = stor.insert(stor_type::value_type(key, sv(ts, obj)));
    if (!r.second) {
        if (r.first->second.ts < ts)
            r.first->second.ts = ts;
        blob_destroy(obj);
        return 1;
    } else {
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
