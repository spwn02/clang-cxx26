//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_TASK_H
#define _LIBCPP___IO_TASK_H

#include <__assert>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__cstddef/max_align_t.h>
#include <__cstddef/size_t.h>
#include <__exception/exception_ptr.h>
#include <__io/frame_allocator.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__memory/addressof.h>
#include <__memory/construct_at.h>
#include <__memory_resource/memory_resource.h>
#include <__new/global_new_delete.h>
#include <__stop_token/stop_token.h>
#include <__type_traits/is_array.h>
#include <__type_traits/is_object.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/remove_reference.h>
#include <__utility/exchange.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// Awaiting one of these inside a task yields a part of the task's environment without suspending (P4003R3 3.3,
// `co_await get_stop_token`). Capy spells them `this_coro::stop_token` and so on; the names here are in std::io.
struct __get_stop_token_t {};
struct __get_environment_t {};
inline constexpr __get_stop_token_t get_stop_token{};
inline constexpr __get_environment_t get_environment{};

// The frame allocator travels in a footer after the coroutine frame, so that operator delete frees the frame to the
// resource it came from no matter what the out-of-band slot holds at that point.
inline constexpr size_t __frame_footer_align = alignof(pmr::memory_resource*);

_LIBCPP_HIDE_FROM_ABI inline size_t __frame_footer_offset(size_t __n) noexcept {
  return (__n + __frame_footer_align - 1) & ~(__frame_footer_align - 1);
}

_LIBCPP_HIDE_FROM_ABI inline void* __frame_allocate(size_t __n) {
  pmr::memory_resource* __mr = get_cached_frame_allocator();
  size_t __off               = __frame_footer_offset(__n);
  size_t __total             = __off + sizeof(pmr::memory_resource*);
  void* __p                  = __mr ? __mr->allocate(__total, alignof(max_align_t)) : ::operator new(__total);
  __builtin_memcpy(static_cast<char*>(__p) + __off, &__mr, sizeof(__mr));
  return __p;
}

_LIBCPP_HIDE_FROM_ABI inline void __frame_deallocate(void* __p, size_t __n) noexcept {
  size_t __off   = __frame_footer_offset(__n);
  size_t __total = __off + sizeof(pmr::memory_resource*);
  pmr::memory_resource* __mr;
  __builtin_memcpy(&__mr, static_cast<char*>(__p) + __off, sizeof(__mr));
  if (__mr)
    __mr->deallocate(__p, __total, alignof(max_align_t));
  else
    ::operator delete(__p);
}

template <class _Tp>
class task;

// Result storage of the promise; the value of a coroutine that co_returns a T.
template <class _Tp>
class __task_result {
  alignas(_Tp) unsigned char __buf_[sizeof(_Tp)];
  bool __has_ = false;

public:
  _LIBCPP_HIDE_FROM_ABI __task_result() = default;
  __task_result(const __task_result&)   = delete;
  _LIBCPP_HIDE_FROM_ABI ~__task_result() {
    if (__has_)
      std::__destroy_at(reinterpret_cast<_Tp*>(__buf_));
  }

  template <class _Up>
  _LIBCPP_HIDE_FROM_ABI void return_value(_Up&& __u) {
    std::__construct_at(reinterpret_cast<_Tp*>(__buf_), std::forward<_Up>(__u));
    __has_ = true;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp& result() noexcept { return *reinterpret_cast<_Tp*>(__buf_); }
};

template <>
class __task_result<void> {
public:
  _LIBCPP_HIDE_FROM_ABI void return_void() noexcept {}
};

// The promise. It is IoRunnable-compatible: set_continuation, set_environment, exception and (for non-void) result.
template <class _Tp>
class __task_promise : public __task_result<_Tp> {
  coroutine_handle<> __continuation_ = noop_coroutine();
  const io_env* __env_               = nullptr;
  exception_ptr __ep_;

  // Resuming the coroutine for the first time also refreshes the out-of-band slot, so that tasks called by the body
  // allocate from the environment's frame allocator.
  struct __initial_awaiter {
    __task_promise* __p_;
    _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
    _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<>) const noexcept {}
    _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {
      _LIBCPP_ASSERT_INTERNAL(__p_->__env_ != nullptr, "io::task started without an environment");
      set_cached_frame_allocator(__p_->__env_->frame_allocator);
    }
  };

