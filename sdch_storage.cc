#include <map>
#include <string>

#include <cassert>

#include "sdch_storage.h"

namespace sdch {

Storage::Storage() : max_size_(10000000) {}

bool Storage::clear(time_t ts) { return true; }

bool Storage::store(const char* key, Value&& value) {
  auto r = values_.insert(StoreType::value_type(key, std::move(value)));
  if (!r.second) {
#if 0
        if (r.first->second.ts < ts)
            r.first->second.ts = ts;
#endif
    return false;
  }

  lru_.insert(LRUType::value_type(r.first->second.ts, key));
  total_size_ += r.first->second.blob.size();

  // Remove oldest entries if we exceeded max_size_
  for (auto i = lru_.begin(); total_size_ > max_size_ && i != lru_.end(); ) {
    auto si = values_.find(i->second);
    if (si == values_.end()) {
      assert(0);
      continue;
    }

    total_size_ -= si->second.blob.size();
    values_.erase(si);
    lru_.erase(i++);
  }

  return true;
}

Storage::Value* Storage::find(const char* key) {
  auto i = values_.find(key);
  if (i == values_.end())
    return nullptr;

  // TODO Update LRU?
  return &i->second;
}

}  // namespace sdch
