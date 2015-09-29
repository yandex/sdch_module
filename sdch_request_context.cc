// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_request_context.h"

#include "sdch_module.h"

namespace sdch {

RequestContext::RequestContext(ngx_http_request_t* r) : request(r) {
}

RequestContext* RequestContext::get(ngx_http_request_t* r) {
  return static_cast<RequestContext*>(ngx_http_get_module_ctx(r, sdch_module));
}

}  // namespace sdch
