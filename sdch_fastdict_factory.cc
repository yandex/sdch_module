#include <cassert>

#include "sdch_fastdict_factory.h"

#include <boost/make_shared.hpp>

namespace sdch {

FastdictFactory::Value::~Value() {}

FastdictFactory::FastdictFactory() : max_size_(10000000) {}

bool FastdictFactory::clear(time_t ts) { return true; }

Dictionary* FastdictFactory::create_dictionary(const char* buf, size_t len) {
  ValuePtr v = boost::make_shared<Value>(time(NULL));
  if (!v->dict.init(buf, buf, buf + len)) {
    return NULL;
  }

  if (!store(v->dict.client_id(), v)) {
    return NULL;
  }

  return &v->dict;
}

bool FastdictFactory::store(Dictionary::id_t key, ValuePtr value) {
  std::pair<StoreType::iterator, bool> r =
      values_.insert(std::make_pair(key, value));
  if (!r.second) {
#if 0
        if (r.first->second.ts < ts)
            r.first->second.ts = ts;
#endif
    return false;
  }

  lru_.insert(LRUType::value_type(r.first->second->ts, key));
  total_size_ += r.first->second->dict.size();

  // Remove oldest entries if we exceeded max_size_
  for (LRUType::iterator i = lru_.begin();
       total_size_ > max_size_ && i != lru_.end();) {
    StoreType::iterator si = values_.find(i->second);
    if (si == values_.end()) {
      assert(0);
      continue;
    }

    if (!si->second.unique()) {  // XXX
      ++i;
      continue;
    }

    total_size_ -= si->second->dict.size();
    values_.erase(si);
    lru_.erase(i++);
  }

  return true;
}

FastdictFactory::ValuePtr FastdictFactory::find(const Dictionary::id_t& key) {
  StoreType::iterator i = values_.find(key);
  if (i == values_.end())
    return ValuePtr();

  // TODO Update LRU?
  return i->second;
}

}  // namespace sdch
