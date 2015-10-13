// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_output_handler.h"

#include <cassert>
#include "sdch_config.h"
#include "sdch_request_context.h"

namespace sdch {

namespace {

static ngx_int_t tr_filter_get_buf(RequestContext *ctx);
static ngx_int_t tr_filter_out_buf_out(RequestContext *ctx);

ssize_t tr_filter_write(RequestContext* ctx, const char* buf, size_t len) {
  int rlen = 0;

  while (len > 0) {
    if (ctx->out_buf) {
      auto l0 = std::min((off_t)len, ctx->out_buf->end - ctx->out_buf->last);
      if (l0 > 0) {
        memcpy(ctx->out_buf->last, buf, l0);
        len -= l0;
        buf += l0;
        ctx->out_buf->last += l0;
        rlen += l0;
      }
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

  ctx->total_out += rlen;
  return rlen;
}

static ngx_int_t tr_filter_get_buf(RequestContext* ctx) {
  ngx_http_request_t* r = ctx->request;

  assert(!ctx->out_buf || !ngx_buf_size(ctx->out_buf));

  ngx_log_debug0(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "tr_filter_get_buf");

  if (ctx->free) {
    ctx->out_buf = ctx->free->buf;
    ctx->free = ctx->free->next;

  } else {
    auto* conf = Config::get(ctx->request);
    ctx->out_buf = ngx_create_temp_buf(r->pool, conf->bufs.size);
    if (ctx->out_buf == nullptr) {
      return NGX_ERROR;
    }

    ctx->out_buf->tag = (ngx_buf_tag_t) & sdch_module;
    ctx->out_buf->recycled = 1;
    ctx->bufs++;
  }

  return NGX_OK;
}

static ngx_int_t
tr_filter_out_buf_out(RequestContext *ctx)
{
    ngx_chain_t *cl = ngx_alloc_chain_link(ctx->request->pool);
    if (cl == nullptr) {
        return NGX_ERROR;
    }

    cl->buf = ctx->out_buf;
    cl->next = nullptr;
    *ctx->last_out = cl;
    ctx->last_out = &cl->next;

    ctx->out_buf = nullptr;

    return NGX_OK;
}
}  // namespace

OutputHandler::OutputHandler(RequestContext* ctx, Handler* next)
    : Handler(next), ctx_(ctx) {}

OutputHandler::~OutputHandler() {}

bool OutputHandler::init(RequestContext* ctx) { return true; }

ssize_t OutputHandler::on_data(const char* buf, size_t len) {
  ssize_t res = tr_filter_write(ctx_, buf, len);
  // TODO Implement it
  if (next_)
    res = next_->on_data(buf, len);
  return res;
}

int OutputHandler::on_finish() {
  ctx_->out_buf->last_buf = 1;
  return tr_filter_out_buf_out(ctx_);
}

}  // namespace sdch
