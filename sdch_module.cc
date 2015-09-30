
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <assert.h>

#include "sdch_module.h"

#include "sdch_autoauto_handler.h"
#include "sdch_config.h"
#include "sdch_dump_handler.h"
#include "sdch_encoding_handler.h"
#include "sdch_main_config.h"
#include "sdch_output_handler.h"
#include "sdch_pool_alloc.h"
#include "sdch_request_context.h"

namespace sdch {

static closefunc tr_filter_close;
//static void tr_filter_memory(ngx_http_request_t *r, RequestContext *ctx);
static ngx_int_t tr_filter_buffer(RequestContext *ctx, ngx_chain_t *in);
static ngx_int_t tr_filter_out_buf_out(RequestContext *ctx);
static ngx_int_t tr_filter_deflate_start(RequestContext *ctx);
static ngx_int_t tr_filter_add_data(RequestContext *ctx);
static ngx_int_t tr_filter_get_buf(RequestContext *ctx);
static ngx_int_t tr_filter_deflate(RequestContext *ctx);
static ngx_int_t tr_filter_deflate_end(RequestContext *ctx);

#if 0
static void *ngx_http_gzip_filter_alloc(void *opaque, u_int items,
    u_int size);
static void ngx_http_gzip_filter_free(void *opaque, void *address);
#endif
static void ngx_http_gzip_filter_free_copy_buf(ngx_http_request_t *r,
    RequestContext *ctx);

static ngx_int_t tr_add_variables(ngx_conf_t *cf);
static ngx_int_t tr_ratio_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t tr_filter_init(ngx_conf_t *cf);
static void *tr_create_conf(ngx_conf_t *cf);
static char *tr_merge_conf(ngx_conf_t *cf,
    void *parent, void *child);
static void *tr_create_main_conf(ngx_conf_t *cf);
static char *tr_init_main_conf(ngx_conf_t *cf, void *conf);

static char *tr_set_sdch_dict(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static void get_dict_ids(const void *buf, size_t buflen,
	unsigned char user_dictid[9], unsigned char server_dictid[9]);
#if 0
static char *ngx_http_gzip_window(ngx_conf_t *cf, void *post, void *data);
static char *ngx_http_gzip_hash(ngx_conf_t *cf, void *post, void *data);
#endif


#if 0
static ngx_conf_num_bounds_t  ngx_http_gzip_comp_level_bounds = {
    ngx_conf_check_num_bounds, 1, 9
};

static ngx_conf_post_handler_pt  ngx_http_gzip_window_p = ngx_http_gzip_window;
static ngx_conf_post_handler_pt  ngx_http_gzip_hash_p = ngx_http_gzip_hash;
#endif


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

    { ngx_string("sdch_stor_size"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(MainConfig, stor_size),
      &tr_stor_size_bounds },

#if 0
    { ngx_string("gzip_comp_level"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, level),
      &ngx_http_gzip_comp_level_bounds },

#if 0
    { ngx_string("gzip_window"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, wbits),
      &ngx_http_gzip_window_p },
#endif

    { ngx_string("gzip_hash"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, memlevel),
      &ngx_http_gzip_hash_p },

    { ngx_string("postpone_gzipping"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, postpone_gzipping),
      nullptr },

    { ngx_string("gzip_no_buffer"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, no_buffer),
      nullptr },

    { ngx_string("gzip_min_length"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, min_length),
      nullptr },
#endif

      ngx_null_command
};


static ngx_http_module_t  sdch_module_ctx = {
    tr_add_variables,                                  /* preconfiguration */
    tr_filter_init,             /* postconfiguration */

    tr_create_main_conf,                   /* create main configuration */
    tr_init_main_conf,                     /* init main configuration */

    nullptr,                                  /* create server configuration */
    nullptr,                                  /* merge server configuration */

    tr_create_conf,             /* create location configuration */
    tr_merge_conf               /* merge location configuration */
};


//static ngx_str_t  ngx_http_gzip_ratio = ngx_string("gzip_ratio");

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

static ngx_table_elt_t*
header_find(ngx_list_t *headers, const char *key, ngx_str_t *value)
{
	size_t keylen = strlen(key);
	ngx_list_part_t *part = &headers->part;
	ngx_table_elt_t* data = static_cast<ngx_table_elt_t*>(part->elts);
	unsigned i;

	for (i = 0 ;; i++) {

		if (i >= part->nelts) {
			if (part->next == nullptr) {
				break;
			}

			part = part->next;
			data = static_cast<ngx_table_elt_t*>(part->elts);
			i = 0;
		}
		if (data[i].key.len == keylen && ngx_strncasecmp(data[i].key.data, (u_char*)key, keylen) == 0) {
                        if (value) {
                        	*value = data[i].value;
			}
			return &data[i];
		}
	}
	return 0;
}

static ngx_int_t
ngx_http_sdch_ok(ngx_http_request_t *r)
{
    time_t                     date, expires;
    ngx_uint_t                 p;
    ngx_array_t               *cc;
    ngx_table_elt_t           *e, *d;
    Config                 *clcf;

    if (r != r->main) {
        return NGX_DECLINED;
    }

    clcf = Config::get(r);

    if (r->headers_in.via == nullptr) {
        goto ok;
    }

    p = clcf->sdch_proxied;

    if (p & NGX_HTTP_GZIP_PROXIED_OFF) {
        return NGX_DECLINED;
    }

    if (p & NGX_HTTP_GZIP_PROXIED_ANY) {
        goto ok;
    }

    if (r->headers_in.authorization && (p & NGX_HTTP_GZIP_PROXIED_AUTH)) {
        goto ok;
    }

    e = r->headers_out.expires;

    if (e) {

        if (!(p & NGX_HTTP_GZIP_PROXIED_EXPIRED)) {
            return NGX_DECLINED;
        }

        expires = ngx_http_parse_time(e->value.data, e->value.len);
        if (expires == NGX_ERROR) {
            return NGX_DECLINED;
        }

        d = r->headers_out.date;

        if (d) {
            date = ngx_http_parse_time(d->value.data, d->value.len);
            if (date == NGX_ERROR) {
                return NGX_DECLINED;
            }

        } else {
            date = ngx_time();
        }

        if (expires < date) {
            goto ok;
        }

        return NGX_DECLINED;
    }

    cc = &r->headers_out.cache_control;

    if (cc->elts) {

        if ((p & NGX_HTTP_GZIP_PROXIED_NO_CACHE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_no_cache,
                                                 nullptr)
               >= 0)
        {
            goto ok;
        }

        if ((p & NGX_HTTP_GZIP_PROXIED_NO_STORE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_no_store,
                                                 nullptr)
               >= 0)
        {
            goto ok;
        }

        if ((p & NGX_HTTP_GZIP_PROXIED_PRIVATE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_private,
                                                 nullptr)
               >= 0)
        {
            goto ok;
        }

        return NGX_DECLINED;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_LM) && r->headers_out.last_modified) {
        return NGX_DECLINED;
    }

    if ((p & NGX_HTTP_GZIP_PROXIED_NO_ETAG) && r->headers_out.etag) {
        return NGX_DECLINED;
    }

ok:

    //r->gzip_ok = 1;

    return NGX_OK;
}

static sdch_dict_conf *
find_dict(u_char *h, Config *conf)
{
    unsigned int i;
    sdch_dict_conf *dict_conf;

    dict_conf = static_cast<sdch_dict_conf*>(conf->dict_conf_storage->elts);
    for (i = 0; i < conf->dict_conf_storage->nelts; i++) {
        if (ngx_strncmp(h, dict_conf[i].dict->user_dictid, 8) == 0)
            return &dict_conf[i];
    }
    return nullptr;
}

static blob_type
find_quasidict(u_char *h, struct sv **v)
{
    char nm[9];
    nm[8] = 0;
    memcpy(nm, h, 8);

    blob_type b = nullptr;
    stor_find(nm, &b, v);
    return b;
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

static ngx_int_t
x_sdch_encode_0_header(ngx_http_request_t *r, int ins)
{
    ngx_table_elt_t *h = header_find(&r->headers_out.headers,
        "x-sdch-encode", nullptr);
    if (!ins) {
        if (h != nullptr) {
            h->hash = 0;
            h->value.len = 0;
        }
        return NGX_OK;
    }
    if (h == nullptr) {
        h = static_cast<ngx_table_elt_t*>(ngx_list_push(&r->headers_out.headers));
    }
    if (h == nullptr) {
        return NGX_ERROR;
    }

    h->hash = 1;
    ngx_str_set(&h->key, "X-Sdch-Encode");
    ngx_str_set(&h->value, "0");
    
    return NGX_OK;
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

static sdch_dict_conf *
choose_bestdict(sdch_dict_conf *old, sdch_dict_conf *n, u_char *group,
    u_int grl)
{
    if (old == nullptr)
        return n;
    if (n == nullptr)
        return old;

    int om = (ngx_memn2cmp(old->groupname.data, group,
        old->groupname.len, grl) == 0);
    int nm = (ngx_memn2cmp(n->groupname.data, group,
        n->groupname.len, grl) == 0);
    if (om && !nm)
        return old;
    if (nm && !om)
        return n;
    if (n->priority < old->priority)
        return n;
    return old;
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

static ngx_int_t
tr_header_filter(ngx_http_request_t *r)
{
    ngx_table_elt_t       *h;
    RequestContext   *ctx = nullptr;
    Config  *conf;

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http sdch filter header 000");

    conf = Config::get(r);

    ngx_str_t val;
    if (header_find(&r->headers_in.headers, "accept-encoding", &val) == 0 ||
        ngx_strstrn(val.data, const_cast<char*>("sdch"), val.len) == 0) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http sdch filter header: no sdch in accept-encoding");
        return ngx_http_next_header_filter(r);
    }
    if (header_find(&r->headers_in.headers, "avail-dictionary", &val) == 0) {
        ngx_str_set(&val, "");
    }
    int sdch_expected = (val.len > 0);

    if (should_process(r, conf) != NGX_OK)
    {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http sdch filter header: skipping request");
        ngx_int_t e = x_sdch_encode_0_header(r, sdch_expected);
        if (e)
            return e;
        return ngx_http_next_header_filter(r);
    }

    if (r->header_only)
    {
        return ngx_http_next_header_filter(r);
    }
    //r->gzip_vary = 1;

#if (NGX_HTTP_DEGRADATION)
    {
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->gzip_disable_degradation && ngx_http_degraded(r)) {
        return ngx_http_next_header_filter(r);
    }
    }
#endif

    if (ngx_http_sdch_ok(r) != NGX_OK) {
      return ngx_http_next_header_filter(r);
    }

    unsigned int ctxstore = 0;
    if (ngx_strcmp(val.data, "AUTOAUTO") == 0 && conf->enable_quasi) {
        ctxstore = 1;

        ngx_table_elt_t *h = static_cast<ngx_table_elt_t*>(ngx_list_push(&r->headers_out.headers));
        if (h == nullptr) {
            return NGX_ERROR;
        }

        h->hash = 1;
        ngx_str_set(&h->key, "X-Sdch-Use-As-Dictionary");
        ngx_str_set(&h->value, "1");
    }

    ngx_str_t group;
    if (ngx_http_complex_value(r, &conf->sdch_groupcv, &group) != NGX_OK) {
        return NGX_ERROR;
    }
    sdch_dict_conf *bestdict = nullptr;
    blob_type quasidict_blob = nullptr;
    struct sv *ctxstuc = nullptr;
    while (val.len >= 8) {
        sdch_dict_conf *d = find_dict(val.data, conf);
        bestdict = choose_bestdict(bestdict, d, group.data, group.len);
        if (quasidict_blob == nullptr && d == nullptr) {
            quasidict_blob = find_quasidict(val.data, &ctxstuc);
            ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                   "find_quasidict %.8s -> %p", val.data, quasidict_blob);
        }
        val.data += 8; val.len -= 8;
        unsigned l = strspn((char*)val.data, " \t,");
        if (l > val.len)
            l = val.len;
        val.data += l; val.len -= l;
    }
    if (bestdict != nullptr) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
            "group: %s prio: %d best: %d", bestdict->groupname.data,
            bestdict->priority, bestdict->best);
    } else {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
            "nobestdict");
    }
    if (bestdict == nullptr || ngx_memn2cmp(bestdict->groupname.data, group.data, 
            bestdict->groupname.len, group.len) || !bestdict->best) {
    	ngx_int_t e = get_dictionary_header(r, conf);
    	if (e)
    	    return e;
        if (bestdict == nullptr && quasidict_blob == nullptr) {
            e = x_sdch_encode_0_header(r, sdch_expected);
            if (e)
                return e;
            if (ctxstore == 0)
                return ngx_http_next_header_filter(r);
        }
    }

    ctx = pool_alloc<RequestContext>(r, r);
    if (ctx == nullptr) {
        return NGX_ERROR;
    }

    ctx->request = r;
    ctx->buffering = (conf->postpone_gzipping != 0);
    if (bestdict != nullptr) {
        ctx->dict = bestdict->dict;
    } else if (quasidict_blob != nullptr) {
        ctx->dict = &ctx->fdict;
        ctx->fdict.dict = quasidict_blob;
        size_t sz = blob_data_size(quasidict_blob)-8;
        memcpy(ctx->fdict.server_dictid, blob_data_begin(quasidict_blob)+sz, 8);
        get_hashed_dict(blob_data_begin(quasidict_blob), blob_data_begin(quasidict_blob)+sz,
            1, &ctx->fdict.hashed_dict);
    } else {
        ctx->dict = nullptr;
    }
    ctx->store = ctxstore;
    ctx->stuc = ctxstuc;

    if (ctx->dict != nullptr) {
        h = static_cast<ngx_table_elt_t*>(ngx_list_push(&r->headers_out.headers));
        if (h == nullptr) {
            return NGX_ERROR;
        }

        h->hash = 1;
        ngx_str_set(&h->key, "Content-Encoding");
        ngx_str_set(&h->value, "sdch");
        r->headers_out.content_encoding = h;
        ngx_int_t e = x_sdch_encode_0_header(r, 0);
        if (e) {
            return e;
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
    int                   rc;
    ngx_chain_t          *cl;
    RequestContext  *ctx = RequestContext::get(r);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http sdch filter body 000");

    if (ctx == nullptr || ctx->done || r->header_only) {
        return ngx_http_next_body_filter(r, in);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http sdch filter body 001 free=%p busy=%p out=%p", 
                   ctx->free, ctx->busy, ctx->out);

    if (ctx->buffering) {

        /*
         * With default memory settings zlib starts to output gzipped data
         * only after it has got about 90K, so it makes sense to allocate
         * zlib memory (200-400K) only after we have enough data to compress.
         * Although we copy buffers, nevertheless for not big responses
         * this allows to allocate zlib memory, to compress and to output
         * the response in one step using hot CPU cache.
         */

        if (in) {
            switch (tr_filter_buffer(ctx, in)) {

            case NGX_OK:
                return NGX_OK;

            case NGX_DONE:
                in = nullptr;
                break;

            default:  /* NGX_ERROR */
                goto failed;
            }

        } else {
            ctx->buffering = 0;
        }
    }

    if (! ctx->started) {
        if (tr_filter_deflate_start(ctx) != NGX_OK) {
            goto failed;
        }
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http sdch filter started");

    if (in) {
        if (ngx_chain_add_copy(r->pool, &ctx->in, in) != NGX_OK) {
            goto failed;
        }
    }

    if (ctx->nomem) {

        /* flush busy buffers */

        if (ngx_http_next_body_filter(r, nullptr) == NGX_ERROR) {
            goto failed;
        }

        cl = nullptr;

        ngx_chain_update_chains(r->pool, &ctx->free, &ctx->busy, &cl,
                                (ngx_buf_tag_t) &sdch_module);
        ctx->nomem = 0;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http sdch filter mainloop entry");

    for ( ;; ) {

        /* cycle while we can write to a client */

        for ( ;; ) {

            /* cycle while there is data to feed zlib and ... */

            rc = tr_filter_add_data(ctx);

            if (rc == NGX_DECLINED) {
                break;
            }

            if (rc == NGX_AGAIN) {
                continue;
            }


            rc = tr_filter_deflate(ctx);

            if (rc == NGX_OK) {
                break;
            }

            if (rc == NGX_ERROR) {
                goto failed;
            }

            /* rc == NGX_AGAIN */
        }

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http sdch filter loop1 exit");
        if (ctx->out == nullptr) {
            ngx_http_gzip_filter_free_copy_buf(r, ctx);

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http tr filter loop ret1 %p", ctx->busy);
            return ctx->busy ? NGX_AGAIN : NGX_OK;
        }

        rc = ngx_http_next_body_filter(r, ctx->out);

        if (rc == NGX_ERROR) {
            goto failed;
        }

        ngx_http_gzip_filter_free_copy_buf(r, ctx);

        ngx_chain_update_chains(r->pool, &ctx->free, &ctx->busy, &ctx->out,
                                (ngx_buf_tag_t) &sdch_module);
        ctx->last_out = &ctx->out;
        ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http tr filter loop update_chains free=%p busy=%p out=%p", 
                       ctx->free, ctx->busy, ctx->out);

        ctx->nomem = 0;

        if (ctx->done) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http sdch filter done return %d", rc);
            return rc;
        }
    }

    /* unreachable */

failed:

    ctx->done = 1;

#if 0
    if (ctx->preallocated) {
        deflateEnd(&ctx->zstream);

        ngx_pfree(r->pool, ctx->preallocated);
    }
#endif

    ngx_http_gzip_filter_free_copy_buf(r, ctx);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
               "http sdch filter failed return");
    return NGX_ERROR;
}

