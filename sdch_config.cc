// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_config.h"

namespace sdch {

Config::Config(ngx_pool_t* pool)
    : dict_storage(
          pool_alloc<DictStorage>(pool, DictStorage::allocator_type(pool))),
      dict_conf_storage(
          pool_alloc<DictConfStorage>(pool,
                                      DictConfStorage::allocator_type(pool))) {}

Config::~Config() {}

Config* Config::get(ngx_http_request_t* r) {
  return static_cast<Config*>(ngx_http_get_module_loc_conf(r, sdch_module));
}

}  // namespace sdch
