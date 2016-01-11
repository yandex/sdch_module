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

#include <boost/move/unique_ptr.hpp>
#include <boost/move/utility.hpp>

#include <google/vcencoder.h>

namespace sdch {

// In-memory Dictionary representation
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
  Dictionary(BOOST_RV_REF(Dictionary) other)
    : hashed_dict_(boost::move(other.hashed_dict_)),
      size_(boost::move(other.size_)),
      client_id_(boost::move(other.client_id_)),
      server_id_(boost::move(other.server_id_)) {}
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
  bool init(const char* begin, const char* payload, const char* end);

  // It's not really good. We should integrate with nginx's pool allocations.
  // But this will do for now.
  boost::movelib::unique_ptr<open_vcdiff::HashedDictionary> hashed_dict_;

  size_t size_;
  id_t client_id_;
  id_t server_id_;

  BOOST_MOVABLE_BUT_NOT_COPYABLE(Dictionary);
};


}  // namespace sdch

#endif  // SDCH_DICTIONARY_H_

