
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <assert.h>

#include "sdch_module.h"

#include "sdch_autoauto_handler.h"
#include "sdch_config.h"
#include "sdch_dictionary_factory.h"
#include "sdch_dump_handler.h"
#include "sdch_encoding_handler.h"
#include "sdch_main_config.h"
#include "sdch_output_handler.h"
#include "sdch_pool_alloc.h"
#include "sdch_request_context.h"

extern "C" {
ngx_flag_t sdch_need_vary(ngx_http_request_t *r) {
    sdch::Config* conf = sdch::Config::get(r);
    return conf->vary;
}
}

namespace sdch {

static ngx_int_t add_variables(ngx_conf_t* cf);
static ngx_int_t ratio_variable(ngx_http_request_t* r,
                                   ngx_http_variable_value_t* v,
                                   uintptr_t data);

static ngx_int_t filter_init(ngx_conf_t* cf);
static void* create_conf(ngx_conf_t* cf);
static char* merge_conf(ngx_conf_t* cf, void* parent, void* child);
static void* create_main_conf(ngx_conf_t* cf);
static char* init_main_conf(ngx_conf_t* cf, void* conf);

static char* set_sdch_dict(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

static ngx_conf_bitmask_t  ngx_http_sdch_proxied_mask[] = {
    { ngx_string("off"), NGX_HTTP_GZIP_PROXIED_OFF },
    { ngx_string("expired"), NGX_HTTP_GZIP_PROXIED_EXPIRED },
    { ngx_string("no-cache"), NGX_HTTP_GZIP_PROXIED_NO_CACHE },
    { ngx_string("no-store"), NGX_HTTP_GZIP_PROXIED_NO_STORE },
    { ngx_string("private"), NGX_HTTP_GZIP_PROXIED_PRIVATE },
    { ngx_string("no_last_modified"), NGX_HTTP_GZIP_PROXIED_NO_LM },
    { ngx_string("no_etag"), NGX_HTTP_GZIP_PROXIED_NO_ETAG },
    { ngx_string("auth"), NGX_HTTP_GZIP_PROXIED_AUTH },
    { ngx_string("any"), NGX_HTTP_GZIP_PROXIED_ANY },
    { ngx_null_string, 0 }
};
static ngx_str_t  ngx_http_gzip_no_cache = ngx_string("no-cache");
static ngx_str_t  ngx_http_gzip_no_store = ngx_string("no-store");
static ngx_str_t  ngx_http_gzip_private = ngx_string("private");

static ngx_conf_num_bounds_t stor_size_bounds = {
    ngx_conf_check_num_bounds, 1, 0xffffffffU
};

static ngx_str_t sdch_default_types[] = {
    ngx_string("text/html"),
    ngx_string("text/css"),
    ngx_string("application/javascript"),
    ngx_string("application/x-sdch-dictionary"),
    ngx_null_string
};

static ngx_str_t nodict_default_types[] = {
    ngx_string("application/x-sdch-dictionary"),
    ngx_null_string
};

static ngx_command_t  filter_commands[] = {

    { ngx_string("sdch"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, enable),
      NULL },

    { ngx_string("sdch_disablecv"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_disablecv_s),
      NULL },

    { ngx_string("sdch_buffers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_conf_set_bufs_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, bufs),
      NULL },

    { ngx_string("sdch_dict"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE123,
      set_sdch_dict,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("sdch_group"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_group),
      NULL },

    { ngx_string("sdch_url"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_url),
      NULL },

   { ngx_string("sdch_dumpdir"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_dumpdir),
      NULL },

    { ngx_string("sdch_proxied"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_proxied),
      &ngx_http_sdch_proxied_mask },


    { ngx_string("sdch_types"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_1MORE,
      ngx_http_types_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, types_keys),
      &sdch_default_types[0] },

    { ngx_string("sdch_nodict_types"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_1MORE,
      ngx_http_types_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, nodict_types_keys),
      &nodict_default_types[0] },

    { ngx_string("sdch_fastdict"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, enable_fastdict),
      NULL },

    { ngx_string("sdch_min_length"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, min_length),
      NULL },

    { ngx_string("sdch_vary"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, vary),
      NULL },

    { ngx_string("sdch_stor_size"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(MainConfig, stor_size),
      &stor_size_bounds },

      ngx_null_command
};


static ngx_http_module_t  sdch_module_ctx = {
    add_variables,           /* preconfiguration */
    filter_init,             /* postconfiguration */

    create_main_conf,        /* create main configuration */
    init_main_conf,          /* init main configuration */

    NULL,                 /* create server configuration */
    NULL,                 /* merge server configuration */

    create_conf,             /* create location configuration */
    merge_conf               /* merge location configuration */
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

#if 0
#include <execinfo.h>
static void
backtrace_log(ngx_log_t *log)
{
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    unsigned i;

    for (i = 0; i < frames; ++i) {
        ngx_log_error(NGX_LOG_INFO, log, 0, "frame %d: %s", i, strs[i]);
    }
}
#endif

static ngx_table_elt_t* header_find(ngx_list_t* headers,
                                    const char* key,
                                    ngx_str_t* value) {
  size_t keylen = strlen(key);
  ngx_list_part_t* part = &headers->part;
  ngx_table_elt_t* data = static_cast<ngx_table_elt_t*>(part->elts);
  unsigned i;

  for (i = 0;; i++) {

    if (i >= part->nelts) {
      if (part->next == NULL) {
        break;
      }

      part = part->next;
      data = static_cast<ngx_table_elt_t*>(part->elts);
      i = 0;
    }
    if (data[i].key.len == keylen &&
        ngx_strncasecmp(data[i].key.data, (u_char*)key, keylen) == 0) {
      if (value) {
        *value = data[i].value;
      }
      return &data[i];
    }
  }
  return 0;
}

static ngx_int_t ngx_http_sdch_ok(ngx_http_request_t* r) {
  if (r != r->main) {
    return NGX_DECLINED;
  }

  Config* conf = Config::get(r);

  if (r->headers_in.via == NULL) {
    return NGX_OK;
  }

  ngx_uint_t p = conf->sdch_proxied;

  if (p & NGX_HTTP_GZIP_PROXIED_OFF) {
    return NGX_DECLINED;
  }

  if (p & NGX_HTTP_GZIP_PROXIED_ANY) {
    return NGX_OK;
  }

  if (r->headers_in.authorization && (p & NGX_HTTP_GZIP_PROXIED_AUTH)) {
    return NGX_OK;
  }

  ngx_table_elt_t* e = r->headers_out.expires;
  if (e) {
    if (!(p & NGX_HTTP_GZIP_PROXIED_EXPIRED)) {
      return NGX_DECLINED;
    }

    time_t date;
    time_t expires = ngx_http_parse_time(e->value.data, e->value.len);
    if (expires == NGX_ERROR) {
      return NGX_DECLINED;
    }

    ngx_table_elt_t* d = r->headers_out.date;
    if (d) {
      date = ngx_http_parse_time(d->value.data, d->value.len);
      if (date == NGX_ERROR) {
        return NGX_DECLINED;
      }

    } else {
      date = ngx_time();
    }

    if (expires < date) {
      return NGX_OK;
    }

    return NGX_DECLINED;
  }

  ngx_array_t* cc = &r->headers_out.cache_control;

  if (cc->elts) {

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_CACHE) &&
        ngx_http_parse_multi_header_lines(
            cc, &ngx_http_gzip_no_cache, NULL) >= 0) {
      return NGX_OK;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_STORE) &&
        ngx_http_parse_multi_header_lines(
            cc, &ngx_http_gzip_no_store, NULL) >= 0) {
      return NGX_OK;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_PRIVATE) &&
        ngx_http_parse_multi_header_lines(
            cc, &ngx_http_gzip_private, NULL) >= 0) {
      return NGX_OK;
    }

    return NGX_DECLINED;
  }

  if ((p & NGX_HTTP_GZIP_PROXIED_NO_LM) && r->headers_out.last_modified) {
    return NGX_DECLINED;
  }

  if ((p & NGX_HTTP_GZIP_PROXIED_NO_ETAG) && r->headers_out.etag) {
    return NGX_DECLINED;
  }

  return NGX_OK;
}

static FastdictFactory::ValuePtr find_quasidict(ngx_http_request_t* r,
                                        const u_char* const h) {
  Dictionary::id_t id;
  std::copy(h, h + 8, id.data());
  MainConfig* main = MainConfig::get(r);
  return main->fastdict_factory.find(id);
}

static ngx_int_t
get_dictionary_header(ngx_http_request_t *r, Config *conf)
{
    if (header_find(&r->headers_out.headers, "get-dictionary", NULL)) {
        return NGX_OK;
    }
    if (ngx_http_test_content_type(r, &conf->nodict_types) != NULL) {
        return NGX_OK;
    }
    ngx_str_t val;
    if (ngx_http_complex_value(r, &conf->sdch_urlcv, &val) != NGX_OK) {
        return NGX_ERROR;
    }
    if (val.len == 0) {
        return NGX_OK;
    }

    ngx_table_elt_t *h;
    h = static_cast<ngx_table_elt_t*>(ngx_list_push(&r->headers_out.headers));
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->hash = 1;
    ngx_str_set(&h->key, "Get-Dictionary");
    h->value = val;
    return NGX_OK;
}

template <size_t K, size_t V>
ngx_int_t create_output_header(ngx_http_request_t* r,
                               const char (&key)[K],
                               const char (&value)[V],
                               ngx_table_elt_t* prev = NULL) {
  ngx_table_elt_t* h = prev
                       ? prev
                       : static_cast<ngx_table_elt_t*>(
                            ngx_list_push(&r->headers_out.headers));
  if (h == NULL) {
    return NGX_ERROR;
  }

  h->hash = 1;
  ngx_str_set(&h->key, key);
  ngx_str_set(&h->value, value);

  return NGX_OK;
}

static ngx_int_t
x_sdch_encode_0_header(ngx_http_request_t *r, bool sdch_expected)
{
  ngx_table_elt_t* h =
      header_find(&r->headers_out.headers, "x-sdch-encode", NULL);
  if (!sdch_expected) {
    if (h != NULL) {
      h->hash = 0;
      h->value.len = 0;
    }
    return NGX_OK;
  }
  return create_output_header(r, "X-Sdch-Encode", "0", h);
}

static ngx_int_t
expand_disable(ngx_http_request_t *r, Config *conf)
{
    ngx_str_t dv;
    if (ngx_http_complex_value(r, &conf->sdch_disablecv, &dv) != NGX_OK) {
        return 0;
    }
    if (dv.len != 0 && dv.data[0] != '0') {
        return 1;
    }
    return 0;
}

// Check should we process request at all
static bool should_process(ngx_http_request_t* r, Config* conf,
                                bool* sdch_encoded) {
  if (!conf->enable) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: not enabled");

    return false;
  }

  if (r->headers_out.status != NGX_HTTP_OK
    && r->headers_out.status != NGX_HTTP_FORBIDDEN
    && r->headers_out.status != NGX_HTTP_NOT_FOUND) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: unsupported status");

    return false;
  }

