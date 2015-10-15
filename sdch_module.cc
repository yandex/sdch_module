
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

namespace sdch {

namespace {

}  // namespace

static ngx_int_t tr_add_variables(ngx_conf_t* cf);
static ngx_int_t tr_ratio_variable(ngx_http_request_t* r,
                                   ngx_http_variable_value_t* v,
                                   uintptr_t data);

static ngx_int_t tr_filter_init(ngx_conf_t* cf);
static void* tr_create_conf(ngx_conf_t* cf);
static char* tr_merge_conf(ngx_conf_t* cf, void* parent, void* child);
static void* tr_create_main_conf(ngx_conf_t* cf);
static char* tr_init_main_conf(ngx_conf_t* cf, void* conf);

static char* tr_set_sdch_dict(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

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

static ngx_conf_num_bounds_t tr_stor_size_bounds = {
    ngx_conf_check_num_bounds, 1, 0xffffffffU
};

static ngx_command_t  tr_filter_commands[] = {

    { ngx_string("sdch"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, enable),
      nullptr },

    { ngx_string("sdch_disablecv"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_disablecv_s),
      nullptr },

    { ngx_string("sdch_buffers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_conf_set_bufs_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, bufs),
      nullptr },

    { ngx_string("sdch_dict"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE123,
      tr_set_sdch_dict,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      nullptr },

    { ngx_string("sdch_group"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_group),
      nullptr },

    { ngx_string("sdch_url"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_url),
      nullptr },

    { ngx_string("sdch_maxnoadv"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_maxnoadv),
      nullptr },

    { ngx_string("sdch_dumpdir"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, sdch_dumpdir),
      nullptr },

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
      &ngx_http_html_default_types[0] },

    { ngx_string("sdch_quasi"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, enable_quasi),
      nullptr },

    { ngx_string("sdch_min_length"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, min_length),
      nullptr },

    { ngx_string("sdch_stor_size"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(MainConfig, stor_size),
      &tr_stor_size_bounds },

      ngx_null_command
};


static ngx_http_module_t  sdch_module_ctx = {
    tr_add_variables,           /* preconfiguration */
    tr_filter_init,             /* postconfiguration */

    tr_create_main_conf,        /* create main configuration */
    tr_init_main_conf,          /* init main configuration */

    nullptr,                    /* create server configuration */
    nullptr,                    /* merge server configuration */

    tr_create_conf,             /* create location configuration */
    tr_merge_conf               /* merge location configuration */
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
      if (part->next == nullptr) {
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

  auto* conf = Config::get(r);

  if (r->headers_in.via == nullptr) {
    return NGX_OK;
  }

  auto p = conf->sdch_proxied;

  if (p & NGX_HTTP_GZIP_PROXIED_OFF) {
    return NGX_DECLINED;
  }

  if (p & NGX_HTTP_GZIP_PROXIED_ANY) {
    return NGX_OK;
  }

  if (r->headers_in.authorization && (p & NGX_HTTP_GZIP_PROXIED_AUTH)) {
    return NGX_OK;
  }

  auto e = r->headers_out.expires;
  if (e) {
    if (!(p & NGX_HTTP_GZIP_PROXIED_EXPIRED)) {
      return NGX_DECLINED;
    }

    time_t date;
    time_t expires = ngx_http_parse_time(e->value.data, e->value.len);
    if (expires == NGX_ERROR) {
      return NGX_DECLINED;
    }

    auto d = r->headers_out.date;
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

  auto cc = &r->headers_out.cache_control;

  if (cc->elts) {

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_CACHE) &&
        ngx_http_parse_multi_header_lines(
            cc, &ngx_http_gzip_no_cache, nullptr) >= 0) {
      return NGX_OK;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_STORE) &&
        ngx_http_parse_multi_header_lines(
            cc, &ngx_http_gzip_no_store, nullptr) >= 0) {
      return NGX_OK;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_PRIVATE) &&
        ngx_http_parse_multi_header_lines(
            cc, &ngx_http_gzip_private, nullptr) >= 0) {
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

static Storage::ValueHolder find_quasidict(ngx_http_request_t* r, u_char* h) {
  char nm[9];
  nm[8] = 0;
  memcpy(nm, h, 8);

  auto* main = MainConfig::get(r);
  return main->storage.find(nm);
}

static ngx_int_t
get_dictionary_header(ngx_http_request_t *r, Config *conf)
{
    ngx_str_t val;
    if (ngx_http_complex_value(r, &conf->sdch_urlcv, &val) != NGX_OK) {
        return NGX_ERROR;
    }
    if (val.len == 0) {
        return NGX_OK;
    }

    ngx_table_elt_t *h;
    h = static_cast<ngx_table_elt_t*>(ngx_list_push(&r->headers_out.headers));
    if (h == nullptr) {
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
                               ngx_table_elt_t* prev = nullptr) {
  ngx_table_elt_t* h = prev
                       ? prev
                       : static_cast<ngx_table_elt_t*>(
                            ngx_list_push(&r->headers_out.headers));
  if (h == nullptr) {
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
      header_find(&r->headers_out.headers, "x-sdch-encode", nullptr);
  if (!sdch_expected) {
    if (h != nullptr) {
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
static ngx_int_t should_process(ngx_http_request_t* r, Config* conf) {
  if (!conf->enable) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: not enabled");

    return NGX_HTTP_FORBIDDEN;
  }

  if (r->headers_out.status != NGX_HTTP_OK
    && r->headers_out.status != NGX_HTTP_FORBIDDEN
    && r->headers_out.status != NGX_HTTP_NOT_FOUND) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: unsupported status");

    return NGX_HTTP_FORBIDDEN;
  }

  if (r->headers_out.content_encoding
    && r->headers_out.content_encoding->value.len) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: content is already encoded");
    return NGX_HTTP_FORBIDDEN;
  }

  if (r->headers_out.content_length_n != -1
    && r->headers_out.content_length_n < conf->min_length) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: content is too small");
    return NGX_HTTP_FORBIDDEN;
  }

  if (ngx_http_test_content_type(r, &conf->types) == nullptr) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: unsupported content type");
    return NGX_HTTP_FORBIDDEN;
  }

  if (expand_disable(r, conf)) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: CV disabled");
    return NGX_HTTP_FORBIDDEN;
  }

  return NGX_OK;
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
                            Storage::ValueHolder& quasidict) {
  DictConfig* bestdict = nullptr;
  while (val.len >= 8) {
    DictConfig* d = dict_factory->find_dictionary(val.data);
    bestdict = dict_factory->choose_best_dictionary(bestdict, d, group);
    if (quasidict == nullptr && d == nullptr) {
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
    auto l = std::min(strspn((char*)val.data, " \t,"), val.len);
    val.data += l;
    val.len -= l;
  }
  if (bestdict != nullptr) {
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

  if (bestdict == nullptr) {
    // Use quasi if there it's available and there is no predefined one
    if (quasidict != nullptr) {
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
tr_header_filter(ngx_http_request_t *r)
{
  ngx_log_debug(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http sdch filter header 000");

  auto* conf = Config::get(r);

  ngx_str_t val;
  if (header_find(&r->headers_in.headers, "accept-encoding", &val) == 0 ||
      ngx_strstrn(val.data, const_cast<char*>("sdch"), val.len) == 0) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  0,
                  "http sdch filter header: no sdch in accept-encoding");
    return ngx_http_next_header_filter(r);
  }

  if (header_find(&r->headers_in.headers, "avail-dictionary", &val) == 0) {
    ngx_str_set(&val, "");
  }
  bool sdch_expected = (val.len > 0);

  if (should_process(r, conf) != NGX_OK) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  0,
                  "http sdch filter header: skipping request");
    ngx_int_t e = x_sdch_encode_0_header(r, sdch_expected);
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

  bool store_as_quasi = false;
  // FIXME We don't create quasi when Browser announce support for it, but has
  // another dictionary. Is it desired behavior?
  if (ngx_strcmp(val.data, "AUTOAUTO") == 0 && conf->enable_quasi) {
    store_as_quasi = true;
    if (create_output_header(r, "X-Sdch-Use-As-Dictionary", "1") != NGX_OK)
      return NGX_ERROR;
  }

  ngx_str_t group;
  if (ngx_http_complex_value(r, &conf->sdch_groupcv, &group) != NGX_OK) {
    return NGX_ERROR;
  }

  Dictionary* dict = nullptr;
  Storage::ValueHolder quasidict;
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
    auto e = get_dictionary_header(r, conf);
    if (e != NGX_OK)
      return e;
  }

  // Actually it wasn't selected at all.
  if (dict == nullptr) {
    auto e = x_sdch_encode_0_header(r, sdch_expected);
    if (e != NGX_OK)
      return e;
    // And we are not creating quasi one.
    if (!store_as_quasi)
      return ngx_http_next_header_filter(r);
  }


  auto* ctx = pool_alloc<RequestContext>(r, r);
  if (ctx == nullptr) {
    return NGX_ERROR;
  }

  // Allocate Handlers chain in reverse order
  // Last will be OutputHandler.
  ctx->handler = pool_alloc<OutputHandler>(r, ctx, ngx_http_next_body_filter);
  if (ctx->handler == nullptr)
    return NGX_ERROR;

  // If we have actual Dictionary - do encode response
  if (dict != nullptr) {
    if (create_output_header(r, "Content-Encoding", "sdch") != NGX_OK) {
      return NGX_ERROR;
    }

    ctx->handler = pool_alloc<EncodingHandler>(r,
                                               ctx->handler,
                                               dict,
                                               std::move(quasidict));
    if (ctx->handler == nullptr) {
      return NGX_ERROR;
    }
  }

  if (conf->sdch_dumpdir.len > 0) {
    ctx->handler = pool_alloc<DumpHandler>(r, ctx->handler);
    if (ctx->handler == nullptr) {
      return NGX_ERROR;
    }
  }

  // If we have to create new quasi-dictionary
  if (store_as_quasi) {
    ctx->handler = pool_alloc<AutoautoHandler>(r, ctx, ctx->handler);
    if (ctx->handler == nullptr) {
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
tr_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    RequestContext  *ctx = RequestContext::get(r);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http sdch filter body 000");

    if (ctx == nullptr || ctx->done || r->header_only) {
        return ngx_http_next_body_filter(r, in);
    }

    if (!ctx->started) {
        ctx->started = true;
        for (auto* h = ctx->handler; h; h = h->next()) {
          if (!h->init(ctx)) {
            ctx->done = true;
            return NGX_ERROR;
          }
        }
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http sdch filter started");

    // cycle while there is data to handle
    for (; in ; in = in->next) {
        if (in->buf->flush) {
          ctx->need_flush = true;
        }

        auto buf_size = ngx_buf_size(in->buf);
        auto status = ctx->handler->on_data(reinterpret_cast<char*>(in->buf->pos),
                                            buf_size);
        in->buf->pos = in->buf->last;
        ctx->total_in += buf_size;

        if (status == Status::ERROR) {
          ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "sdch failed");
          ctx->done = true;
          return NGX_ERROR;
        }

        if (in->buf->last_buf) {
            ngx_log_error(NGX_LOG_ALERT, ctx->request->connection->log, 0, "closing ctx");
            ctx->done = true;
            return ctx->handler->on_finish() == Status::OK ? NGX_OK : NGX_ERROR;
        }
    }

    return NGX_OK;
}


static ngx_str_t tr_ratio = ngx_string("sdch_ratio");

static ngx_int_t
tr_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &tr_ratio, NGX_HTTP_VAR_NOHASH);
    if (var == nullptr) {
        return NGX_ERROR;
    }

    var->get_handler = tr_ratio_variable;

    return NGX_OK;
}


static ngx_int_t
tr_ratio_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t            zint, zfrac;
    RequestContext  *ctx = RequestContext::get(r);

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;


    if (ctx == nullptr || ctx->total_out == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = static_cast<u_char*>(ngx_pnalloc(r->pool, NGX_INT32_LEN + 3));
    if (v->data == nullptr) {
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
tr_create_main_conf(ngx_conf_t *cf)
{
    return pool_alloc<MainConfig>(cf);
}

static char *
tr_init_main_conf(ngx_conf_t *cf, void *cnf)
{
    MainConfig *conf = static_cast<MainConfig*>(cnf);
    if (conf->stor_size != NGX_CONF_UNSET_SIZE)
        conf->storage.set_max_size(conf->stor_size);
    return NGX_CONF_OK;
}

static void *
tr_create_conf(ngx_conf_t *cf)
{
  return pool_alloc<Config>(cf, cf->pool);
}

static char *
tr_set_sdch_dict(ngx_conf_t *cf, ngx_command_t *cmd, void *cnf)
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

    auto* dict = conf->dict_factory->allocate_dictionary();
    if (!dict->init_from_file((const char*)value[1].data)) {
      ngx_conf_log_error(
          NGX_LOG_EMERG, cf, 0, "get_hashed_dict %s failed", value[1].data);
      return const_cast<char*>("Get hashed dict failed");
    }
    ngx_conf_log_error(NGX_LOG_NOTICE,
                       cf,
                       0,
                       "dictionary ids: user %s server %s",
                       dict->client_id().c_str(),
                       dict->server_id().c_str());

    auto* sdc = conf->dict_factory->store_config(
        dict,
        groupname,
        prio);

    return NGX_CONF_OK;
}


static char *
tr_merge_conf(ngx_conf_t *cf, void *parent, void *child)
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
                             ngx_http_html_default_types)
        != NGX_OK) {
        return const_cast<char*>("Can't merge config");
    }

    // FIXME Should we still merge it?
    if (!conf->enable)
      return NGX_CONF_OK;

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

    ngx_conf_merge_uint_value(conf->sdch_maxnoadv, prev->sdch_maxnoadv, 0);

    ngx_conf_merge_str_value(conf->sdch_dumpdir, prev->sdch_dumpdir, "");

    ngx_conf_merge_value(conf->enable_quasi, prev->enable_quasi, 1);

    return NGX_CONF_OK;
}


static ngx_int_t
tr_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = tr_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = tr_body_filter;

    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "http sdch filter init");
    return NGX_OK;
}

}  // namespace sdch

// It should be outside namespace
ngx_module_t  sdch_module = {
    NGX_MODULE_V1,
    &sdch::sdch_module_ctx,           /* module context */
    sdch::tr_filter_commands,         /* module directives */
    NGX_HTTP_MODULE,                  /* module type */
    nullptr,                          /* init master */
    nullptr,                          /* init module */
    nullptr,                          /* init process */
    nullptr,                          /* init thread */
    nullptr,                          /* exit thread */
    nullptr,                          /* exit process */
    nullptr,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

