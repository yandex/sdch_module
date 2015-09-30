// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_MAIN_CONFIG_H_
#define SDCH_MAIN_CONFIG_H_

extern "C" {
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_config.h>
}

namespace sdch {

class MainConfig {
 public:
  MainConfig();
  ~MainConfig();

  ngx_uint_t stor_size = NGX_CONF_UNSET_SIZE;
};


}  // namespace sdch

#endif  // SDCH_MAIN_CONFIG_H_

