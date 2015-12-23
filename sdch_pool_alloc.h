// Copyright (c) 2014 Yandex LLC. All rights reserved.
// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef POOL_ALLOC_H_
#define POOL_ALLOC_H_

extern "C" {
#include <ngx_core.h>  // For ngx_pool_t declaration
}

#include <stdint.h>
#include <stdexcept>    // std::bad_alloc
#include <utility>      // std::forward

namespace sdch {

template<typename Type>
void pool_cleanup(void* data) {
  Type* var = static_cast<Type*>(data);
  if (var == NULL)
    return;

  // Call destructor directly. Memory is handled by pool.
  var->~Type();
}

template<typename Type>
Type*
pool_alloc(ngx_pool_t* pool) {
  Type* res = static_cast<Type*>(ngx_pcalloc(pool, sizeof(Type)));
  if (res == NULL) {
    return NULL;
  }

  // Set cleanup handler
  ngx_pool_cleanup_t* cleanup = ngx_pool_cleanup_add(pool, 0);
  if (cleanup == NULL) {
    // ngx_conf_log_error(
    //    NGX_LOG_ERR, cf, 0, "failed to register a cleanup handler");
    pool_cleanup<Type>(res);
    return NULL;
  }

  cleanup->handler = pool_cleanup<Type>;
  cleanup->data = res;

  return res;
}

// pool_alloc from some Pool holder. E.g. request. Or conf.
// My template-fu isn't strong enough to correctly use something like
// std::is_member_object_pointer. So we redirect everything which is not
// ngx_pool_t to avoid ambiguity.
template<typename Type, typename Holder>
Type*
pool_alloc(Holder* h) {
  return pool_alloc<Type>(h->pool);
}

// STL compatible allocator
// We don't deallocate. And don't set pool_cleanup handler. Because it will be
// used in STL containers which will do "proper" memory management.
template<typename T>
class PoolAllocator {
 public:
  explicit PoolAllocator(ngx_pool_t* pool) : pool_(pool) {}

  typedef T value_type;
  typedef T* pointer;

  pointer allocate(std::size_t n) {
    pointer res = static_cast<pointer>(ngx_pcalloc(pool_, n * sizeof(value_type)));
    if (res == NULL) {
      // It's bad. But we can't return NULL
      throw std::bad_alloc();
    }
    return res;
  }

  void deallocate(pointer p, std::size_t n) { /* NOOP */ }

 private:
  ngx_pool_t* pool_;
};

}  // namespace ngx

#endif  // POOL_ALLOC_H_

