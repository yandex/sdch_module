// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_HANDLER_H_
#define SDCH_HANDLER_H_

#include <cstring>  // For size_t

namespace sdch {

// SDCH Handler chain.
class Handler {
 public:
  // Construct Handler with pointer to the next Handler.
  // We don't own this pointer. It's owned by nginx pool.
  explicit Handler(Handler* next) : next_(next) {}
  virtual ~Handler();

  // Handle chunk of data. For example encode it with VCDIFF.
  // Almost every Handler should call next_->on_data() to keep chain.
  virtual void on_data(const char* buf, size_t len) = 0;

 protected:
  Handler* next_;
};


}  // namespace sdch

#endif  // SDCH_HANDLER_H_

