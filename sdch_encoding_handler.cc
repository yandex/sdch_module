// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_encoding_handler.h"

#include <cassert>

namespace sdch {

EncodingHandler::EncodingHandler(RequestContext* ctx, Handler* next)
    : Handler(next) {
  assert(next_);

  // tr_filter_write(ctx, ctx->dict->server_dictid, 9);
  // get_vcd_encoder(ctx->dict->hashed_dict, ctx, &ctx->enc);
}

EncodingHandler::~EncodingHandler() {}

ssize_t EncodingHandler::on_data(const char* buf, size_t len) {
  // TODO Implement it
  return next_->on_data(buf, len);
}

void EncodingHandler::on_finish() {
  // TODO Implement it
  return next_->on_finish();
}

}  // namespace sdch
