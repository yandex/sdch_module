// Copyright (c) 2015 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_dictionary_factory.h"

#include <algorithm>

namespace sdch {

namespace {

bool compare_dict_conf(const DictConfig& a, const DictConfig& b) {
  ngx_int_t c = ngx_memn2cmp(a.groupname.data, b.groupname.data,
                             a.groupname.len, b.groupname.len);
  if (c != 0)
    return c < 0;
  return a.priority < b.priority;
}

}  // namespace

DictionaryFactory::DictionaryFactory(ngx_pool_t* pool)
    : pool_(pool),
      dict_storage_(DictStorage::allocator_type(pool)),
      conf_storage_(DictConfStorage::allocator_type(pool)) {}

DictConfig* DictionaryFactory::store_config(Dictionary* dict,
                                            ngx_str_t& groupname,
                                            ngx_uint_t prio) {

  conf_storage_.emplace_back();
  DictConfig* res = &*conf_storage_.rbegin();

  res->groupname.len = groupname.len;
  res->groupname.data = ngx_pstrdup(pool_, &groupname);
  res->priority = prio != ngx_uint_t(-1) ? prio : conf_storage_.size() - 1;
  res->dict = dict;
  res->best = false;

  return res;
}

Dictionary* DictionaryFactory::allocate_dictionary() {
  Dictionary* res = pool_alloc<Dictionary>(pool_);
  dict_storage_.push_back(res);
  return res;
}

DictConfig* DictionaryFactory::choose_best_dictionary(DictConfig* old,
                                                      DictConfig* n,
                                                      const ngx_str_t& group) {
  if (old == NULL)
    return n;
  if (n == NULL)
    return old;

  int om =
      ngx_memn2cmp(
          old->groupname.data, group.data, old->groupname.len, group.len) == 0;
  int nm = ngx_memn2cmp(
               n->groupname.data, group.data, n->groupname.len, group.len) == 0;
  if (om && !nm)
    return old;
  if (nm && !om)
    return n;
  if (n->priority < old->priority)
    return n;
  return old;
}

DictConfig* DictionaryFactory::find_dictionary(const u_char* client_id) {
  for (DictConfig* c = conf_storage_.back(); c != conf_storage_.end(); ++c) {
    if (ngx_strncmp(client_id, c->dict->client_id().data(), 8) == 0)
      return c;
  }

  return NULL;
}

void DictionaryFactory::merge(const DictionaryFactory* parent) {
  // Merge dictionaries from parent.
  if (conf_storage_.empty())
    conf_storage_.insert(conf_storage_.end(),
                       parent->conf_storage_.begin(),
                       parent->conf_storage_.end());

  // And sort them
  std::sort(conf_storage_.begin(), conf_storage_.end(), compare_dict_conf);

  if (!conf_storage_.empty()) {
    conf_storage_.begin()->best = 1;
  }
  for (size_t i = 1; i < conf_storage_.size(); ++i) {
    if (ngx_memn2cmp(conf_storage_[i - 1].groupname.data,
                   conf_storage_[i].groupname.data,
                   conf_storage_[i - 1].groupname.len,
                   conf_storage_[i].groupname.len)) {
      conf_storage_[i].best = 1;
    }
  }
}

}  // namespace sdch
