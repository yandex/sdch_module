#ifndef STORAGE_H
#define STORAGE_H

#include <time.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "sdch_dictionary.h"

namespace sdch {

// Simple LRU blob's storage with limit by total size
class Storage {
 public:
  // Stored Value
  struct Value {
    Value(time_t t, Dictionary d)
        : ts(t), dict(std::move(d)), locked(false) {}
    Value(Value&& other)
        : ts(other.ts), dict(std::move(other.dict)), locked(other.locked) {}
    ~Value();

    time_t ts;
    Dictionary dict;
    bool locked;
  };

  Storage();

  bool store(const std::string& key, Value value);
  // Get Value and "lock" it.
  Value* find(const std::string& key);
  // Unlock used Value
  void unlock(Value* v);

  bool clear(time_t ts);

  size_t total_size() const { return total_size_; }

  size_t max_size() const { return max_size_; }
  void set_max_size(size_t max_size) { max_size_ = max_size; }

 private:
  using StoreType = std::map<std::string, Value>;
  using LRUType = std::multimap<time_t, std::string>;

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
