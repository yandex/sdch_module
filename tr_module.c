
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "azlib.h"
#include "pzlib.h"
#include "vcd-h1.h"

typedef struct {
    ngx_flag_t           enable;
    ngx_flag_t           no_buffer;

    ngx_hash_t           types;

    ngx_bufs_t           bufs;

    size_t               postpone_gzipping;
    ngx_int_t            level;
    size_t               memlevel;
    ssize_t              min_length;

    ngx_array_t         *types_keys;
    
    ngx_str_t            dict;
    hashed_dictionary_p  hashed_dict;
    unsigned char user_dictid[9], server_dictid[9];

    ngx_str_t            sdch_url;

    ngx_uint_t           sdch_proxied;
} tr_conf_t;

typedef struct {
    ngx_chain_t         *in;
    ngx_chain_t         *free;
    ngx_chain_t         *busy;
    ngx_chain_t         *out;
    ngx_chain_t        **last_out;

    ngx_chain_t         *copied;
    ngx_chain_t         *copy_buf;

    ngx_buf_t           *in_buf;
    ngx_buf_t           *out_buf;
    ngx_int_t            bufs;


    unsigned             started:1;
    unsigned             flush:4;
    unsigned             redo:1;
    unsigned             done:1;
    unsigned             nomem:1;
    unsigned             buffering:1;

    size_t               zin;
    size_t               zout;

    //uint32_t             crc32;
    z_stream             zstream;
    ngx_http_request_t  *request;
    
    //void		*tr1cookie;
    vcd_encoder_p        enc;
} tr_ctx_t;


static pssize_type tr_filter_write(void *ctx0, const void *buf, psize_type len);
static void tr_filter_memory(ngx_http_request_t *r,
    tr_ctx_t *ctx);
static ngx_int_t tr_filter_buffer(tr_ctx_t *ctx,
    ngx_chain_t *in);
static ngx_int_t tr_filter_deflate_start(tr_ctx_t *ctx);
static ngx_int_t tr_filter_add_data(tr_ctx_t *ctx);
static ngx_int_t tr_filter_get_buf(tr_ctx_t *ctx);
static ngx_int_t tr_filter_deflate(tr_ctx_t *ctx);
static ngx_int_t tr_filter_deflate_end(tr_ctx_t *ctx);

#if 0
static void *ngx_http_gzip_filter_alloc(void *opaque, u_int items,
    u_int size);
static void ngx_http_gzip_filter_free(void *opaque, void *address);
#endif
static void ngx_http_gzip_filter_free_copy_buf(ngx_http_request_t *r,
    tr_ctx_t *ctx);

static ngx_int_t tr_filter_init(ngx_conf_t *cf);
static void *tr_create_conf(ngx_conf_t *cf);
static char *tr_merge_conf(ngx_conf_t *cf,
    void *parent, void *child);

