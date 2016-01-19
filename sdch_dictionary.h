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

// In-memory Dictionary representation
// Should be allocated from nginx pool
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

  Dictionary();
  ~Dictionary();

  // Init dictionary. Returns false in case of errors.
  bool init_from_file(const char* filename);
  bool init_quasy(const char* buf, size_t len);

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
  bool init(const char* begin,
            const char* payload,
            const char* end);

  std::auto_ptr<open_vcdiff::HashedDictionary> hashed_dict_;

  size_t size_;
  id_t client_id_;
  id_t server_id_;

  std::vector<char> blob_;
};


}  // namespace sdch

#endif  // SDCH_DICTIONARY_H_

