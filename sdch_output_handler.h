// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_OUTPUT_HANDLER_H_
#define SDCH_OUTPUT_HANDLER_H_

#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// nginx output handler. Will pass data to nginx.
class OutputHandler : public Handler {
 public:
  OutputHandler(RequestContext* ctx, Handler* next);
  ~OutputHandler();

  bool init(RequestContext* ctx) override;

  ssize_t on_data(const char* buf, size_t len) override;

 private:
  RequestContext* ctx_;
};


}  // namespace sdch

#endif  // SDCH_OUTPUT_HANDLER_H_

