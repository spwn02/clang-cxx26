//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_RUN_H
#define _LIBCPP___IO_RUN_H

#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__exception/terminate.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/executor_ref.h>
#include <__io/frame_allocator.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__io/run_async.h>
#include <__memory/addressof.h>
#include <__memory_resource/memory_resource.h>
#include <__stop_token/stop_token.h>
#include <__type_traits/is_void.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The coroutine that runs a subtask on another executor: it starts the subtask there and, when the subtask completes,
// resumes the parent on the parent's executor and destroys itself.
struct __run_return {
  struct promise_type {
    executor_ref __parent_executor_;
    continuation* __parent_cont_ = nullptr;
    execution_context::__frame_node __node_; // registered with the context of the parent, see execution_context

    struct __final_awaiter {
      _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
      _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<promise_type> __h) const noexcept {
        executor_ref __ex    = __h.promise().__parent_executor_;
        continuation* __cont = __h.promise().__parent_cont_;
        __h.destroy();
        return __ex.dispatch(*__cont);
      }
      _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
    };

    _LIBCPP_HIDE_FROM_ABI __run_return get_return_object() noexcept {
      return __run_return{coroutine_handle<promise_type>::from_promise(*this)};
    }
    _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI __final_awaiter final_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI void return_void() noexcept {}
    _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept { std::terminate(); }
  };

  coroutine_handle<promise_type> __h_;
};

template <class _Task>
_LIBCPP_HIDE_FROM_ABI __run_return __run_trampoline(_Task* __t, const io_env* __env) {
  co_await __run_async_start<_Task>{*__t, __env};
}

struct __no_executor {};

// The awaitable of a run(...)(task) expression. It owns the subtask, the executor and the environment of the subtask.
// An unspecified stop token or frame allocator is inherited from the awaiting coroutine's environment; so is the
// executor.
template <class _Task, class _Ex>
class [[nodiscard]] __run_awaitable {
  _Task __t_;
  _Ex __ex_;
  stop_token __st_;
  pmr::memory_resource* __mr_;
  bool __has_st_;
  io_env __env_;
  continuation __parent_cont_;
  continuation __start_cont_;

public:
  _LIBCPP_HIDE_FROM_ABI __run_awaitable(_Task __t, _Ex __ex, stop_token __st, bool __has_st, pmr::memory_resource* __mr)
      : __t_(std::move(__t)), __ex_(std::move(__ex)), __st_(std::move(__st)), __mr_(__mr), __has_st_(__has_st) {}
  __run_awaitable(const __run_awaitable&)            = delete;
  __run_awaitable& operator=(const __run_awaitable&) = delete;

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<> __h, const io_env* __parent) {
    __env_.stop_token      = __has_st_ ? __st_ : __parent->stop_token;
    __env_.frame_allocator = __mr_ ? __mr_ : __parent->frame_allocator;
    if constexpr (same_as<_Ex, __no_executor>) {
      __env_.executor = __parent->executor;
      __t_.handle().promise().set_continuation(__h);
      __t_.handle().promise().set_environment(&__env_);
      return __t_.handle();
    } else {
      __env_.executor                           = executor_ref(__ex_);
      __parent_cont_.h                          = __h;
      __parent_cont_.__chained_                 = true;
      __run_return __tramp                      = io::__run_trampoline(std::addressof(__t_), std::addressof(__env_));
      __tramp.__h_.promise().__parent_executor_ = __parent->executor;
      __tramp.__h_.promise().__parent_cont_     = &__parent_cont_;
      __start_cont_.h                           = __tramp.__h_;
      __start_cont_.__chained_                  = true;
      __tramp.__h_.promise().__node_.__register(__parent->executor.context(), __tramp.__h_);
      return __env_.executor.dispatch(__start_cont_);
    }
  }

  _LIBCPP_HIDE_FROM_ABI decltype(auto) await_resume() { return __t_.await_resume(); }
};

// Phase one of run(...)(task); the constructor caches the frame allocator when one is given.
template <class _Ex>
class [[nodiscard]] __run_wrapper {
  _Ex __ex_;
  stop_token __st_;
  bool __has_st_;
  pmr::memory_resource* __mr_;
  pmr::memory_resource* __saved_;

public:
  _LIBCPP_HIDE_FROM_ABI __run_wrapper(_Ex __ex, stop_token __st, bool __has_st, pmr::memory_resource* __mr)
      : __ex_(std::move(__ex)),
        __st_(std::move(__st)),
        __has_st_(__has_st),
        __mr_(__mr),
        __saved_(io::get_cached_frame_allocator()) {
    if (__mr_)
      io::set_cached_frame_allocator(__mr_);
  }

  template <class _Task>
    requires IoRunnable<_Task>
  _LIBCPP_HIDE_FROM_ABI auto operator()(_Task __t) && {
    io::set_cached_frame_allocator(__saved_);
    return __run_awaitable<_Task, _Ex>(std::move(__t), std::move(__ex_), std::move(__st_), __has_st_, __mr_);
  }
};

// P4003R3 [4.6]: co_await run(...)(task) runs a subtask with a switched executor, stop token and/or frame allocator;
// whatever is not given is inherited. The parent resumes on its own executor when the subtask completes.
template <class _Ex>
  requires Executor<_Ex>
_LIBCPP_HIDE_FROM_ABI auto run(_Ex __ex) {
  return __run_wrapper<_Ex>(std::move(__ex), stop_token{}, false, nullptr);
}
template <class _Ex>
  requires Executor<_Ex>
_LIBCPP_HIDE_FROM_ABI auto run(_Ex __ex, stop_token __st) {
  return __run_wrapper<_Ex>(std::move(__ex), std::move(__st), true, nullptr);
}
template <class _Ex>
  requires Executor<_Ex>
_LIBCPP_HIDE_FROM_ABI auto run(_Ex __ex, pmr::memory_resource* __mr) {
  return __run_wrapper<_Ex>(std::move(__ex), stop_token{}, false, __mr);
}
template <class _Ex>
  requires Executor<_Ex>
_LIBCPP_HIDE_FROM_ABI auto run(_Ex __ex, stop_token __st, pmr::memory_resource* __mr) {
  return __run_wrapper<_Ex>(std::move(__ex), std::move(__st), true, __mr);
}
_LIBCPP_HIDE_FROM_ABI inline auto run(stop_token __st) {
  return __run_wrapper<__no_executor>(__no_executor{}, std::move(__st), true, nullptr);
}
_LIBCPP_HIDE_FROM_ABI inline auto run(pmr::memory_resource* __mr) {
  return __run_wrapper<__no_executor>(__no_executor{}, stop_token{}, false, __mr);
}
_LIBCPP_HIDE_FROM_ABI inline auto run(stop_token __st, pmr::memory_resource* __mr) {
  return __run_wrapper<__no_executor>(__no_executor{}, std::move(__st), true, __mr);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_RUN_H
