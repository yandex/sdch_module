// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_REQUEST_CONTEXT_H_
#define SDCH_REQUEST_CONTEXT_H_

extern "C" {

#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>

}

#include "sdch_module.h"  // TODO Remove. We need proper sdch::Dictionary

namespace sdch {

class Handler;

// Context used inside nginx to keep relevant data.
struct RequestContext {
 public:
  // Create RequestContext.
  RequestContext(ngx_http_request_t* r);

  // Fetch RequestContext associated with nginx request
  static RequestContext* get(ngx_http_request_t* r);

  // TODO Remove
  void                *coo;
  vcd_encoder_p        enc;
  blob_type            blob;

  ngx_http_request_t* request;
  Handler             *handler;


  ngx_chain_t* in;
  ngx_chain_t* free;
  ngx_chain_t* busy;
  ngx_chain_t* out;
  ngx_chain_t** last_out;

  ngx_chain_t* copied;
  ngx_chain_t* copy_buf;

  ngx_buf_t* in_buf;
  ngx_buf_t* out_buf;
  ngx_int_t bufs;

  struct sdch_dict* dict;
  struct sdch_dict fdict;

  unsigned started : 1;
  unsigned flush : 4;
  unsigned redo : 1;
  unsigned done : 1;
  unsigned nomem : 1;
  unsigned buffering : 1;

  unsigned store : 1;

  size_t zin;
  size_t zout;

  z_stream zstream;

  struct sv* stuc;
};


}  // namespace sdch

#endif  // SDCH_REQUEST_CONTEXT_H_

