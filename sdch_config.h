// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_CONFIG_H_
#define SDCH_CONFIG_H_

extern "C" {
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace sdch {

class Config {
 public:
  Config();
  ~Config();

  static Config* get(ngx_http_request_t* r);

  ngx_flag_t enable;
  ngx_flag_t no_buffer;

  ngx_hash_t types;

  ngx_str_t sdch_disablecv_s;
  ngx_http_complex_value_t sdch_disablecv;

  ngx_bufs_t bufs;

  size_t postpone_gzipping;
  ngx_int_t level;
  size_t memlevel;
  ssize_t min_length;

  ngx_array_t* types_keys;

  ngx_array_t* dict_storage;
  ngx_array_t* dict_conf_storage;
  ngx_int_t confdictnum;

  ngx_str_t sdch_group;
  ngx_http_complex_value_t sdch_groupcv;

  ngx_str_t sdch_url;
  ngx_http_complex_value_t sdch_urlcv;

  ngx_uint_t sdch_maxnoadv;

  ngx_uint_t sdch_proxied;

  ngx_str_t sdch_dumpdir;

  ngx_flag_t enable_quasi;
};


}  // namespace sdch

#endif  // SDCH_CONFIG_H_