static ngx_int_t
tr_filter_buffer(RequestContext *ctx, ngx_chain_t *in)
{
    size_t                 size, buffered;
    ngx_buf_t             *b, *buf;
    ngx_chain_t           *cl, **ll;
    ngx_http_request_t    *r;
    Config  *conf;

    r = ctx->request;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "tr_filter_buffer");

    //r->connection->buffered |= NGX_HTTP_GZIP_BUFFERED;

    buffered = 0;
    ll = &ctx->in;

    for (cl = ctx->in; cl; cl = cl->next) {
        buffered += cl->buf->last - cl->buf->pos;
        ll = &cl->next;
    }

    conf = Config::get(r);

    while (in) {
        cl = ngx_alloc_chain_link(r->pool);
        if (cl == nullptr) {
            return NGX_ERROR;
        }

        b = in->buf;

        size = b->last - b->pos;
        buffered += size;

        if (b->flush || b->last_buf || buffered > conf->postpone_gzipping) {
            ctx->buffering = 0;
        }

        if (ctx->buffering && size) {

            buf = ngx_create_temp_buf(r->pool, size);
            if (buf == nullptr) {
                return NGX_ERROR;
            }

            buf->last = ngx_cpymem(buf->pos, b->pos, size);
            b->pos = b->last;

            buf->last_buf = b->last_buf;
            buf->tag = (ngx_buf_tag_t) &sdch_module;

            cl->buf = buf;

        } else {
            cl->buf = b;
        }

        *ll = cl;
        ll = &cl->next;
        in = in->next;
    }

    *ll = nullptr;

    return ctx->buffering ? NGX_OK : NGX_DONE;
}


