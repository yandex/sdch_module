// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_ENCODING_HANDLER_H_
#define SDCH_ENCODING_HANDLER_H_

#include "sdch_handler.h"

#include <google/vcencoder.h>

#include "sdch_fastdict_factory.h"

namespace sdch {

class Dictionary;
class RequestContext;

// Actual VCDiff encoding handler
class EncodingHandler : public Handler,
                        public open_vcdiff::OutputStringInterface {
 public:
  EncodingHandler(Handler* next,
                  Dictionary* dict,
                  FastdictFactory::ValuePtr quasidict);
  ~EncodingHandler();

  // sdch::Handler implementation
  virtual bool init(RequestContext* ctx);
  virtual ngx_int_t on_data(const uint8_t* buf, size_t len);
  virtual ngx_int_t on_finish();

  // open_vcdiff::OutputStringInterface implementation
  virtual open_vcdiff::OutputStringInterface& append(const char* s, size_t n);
  virtual void clear();
  virtual void push_back(char c);
  virtual void ReserveAdditionalBytes(size_t res_arg);
  virtual size_t size() const;

 private:
  class OutHelper;

  RequestContext*   ctx_;
  Dictionary*       dict_;
  FastdictFactory::ValuePtr quasidict_;

  // Actual encoder.
  open_vcdiff::VCDiffStreamingEncoder enc_;

  // For OutputStringInterface implementation
  size_t cursize_;

  ngx_int_t next_status_;
};


}  // namespace sdch

#endif  // SDCH_ENCODING_HANDLER_H_