  if (r->headers_out.content_encoding
    && r->headers_out.content_encoding->value.len) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: content is already encoded");
    const ngx_str_t &val = r->headers_out.content_encoding->value;
    if (ngx_strstrn(val.data, const_cast<char*>("sdch"), val.len) != 0) // XXX
      *sdch_encoded = true;
    return false;
  }

  if (r->headers_out.content_length_n != -1
    && r->headers_out.content_length_n < conf->min_length) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: content is too small");
    return false;
  }

  if (ngx_http_test_content_type(r, &conf->types) == NULL) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: unsupported content type");
    return false;
  }

  if (expand_disable(r, conf)) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: CV disabled");
    return false;
  }

  return true;
}

// Select Dictionary based on available dictionaries, group, support for quasis
// and phase of the moon.
ngx_int_t select_dictionary(ngx_http_request_t* r,
                            DictionaryFactory* dict_factory,
                            ngx_str_t val,
                            const ngx_str_t group,
                            bool sdch_expected,
                            Dictionary*& dict,
                            bool& is_best,
                            FastdictFactory::ValuePtr& quasidict) {
  DictConfig* bestdict = NULL;
  while (val.len >= 8) {
    DictConfig* d = dict_factory->find_dictionary(val.data);
    bestdict = dict_factory->choose_best_dictionary(bestdict, d, group);
    if (quasidict == NULL && d == NULL) {
      quasidict = find_quasidict(r, val.data);
      ngx_log_error(NGX_LOG_INFO,
                    r->connection->log,
                    0,
                    "find_quasidict %.8s -> %p",
                    val.data,
                    quasidict.get());
    }
    val.data += 8;
    val.len -= 8;
    size_t l = std::min(strspn((char*)val.data, " \t,"), val.len);
    val.data += l;
    val.len -= l;
  }
  if (bestdict != NULL) {
    ngx_log_error(NGX_LOG_INFO,
                  r->connection->log,
                  0,
                  "group: %s prio: %d best: %d",
                  bestdict->groupname.data,
                  bestdict->priority,
                  bestdict->best);
  } else {
    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "nobestdict");
  }

  if (bestdict == NULL) {
    // Use quasi if there it's available and there is no predefined one
    if (quasidict != NULL) {
      dict = &quasidict->dict;
    }
    is_best = false;
  }
  else {
    // If we found dictionary, but is should be best and in correct group to be
    // actually THE best.
    is_best = bestdict->best && ngx_memn2cmp(bestdict->groupname.data,
                                             group.data,
                                             bestdict->groupname.len,
                                             group.len) == 0;
    dict = bestdict->dict;
  }

  return NGX_OK;
}