static ngx_int_t
tr_filter_deflate_start(RequestContext *ctx)
{
    ngx_http_request_t *r = ctx->request;
    Config             *conf = Config::get(r);

    ctx->started = 1;

//INIT
    ctx->last_out = &ctx->out;

    // Last will be OutputHandler.
    ctx->handler = pool_alloc<OutputHandler>(r, ctx, nullptr);
    if (ctx->handler == nullptr)
      return NGX_ERROR;

    if (ctx->dict != nullptr) {
      ctx->handler = pool_alloc<EncodingHandler>(r, ctx, ctx->handler);
      if (ctx->handler == nullptr)
        return NGX_ERROR;
    }
    if (conf->sdch_dumpdir.len > 0) {
      ctx->handler = pool_alloc<DumpHandler>(r, ctx, ctx->handler);
      if (ctx->handler == nullptr)
        return NGX_ERROR;
    }
    if (ctx->store) {
      ctx->handler = pool_alloc<AutoautoHandler>(r, ctx, ctx->handler);
      if (ctx->handler == nullptr)
        return NGX_ERROR;
    }

    return NGX_OK;
}


static ngx_int_t
tr_filter_add_data(RequestContext *ctx)
{
#if (NGX_DEBUG)
    ngx_http_request_t *r = ctx->request;
#endif
    if (ctx->zstream.avail_in || ctx->flush != Z_NO_FLUSH || ctx->redo) {
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch in: %p", ctx->in);

    if (ctx->in == nullptr) {
        return NGX_DECLINED;
    }

    if (ctx->copy_buf) {

        /*
         * to avoid CPU cache trashing we do not free() just quit buf,
         * but postpone free()ing after zlib compressing and data output
         */

        ctx->copy_buf->next = ctx->copied;
        ctx->copied = ctx->copy_buf;
        ctx->copy_buf = nullptr;
    }

    ctx->in_buf = ctx->in->buf;

    if (ctx->in_buf->tag == (ngx_buf_tag_t) &sdch_module) {
        ctx->copy_buf = ctx->in;
    }

    ctx->in = ctx->in->next;

    ctx->zstream.next_in = ctx->in_buf->pos;
    ctx->zstream.avail_in = ctx->in_buf->last - ctx->in_buf->pos;

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch in_buf:%p ni:%p ai:%ud",
                   ctx->in_buf,
                   ctx->zstream.next_in, ctx->zstream.avail_in);

    if (ctx->in_buf->last_buf) {
        ctx->flush = Z_FINISH;

    } else if (ctx->in_buf->flush) {
        ctx->flush = Z_SYNC_FLUSH;
    }

    if (ctx->zstream.avail_in) {

        //ctx->crc32 = crc32(ctx->crc32, ctx->zstream.next_in,
        //                   ctx->zstream.avail_in);

    } else if (ctx->flush == Z_NO_FLUSH) {
        return NGX_AGAIN;
    }

    return NGX_OK;
}


