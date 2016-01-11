// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_MAIN_CONFIG_H_
#define SDCH_MAIN_CONFIG_H_

extern "C" {
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_config.h>
}

#include "sdch_storage.h"

namespace sdch {

class MainConfig {
 public:
  MainConfig();
  ~MainConfig();

  static MainConfig* get(ngx_http_request_t* r);

  Storage storage;
  // TODO Change config handling to pass it to Storage directly
  ngx_uint_t stor_size;
};


}  // namespace sdch

#endif  // SDCH_MAIN_CONFIG_H_

