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
  out_buf_->last_buf = 1;
  return flush_out_buf(false);
}

Status OutputHandler::write(const char* buf, size_t len) {
  int rlen = 0;

  while (len > 0) {
    if (out_buf_) {
      auto l0 = std::min((off_t)len, out_buf_->end - out_buf_->last);
      if (l0 > 0) {
        memcpy(out_buf_->last, buf, l0);
        len -= l0;
        buf += l0;
        out_buf_->last += l0;
        rlen += l0;
      }
    }

    // We have filled out_buf. Flush it.
    if (len > 0 || ctx_->need_flush) {
      if (out_buf_) {
        auto rc = flush_out_buf(true);
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

  assert(!out_buf_ || !ngx_buf_size(out_buf_));

  ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "sdch get_buf");

  if (ctx_->free) {
    out_buf_ = ctx_->free->buf;
    ctx_->free = ctx_->free->next;

  } else {
    auto* conf = Config::get(ctx_->request);
    out_buf_ = ngx_create_temp_buf(r->pool, conf->bufs.size);
    if (out_buf_ == nullptr) {
      return Status::ERROR;
    }

    out_buf_->tag = (ngx_buf_tag_t) & sdch_module;
    out_buf_->recycled = 1;
    ctx_->bufs++;
  }

  return Status::OK;
}

Status OutputHandler::flush_out_buf(bool flush) {
  ngx_chain_t* cl = ngx_alloc_chain_link(ctx_->request->pool);
  if (cl == nullptr) {
    return Status::ERROR;
  }

  cl->buf = out_buf_;
  cl->buf->flush = flush ? 1 : 0;
  cl->next = nullptr;
  *ctx_->last_out = cl;
  ctx_->last_out = &cl->next;

  out_buf_ = nullptr;

  return Status::OK;
}

}  // namespace sdch
