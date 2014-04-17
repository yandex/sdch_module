#ifndef STORAGE_H
#define STORAGE_H

#include <time.h>

struct blob_s;
#ifdef __cplusplus
#include <vector>

struct blob_s {
    std::vector<char> data;
};

extern "C" {
#endif
typedef struct blob_s *blob_type;

int blob_create(blob_type *obj);
int blob_append(blob_type obj, const void *s, int len);
int blob_destroy(blob_type obj);
const void *blob_data_begin(blob_type obj);
size_t blob_data_size(blob_type obj);

struct sv;
int stor_store(const char *key, time_t ts, blob_type obj);
int stor_find(const char *key, blob_type *obj, struct sv**);
int stor_unlock(struct sv*);
int stor_clear(time_t ts);
extern size_t max_stor_size;

#ifdef __cplusplus
}
#endif

#endif
