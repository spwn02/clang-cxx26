//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_RUN_ASYNC_H
#define _LIBCPP___IO_RUN_ASYNC_H

#include <__concepts/convertible_to.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__exception/exception_ptr.h>
#include <__exception/terminate.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/executor_ref.h>
#include <__io/frame_allocator.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__memory_resource/memory_resource.h>
#include <__stop_token/stop_token.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
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

// Marks a missing handler.
struct __no_handler {};

// The coroutine that owns one launched chain. It starts suspended, is started by a post to the executor, awaits the
// task with the launch environment, runs the handlers and destroys itself when it finishes.
struct __run_async_root {
  struct promise_type {
    continuation __start_;
    execution_context::__frame_node __node_; // registered until the chain has finished, see execution_context

    _LIBCPP_HIDE_FROM_ABI __run_async_root get_return_object() noexcept {
      return __run_async_root{coroutine_handle<promise_type>::from_promise(*this)};
    }
    _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI suspend_never final_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI void return_void() noexcept {}
    // Without an error handler the exception is rethrown out of resume(), that is, on the thread of the executor that
    // runs the chain. An executor that does not catch it terminates the program.
    _LIBCPP_HIDE_FROM_ABI void unhandled_exception() {
#  if _LIBCPP_HAS_EXCEPTIONS
      throw;
#  else
      std::terminate();
#  endif
    }
  };

  coroutine_handle<promise_type> __h_;
};

// Leaves the registry of the context when the chain has finished, before the work count is released (releasing it may
// let join() return and the context be destroyed while the frame is still ending).
struct __run_async_unregister {
  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
  _LIBCPP_HIDE_FROM_ABI bool await_suspend(coroutine_handle<__run_async_root::promise_type> __h) const noexcept {
    __h.promise().__node_.__unregister();
    return false;
  }
  _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
};

template <class _Task>
struct __run_async_start {
  _Task& __t_;
  const io_env* __env_;
  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<> __h) noexcept {
    __t_.handle().promise().set_continuation(__h);
    __t_.handle().promise().set_environment(__env_);
    return __t_.handle();
  }
  _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
};

template <class _Ex>
_LIBCPP_HIDE_FROM_ABI executor_ref __run_async_ref(const _Ex& __ex) noexcept {
  return executor_ref(__ex);
}
_LIBCPP_HIDE_FROM_ABI inline executor_ref __run_async_ref(const executor_ref& __ex) noexcept { return __ex; }

template <class _Ex>
concept __counts_work = requires(const _Ex& __e) {
  __e.on_work_started();
  __e.on_work_finished();
};

// The parameters are copied into the coroutine frame, which is why the executor outlives the chain.
template <class _Ex, class _Task, class _OnValue, class _OnError>
_LIBCPP_HIDE_FROM_ABI __run_async_root
__run_async_body(_Ex __ex, stop_token __st, pmr::memory_resource* __mr, _Task __t, _OnValue __ov, _OnError __oe) {
  io_env __env{io::__run_async_ref(__ex), std::move(__st), __mr};
  co_await __run_async_start<_Task>{__t, &__env};

  exception_ptr __ep = __t.handle().promise().exception();
  if (__ep) {
    if constexpr (same_as<_OnError, __no_handler>)
      std::rethrow_exception(__ep);
    else
      __oe(__ep);
  } else {
    if constexpr (is_void_v<decltype(__t.await_resume())>) {
      if constexpr (!same_as<_OnValue, __no_handler>)
        __ov();
    } else {
      if constexpr (!same_as<_OnValue, __no_handler>)
        __ov(std::move(__t.handle().promise().result()));
    }
  }

  // The task frame goes back to its allocator before the work is reported finished: join() may release the allocator.
  {
    _Task __done = std::move(__t);
  }
  co_await __run_async_unregister{};
  if constexpr (__counts_work<_Ex>)
    __ex.on_work_finished();
}

// Phase one of the two-phase launch: the constructor caches the frame allocator, so that the task that the second
// call receives is allocated from it.
template <class _Ex, class _OnValue, class _OnError>
class [[nodiscard]] __run_async_wrapper {
  _Ex __ex_;
  stop_token __st_;
  pmr::memory_resource* __mr_;
  _OnValue __ov_;
  _OnError __oe_;

public:
  _LIBCPP_HIDE_FROM_ABI
  __run_async_wrapper(_Ex __ex, stop_token __st, pmr::memory_resource* __mr, _OnValue __ov, _OnError __oe)
      : __ex_(std::move(__ex)), __st_(std::move(__st)), __mr_(__mr), __ov_(std::move(__ov)), __oe_(std::move(__oe)) {
    io::set_cached_frame_allocator(__mr_);
  }

