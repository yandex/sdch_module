// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_ENCODING_HANDLER_H_
#define SDCH_ENCODING_HANDLER_H_

#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// Actual VCDiff encoding handler
class EncodingHandler : public Handler {
 public:
  // TODO: Remove fat RequestContext and only pass required parameters
  EncodingHandler(RequestContext* ctx, Handler* next);
  ~EncodingHandler();

  ssize_t on_data(const char* buf, size_t len) override;

  void on_finish() override;

 private:
};


}  // namespace sdch

#endif  // SDCH_ENCODING_HANDLER_H_

