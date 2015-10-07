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
#include <string>

namespace open_vcdiff {
class HashedDictionary;
}

namespace sdch {

// In-memory Dictionary representation
class Dictionary {
 public:
  Dictionary();
  Dictionary(Dictionary&& other);
  ~Dictionary();

  // Init dictionary. Returns false in case of errors.
  bool init(const char* begin, const char* end, bool is_quasi);

  // Size of dictionary
  size_t size() const {
    return size_;
  }

  const std::string& client_id() const {
    return client_id_;
  }

  const std::string& server_id() const {
    return server_id_;
  }

  const open_vcdiff::HashedDictionary* hashed_dict() const {
    return hashed_dict_.get();
  }

 private:

  // It's not really good. We should integrate with nginx's pool allocations.
  // But this will do for now.
  std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dict_;

  size_t size_;
  std::string client_id_;
  std::string server_id_;
};


}  // namespace sdch

#endif  // SDCH_DICTIONARY_H_

