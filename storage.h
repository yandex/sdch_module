#ifndef STORAGE_H
#define STORAGE_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct blob_s;
typedef struct blob_s *blob_type;

int blob_create(blob_type *obj);
int blob_append(blob_type obj, const void *s, int len);
int blob_destroy(blob_type obj);
const void *blob_data_begin(blob_type obj);
size_t blob_data_size();

int stor_store(const char *key, time_t ts, blob_type obj);
int stor_find(const char *key, blob_type *obj);
int stor_clear(time_t ts);

#ifdef __cplusplus
}
#endif

#endif
