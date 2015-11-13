// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_CONFIG_H_
#define SDCH_CONFIG_H_

extern "C" {
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_config.h>
}

#include <vector>

#include "sdch_dict_config.h"
#include "sdch_pool_alloc.h"

namespace sdch {

class DictionaryFactory;

class Config {
 public:
  explicit Config(ngx_pool_t* pool);
  ~Config();

  static Config* get(ngx_http_request_t* r);

  ngx_flag_t enable     = NGX_CONF_UNSET;

  ngx_hash_t types;
  ngx_array_t* types_keys;

  ngx_str_t sdch_disablecv_s;
  ngx_http_complex_value_t sdch_disablecv;

  static ngx_int_t default_bufs_size() { return ngx_pagesize; }
  static size_t default_bufs_num() { return 128 * 1024 / ngx_pagesize; }
  ngx_bufs_t bufs;

  ssize_t min_length = NGX_CONF_UNSET_SIZE;

  ngx_str_t sdch_group;
  ngx_http_complex_value_t sdch_groupcv;

  ngx_str_t sdch_url;
  ngx_http_complex_value_t sdch_urlcv;

  ngx_uint_t sdch_proxied;

  ngx_str_t sdch_dumpdir;

  ngx_flag_t enable_quasi     = NGX_CONF_UNSET;

  DictionaryFactory* dict_factory;
};


}  // namespace sdch

#endif  // SDCH_CONFIG_H_