static ngx_int_t
tr_filter_get_buf(RequestContext *ctx)
{
    ngx_http_request_t *r = ctx->request;
    Config  *conf;

    assert (ctx->zstream.avail_out == 0);

    conf = Config::get(r);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "tr_filter_get_buf");

    if (ctx->free) {
        ctx->out_buf = ctx->free->buf;
        ctx->free = ctx->free->next;

    } else if (1 /* ctx->bufs < conf->bufs.num */) {

        ctx->out_buf = ngx_create_temp_buf(r->pool, conf->bufs.size);
        if (ctx->out_buf == nullptr) {
            return NGX_ERROR;
        }

        ctx->out_buf->tag = (ngx_buf_tag_t) &sdch_module;
        ctx->out_buf->recycled = 1;
        ctx->bufs++;

    } else {
        ctx->nomem = 1;
        return NGX_DECLINED;
    }

    ctx->zstream.next_out = ctx->out_buf->pos;
    ctx->zstream.avail_out = conf->bufs.size;

    return NGX_OK;
}

static ngx_int_t
tr_filter_out_buf_out(RequestContext *ctx)
{
    ctx->out_buf->last = ctx->zstream.next_out;
    assert(ctx->out_buf->last != ctx->out_buf->pos);
    ngx_chain_t *cl = ngx_alloc_chain_link(ctx->request->pool);
    if (cl == nullptr) {
        return NGX_ERROR;
    }

    cl->buf = ctx->out_buf;
    cl->next = nullptr;
    *ctx->last_out = cl;
    ctx->last_out = &cl->next;
    
    ctx->out_buf = nullptr;
    ctx->zstream.avail_out = 0;

    return NGX_OK;
}

