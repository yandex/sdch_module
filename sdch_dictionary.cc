// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_dictionary.h"

#include <cstring>
#include <google/vcencoder.h>

namespace sdch {

namespace {

// Skip dictionary headers in case of on-disk dictionary
const char *get_dict_payload(const char *dictbegin, const char *dictend)
{
	const char *nl = dictbegin;
	while (nl < dictend) {
		if (*nl == '\n')
			return nl+1;
		nl = (const char*)memchr(nl, '\n', dictend-nl);
		if (nl == dictend)
			return nl;
		++nl;
	}
	return dictend;
}

}

Dictionary::Dictionary() {}

Dictionary::~Dictionary() {}

bool Dictionary::init(const char* begin, const char* end, bool is_quasi) {
  const auto* dict_payload = is_quasi ? begin : get_dict_payload(begin, end);
  hashed_dict_.reset(
      new open_vcdiff::HashedDictionary(dict_payload, end - dict_payload));
  return hashed_dict_->Init();
}

}  // namespace sdch
