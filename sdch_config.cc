// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_config.h"

#include "sdch_dictionary_factory.h"
#include "sdch_module.h"

namespace sdch {

Config::Config(ngx_pool_t* pool)
    : enable(NGX_CONF_UNSET),
      min_length(NGX_CONF_UNSET_SIZE),
      enable_fastdict(NGX_CONF_UNSET),
      vary(NGX_CONF_UNSET),
      dict_factory(new (pool) DictionaryFactory(pool)) {}

Config::~Config() {}

Config* Config::get(ngx_http_request_t* r) {
  return static_cast<Config*>(ngx_http_get_module_loc_conf(r, sdch_module));
}

}  // namespace sdch
