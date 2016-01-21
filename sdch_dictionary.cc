// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_dictionary.h"

#include <cassert>
#include <cstring>
#include <vector>
#include <google/vcencoder.h>
#include <openssl/sha.h>

#include "sdch_fdholder.h"

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

void encode_id(u_char* sha, Dictionary::id_t& id) {
	ngx_str_t src = {6, sha};
	ngx_str_t dst = {8, id.data()};
	ngx_encode_base64url(&dst, &src);
}

void get_dict_ids(const char* buf,
                  size_t buflen,
                  Dictionary::id_t& client_id,
                  Dictionary::id_t& server_id) {
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, buf, buflen);
  unsigned char sha[32];
  SHA256_Final(sha, &ctx);

  encode_id(sha, client_id);
  encode_id(sha + 6, server_id);
}

bool read_file(const char* fn, std::vector<char>& blob) {
  blob.clear();
  FDHolder fd(open(fn, O_RDONLY));
  if (fd == -1)
    return false;
  struct stat st;
  if (fstat(fd, &st) == -1)
    return false;
  blob.resize(st.st_size);
  if (read(fd, &blob[0], blob.size()) != (ssize_t)blob.size())
    return false;
  return true;
}

}  // namespace

bool Dictionary::init_from_file(const char* filename) {
  std::vector<char> blob;
  if (!read_file(filename, blob))
    return false;

  return init(blob.data(),
              get_dict_payload(blob.data(), blob.data() + blob.size()),
              blob.data() + blob.size());
}

bool Dictionary::init_quasy(const char* buf, size_t len) {
  size_ = len;
  return init(buf, buf, buf + len);
}

bool Dictionary::init(const char* begin,
                      const char* payload,
                      const char* end) {
  hashed_dict_.reset(new open_vcdiff::HashedDictionary(payload, end - payload));
  if (!hashed_dict_->Init())
    return false;

  get_dict_ids(begin, end - begin, client_id_, server_id_);
  return true;
}

}  // namespace sdch
