// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_config.h"

extern "C" {
#include <ngx_config.h>
}

#include "sdch_module.h"

namespace sdch {

Config::Config() {}

Config::~Config() {}

Config* Config::get(ngx_http_request_t* r) {
  return static_cast<Config*>(ngx_http_get_module_loc_conf(r, sdch_module));
}

}  // namespace sdch