void
tr_filter_close(void *c)
{
}

ssize_t
tr_filter_write(RequestContext *ctx, const char *buf, size_t len)
{
  int rlen = 0;

  while (len > 0) {
    unsigned l0 = len;
    if (ctx->zstream.avail_out < l0)
      l0 = ctx->zstream.avail_out;
    if (l0 > 0) {
      memcpy(ctx->zstream.next_out, buf, l0);
      len -= l0;
      buf += l0;
      ctx->zstream.avail_out -= l0;
      ctx->zstream.next_out += l0;
      rlen += l0;
    }
    if (len > 0) {
      if (ctx->out_buf) {
        int rc = tr_filter_out_buf_out(ctx);
        if (rc != NGX_OK)
          return rc;
      }

      tr_filter_get_buf(ctx);
    }
  }
  ctx->zout += rlen;
  return rlen;
}

static ngx_int_t
tr_filter_deflate(RequestContext *ctx)
{
    ngx_http_request_t *r = ctx->request;
    int                    rc;
    ngx_buf_t             *b;
    ngx_chain_t           *cl;
    Config  *conf;

    ngx_log_debug6(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "deflate in: ni:%p no:%p ai:%ud ao:%ud fl:%d redo:%d",
                 ctx->zstream.next_in, ctx->zstream.next_out,
                 ctx->zstream.avail_in, ctx->zstream.avail_out,
                 ctx->flush, ctx->redo);

    int l0 = ctx->handler->on_data(reinterpret_cast<char*>(ctx->zstream.next_in),
                                   ctx->zstream.avail_in);
    ctx->zstream.next_in += l0;
    ctx->zstream.avail_in -= l0;
    ctx->zin += l0;
    rc = (ctx->flush == Z_FINISH) ? Z_STREAM_END : Z_OK;

    if (rc != Z_OK && rc != Z_STREAM_END && rc != Z_BUF_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      "deflate() failed: %d, %d", ctx->flush, rc);
        return NGX_ERROR;
    }

    ngx_log_debug5(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "deflate out: ni:%p no:%p ai:%ud ao:%ud rc:%d",
                   ctx->zstream.next_in, ctx->zstream.next_out,
                   ctx->zstream.avail_in, ctx->zstream.avail_out,
                   rc);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "gzip in_buf:%p pos:%p",
                   ctx->in_buf, ctx->in_buf->pos);

    if (ctx->zstream.next_in) {
        ctx->in_buf->pos = ctx->zstream.next_in;

        if (ctx->zstream.avail_in == 0) {
            ctx->zstream.next_in = nullptr;
        }
    }

    ctx->out_buf->last = ctx->zstream.next_out;

    ctx->redo = 0;

    if (ctx->flush == Z_SYNC_FLUSH) {

        ctx->flush = Z_NO_FLUSH;

        cl = ngx_alloc_chain_link(r->pool);
        if (cl == nullptr) {
            return NGX_ERROR;
        }

        b = ctx->out_buf;

        if (ngx_buf_size(b) == 0) {

            b = static_cast<ngx_buf_t*>(ngx_calloc_buf(ctx->request->pool));
            if (b == nullptr) {
                return NGX_ERROR;
            }

        } else {
            ctx->zstream.avail_out = 0;
        }

        b->flush = 1;

        cl->buf = b;
        cl->next = nullptr;
        *ctx->last_out = cl;
        ctx->last_out = &cl->next;

        return NGX_OK;
    }

    if (rc == Z_STREAM_END) {

        if (tr_filter_deflate_end(ctx) != NGX_OK) {
            return NGX_ERROR;
        }

        return NGX_OK;
    }

    conf = Config::get(r);

    if (conf->no_buffer && ctx->in == nullptr) {
        return tr_filter_out_buf_out(ctx);
    }

    return NGX_AGAIN;
}


