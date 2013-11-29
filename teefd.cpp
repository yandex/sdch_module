#include <fcntl.h>
#include <unistd.h>
#include "teefd.h"

struct teefd_obj {
    int fd;
    writerfunc *wfnext;
    void *coonext;
};

void *make_teefd(const char *fn, writerfunc wf, void *coo)
{
    teefd_obj *r = new teefd_obj;
    r->fd = open(fn, O_WRONLY|O_CREAT|O_EXCL, 0666);
    if (r->fd == -1) {
        delete r;
        return NULL;
    }
    r->wfnext = wf;
    r->coonext = coo;
    return r;
}

pssize_type write_teefd(void *coo, const void *buf, psize_type len)
{
    teefd_obj *to = (teefd_obj *)coo;
    write(to->fd, buf, len);
    return (to->wfnext)(to->coonext, buf, len);
}

void free_teefd(void *coo)
{
    teefd_obj *to = (teefd_obj *)coo;
    close(to->fd);
    delete to;
}
