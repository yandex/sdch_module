// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef POOL_ALLOC_H_
#define POOL_ALLOC_H_

extern "C" {
#include <ngx_core.h>  // For ngx_pool_t declaration
}

#include <type_traits>  // std::enable_if
#include <utility>      // std::forward

namespace sdch {

template<typename Type>
void pool_cleanup(void* data) {
  auto var = static_cast<Type*>(data);
  if (var == nullptr)
    return;

  // Call destructor directly. Memory is handled by pool.
  var->~Type();
}

template<typename Type, typename ... Args>
Type*
pool_alloc(ngx_pool_t* pool, Args&& ... args) {
  auto res = static_cast<Type*>(ngx_pcalloc(pool, sizeof(Type)));
  if (res == nullptr) {
    return nullptr;
  }

  // Call constructor.
  new (res) Type(std::forward<Args>(args)...);

  // Set cleanup handler
  auto* cleanup = ngx_pool_cleanup_add(pool, 0);
  if (cleanup == nullptr) {
    // ngx_conf_log_error(
    //    NGX_LOG_ERR, cf, 0, "failed to register a cleanup handler");
    pool_cleanup<Type>(res);
    return nullptr;
  }

  cleanup->handler = pool_cleanup<Type>;
  cleanup->data = res;

  return res;
}

// pool_alloc from some Pool holder. E.g. request. Or conf.
// My template-fu isn't strong enough to correctly use something like
// std::is_member_object_pointer. So we redirect everything which is not
// ngx_pool_t to avoid ambiguity.
template<typename Type, typename Holder, typename ...Args>
typename std::enable_if<!std::is_same<Holder,ngx_pool_t>::value, Type>::type*
pool_alloc(Holder* h, Args&& ...args) {
  return pool_alloc<Type>(h->pool, std::forward<Args>(args)...);
}


}  // namespace ngx

#endif  // POOL_ALLOC_H_

