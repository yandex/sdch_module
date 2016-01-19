#ifndef STORAGE_H
#define STORAGE_H

#include <time.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "sdch_dictionary.h"

#include <boost/shared_ptr.hpp>

namespace sdch {

// Simple LRU blob's storage with limit by total size
class Storage {
 public:
  // Stored Value
  struct Value {
    Value(time_t t) : ts(t) {}
    Value(time_t t, Dictionary d) : ts(t), dict(d) {}
    ~Value();

    time_t ts;
    Dictionary dict;
  };

  // We are using shared_ptr for refcounting Values to avoid discarding
  // currently in use Dictionaries.
  typedef boost::shared_ptr<Value> ValuePtr;

  Storage();

  bool store(Dictionary::id_t key, Storage::ValuePtr value);
  // Get Value and "lock" it.
  Storage::ValuePtr find(const Dictionary::id_t& key);

  bool clear(time_t ts);

  size_t total_size() const { return total_size_; }

  size_t max_size() const { return max_size_; }
  void set_max_size(size_t max_size) { max_size_ = max_size; }

 private:
  friend class Unlocker;

  typedef std::map<Dictionary::id_t, Storage::ValuePtr> StoreType;
  typedef std::multimap<time_t, Dictionary::id_t>  LRUType;

  // Values
  StoreType values_;
  // LRU of values
  LRUType lru_;
  // Current total size
  size_t total_size_;
  // Maximum total size
  size_t max_size_;
};

}  // namespace sdch

#endif  // STORAGE_H
