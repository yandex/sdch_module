// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_dump_handler.h"

#include <cassert>

namespace sdch {

DumpHandler::DumpHandler(RequestContext* ctx, Handler* next) : Handler(next) {
  assert(next_);
}

DumpHandler::~DumpHandler() {}

ssize_t DumpHandler::on_data(const char* buf, size_t len) {
  ssize_t res = 0;
  // TODO Implement it
  if (next_)
    res = next_->on_data(buf, len);
  return res;
}

void DumpHandler::on_finish() {
  // TODO Implement it
  next_->on_finish();
}

}  // namespace sdch
