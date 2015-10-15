// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_encoding_handler.h"

#include <cassert>


#include "sdch_pool_alloc.h"
#include "sdch_request_context.h"

namespace sdch {

EncodingHandler::EncodingHandler(RequestContext* ctx,
                                 Handler* next,
                                 Dictionary* dict,
                                 Storage::ValueHolder quasidict)
    : Handler(next),
      ctx_(ctx),
      dict_(dict),
      quasidict_(std::move(quasidict)),
      cursize_(0) {
  assert(next_);
}

EncodingHandler::~EncodingHandler() {}

bool EncodingHandler::init(RequestContext* ctx) {
  enc_ = pool_alloc<open_vcdiff::VCDiffStreamingEncoder>(
      ctx_->request,
      dict_->hashed_dict(),
      open_vcdiff::VCD_FORMAT_INTERLEAVED | open_vcdiff::VCD_FORMAT_CHECKSUM,
      false);
  if (enc_ == nullptr)
    return false;

	if (!enc_->StartEncodingToInterface(this))
    return false;

  // Output Dictionary server_id first
  next_->on_data(dict_->server_id().c_str(), 9);

  return true;
}

Status EncodingHandler::on_data(const char* buf, size_t len) {
  // It will call ".append" which will pass it to the next_
  if (len) {
    if (!enc_->EncodeChunkToInterface(buf, len, this))
      return Status::ERROR;
    return next_status_;
  }

  // No data was supplied. Just pass it through.
  return next_->on_data(buf, len);
}

Status EncodingHandler::on_finish() {
  if (!enc_->FinishEncodingToInterface(this))
    return Status::ERROR;

  return next_->on_finish();
}


open_vcdiff::OutputStringInterface& EncodingHandler::append(const char* s,
                                                            size_t n) {
  next_status_ = next_->on_data(s, n);
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
