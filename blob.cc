#include <vector>
#include "storage.h"


int blob_create(blob_s** obj) {
    *obj = new blob_s;
    return 0;
}

int blob_append(blob_s *obj, const void *data, int len) {
    obj->data.insert(obj->data.end(), (const char*)data, (const char *)data + len);
    return 0;
}

int blob_destroy(blob_s *obj) {
    delete obj;
    return 0;
}

const char *blob_data_begin(blob_s *obj) {
    return &obj->data[0];
}

size_t blob_data_size(blob_s *obj) {
    return obj->data.size();
}
