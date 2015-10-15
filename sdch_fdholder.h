// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_FDHOLDER_H_
#define SDCH_FDHOLDER_H_

#include <unistd.h>

namespace sdch {

// Simple RAII holder for opened files.
class FDHolder {
 public:
  explicit FDHolder(int fd) : fd_(fd) {}
  ~FDHolder() {
    if (fd_ != -1)
      close(fd_);
  }

  FDHolder& operator=(FDHolder&& other) {
    fd_ = other.fd_;
    other.fd_ = -1;
  }
  operator int() { return fd_; }

 private:
  int fd_;
  FDHolder& operator=(const FDHolder&);
};


}  // namespace sdch

#endif  // SDCH_FDHOLDER_H_

