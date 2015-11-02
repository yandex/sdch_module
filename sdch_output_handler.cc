// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_output_handler.h"

#include <cassert>
#include "sdch_config.h"
#include "sdch_request_context.h"

namespace sdch {

OutputHandler::OutputHandler(RequestContext* ctx, ngx_http_output_body_filter_pt next_body)
    : Handler(nullptr), ctx_(ctx), next_body_(next_body) {
}

OutputHandler::~OutputHandler() {}

bool OutputHandler::init(RequestContext* ctx) {
  last_out_ = &out_;
  return true;
}

Status OutputHandler::on_data(const char* buf, size_t len) {
  auto res = write(buf, len);
  if (!out_)
    return res;

  return next_body();
}

Status OutputHandler::on_finish() {
  out_buf_->last_buf = 1;
  if (flush_out_buf(false) == Status::ERROR)
    return Status::ERROR;

  return next_body();
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

  if (free_) {
    out_buf_ = free_->buf;
    free_ = free_->next;
  } else {
    auto* conf = Config::get(ctx_->request);
    out_buf_ = ngx_create_temp_buf(r->pool, conf->bufs.size);
    if (out_buf_ == nullptr) {
      return Status::ERROR;
    }

    out_buf_->tag = (ngx_buf_tag_t) & sdch_module;
    out_buf_->recycled = 1;
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
  *last_out_ = cl;
  last_out_ = &out_;

  out_buf_ = nullptr;

  return Status::OK;
}

Status OutputHandler::next_body() {
  auto rc = next_body_(ctx_->request, out_);
  ngx_chain_update_chains(ctx_->request->pool,
                          &free_,
                          &busy_,
                          &out_,
                          (ngx_buf_tag_t) & sdch_module);
  return rc == NGX_OK ? Status::OK : Status::ERROR;
}

}  // namespace sdch
