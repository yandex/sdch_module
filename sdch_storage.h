#ifndef STORAGE_H
#define STORAGE_H

#include <time.h>
#include <map>
#include <vector>

namespace sdch {

// Simple LRU blob's storage with limit by total size
class Storage {
 public:
  // Blob. Just a blob.
  using Blob = std::vector<char>;

  // Stored Value
  struct Value {
    Value(time_t t, Blob bl)
        : ts(t), blob(std::move(bl)) {}

    time_t ts;
    Blob blob;
  };

  Storage();

  bool store(const char* key, Value&& value);
  Value* find(const char* key);

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
