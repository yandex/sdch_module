// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_handler.h"

namespace sdch {

Handler::Handler(Handler* next) : next_(next) {}
Handler::~Handler() {}

}  // namespace sdch
