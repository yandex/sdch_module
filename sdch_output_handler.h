// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_OUTPUT_HANDLER_H_
#define SDCH_OUTPUT_HANDLER_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
}

#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// nginx output handler. Will pass data to nginx.
class OutputHandler : public Handler {
 public:
  OutputHandler(RequestContext* ctx);
  ~OutputHandler();

  bool init(RequestContext* ctx) override;

  Status on_data(const char* buf, size_t len) override;

  Status on_finish() override;

 private:
  Status get_buf();
  Status flush_out_buf(bool flush);
  Status write(const char* buf, size_t len);

  RequestContext* ctx_;
  ngx_buf_t* out_buf_;
};


}  // namespace sdch

#endif  // SDCH_OUTPUT_HANDLER_H_