static ngx_int_t
tr_filter_deflate_end(RequestContext *ctx)
{

    ngx_log_error(NGX_LOG_ALERT, ctx->request->connection->log, 0,
        "closing ctx");
    ctx->handler->on_finish();
#if 0
    FIXME It should go into corresponding Handler::on_finish implementation
    if (ctx->store && !ctx->blob) {
        ngx_log_error(NGX_LOG_ERR, ctx->request->connection->log, 0,
            "storing quasidict: no blob");
    }
    if (ctx->store && ctx->blob) {
        unsigned char user_dictid[9];
        unsigned char server_dictid[9];
        get_dict_ids(blob_data_begin(ctx->blob), blob_data_size(ctx->blob),
            user_dictid, server_dictid);
        if (blob_append(ctx->blob, user_dictid, 8) != 0) {
            blob_destroy(ctx->blob);
        } else {
            stor_store((const char *)user_dictid, time(0), ctx->blob); // XXX
            ngx_log_error(NGX_LOG_ERR, ctx->request->connection->log, 0,
                "storing quasidict %s (%p)", user_dictid, ctx->blob);
        }
    }
    if (ctx->stuc) {
        stor_unlock(ctx->stuc);
    }
#endif

    //ngx_pfree(r->pool, ctx->preallocated);

    ctx->out_buf->last_buf = 1;
    ngx_int_t rc = tr_filter_out_buf_out(ctx);
    if (rc != NGX_OK)
        return rc;

    ctx->zstream.avail_in = 0;
    ctx->zstream.avail_out = 0;

    ctx->done = 1;

    //r->connection->buffered &= ~NGX_HTTP_GZIP_BUFFERED;

    return NGX_OK;
}


