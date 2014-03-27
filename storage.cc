#include <map>
#include <string>

#include "storage.h"

struct sv {
    time_t ts;
    blob_type b;
    sv(time_t t, blob_type bl) : ts(t), b(bl) {}
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

int stor_find(const char *key, blob_type *obj) {
    stor_type::iterator i = stor.find(key);
    if (i == stor.end())
        return 1;
    *obj = i->second.b;
}