  // Symmetric transfer back to whoever awaited the task.
  struct __final_awaiter {
    _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
    _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<__task_promise> __h) const noexcept {
      return __h.promise().__continuation_;
    }
    _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
  };

  // Wraps an IoAwaitable: the environment is injected as the second await_suspend argument and the out-of-band slot is
  // refreshed on every resumption.
  template <class _Aw>
  struct __env_awaiter {
    _Aw& __a_;
    const io_env* __env_;
    _LIBCPP_HIDE_FROM_ABI bool await_ready() { return __a_.await_ready(); }
    _LIBCPP_HIDE_FROM_ABI decltype(auto) await_suspend(coroutine_handle<> __h) {
      return __a_.await_suspend(__h, __env_);
    }
    _LIBCPP_HIDE_FROM_ABI decltype(auto) await_resume() {
      set_cached_frame_allocator(__env_->frame_allocator);
      return __a_.await_resume();
    }
  };

  template <class _Value>
  struct __query_awaiter {
    _Value __v_;
    _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return true; }
    _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<>) const noexcept {}
    _LIBCPP_HIDE_FROM_ABI _Value await_resume() noexcept { return std::move(__v_); }
  };

public:
  _LIBCPP_HIDE_FROM_ABI static void* operator new(size_t __n) { return io::__frame_allocate(__n); }
  _LIBCPP_HIDE_FROM_ABI static void operator delete(void* __p, size_t __n) noexcept {
    io::__frame_deallocate(__p, __n);
  }

  _LIBCPP_HIDE_FROM_ABI task<_Tp> get_return_object() noexcept;
  _LIBCPP_HIDE_FROM_ABI __initial_awaiter initial_suspend() noexcept { return __initial_awaiter{this}; }
  _LIBCPP_HIDE_FROM_ABI __final_awaiter final_suspend() noexcept { return {}; }
  _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept { __ep_ = std::current_exception(); }

  _LIBCPP_HIDE_FROM_ABI exception_ptr exception() const noexcept { return __ep_; }
  _LIBCPP_HIDE_FROM_ABI void set_continuation(coroutine_handle<> __h) noexcept { __continuation_ = __h; }
  _LIBCPP_HIDE_FROM_ABI void set_environment(const io_env* __e) noexcept { __env_ = __e; }
  _LIBCPP_HIDE_FROM_ABI const io_env* environment() const noexcept { return __env_; }

  // A task can only await IoAwaitables (and the query tags): the two-argument await_suspend is the protocol boundary.
  template <class _Aw>
    requires IoAwaitable<remove_cvref_t<_Aw>>
  _LIBCPP_HIDE_FROM_ABI auto await_transform(_Aw&& __a) noexcept {
    return __env_awaiter<remove_reference_t<_Aw>>{__a, __env_};
  }
  template <class _Aw>
    requires(!IoAwaitable<remove_cvref_t<_Aw>>)
  _LIBCPP_HIDE_FROM_ABI void await_transform(_Aw&&) = delete;

  _LIBCPP_HIDE_FROM_ABI auto await_transform(__get_stop_token_t) noexcept {
    return __query_awaiter<stop_token>{__env_->stop_token};
  }
  _LIBCPP_HIDE_FROM_ABI auto await_transform(__get_environment_t) noexcept {
    return __query_awaiter<const io_env*>{__env_};
  }
};

// P4100R1 [8.2]: the lazy coroutine task, with one template parameter. It is IoAwaitable and IoRunnable.
template <class _Tp>
class [[nodiscard]] task {
  static_assert(is_void_v<_Tp> || (is_object_v<_Tp> && !is_array_v<_Tp>),
                "io::task<T> requires T to be void or a non-array object type");

public:
  using promise_type = __task_promise<_Tp>;
  using value_type   = _Tp;

private:
  coroutine_handle<promise_type> __h_;

  friend promise_type;
  _LIBCPP_HIDE_FROM_ABI explicit task(coroutine_handle<promise_type> __h) noexcept : __h_(__h) {}

public:
  _LIBCPP_HIDE_FROM_ABI task() noexcept = default;
  task(const task&)                     = delete;
  task& operator=(const task&)          = delete;
  _LIBCPP_HIDE_FROM_ABI task(task&& __o) noexcept : __h_(std::exchange(__o.__h_, {})) {}
  _LIBCPP_HIDE_FROM_ABI task& operator=(task&& __o) noexcept {
    if (this != std::addressof(__o)) {
      if (__h_)
        __h_.destroy();
      __h_ = std::exchange(__o.__h_, {});
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~task() {
    if (__h_)
      __h_.destroy();
  }

  _LIBCPP_HIDE_FROM_ABI coroutine_handle<promise_type> handle() const noexcept { return __h_; }
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<promise_type> release() noexcept { return std::exchange(__h_, {}); }

  // IoAwaitable
  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<> __cont, const io_env* __env) noexcept {
    __h_.promise().set_continuation(__cont);
    __h_.promise().set_environment(__env);
    return __h_;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp await_resume() {
    if (exception_ptr __ep = __h_.promise().exception())
      std::rethrow_exception(__ep);
    if constexpr (!is_void_v<_Tp>)
      return std::move(__h_.promise().result());
  }
};

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI task<_Tp> __task_promise<_Tp>::get_return_object() noexcept {
  return task<_Tp>(coroutine_handle<__task_promise>::from_promise(*this));
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_TASK_H