static void
ngx_http_gzip_filter_free_copy_buf(ngx_http_request_t *r, RequestContext *ctx)
{
    ngx_chain_t  *cl;

    for (cl = ctx->copied; cl; cl = cl->next) {
        ngx_pfree(r->pool, cl->buf->start);
    }

    ctx->copied = nullptr;
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


    if (ctx == nullptr || ctx->zout == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = static_cast<u_char*>(ngx_pnalloc(r->pool, NGX_INT32_LEN + 3));
    if (v->data == nullptr) {
        return NGX_ERROR;
    }

    zint = (ngx_uint_t) (ctx->zin / ctx->zout);
    zfrac = (ngx_uint_t) ((ctx->zin * 100 / ctx->zout) % 100);

    if ((ctx->zin * 1000 / ctx->zout) % 10 > 4) {

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
        max_stor_size = conf->stor_size;
    return NGX_CONF_OK;
}

static void *
tr_create_conf(ngx_conf_t *cf)
{
  return pool_alloc<Config>(cf);
}

static const char *
init_dict_data(ngx_conf_t *cf, ngx_str_t *dict, struct sdch_dict *data)
{
    blob_create(&data->dict);
    const char * p = read_file((const char*)dict->data, data->dict);
    if (p != nullptr)
        return p;
    if (get_hashed_dict(blob_data_begin(data->dict),
            blob_data_begin(data->dict)+blob_data_size(data->dict),
            0, &data->hashed_dict)) {
    	ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "get_hashed_dict %s failed", dict->data);
    	return "Get hashed dict failed";
    }
    get_dict_ids(blob_data_begin(data->dict), blob_data_size(data->dict),
    		data->user_dictid, data->server_dictid);
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "dictionary ids: user %s server %s",
    		data->user_dictid, data->server_dictid);
    return NGX_CONF_OK;
}

static char *
tr_set_sdch_dict(ngx_conf_t *cf, ngx_command_t *cmd, void *cnf)
{
    Config *conf = static_cast<Config*>(cnf);

    if (cf->args->nelts < 2 || cf->args->nelts > 4) {
        return const_cast<char*>("Wrong number of arguments");
    }
    if (conf->dict_storage == nullptr) {
        conf->dict_storage = ngx_array_create(cf->pool, 2, sizeof(struct sdch_dict));
    }
    if (conf->dict_conf_storage == nullptr) {
        conf->dict_conf_storage = ngx_array_create(cf->pool, 2, sizeof(sdch_dict_conf));
    }
    ngx_str_t *value = static_cast<ngx_str_t*>(cf->args->elts);
    ngx_str_t groupname;
    ngx_str_set(&groupname, "default");
    if (cf->args->nelts >= 3) {
        groupname = value[2];
    }
    ngx_int_t prio = conf->confdictnum++;
    if (cf->args->nelts >= 4) {
        prio = ngx_atoi(value[3].data, value[3].len);
        if (prio == NGX_ERROR) {
            return const_cast<char*>("Can't convert to number");
        }
    }
    struct sdch_dict *data = static_cast<sdch_dict*>(ngx_array_push(conf->dict_storage));
    const char *p = init_dict_data(cf, &value[1], data);
    if (p != nullptr) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s", p);
        return const_cast<char*>(p);
    }
    sdch_dict_conf *sdc = static_cast<sdch_dict_conf*>(ngx_array_push(conf->dict_conf_storage));
    sdc->groupname.len = groupname.len++;
    sdc->groupname.data = ngx_pstrdup(cf->pool, &groupname);
    sdc->priority = prio;
    sdc->dict = data;
    sdc->best = 0;

    return NGX_CONF_OK;
}

