// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_MODULE_H_
#define SDCH_MODULE_H_

#include <sys/types.h>

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include "zlib.h"

extern ngx_module_t sdch_module;

namespace sdch {

class RequestContext;

//  It will be needed by OutputHandler
ssize_t tr_filter_write(RequestContext* ctx, const char *buf, size_t len);

}  // namespace sdch

#endif  // SDCH_MODULE_H_

