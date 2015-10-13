// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_STATUS_H_
#define SDCH_STATUS_H_

namespace sdch {

enum class Status {
  OK,       // everything is ok
  ERROR,    // some bad things happened
  FINISH,   // processing is finished
};


}  // namespace sdch

#endif  // SDCH_STATUS_H_

