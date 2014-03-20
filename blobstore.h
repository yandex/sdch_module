#ifndef BLOBSTORE_H
#define BLOBSTORE_H

#include "pzlib.h"
#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

void *make_blobstore(void *c, blob_type *blob);

#ifdef __cplusplus
}
#endif

#endif
