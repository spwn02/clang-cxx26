//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_JOIN_SUPPORT_H
#define _LIBCPP___IO_JOIN_SUPPORT_H

#include <__atomic/atomic.h>
#include <__atomic/memory_order.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__cstddef/size_t.h>
#include <__exception/exception_ptr.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor_ref.h>
#include <__io/frame_allocator.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__io/task.h>
#include <__stop_token/stop_callback.h>
#include <__stop_token/stop_source.h>
#include <__stop_token/stop_token.h>
#include <__type_traits/decay.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/remove_reference.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <optional>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// What awaiting an IoAwaitable yields.
template <class _Aw>
using __await_result_t _LIBCPP_NODEBUG = decltype(std::declval<_Aw&>().await_resume());

// State shared by the runners of one when_all or when_any call and the awaitable that owns them. The awaitable lives in
// the frame of the awaiting coroutine, which stays suspended until the last runner has finished.
struct __join_core {
  struct __stop_fn {
    stop_source* __s_;
    // Requesting stop can finish the last runner and destroy the state with its source before the request returns; the
    // copy keeps the stop state alive until then.
    _LIBCPP_HIDE_FROM_ABI void operator()() const {
      stop_source __keep = *__s_;
      __keep.request_stop();
    }
  };

  atomic<size_t> __remaining_;
  atomic<bool> __failed_{false};
  atomic<bool> __won_{false};
  size_t __winner_ = 0;
  exception_ptr __first_exception_;
  stop_source __stop_;
  bool __stop_on_exception_;
  continuation __parent_;
  executor_ref __parent_ex_;
  io_env __env_;
  optional<stop_callback<__stop_fn>> __parent_stop_cb_;

  _LIBCPP_HIDE_FROM_ABI __join_core(size_t __n, bool __stop_on_exception) noexcept
      : __remaining_(__n), __stop_on_exception_(__stop_on_exception) {}
  __join_core(const __join_core&)            = delete;
  __join_core& operator=(const __join_core&) = delete;

  // The first exception wins; later ones are dropped.
  _LIBCPP_HIDE_FROM_ABI void __capture(exception_ptr __ep) noexcept {
    bool __expected = false;
    if (__failed_.compare_exchange_strong(__expected, true, memory_order_relaxed))
      __first_exception_ = std::move(__ep);
    if (__stop_on_exception_)
      __stop_.request_stop();
  }

  // Only the first caller gets true.
  _LIBCPP_HIDE_FROM_ABI bool __claim_win() noexcept {
    bool __expected = false;
    return __won_.compare_exchange_strong(__expected, true, memory_order_relaxed);
  }

  // Children run with the caller's executor and frame allocator and with a stop token that fires when the caller's
  // token fires or when the operation itself requests stop.
  _LIBCPP_HIDE_FROM_ABI void __begin(coroutine_handle<> __h, const io_env* __parent) {
    __parent_.h          = __h;
    __parent_.__chained_ = true;
    __parent_ex_         = __parent->executor;
    __env_               = io_env{__parent->executor, __stop_.get_token(), __parent->frame_allocator};
    if (__parent->stop_token.stop_possible())
      __parent_stop_cb_.emplace(__parent->stop_token, __stop_fn{&__stop_});
  }
};

// The coroutine that awaits one child. It starts when its continuation is resumed by the caller's executor, awaits the
// child with the shared child environment, and counts itself out; the last one resumes the parent. The frame destroys
// itself before that, so nothing owns it.
class __join_runner {
public:
  struct promise_type {
    __join_core* __core_;
    execution_context::__frame_node __node_; // registered until the runner has finished, see execution_context

    template <class... _Args>
    _LIBCPP_HIDE_FROM_ABI explicit promise_type(__join_core* __c, _Args&&...) : __core_(__c) {
      __node_.__register(__c->__parent_ex_.context(), coroutine_handle<promise_type>::from_promise(*this));
    }

    _LIBCPP_HIDE_FROM_ABI static void* operator new(size_t __n) { return io::__frame_allocate(__n); }
    _LIBCPP_HIDE_FROM_ABI static void operator delete(void* __p, size_t __n) noexcept {
      io::__frame_deallocate(__p, __n);
    }

    struct __final_awaiter {
      _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
      _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<promise_type> __h) const noexcept {
        __join_core* __c = __h.promise().__core_;
        __h.destroy();
        if (__c->__remaining_.fetch_sub(1, memory_order_acq_rel) != 1)
          return noop_coroutine();
        executor_ref __ex = __c->__parent_ex_;
        return __ex.dispatch(__c->__parent_);
      }
      _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
    };

    template <class _Aw>
    struct __env_awaiter {
      _Aw& __a_;
      const io_env* __env_;
      _LIBCPP_HIDE_FROM_ABI bool await_ready() { return __a_.await_ready(); }
      _LIBCPP_HIDE_FROM_ABI decltype(auto) await_suspend(coroutine_handle<> __h) {
        return __a_.await_suspend(__h, __env_);
      }
      _LIBCPP_HIDE_FROM_ABI decltype(auto) await_resume() {
        io::set_cached_frame_allocator(__env_->frame_allocator);
        return __a_.await_resume();
      }
    };

    _LIBCPP_HIDE_FROM_ABI __join_runner get_return_object() noexcept {
      return __join_runner(coroutine_handle<promise_type>::from_promise(*this));
    }
    _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI __final_awaiter final_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI void return_void() noexcept {}
    _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept { __core_->__capture(std::current_exception()); }

    template <class _Aw>
      requires IoAwaitable<remove_cvref_t<_Aw>>
    _LIBCPP_HIDE_FROM_ABI auto await_transform(_Aw&& __a) noexcept {
      return __env_awaiter<remove_reference_t<_Aw>>{__a, &__core_->__env_};
    }
  };

  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> release() noexcept { return __h_; }

private:
  coroutine_handle<promise_type> __h_;
  _LIBCPP_HIDE_FROM_ABI explicit __join_runner(coroutine_handle<promise_type> __h) noexcept : __h_(__h) {}
};

// Posts the first __n continuations, in order, to the caller's executor. The state may be gone as soon as the last one
// has been posted.
_LIBCPP_HIDE_FROM_ABI inline void __join_post_all(continuation* __conts, size_t __n, executor_ref __ex) {
  for (size_t __i = 0; __i != __n; ++__i) {
    __conts[__i].__chained_ = true; // the runner is destroyed by its context, or ends itself
    __ex.post(__conts[__i]);
  }
}

// Creates runners with the caller's frame allocator in the out-of-band slot.
class __join_frame_scope {
  pmr::memory_resource* __saved_;

public:
  _LIBCPP_HIDE_FROM_ABI explicit __join_frame_scope(const io_env* __env) noexcept
      : __saved_(io::get_cached_frame_allocator()) {
    io::set_cached_frame_allocator(__env->frame_allocator);
  }
  _LIBCPP_HIDE_FROM_ABI ~__join_frame_scope() { io::set_cached_frame_allocator(__saved_); }
  __join_frame_scope(const __join_frame_scope&)            = delete;
  __join_frame_scope& operator=(const __join_frame_scope&) = delete;
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_JOIN_SUPPORT_H
