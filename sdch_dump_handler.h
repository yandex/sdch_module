// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_DUMP_HANDLER_H_
#define SDCH_DUMP_HANDLER_H_

#include "sdch_fdholder.h"
#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// Dumps data into temporary directory
class DumpHandler : public Handler {
 public:
  explicit DumpHandler(Handler* next);
  ~DumpHandler();

  bool init(RequestContext* ctx) override;

  Status on_data(const uint8_t* buf, size_t len) override;

 private:
  FDHolder fd_;
};


}  // namespace sdch

#endif  // SDCH_DUMP_HANDLER_H_

