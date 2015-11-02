// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_OUTPUT_HANDLER_H_
#define SDCH_OUTPUT_HANDLER_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// nginx output handler. Will pass data to nginx.
class OutputHandler : public Handler {
 public:
  OutputHandler(RequestContext* ctx, ngx_http_output_body_filter_pt next_body);
  ~OutputHandler();

  bool init(RequestContext* ctx) override;

  Status on_data(const uint8_t* buf, size_t len) override;

  Status on_finish() override;

 private:
  Status get_buf();
  Status flush_out_buf(bool flush);
  Status write(const uint8_t* buf, size_t len);
  Status next_body();

  RequestContext* ctx_;
  ngx_http_output_body_filter_pt next_body_;

  ngx_buf_t* out_buf_;
  ngx_chain_t* free_;
  ngx_chain_t* busy_;
  ngx_chain_t* out_;
  ngx_chain_t** last_out_;
};


}  // namespace sdch

#endif  // SDCH_OUTPUT_HANDLER_H_

