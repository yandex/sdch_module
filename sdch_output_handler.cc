// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_output_handler.h"

#include "sdch_module.h"

namespace sdch {

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

}  // namespace sdch
