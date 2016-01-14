// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_encoding_handler.h"

#include <cassert>


#include "sdch_request_context.h"

namespace sdch {

EncodingHandler::EncodingHandler(Handler* next,
                                 Dictionary* dict,
                                 Storage::ValuePtr quasidict)
    : Handler(next),
      dict_(dict),
      quasidict_(quasidict),
      enc_(dict_->hashed_dict(),
        open_vcdiff::VCD_FORMAT_INTERLEAVED | open_vcdiff::VCD_FORMAT_CHECKSUM,
        false),
      cursize_(0) {
  assert(next_);
}

EncodingHandler::~EncodingHandler() {}

bool EncodingHandler::init(RequestContext* ctx) {
  // Output Dictionary server_id first
  next_->on_data(dict_->server_id().data(), 8);

  static uint8_t terminator[1] = { 0x0 };
  next_->on_data(terminator, 1);

  if (!enc_.StartEncodingToInterface(this))
    return false;

  return true;
}

Status EncodingHandler::on_data(const uint8_t* buf, size_t len) {
  // It will call ".append" which will pass it to the next_
  if (len) {
    if (!enc_.EncodeChunkToInterface(
             reinterpret_cast<const char*>(buf), len, this))
      return STATUS_ERROR;
    return next_status_;
  }

  // No data was supplied. Just pass it through.
  return next_->on_data(buf, len);
}

Status EncodingHandler::on_finish() {
  if (!enc_.FinishEncodingToInterface(this))
    return STATUS_ERROR;

  return next_->on_finish();
}


open_vcdiff::OutputStringInterface& EncodingHandler::append(const char* s,
                                                            size_t n) {
  next_status_ = next_->on_data(reinterpret_cast<const uint8_t*>(s), n);
  cursize_ += n;
  return *this;
}

void EncodingHandler::clear() { cursize_ = 0; }

void EncodingHandler::push_back(char c) { append(&c, 1); }

void EncodingHandler::ReserveAdditionalBytes(size_t res_arg) {
  // NOOP
}

size_t EncodingHandler::size() const { return cursize_; }

}  // namespace sdch
