#include <stdlib.h>
#include <unistd.h>
#include "pzlib.h"

struct fdobj {
    int fd;
};

void *
make_fd(int fd)
{
    struct fdobj *fo = malloc(sizeof(struct fdobj));
    fo->fd = fd;
    return fo;
}

pssize_type
write_fd(void *fdo, const void *buf, psize_type len)
{
    struct fdobj *fo = fdo;
    return write(fo->fd, buf, len);
}

void
free_fd(void *fo)
{
    free(fo);
}