int
compare_dict_conf(const void *a, const void *b)
{
    const sdch_dict_conf *A = static_cast<const sdch_dict_conf*>(a);
    const sdch_dict_conf *B = static_cast<const sdch_dict_conf*>(b);
    ngx_int_t c = ngx_strcmp(A->groupname.data, B->groupname.data);
    if (c != 0)
        return c;
    if (A->priority < B->priority)
        return -1;
    if (A->priority > B->priority)
        return 1;
    return 0;
}

static char *
tr_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    Config *prev = static_cast<Config*>(parent);
    Config *conf = static_cast<Config*>(child);
    ngx_http_compile_complex_value_t ccv;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_bufs_value(conf->bufs, prev->bufs,
                              (128 * 1024) / ngx_pagesize, ngx_pagesize);

    ngx_conf_merge_str_value(conf->sdch_disablecv_s, prev->sdch_disablecv_s, "");
    ngx_memzero(&ccv, sizeof(ccv));
    ccv.cf = cf;
    ccv.value = &conf->sdch_disablecv_s;
    ccv.complex_value = &conf->sdch_disablecv;
    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return const_cast<char*>("ngx_http_compile_complex_value sdch_disablecv failed");
    }

#if 0
    ngx_conf_merge_value(conf->no_buffer, prev->no_buffer, 0);


    ngx_conf_merge_size_value(conf->postpone_gzipping, prev->postpone_gzipping,
                              0);
    ngx_conf_merge_value(conf->level, prev->level, 1);
    ngx_conf_merge_size_value(conf->wbits, prev->wbits, MAX_WBITS);
    ngx_conf_merge_size_value(conf->memlevel, prev->memlevel,
                              MAX_MEM_LEVEL - 1);
    ngx_conf_merge_value(conf->min_length, prev->min_length, 20);
#endif

    if (ngx_http_merge_types(cf, &conf->types_keys, &conf->types,
                             &prev->types_keys, &prev->types,
                             ngx_http_html_default_types)
        != NGX_OK)
    {
        return const_cast<char*>("Can't merge config");
    }

    if (!conf->enable)
    	return NGX_CONF_OK;

    if (conf->dict_conf_storage == nullptr) {
        conf->dict_conf_storage = prev->dict_conf_storage;
        conf->dict_storage = prev->dict_storage;
    }
    if (conf->dict_conf_storage != nullptr) {
        sdch_dict_conf *dse = static_cast<sdch_dict_conf*>(conf->dict_conf_storage->elts);
        qsort(conf->dict_conf_storage->elts, conf->dict_conf_storage->nelts,
            sizeof(sdch_dict_conf), compare_dict_conf);
        if (conf->dict_conf_storage->nelts > 0) {
            dse[0].best = 1;
        }
        unsigned int i;
        for (i = 1; i < conf->dict_conf_storage->nelts; i++) {
            if (ngx_strcmp(dse[i-1].groupname.data, dse[i].groupname.data)) {
                dse[i].best = 1;
            }
        }
    }

    if (conf->dict_storage == nullptr) {
        conf->dict_storage = ngx_array_create(cf->pool, 2, sizeof(struct sdch_dict));
    }
    if (conf->dict_conf_storage == nullptr) {
        conf->dict_conf_storage = ngx_array_create(cf->pool, 2, sizeof(sdch_dict_conf));
    }

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

#if nginx_version < 1006000
static void
ngx_encode_base64url(ngx_str_t *dst, ngx_str_t *src)
{
	ngx_encode_base64(dst, src);
	unsigned i;

	for (i = 0; i < dst->len; i++) {
		if (dst->data[i] == '+')
			dst->data[i] = '-';
		if (dst->data[i] == '/')
			dst->data[i] = '_';
	}
}
#endif

#include <openssl/sha.h>

static void
get_dict_ids(const void *buf, size_t buflen, unsigned char user_dictid[9], unsigned char server_dictid[9])
{
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, buf, buflen);
	unsigned char sha[32];
	SHA256_Final(sha, &ctx);

	ngx_str_t src = {6, sha};
	ngx_str_t dst = {8, user_dictid};
	ngx_encode_base64url(&dst, &src);

	src.data = sha+6;
	dst.len = 8; dst.data = server_dictid;
	ngx_encode_base64url(&dst, &src);

	user_dictid[8] = 0;
	server_dictid[8] = 0;
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

