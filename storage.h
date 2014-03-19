#ifndef STORAGE_H
#define STORAGE_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

int blob_create(void **obj);
int blob_append(void *obj, const void *s, int len);
int blob_destroy(void *obj);

int stor_store(const char *key, time_t ts, void *obj);
int stor_find(const char *key, void **obj);
int stor_clear(time_t ts);

#ifdef __cplusplus
}
#endif

#endif