static ngx_int_t
header_filter(ngx_http_request_t *r)
{
  ngx_log_debug(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http sdch filter header 000");

  Config* conf = Config::get(r);

  ngx_str_t val;
  // Workaround for nginx's strstrn which is not decrementing "n" while doing
  // outmost loop on strings. So third parameter is length(sdch) - 1.
  if (header_find(&r->headers_in.headers, "accept-encoding", &val) == 0 ||
      (val.len < 4) ||
      ngx_strstrn(val.data, const_cast<char*>("sdch"), size_t(4 - 1)) == 0) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  0,
                  "http sdch filter header: no sdch in accept-encoding");
    return ngx_http_next_header_filter(r);
  }

  // Check that Browser announces FastDict support.
  bool store_as_quasi = false;
  if (header_find(&r->headers_in.headers, "sdch-features", &val) != 0 &&
      ngx_strstrn(val.data, const_cast<char*>("fastdict"), val.len) != 0) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  0,
                  "http sdch filter header: enable FastDict");
    store_as_quasi = true;
  }

  if (header_find(&r->headers_in.headers, "avail-dictionary", &val) == 0) {
    ngx_str_set(&val, "");
  }
  bool sdch_expected = (val.len > 0);

  bool sdch_encoded = false;
  if (!should_process(r, conf, &sdch_encoded)) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  0,
                  "http sdch filter header: skipping request");
    ngx_int_t e = x_sdch_encode_0_header(r, sdch_expected && !sdch_encoded);
    if (e)
      return e;
    return ngx_http_next_header_filter(r);
  }

  if (r->header_only) {
    return ngx_http_next_header_filter(r);
  }

  if (ngx_http_sdch_ok(r) != NGX_OK) {
    return ngx_http_next_header_filter(r);
  }

  ngx_str_t group;
  if (ngx_http_complex_value(r, &conf->sdch_groupcv, &group) != NGX_OK) {
    return NGX_ERROR;
  }

  Dictionary* dict = NULL;
  FastdictFactory::ValuePtr quasidict;
  bool is_best;

  select_dictionary(r,
                    conf->dict_factory,
                    val,
                    group,
                    sdch_expected,
                    dict,
                    is_best,
                    quasidict);

  // No the best Dictionary selected.
  if (!is_best) {
    ngx_int_t e = get_dictionary_header(r, conf);
    if (e != NGX_OK)
      return e;
  }

  // Actually it wasn't selected at all.
  if (dict == NULL) {
    ngx_int_t e = x_sdch_encode_0_header(r, sdch_expected);
    if (e != NGX_OK)
      return e;
    // And we are not creating quasi one.
    if (store_as_quasi) {
      if (create_output_header(r, "X-Sdch-Use-As-Dictionary", "1") != NGX_OK)
        return NGX_ERROR;
    } else {
      return ngx_http_next_header_filter(r);
    }
  }


  RequestContext* ctx = POOL_ALLOC(r, RequestContext, r);
  if (ctx == NULL) {
    return NGX_ERROR;
  }

  // Allocate Handlers chain in reverse order
  // Last will be OutputHandler.
  ctx->handler = POOL_ALLOC(r, OutputHandler, ctx, ngx_http_next_body_filter);
  if (ctx->handler == NULL)
    return NGX_ERROR;

  // If we have actual Dictionary - do encode response
  if (dict != NULL) {
    if (create_output_header(r, "Content-Encoding", "sdch") != NGX_OK) {
      return NGX_ERROR;
    }

    if (x_sdch_encode_0_header(r, false) != NGX_OK) {
      return NGX_ERROR;
    }

    if (conf->vary == 1) {
      if (create_output_header(r, "Vary", "Accept-Encoding, Avail-Dictionary") != NGX_OK) {
        return NGX_ERROR;
      }
    }

    ctx->handler = POOL_ALLOC(r,
        EncodingHandler, ctx->handler, dict, quasidict);

    if (ctx->handler == NULL) {
      return NGX_ERROR;
    }
  }

  if (conf->sdch_dumpdir.len > 0) {
    ctx->handler = POOL_ALLOC(r, DumpHandler, ctx->handler);
    if (ctx->handler == NULL) {
      return NGX_ERROR;
    }
  }

  // If we have to create new quasi-dictionary
  if (store_as_quasi) {
    ctx->handler = POOL_ALLOC(r, AutoautoHandler, ctx, ctx->handler);
    if (ctx->handler == NULL) {
      return NGX_ERROR;
    }
  }

  r->main_filter_need_in_memory = 1;

  ngx_http_clear_content_length(r);
  ngx_http_clear_accept_ranges(r);

