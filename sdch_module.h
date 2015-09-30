// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_MODULE_H_
#define SDCH_MODULE_H_

#include "blobstore.h"
#include "teefd.h"
#include "vcd-h1.h"
#include "zlib.h"

extern ngx_module_t sdch_module;

namespace sdch {

class RequestContext;

struct sdch_dict {
    blob_type dict;
    hashed_dictionary_p  hashed_dict;
    unsigned char user_dictid[9], server_dictid[9];
};

typedef struct {
    ngx_str_t            groupname;
    ngx_uint_t           priority;
    int                  best;
    struct sdch_dict    *dict;
} sdch_dict_conf;


}  // namespace sdch

#endif  // SDCH_MODULE_H_

