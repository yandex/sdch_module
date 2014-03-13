#include <fcntl.h>
#include <unistd.h>
#include "teefd.h"

struct teefd_obj {
    struct pz pzh;
    int fd;
    void *coonext;
};

static writerfunc write_teefd;
static closefunc free_teefd;

void *make_teefd(const char *fn, void *coo)
{
    teefd_obj *r = new teefd_obj;
    r->pzh.wf = write_teefd;
    r->pzh.cf = free_teefd;
    
    r->fd = open(fn, O_WRONLY|O_CREAT|O_EXCL, 0666);
    if (r->fd == -1) {
        delete r;
        return NULL;
    }
    r->coonext = coo;
    return r;
}

pssize_type write_teefd(void *coo, const void *buf, psize_type len)
{
    teefd_obj *to = (teefd_obj *)coo;
    write(to->fd, buf, len);
    return do_write(to->coonext, buf, len);
}

void free_teefd(void *coo)
{
    teefd_obj *to = (teefd_obj *)coo;
    close(to->fd);
    do_close(to->coonext);
    delete to;
}