#if nginx_version > 1005000
  ngx_http_clear_etag(r);
#else
//   TODO(wawa): adverse impact should be verified if any
#endif

  return ngx_http_next_header_filter(r);
}


static ngx_int_t
body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
  RequestContext* ctx = RequestContext::get(r);

  ngx_log_debug0(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http sdch filter body 000");

  if (ctx == NULL || ctx->done || r->header_only) {
    return ngx_http_next_body_filter(r, in);
  }

  if (!ctx->started) {
    ctx->started = true;
    for (Handler* h = ctx->handler; h; h = h->next()) {
      if (!h->init(ctx)) {
        ctx->done = true;
        return NGX_ERROR;
      }
    }
  }

  ngx_log_debug0(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http sdch filter started");

  // cycle while there is data to handle
  for (; in; in = in->next) {
    if (in->buf->flush) {
      ctx->need_flush = true;
    }

    off_t buf_size = ngx_buf_size(in->buf);
    ngx_int_t status = ctx->handler->on_data(in->buf->pos, buf_size);
    in->buf->pos = in->buf->last;
    ctx->total_in += buf_size;

    if (status == NGX_ERROR) {
      ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "sdch failed");
      ctx->done = true;
      return NGX_ERROR;
    }

    if (in->buf->last_buf) {
      ngx_log_debug(NGX_LOG_DEBUG_HTTP,
          ctx->request->connection->log, 0, "closing ctx");
      ctx->done = true;
      return ctx->handler->on_finish();
    }
  }

  return NGX_OK;
}


