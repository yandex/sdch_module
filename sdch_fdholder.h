// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_FDHOLDER_H_
#define SDCH_FDHOLDER_H_

#include <unistd.h>

#include <boost/move/utility.hpp>

namespace sdch {

// Simple RAII holder for opened files.
class FDHolder {
 public:
  explicit FDHolder(int fd) : fd_(fd) {}
  ~FDHolder() {
    if (fd_ != -1)
      close(fd_);
  }

  FDHolder(BOOST_RV_REF(FDHolder) other) : fd_(other.fd_) {
    other.fd_ = -1;
  }

  FDHolder& operator=(BOOST_RV_REF(FDHolder) other) {
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  operator int() { return fd_; }

 private:
  int fd_;

  BOOST_MOVABLE_BUT_NOT_COPYABLE(FDHolder);
};


}  // namespace sdch

#endif  // SDCH_FDHOLDER_H_