static void get_dict_ids(const char *buf, size_t buflen,
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



static ngx_command_t  tr_filter_commands[] = {

    { ngx_string("sdch"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(tr_conf_t, enable),
      NULL },

    { ngx_string("sdch_buffers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_conf_set_bufs_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(tr_conf_t, bufs),
      NULL },

    { ngx_string("sdch_dict"),
   	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_conf_set_str_slot,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(tr_conf_t, dict),
	  NULL },

    { ngx_string("sdch_url"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(tr_conf_t, sdch_url),
      NULL },

    { ngx_string("sdch_proxied"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(tr_conf_t, sdch_proxied),
      &ngx_http_sdch_proxied_mask },


#if 0
    { ngx_string("gzip_types"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_types_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, types_keys),
      &ngx_http_html_default_types[0] },

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
      NULL },

    { ngx_string("gzip_no_buffer"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, no_buffer),
      NULL },

    { ngx_string("gzip_min_length"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_gzip_conf_t, min_length),
      NULL },
#endif

      ngx_null_command
};


static ngx_http_module_t  tr_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    tr_filter_init,             /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    tr_create_conf,             /* create location configuration */
    tr_merge_conf               /* merge location configuration */
};


ngx_module_t  tr_filter_module = {
    NGX_MODULE_V1,
    &tr_filter_module_ctx,      /* module context */
    tr_filter_commands,         /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

//static ngx_str_t  ngx_http_gzip_ratio = ngx_string("gzip_ratio");

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

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

static int
header_find(ngx_list_t *headers, char *key, ngx_str_t *value)
{
	size_t keylen = strlen(key);
	ngx_list_part_t *part = &headers->part;
	ngx_table_elt_t* data = part->elts;
	unsigned i;

	for (i = 0 ;; i++) {

		if (i >= part->nelts) {
			if (part->next == NULL) {
				break;
			}

			part = part->next;
			data = part->elts;
			i = 0;
		}
		if (data[i].key.len == keylen && ngx_strncasecmp(data[i].key.data, key, keylen) == 0) {
			*value = data[i].value;
			return 1;
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
    ngx_table_elt_t           *e, *d, *ae;
    tr_conf_t                 *clcf;

//    r->gzip_tested = 1;

    if (r != r->main) {
        return NGX_DECLINED;
    }

#if (NGX_HTTP_SPDY)
    //if (r->spdy_stream) {
    //    r->gzip_ok = 1;
    //    return NGX_OK;
    //}
#endif

    clcf = ngx_http_get_module_loc_conf(r, tr_filter_module);

    if (r->headers_in.via == NULL) {
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
                                                 NULL)
               >= 0)
        {
            goto ok;
        }

        if ((p & NGX_HTTP_GZIP_PROXIED_NO_STORE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_no_store,
                                                 NULL)
               >= 0)
        {
            goto ok;
        }

        if ((p & NGX_HTTP_GZIP_PROXIED_PRIVATE)
            && ngx_http_parse_multi_header_lines(cc, &ngx_http_gzip_private,
                                                 NULL)
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

static ngx_int_t
tr_header_filter(ngx_http_request_t *r)
{
    ngx_table_elt_t       *h;
    tr_ctx_t   *ctx;
    tr_conf_t  *conf;

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                   "http tr filter header 000");

    conf = ngx_http_get_module_loc_conf(r, tr_filter_module);

    if (!conf->enable
        || (r->headers_out.status != NGX_HTTP_OK
            && r->headers_out.status != NGX_HTTP_FORBIDDEN
            && r->headers_out.status != NGX_HTTP_NOT_FOUND)
        || (r->headers_out.content_encoding
            && r->headers_out.content_encoding->value.len)
        || (r->headers_out.content_length_n != -1
            && r->headers_out.content_length_n < conf->min_length)
        || ngx_http_test_content_type(r, &conf->types) == NULL
        || r->header_only)
    {
        int rf = ngx_http_next_header_filter(r);
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
            "tr filter header p1 %d", rf);
        backtrace_log(r->connection->log);
        return rf;
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

#if 0
    if (!r->gzip_tested) {
        if (ngx_http_gzip_ok(r) != NGX_OK) {
            return ngx_http_next_header_filter(r);
        }

    } else if (!r->gzip_ok) {
        return ngx_http_next_header_filter(r);
    }
#else
    if (ngx_http_sdch_ok(r) != NGX_OK) {
    	return ngx_http_next_header_filter(r);
    }

    ngx_str_t val;
    if (header_find(&r->headers_in.headers, "accept-encoding", &val) == 0 ||
    	ngx_strstrn(val.data, "sdch", val.len) == 0)
    	return ngx_http_next_header_filter(r);
    if (
    	header_find(&r->headers_in.headers, "avail-dictionary", &val) == 0 ||
    	val.len != 8 || strncmp(val.data, conf->user_dictid, 8) != 0
    	) {
        h = ngx_list_push(&r->headers_out.headers);
        if (h == NULL) {
            return NGX_ERROR;
        }

        h->hash = 1;
        ngx_str_set(&h->key, "Get-Dictionary");
        h->value = conf->sdch_url;
    	return ngx_http_next_header_filter(r);
    }
#endif

    ctx = ngx_pcalloc(r->pool, sizeof(tr_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_http_set_ctx(r, ctx, tr_filter_module);

    ctx->request = r;
    ctx->buffering = (conf->postpone_gzipping != 0);

    tr_filter_memory(r, ctx);

#if 1
    h = ngx_list_push(&r->headers_out.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->hash = 1;
    ngx_str_set(&h->key, "Content-Encoding");
    ngx_str_set(&h->value, "sdch");
    r->headers_out.content_encoding = h;
#endif

    r->main_filter_need_in_memory = 1;

    ngx_http_clear_content_length(r);
    ngx_http_clear_accept_ranges(r);

//   TODO(wawa): adverse impact should be verified if any
//   ngx_http_clear_etag(r);

    return ngx_http_next_header_filter(r);
}


static ngx_int_t
tr_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    int                   rc;
    ngx_chain_t          *cl;
    tr_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, tr_filter_module);

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                   "http tr filter body 000");

    if (ctx == NULL || ctx->done || r->header_only) {
        return ngx_http_next_body_filter(r, in);
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http tr filter");

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
                in = NULL;
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
                   "http tr filter started");

    if (in) {
        if (ngx_chain_add_copy(r->pool, &ctx->in, in) != NGX_OK) {
            goto failed;
        }
    }

    if (ctx->nomem) {

        /* flush busy buffers */

        if (ngx_http_next_body_filter(r, NULL) == NGX_ERROR) {
            goto failed;
        }

        cl = NULL;

        ngx_chain_update_chains(r->pool, &ctx->free, &ctx->busy, &cl,
                                (ngx_buf_tag_t) &tr_filter_module);
        ctx->nomem = 0;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http tr filter mainloop entry");

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


            /* ... there are buffers to write zlib output */

            rc = tr_filter_get_buf(ctx);

            if (rc == NGX_DECLINED) {
                break;
            }

            if (rc == NGX_ERROR) {
                goto failed;
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
                       "http tr filter loop1 exit");
        if (ctx->out == NULL) {
            ngx_http_gzip_filter_free_copy_buf(r, ctx);

            return ctx->busy ? NGX_AGAIN : NGX_OK;
        }

        rc = ngx_http_next_body_filter(r, ctx->out);

        if (rc == NGX_ERROR) {
            goto failed;
        }

        ngx_http_gzip_filter_free_copy_buf(r, ctx);

        ngx_chain_update_chains(r->pool, &ctx->free, &ctx->busy, &ctx->out,
                                (ngx_buf_tag_t) &tr_filter_module);
        ctx->last_out = &ctx->out;

        ctx->nomem = 0;

        if (ctx->done) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http tr filter done return %d", rc);
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
               "http tr filter failed return");
    return NGX_ERROR;
}


static void
tr_filter_memory(ngx_http_request_t *r, tr_ctx_t *ctx)
{
    tr_conf_t  *conf;

    conf = ngx_http_get_module_loc_conf(r, tr_filter_module);
}


static ngx_int_t
tr_filter_buffer(tr_ctx_t *ctx, ngx_chain_t *in)
{
    size_t                 size, buffered;
    ngx_buf_t             *b, *buf;
    ngx_chain_t           *cl, **ll;
    ngx_http_request_t    *r;
    tr_conf_t  *conf;

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

    conf = ngx_http_get_module_loc_conf(r, tr_filter_module);

    while (in) {
        cl = ngx_alloc_chain_link(r->pool);
        if (cl == NULL) {
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
            if (buf == NULL) {
                return NGX_ERROR;
            }

            buf->last = ngx_cpymem(buf->pos, b->pos, size);
            b->pos = b->last;

            buf->last_buf = b->last_buf;
            buf->tag = (ngx_buf_tag_t) &tr_filter_module;

            cl->buf = buf;

        } else {
            cl->buf = b;
        }

        *ll = cl;
        ll = &cl->next;
        in = in->next;
    }

    *ll = NULL;

    return ctx->buffering ? NGX_OK : NGX_DONE;
}


static ngx_int_t
tr_filter_deflate_start(tr_ctx_t *ctx)
{
    ngx_http_request_t *r = ctx->request;
    //int                    rc;
    tr_conf_t  *conf;

    conf = ngx_http_get_module_loc_conf(r, tr_filter_module);

    ctx->started = 1;

//INIT
     //aDeflateInit(&ctx->zstream);
     //ctx->tr1cookie = make_tr1(tr_filter_write, ctx);
    tr_filter_write(ctx, conf->server_dictid, 9);
    get_vcd_encoder(conf->hashed_dict, tr_filter_write, ctx, &ctx->enc);
#if 0
    if (rc != Z_OK) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      "deflateInit2() failed: %d", rc);
        return NGX_ERROR;
    }
#endif

//    r->connection->buffered |= NGX_HTTP_GZIP_BUFFERED;

    ctx->last_out = &ctx->out;
    //ctx->crc32 = crc32(0L, Z_NULL, 0);
    //ctx->flush = Z_NO_FLUSH;

    return NGX_OK;
}


static ngx_int_t
tr_filter_add_data(tr_ctx_t *ctx)
{
    ngx_http_request_t *r = ctx->request;
    if (ctx->zstream.avail_in || ctx->flush != Z_NO_FLUSH || ctx->redo) {
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "tr in: %p", ctx->in);

    if (ctx->in == NULL) {
        return NGX_DECLINED;
    }

    if (ctx->copy_buf) {

        /*
         * to avoid CPU cache trashing we do not free() just quit buf,
         * but postpone free()ing after zlib compressing and data output
         */

        ctx->copy_buf->next = ctx->copied;
        ctx->copied = ctx->copy_buf;
        ctx->copy_buf = NULL;
    }

    ctx->in_buf = ctx->in->buf;

    if (ctx->in_buf->tag == (ngx_buf_tag_t) &tr_filter_module) {
        ctx->copy_buf = ctx->in;
    }

    ctx->in = ctx->in->next;

    ctx->zstream.next_in = ctx->in_buf->pos;
    ctx->zstream.avail_in = ctx->in_buf->last - ctx->in_buf->pos;

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "tr in_buf:%p ni:%p ai:%ud",
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
tr_filter_get_buf(tr_ctx_t *ctx)
{
    ngx_http_request_t *r = ctx->request;
    tr_conf_t  *conf;

    if (ctx->zstream.avail_out) {
        return NGX_OK;
    }

    conf = ngx_http_get_module_loc_conf(r, tr_filter_module);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "tr_filter_get_buf");

    if (ctx->free) {
        ctx->out_buf = ctx->free->buf;
        ctx->free = ctx->free->next;

    } else if (ctx->bufs < conf->bufs.num) {

        ctx->out_buf = ngx_create_temp_buf(r->pool, conf->bufs.size);
        if (ctx->out_buf == NULL) {
            return NGX_ERROR;
        }

        ctx->out_buf->tag = (ngx_buf_tag_t) &tr_filter_module;
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

pssize_type
tr_filter_write(void *ctx0, const void *buf, psize_type len)
{
    tr_ctx_t *ctx = ctx0;
    const char *buff = buf;
    int rlen = 0;
    
    while (len > 0) {
        unsigned l0 = len;
        if (ctx->zstream.avail_out < l0)
            l0 = ctx->zstream.avail_out;
        if (l0 > 0) {
            memcpy(ctx->zstream.next_out, buff, l0);
            len -= l0;
            ctx->zstream.avail_out -= l0;
            ctx->zstream.next_out += l0;
            rlen += l0;
        }
        if (len > 0)
            tr_filter_get_buf(ctx);
    }
    return rlen;
}

static ngx_int_t
tr_filter_deflate(tr_ctx_t *ctx)
{
    ngx_http_request_t *r = ctx->request;
    int                    rc;
    ngx_buf_t             *b;
    ngx_chain_t           *cl;
    tr_conf_t  *conf;

    ngx_log_debug6(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "deflate in: ni:%p no:%p ai:%ud ao:%ud fl:%d redo:%d",
                 ctx->zstream.next_in, ctx->zstream.next_out,
                 ctx->zstream.avail_in, ctx->zstream.avail_out,
                 ctx->flush, ctx->redo);

    //rc = aDeflate(&ctx->zstream, ctx->flush);
    //int l0 = do_tr1(ctx->tr1cookie, ctx->zstream.next_in, ctx->zstream.avail_in);
    int l0 = vcdwriter(ctx->enc, ctx->zstream.next_in, ctx->zstream.avail_in);
    ctx->zstream.next_in += l0;
    ctx->zstream.avail_in -= l0;
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
            ctx->zstream.next_in = NULL;
        }
    }

    ctx->out_buf->last = ctx->zstream.next_out;

    if (ctx->zstream.avail_out == 0) {

        /* zlib wants to output some more gzipped data */

        cl = ngx_alloc_chain_link(r->pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        cl->buf = ctx->out_buf;
        cl->next = NULL;
        *ctx->last_out = cl;
        ctx->last_out = &cl->next;

        ctx->redo = 1;

        return NGX_AGAIN;
    }

    ctx->redo = 0;

    if (ctx->flush == Z_SYNC_FLUSH) {

        ctx->flush = Z_NO_FLUSH;

        cl = ngx_alloc_chain_link(r->pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        b = ctx->out_buf;

        if (ngx_buf_size(b) == 0) {

            b = ngx_calloc_buf(ctx->request->pool);
            if (b == NULL) {
                return NGX_ERROR;
            }

        } else {
            ctx->zstream.avail_out = 0;
        }

        b->flush = 1;

        cl->buf = b;
        cl->next = NULL;
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

    conf = ngx_http_get_module_loc_conf(r, tr_filter_module);

    if (conf->no_buffer && ctx->in == NULL) {

        cl = ngx_alloc_chain_link(r->pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        cl->buf = ctx->out_buf;
        cl->next = NULL;
        *ctx->last_out = cl;
        ctx->last_out = &cl->next;

        return NGX_OK;
    }

    return NGX_AGAIN;
}


static ngx_int_t
tr_filter_deflate_end(tr_ctx_t *ctx)
{
    ngx_http_request_t *r = ctx->request;
    //int                rc;
    //ngx_buf_t         *b;
    ngx_chain_t       *cl;
    //struct gztrailer  *trailer;

    ctx->zin = ctx->zstream.total_in;
    ctx->zout = ctx->zstream.total_out;

#if 0
    rc = aDeflateEnd(&ctx->zstream);

    if (rc != Z_OK) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      "deflateEnd() failed: %d", rc);
        return NGX_ERROR;
    }
#else
    //free_tr1(ctx->tr1cookie);
    vcdclose(ctx->enc);
#endif

    //ngx_pfree(r->pool, ctx->preallocated);

    cl = ngx_alloc_chain_link(r->pool);
    if (cl == NULL) {
        return NGX_ERROR;
    }

    cl->buf = ctx->out_buf;
    cl->next = NULL;
    *ctx->last_out = cl;
    ctx->last_out = &cl->next;
    
    ctx->out_buf->last_buf = 1;

    ctx->zstream.avail_in = 0;
    ctx->zstream.avail_out = 0;

    ctx->done = 1;

    //r->connection->buffered &= ~NGX_HTTP_GZIP_BUFFERED;

    return NGX_OK;
}


static void
ngx_http_gzip_filter_free_copy_buf(ngx_http_request_t *r,
    tr_ctx_t *ctx)
{
    ngx_chain_t  *cl;

    for (cl = ctx->copied; cl; cl = cl->next) {
        ngx_pfree(r->pool, cl->buf->start);
    }

    ctx->copied = NULL;
}


#if 0
static ngx_int_t
ngx_http_gzip_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &ngx_http_gzip_ratio, NGX_HTTP_VAR_NOHASH);
    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_gzip_ratio_variable;

    return NGX_OK;
}


static ngx_int_t
ngx_http_gzip_ratio_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t            zint, zfrac;
    ngx_http_gzip_ctx_t  *ctx;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    ctx = ngx_http_get_module_ctx(r, ngx_http_gzip_filter_module);

    if (ctx == NULL || ctx->zout == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = ngx_pnalloc(r->pool, NGX_INT32_LEN + 3);
    if (v->data == NULL) {
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
#endif


static void *
tr_create_conf(ngx_conf_t *cf)
{
    tr_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(tr_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->bufs.num = 0;
     *     conf->types = { NULL };
     *     conf->types_keys = NULL;
     */

    conf->enable = NGX_CONF_UNSET;
    
    ngx_str_null(&conf->dict);
    conf->hashed_dict = NULL;
#if 0
    conf->no_buffer = NGX_CONF_UNSET;

    conf->postpone_gzipping = NGX_CONF_UNSET_SIZE;
    conf->level = NGX_CONF_UNSET;
    conf->wbits = NGX_CONF_UNSET_SIZE;
    conf->memlevel = NGX_CONF_UNSET_SIZE;
    conf->min_length = NGX_CONF_UNSET;
#endif

    return conf;
}


static char *
tr_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    tr_conf_t *prev = parent;
    tr_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_bufs_value(conf->bufs, prev->bufs,
                              (128 * 1024) / ngx_pagesize, ngx_pagesize);
#if 0
    ngx_conf_merge_value(conf->no_buffer, prev->no_buffer, 0);


    ngx_conf_merge_size_value(conf->postpone_gzipping, prev->postpone_gzipping,
                              0);
    ngx_conf_merge_value(conf->level, prev->level, 1);
    ngx_conf_merge_size_value(conf->wbits, prev->wbits, MAX_WBITS);
    ngx_conf_merge_size_value(conf->memlevel, prev->memlevel,
                              MAX_MEM_LEVEL - 1);
    ngx_conf_merge_value(conf->min_length, prev->min_length, 20);

    if (ngx_http_merge_types(cf, &conf->types_keys, &conf->types,
                             &prev->types_keys, &prev->types,
                             ngx_http_html_default_types)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }
#endif

    if (!conf->enable)
    	return NGX_CONF_OK;

    ngx_conf_merge_str_value(conf->dict, prev->dict, "");
    if (get_hashed_dict(conf->dict.data, &conf->hashed_dict)) {
    	ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "get_hashed_dict %s failed", conf->dict.data);
    	return NGX_CONF_ERROR;
    }
    get_dict_ids(get_dictionary_begin(conf->hashed_dict), get_dictionary_size(conf->hashed_dict),
    		conf->user_dictid, conf->server_dictid);
    ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "dictionary ids: user %s server %s",
    		conf->user_dictid, conf->server_dictid);

    return NGX_CONF_OK;
}


static ngx_int_t
tr_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = tr_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = tr_body_filter;

    ngx_conf_log_error(NGX_LOG_INFO, cf, 0,
                   "http tr filter init");
    return NGX_OK;
}

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

#include <openssl/sha.h>

static void
get_dict_ids(const char *buf, size_t buflen, unsigned char user_dictid[9], unsigned char server_dictid[9])
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

#if 0
static char *
ngx_http_gzip_window(ngx_conf_t *cf, void *post, void *data)
{
    size_t *np = data;

    size_t  wbits, wsize;

    wbits = 15;

    for (wsize = 32 * 1024; wsize > 256; wsize >>= 1) {

        if (wsize == *np) {
            *np = wbits;

            return NGX_CONF_OK;
        }

        wbits--;
    }

    return "must be 512, 1k, 2k, 4k, 8k, 16k, or 32k";
}


static char *
ngx_http_gzip_hash(ngx_conf_t *cf, void *post, void *data)
{
    size_t *np = data;

    size_t  memlevel, hsize;

    memlevel = 9;

    for (hsize = 128 * 1024; hsize > 256; hsize >>= 1) {

        if (hsize == *np) {
            *np = memlevel;

            return NGX_CONF_OK;
        }

        memlevel--;
    }

    return "must be 512, 1k, 2k, 4k, 8k, 16k, 32k, 64k, or 128k";
}
#endif
