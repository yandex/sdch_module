#ifndef PZLIB_H
#define PZLIB_H

typedef unsigned int psize_type;
typedef int pssize_type;
typedef pssize_type writerfunc(void *cookie, const void *buf, psize_type len);
typedef void closefunc(void*);

struct pz {
    writerfunc *wf;
    closefunc *cf;
};

inline pssize_type do_write(void *cookie, const void *buf, psize_type len) {
    struct pz* p = (struct pz*)cookie;
    return (p->wf)(cookie, buf, len);
}

inline void do_close(void *cookie) {
    struct pz* p = (struct pz*)cookie;
    (p->cf)(cookie);
}

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
