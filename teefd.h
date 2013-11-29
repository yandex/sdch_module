#ifndef TEEFD_H
#define TEEFD_H

#include "pzlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void *make_teefd(const char *fn, writerfunc wf, void *coo);
extern writerfunc write_teefd;
extern closefunc free_teefd;

#ifdef __cplusplus
}
#endif


#endif
