#ifndef BLOBSTORE_H
#define BLOBSTORE_H

#include "pzlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void *make_blobstore(void *c, void **blob);

#ifdef __cplusplus
}
#endif

#endif
