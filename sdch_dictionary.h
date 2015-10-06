// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_DICTIONARY_H_
#define SDCH_DICTIONARY_H_

#include <memory>

namespace open_vcdiff {
class HashedDictionary;
}

namespace sdch {

// In-memory Dictionary representation
class Dictionary {
 public:
  Dictionary();

  // Init dictionary. Returns false in case of errors.
  bool init(const char* begin, const char* end, bool is_quasi);

 private:
	std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dict_;
};


}  // namespace sdch

#endif  // SDCH_DICTIONARY_H_

