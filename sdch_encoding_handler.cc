// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_encoding_handler.h"

#include <cassert>

namespace sdch {

EncodingHandler::EncodingHandler(RequestContext* ctx, Handler* next) {
  assert(next_);
}

ssize_t EncodingHandler::on_data(const char* buf, size_t len) {
  // TODO Implement it
  return next_->on_data(buf, len);
}

void EncodingHandler::on_finish() {
  return next_->on_finish();
}

}  // namespace sdch
