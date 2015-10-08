// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_DICT_CONFIG_H_
#define SDCH_DICT_CONFIG_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
}

namespace sdch {

class Dictionary;

struct DictConfig {
  ngx_str_t groupname;
  ngx_uint_t priority;
  int best;
  Dictionary* dict;
};


}  // namespace sdch

#endif  // SDCH_DICT_CONFIG_H_

