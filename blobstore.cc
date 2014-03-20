#include "blobstore.h"
#include "storage.h"

static writerfunc write_blobstore;
static closefunc close_blobstore;

struct blob_obj {
    struct pz pzh;
    blob_type *blob;
    void *coonext;
    blob_obj() {
        pzh.wf = write_blobstore;
        pzh.cf = close_blobstore;
    }
};

void *make_blobstore(void *c, blob_type *blob)
{
    blob_obj *r = new blob_obj;
    r->blob = blob;
    blob_create(r->blob);
    r->coonext = c;
    
    return r;
}

pssize_type write_blobstore(void *coo, const void *buf, psize_type len)
{
    blob_obj *bo = (blob_obj *)coo;
    if (blob_append(*(bo->blob), buf, len) != 0)
        bo->blob = NULL;
    return do_write(bo->coonext, buf, len);
}

void close_blobstore(void *coo)
{
    blob_obj *bo = (blob_obj *)coo;
    do_close(bo->coonext);
}