static ngx_str_t ratio = ngx_string("sdch_ratio");

static ngx_int_t
add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &ratio, NGX_HTTP_VAR_NOHASH);
    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ratio_variable;

    return NGX_OK;
}


static ngx_int_t
ratio_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t            zint, zfrac;
    RequestContext  *ctx = RequestContext::get(r);

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;


    if (ctx == NULL || ctx->total_out == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = static_cast<u_char*>(ngx_pnalloc(r->pool, NGX_INT32_LEN + 3));
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    zint = (ngx_uint_t) (ctx->total_in / ctx->total_out);
    zfrac = (ngx_uint_t) ((ctx->total_in * 100 / ctx->total_out) % 100);

    if ((ctx->total_in * 1000 / ctx->total_out) % 10 > 4) {

        /* the rounding, e.g., 2.125 to 2.13 */

        zfrac++;

        if (zfrac > 99) {
            zint++;
            zfrac = 0;
        }
    }

    v->len = ngx_sprintf(v->data, "%ui.%02ui", zint, zfrac) - v->data;

    return NGX_OK;
}

static void *
create_main_conf(ngx_conf_t *cf)
{
    return POOL_ALLOC(cf, MainConfig);
}

static char *
init_main_conf(ngx_conf_t *cf, void *cnf)
{
    MainConfig *conf = static_cast<MainConfig*>(cnf);
    if (conf->stor_size != NGX_CONF_UNSET_SIZE)
        conf->fastdict_factory.set_max_size(conf->stor_size);
    return NGX_CONF_OK;
}