  // Phase two: starts the task on the executor. The executor's work count is raised until the chain has finished.
  template <class _Task>
    requires IoRunnable<_Task>
  _LIBCPP_HIDE_FROM_ABI void operator()(_Task __t) && {
    io::set_cached_frame_allocator(nullptr);
    if constexpr (__counts_work<_Ex>)
      __ex_.on_work_started();
    __run_async_root __root =
        io::__run_async_body(__ex_, std::move(__st_), __mr_, std::move(__t), std::move(__ov_), std::move(__oe_));
    auto __h                          = __root.__h_;
    __h.promise().__start_.h          = __h;
    __h.promise().__start_.__chained_ = true;
    __h.promise().__node_.__register(io::__run_async_ref(__ex_).context(), __h);
    __ex_.post(__h.promise().__start_);
  }
};

template <class _Ex, class _OnValue, class _OnError>
_LIBCPP_HIDE_FROM_ABI auto
__run_async_finish(_Ex __ex, stop_token __st, pmr::memory_resource* __mr, _OnValue __ov, _OnError __oe) {
  if (!__mr)
    __mr = io::__run_async_ref(__ex).context().get_frame_allocator();
  return __run_async_wrapper<_Ex, _OnValue, _OnError>(
      std::move(__ex), std::move(__st), __mr, std::move(__ov), std::move(__oe));
}

// Peels the optional stop token and frame allocator, then the optional handlers (one handler serves both outcomes).
template <class _Ex>
_LIBCPP_HIDE_FROM_ABI auto __run_async_parse(_Ex __ex, stop_token __st, pmr::memory_resource* __mr) {
  return io::__run_async_finish(std::move(__ex), std::move(__st), __mr, __no_handler{}, __no_handler{});
}

template <class _Ex, class _A0, class... _Rest>
_LIBCPP_HIDE_FROM_ABI auto
__run_async_parse(_Ex __ex, stop_token __st, pmr::memory_resource* __mr, _A0&& __a0, _Rest&&... __rest) {
  if constexpr (same_as<remove_cvref_t<_A0>, stop_token>) {
    return io::__run_async_parse(std::move(__ex), std::forward<_A0>(__a0), __mr, std::forward<_Rest>(__rest)...);
  } else if constexpr (convertible_to<_A0, pmr::memory_resource*>) {
    return io::__run_async_parse(
        std::move(__ex), std::move(__st), static_cast<pmr::memory_resource*>(__a0), std::forward<_Rest>(__rest)...);
  } else if constexpr (sizeof...(_Rest) == 0) {
    remove_cvref_t<_A0> __h(std::forward<_A0>(__a0));
    remove_cvref_t<_A0> __h2(__h);
    return io::__run_async_finish(std::move(__ex), std::move(__st), __mr, std::move(__h), std::move(__h2));
  } else {
    static_assert(sizeof...(_Rest) == 1, "run_async takes at most an on_value and an on_error handler");
    return io::__run_async_finish(
        std::move(__ex),
        std::move(__st),
        __mr,
        remove_cvref_t<_A0>(std::forward<_A0>(__a0)),
        remove_cvref_t<_Rest>(std::forward<_Rest>(__rest))...);
  }
}

// P4003R3 [4.6]: run_async(executor-or-env [, stop_token] [, frame allocator] [, on_value, on_error])(task). Without
// handlers the result is discarded and an exception is rethrown on the executor's thread. With the executor form the
// executor's outstanding-work count is held for the duration of the chain.
template <class _Ex, class... _Args>
  requires Executor<_Ex>
_LIBCPP_HIDE_FROM_ABI auto run_async(_Ex __ex, _Args&&... __args) {
  return io::__run_async_parse(std::move(__ex), stop_token{}, nullptr, std::forward<_Args>(__args)...);
}

// The environment form copies the executor reference, the stop token and the frame allocator out of the environment
// (the executor must outlive the chain); no work is counted, since executor_ref cannot count it.
template <class... _Args>
_LIBCPP_HIDE_FROM_ABI auto run_async(const io_env& __env, _Args&&... __args) {
  return io::__run_async_parse(__env.executor, __env.stop_token, __env.frame_allocator, std::forward<_Args>(__args)...);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_RUN_ASYNC_H
