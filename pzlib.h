#ifndef PZLIB_H
#define PZLIB_H

typedef unsigned int psize_type;
typedef int pssize_type;
typedef pssize_type writerfunc(void *cookie, const void *buf, psize_type len);
typedef void closefunc(void*);

#ifdef __cplusplus
extern "C" {
#endif

void *make_fd(int fd);
extern writerfunc write_fd;
extern closefunc free_fd;

#ifdef __cplusplus
}
#endif

#endif
