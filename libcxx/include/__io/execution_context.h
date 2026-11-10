//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_EXECUTION_CONTEXT_H
#define _LIBCPP___IO_EXECUTION_CONTEXT_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__memory/addressof.h>
#include <__memory/unique_ptr.h>
#include <__memory_resource/memory_resource.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__type_traits/is_base_of.h>
#include <__type_traits/is_constructible.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__vector/vector.h>
#include <stdexcept>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4003R3 [4.4]: the base class of everything that runs work. It owns a registry of services (at most one per type),
// shuts them down in reverse order of creation and holds the default frame allocator of the coroutine chains that are
// launched through it.
//
// A derived context must call shutdown() and destroy() from its destructor, before its own members are destroyed;
// ~execution_context() calls them again, which is harmless because both are idempotent.
class execution_context {
public:
  _LIBCPP_DIAGNOSTIC_PUSH
  _LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
  class service {
  public:
    _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual ~service() = default;

  protected:
    _LIBCPP_HIDE_FROM_ABI service()                       = default;
    _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual void shutdown() = 0;

  private:
    friend class execution_context;
  };
  _LIBCPP_DIAGNOSTIC_POP

  execution_context(const execution_context&)            = delete;
  execution_context& operator=(const execution_context&) = delete;

  _LIBCPP_HIDE_FROM_ABI ~execution_context() {
    shutdown();
    destroy();
  }
  _LIBCPP_HIDE_FROM_ABI execution_context() = default;

  // Returns the service of type _Tp, creating it with _Tp(execution_context&) if the context has none yet. Thread-safe.
  // The constructor of the service may itself call use_service() on the same context.
  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI _Tp& use_service() {
    static_assert(is_base_of_v<service, _Tp>, "use_service<T>() requires T to derive from execution_context::service");
    static_assert(is_constructible_v<_Tp, execution_context&>,
                  "use_service<T>() requires T to be constructible from execution_context&");
    if (service* __existing = __find(__id<_Tp>()))
      return static_cast<_Tp&>(*__existing);
    unique_ptr<_Tp> __created(new _Tp(*this));
    return static_cast<_Tp&>(*__insert(__id<_Tp>(), std::move(__created), /*__must_be_new=*/false));
  }

  // Constructs a service of type _Tp from (execution_context&, __args...) and registers it. Thread-safe. Throws
  // invalid_argument if the context already has a service of type _Tp.
  template <class _Tp, class... _Args>
  _LIBCPP_HIDE_FROM_ABI _Tp& make_service(_Args&&... __args) {
    static_assert(is_base_of_v<service, _Tp>, "make_service<T>() requires T to derive from execution_context::service");
    static_assert(is_constructible_v<_Tp, execution_context&, _Args...>,
                  "make_service<T>() requires T to be constructible from (execution_context&, Args...)");
    if (__find(__id<_Tp>()))
      std::__throw_invalid_argument("execution_context::make_service: the service already exists");
    unique_ptr<_Tp> __created(new _Tp(*this, std::forward<_Args>(__args)...));
    return static_cast<_Tp&>(*__insert(__id<_Tp>(), std::move(__created), /*__must_be_new=*/true));
  }

  // The default frame allocator for the coroutine chains launched through this context. It is nullptr until set, which
  // means: frames use the global operator new.
  _LIBCPP_HIDE_FROM_ABI pmr::memory_resource* get_frame_allocator() const noexcept { return __frame_allocator_; }
  _LIBCPP_HIDE_FROM_ABI void set_frame_allocator(pmr::memory_resource* __mr) noexcept { __frame_allocator_ = __mr; }

protected:
  // Calls shutdown() on each service, in reverse order of creation. Idempotent. A service that is created after this
  // call is shut down as soon as it is created. Not thread-safe.
  _LIBCPP_HIDE_FROM_ABI void shutdown() noexcept {
    // Shutting down a service may create another one (and the registry may reallocate), so index instead of iterating.
    __shut_down_ = true;
    for (size_t __i = __services_.size(); __i != 0; --__i) {
      __entry& __e = __services_[__i - 1];
      if (!__e.__is_shut_down_) {
        __e.__is_shut_down_ = true;
        __e.__service_->shutdown();
      }
    }
  }

  // Deletes each service, in reverse order of creation, each one just after it is removed from the registry.
  // Idempotent. Not thread-safe.
  _LIBCPP_HIDE_FROM_ABI void destroy() noexcept {
    while (!__services_.empty()) {
      unique_ptr<service> __last = std::move(__services_.back().__service_);
      __services_.pop_back();
      __last.reset();
    }
  }

private:
  struct __entry {
    const void* __id_;
    unique_ptr<service> __service_;
    bool __is_shut_down_;
  };

  // A per-type identity that needs neither RTTI nor a central registration.
  template <class _Tp>
  static inline char __id_tag_ = 0;
  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI static const void* __id() noexcept {
    return std::addressof(__id_tag_<_Tp>);
  }

  _LIBCPP_HIDE_FROM_ABI service* __find(const void* __id) const {
    lock_guard<mutex> __lock(__mutex_);
    for (const __entry& __e : __services_)
      if (__e.__id_ == __id)
        return __e.__service_.get();
    return nullptr;
  }

  // Registers a service that was constructed outside the lock. If another thread (or the constructor of the service
  // itself) registered the same type in the meantime, use_service() returns that one and discards this one, and
  // make_service() reports the conflict.
  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI service* __insert(const void* __id, unique_ptr<_Tp> __created, bool __must_be_new) {
    service* __raw = __created.get();
    bool __run_shutdown;
    {
      lock_guard<mutex> __lock(__mutex_);
      for (const __entry& __e : __services_) {
        if (__e.__id_ == __id) {
          if (__must_be_new)
            std::__throw_invalid_argument("execution_context::make_service: the service already exists");
          return __e.__service_.get();
        }
      }
      __services_.push_back(__entry{__id, unique_ptr<service>(__created.release()), __shut_down_});
      __run_shutdown = __shut_down_;
    }
    if (__run_shutdown)
      __raw->shutdown();
    return __raw;
  }

  mutable mutex __mutex_;
  vector<__entry> __services_;
  bool __shut_down_                        = false;
  pmr::memory_resource* __frame_allocator_ = nullptr;
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_EXECUTION_CONTEXT_H
