// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_DICTIONARY_H_
#define SDCH_DICTIONARY_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <memory>
#include <vector>

#include "sdch_pool_alloc.h"

namespace open_vcdiff {
class HashedDictionary;
}

namespace sdch {

class DictionaryFactory;

// In-memory Dictionary representation
// Should NOT be allocated from nginx pool
class Dictionary {
 public:
  class id_t {
   public:
    uint8_t* data() { return id_; }
    const uint8_t* data() const { return id_; }
    size_t size() const { return 8; }

    friend bool operator<(const id_t& left, const id_t& right) {
      return ngx_strncmp(left.id_, right.id_, 8) < 0;
    }

   private:
    uint8_t id_[8];
  };

  // Size of dictionary
  size_t size() const {
    return size_;
  }

  const id_t& client_id() const {
    return client_id_;
  }

  const id_t& server_id() const {
    return server_id_;
  }

  const open_vcdiff::HashedDictionary* hashed_dict() const {
    return hashed_dict_.get();
  }

 private:
  friend class DictionaryFactory;
  friend class Storage;

  bool init(const char* begin,
            const char* payload,
            const char* end);

  std::auto_ptr<open_vcdiff::HashedDictionary> hashed_dict_;

  size_t size_;
  id_t client_id_;
  id_t server_id_;
};


}  // namespace sdch

#endif  // SDCH_DICTIONARY_H_

