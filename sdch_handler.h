// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_HANDLER_H_
#define SDCH_HANDLER_H_

#include <sys/types.h>  // For size_t and ssize_t

namespace sdch {

class RequestContext;

// SDCH Handler chain.
class Handler {
 public:
  // Construct Handler with pointer to the next Handler.
  // We don't own this pointer. It's owned by nginx pool.
  explicit Handler(Handler* next);
  virtual ~Handler();

  // Called after constructor to avoid exceptions
  // Should return true if inited successfully
  virtual bool init(RequestContext* ctx) = 0;

  // Handle chunk of data. For example encode it with VCDIFF.
  // Almost every Handler should call next_->on_data() to keep chain.
  virtual ssize_t on_data(const char* buf, size_t len) = 0;

  // Called when request processing finished.
  // Default implementation just invokes next_->on_finish();
  virtual int on_finish();

 protected:
  Handler* next_;
};


}  // namespace sdch

#endif  // SDCH_HANDLER_H_

