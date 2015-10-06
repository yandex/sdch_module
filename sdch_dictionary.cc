// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_dictionary.h"

#include <cstring>
#include <google/vcencoder.h>
#include <openssl/sha.h>

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

#if nginx_version < 1006000
static void
ngx_encode_base64url(ngx_str_t *dst, ngx_str_t *src)
{
	ngx_encode_base64(dst, src);
	unsigned i;

	for (i = 0; i < dst->len; i++) {
		if (dst->data[i] == '+')
			dst->data[i] = '-';
		if (dst->data[i] == '/')
			dst->data[i] = '_';
	}
}
#endif

void encode_id(u_char* sha, std::string& id) {
  id.resize(8);
	ngx_str_t src = {6, sha};
	ngx_str_t dst = {8, reinterpret_cast<u_char*>(&id[0])};
	ngx_encode_base64url(&dst, &src);
}

void get_dict_ids(const char* buf,
                  size_t buflen,
                  std::string& client_id,
                  std::string& server_id) {
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, buf, buflen);
  unsigned char sha[32];
  SHA256_Final(sha, &ctx);

  encode_id(sha, client_id);
  encode_id(sha + 6, server_id);
}

}  // namespace

Dictionary::Dictionary() {}

Dictionary::~Dictionary() {}

bool Dictionary::init(const char* begin, const char* end, bool is_quasi) {
  const auto* payload = is_quasi ? begin : get_dict_payload(begin, end);
  size_t size = end - payload;
  hashed_dict_.reset(new open_vcdiff::HashedDictionary(payload, size));
  if (!hashed_dict_->Init())
    return false;

  get_dict_ids(begin, end - begin, client_id_, server_id_);
  return true;
}

}  // namespace sdch
