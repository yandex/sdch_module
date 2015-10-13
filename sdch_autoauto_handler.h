// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_AUTOAUTO_HANDLER_H_
#define SDCH_AUTOAUTO_HANDLER_H_

#include <vector>

#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// Create YaSDCH dictionary 
class AutoautoHandler : public Handler {
 public:
  AutoautoHandler(RequestContext* ctx, Handler* next);
  ~AutoautoHandler();

  bool init(RequestContext* ctx) override;

  Status on_data(const char* buf, size_t len) override;
  Status on_finish() override;

 private:
  // Keep context. For logging purpose mostly.
  RequestContext* ctx_;

  // Storage for data passing by. We'll create actual dictionary in on_finish
  std::vector<char> blob_;
};


}  // namespace sdch

#endif  // SDCH_AUTOAUTO_HANDLER_H_