static void *
create_conf(ngx_conf_t *cf)
{
  return POOL_ALLOC(cf, Config, cf->pool);
}

static char *
set_sdch_dict(ngx_conf_t *cf, ngx_command_t *cmd, void *cnf)
{
    Config *conf = static_cast<Config*>(cnf);

    if (cf->args->nelts < 2 || cf->args->nelts > 4) {
        return const_cast<char*>("Wrong number of arguments");
    }
    ngx_str_t *value = static_cast<ngx_str_t*>(cf->args->elts);
    ngx_str_t groupname;
    ngx_str_set(&groupname, "default");
    if (cf->args->nelts >= 3) {
        groupname = value[2];
    }

    ngx_int_t prio = -1;
    if (cf->args->nelts >= 4) {
        prio = ngx_atoi(value[3].data, value[3].len);
        if (prio == NGX_ERROR) {
            return const_cast<char*>("Can't convert to number");
        }
    }

    Dictionary* dict = conf->dict_factory->load_dictionary((const char*)value[1].data);
    if (!dict) {
      ngx_conf_log_error(
          NGX_LOG_EMERG, cf, 0, "get_hashed_dict %s failed", value[1].data);
      return const_cast<char*>("Can't load dictionary");
    }
    ngx_conf_log_error(NGX_LOG_NOTICE,
                       cf,
                       0,
                       "dictionary ids: user %*s server %*s",
                       dict->client_id().size(), dict->client_id().data(),
                       dict->server_id().size(), dict->server_id().data());

    conf->dict_factory->store_config(
        dict,
        groupname,
        prio);

    return NGX_CONF_OK;
}


static char *
merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    Config *prev = static_cast<Config*>(parent);
    Config *conf = static_cast<Config*>(child);
    ngx_http_compile_complex_value_t ccv;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_bufs_value(conf->bufs,
                              prev->bufs,
                              Config::default_bufs_num(),
                              Config::default_bufs_size());

    ngx_conf_merge_str_value(conf->sdch_disablecv_s, prev->sdch_disablecv_s, "");
    ngx_memzero(&ccv, sizeof(ccv));
    ccv.cf = cf;
    ccv.value = &conf->sdch_disablecv_s;
    ccv.complex_value = &conf->sdch_disablecv;
    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return const_cast<char*>("ngx_http_compile_complex_value sdch_disablecv failed");
    }

    if (ngx_http_merge_types(cf, &conf->types_keys, &conf->types,
                             &prev->types_keys, &prev->types,
                             sdch_default_types)
        != NGX_OK) {
        return const_cast<char*>("Can't merge config");
    }

    if (ngx_http_merge_types(cf, &conf->nodict_types_keys, &conf->nodict_types,
                             &prev->nodict_types_keys, &prev->nodict_types,
                             nodict_default_types)
        != NGX_OK) {
        return const_cast<char*>("Can't merge config");
    }

    // Merge dictionaries from parent.
    conf->dict_factory->merge(prev->dict_factory);

    ngx_conf_merge_str_value(conf->sdch_group, prev->sdch_group, "default");
    ccv.value = &conf->sdch_group;
    ccv.complex_value = &conf->sdch_groupcv;
    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return const_cast<char*>("ngx_http_compile_complex_value sdch_group failed");
    }

    ngx_conf_merge_str_value(conf->sdch_url, prev->sdch_url, "");
    ccv.value = &conf->sdch_url;
    ccv.complex_value = &conf->sdch_urlcv;
    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return const_cast<char*>("ngx_http_compile_complex_value sdch_url failed");
    }

    ngx_conf_merge_str_value(conf->sdch_dumpdir, prev->sdch_dumpdir, "");

    ngx_conf_merge_value(conf->enable_fastdict, prev->enable_fastdict, 1);

    ngx_conf_merge_value(conf->vary, prev->vary, 1);

    return NGX_CONF_OK;
}


static ngx_int_t
filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = body_filter;

    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "http sdch filter init");
    return NGX_OK;
}

}  // namespace sdch

// It should be outside namespace
ngx_module_t  sdch_module = {
    NGX_MODULE_V1,
    &sdch::sdch_module_ctx,           /* module context */
    sdch::filter_commands,            /* module directives */
    NGX_HTTP_MODULE,                  /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

