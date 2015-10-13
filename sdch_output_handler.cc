// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_output_handler.h"

#include <cassert>
#include "sdch_config.h"
#include "sdch_request_context.h"

namespace sdch {

OutputHandler::OutputHandler(RequestContext* ctx)
    : Handler(nullptr), ctx_(ctx) {}

OutputHandler::~OutputHandler() {}

bool OutputHandler::init(RequestContext* ctx) { return true; }

Status OutputHandler::on_data(const char* buf, size_t len) {
  auto res = write(buf, len);
  if (res != Status::OK)
    return res;
  return ctx_->last_buf ? Status::FINISH : res;
}

Status OutputHandler::on_finish() {
  ctx_->out_buf->last_buf = 1;
  return out_buf_out();
}

Status OutputHandler::write(const char* buf, size_t len) {
  int rlen = 0;

  while (len > 0) {
    if (ctx_->out_buf) {
      auto l0 = std::min((off_t)len, ctx_->out_buf->end - ctx_->out_buf->last);
      if (l0 > 0) {
        memcpy(ctx_->out_buf->last, buf, l0);
        len -= l0;
        buf += l0;
        ctx_->out_buf->last += l0;
        rlen += l0;
      }
    }
    if (len > 0) {
      if (ctx_->out_buf) {
        auto rc = out_buf_out();
        if (rc != Status::OK)
          return rc;
      }

      get_buf();
    }
  }

  ctx_->total_out += rlen;
  return Status::OK;
}

Status OutputHandler::get_buf() {
  ngx_http_request_t* r = ctx_->request;

  assert(!ctx_->out_buf || !ngx_buf_size(ctx_->out_buf));

  ngx_log_debug0(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "tr_filter_get_buf");

  if (ctx_->free) {
    ctx_->out_buf = ctx_->free->buf;
    ctx_->free = ctx_->free->next;

  } else {
    auto* conf = Config::get(ctx_->request);
    ctx_->out_buf = ngx_create_temp_buf(r->pool, conf->bufs.size);
    if (ctx_->out_buf == nullptr) {
      return Status::ERROR;
    }

    ctx_->out_buf->tag = (ngx_buf_tag_t) & sdch_module;
    ctx_->out_buf->recycled = 1;
    ctx_->bufs++;
  }

  return Status::OK;
}

Status OutputHandler::out_buf_out() {
  ngx_chain_t* cl = ngx_alloc_chain_link(ctx_->request->pool);
  if (cl == nullptr) {
    return Status::ERROR;
  }

  cl->buf = ctx_->out_buf;
  cl->next = nullptr;
  *ctx_->last_out = cl;
  ctx_->last_out = &cl->next;

  ctx_->out_buf = nullptr;

  return Status::OK;
}

}  // namespace sdch
