//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_TIMER_TIMER_H
#define _LIBCPP___IO_TIMER_TIMER_H

#include <__chrono/duration.h>
#include <__chrono/steady_clock.h>
#include <__chrono/time_point.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/io_env.h>
#include <__io/support/config.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/move.h>

#if _LIBCPP_HAS_IO_REACTOR
#  include <__io_context/op.h>
#  include <__io_context/reactor.h>
#  include <__io_stream/error.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_REACTOR

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.8] (Paper 8); provisional specification: Corosio's timer. A timer has an expiry time on steady_clock and
// any number of coroutines can wait for it. It belongs to an io_context (the constructors take the context or one of
// its executors) and its waits are completed by the threads that run that context: the awaiting coroutine is resumed on
// its own executor.
//
// A wait that is cancelled (by cancel(), cancel_one(), by changing the expiry, by destroying the timer or by a stop
// request on the stop token of the awaiting coroutine) completes with error::canceled; a wait on a timer whose expiry
// has passed completes without suspending. The precision is that of the platform: see __reactor_service.
//
// Distinct timers can be used concurrently; the member functions of one timer must not be called concurrently, with the
// exception that cancel() may be called while another thread awaits. The context must outlive the timer, and a timer
// must not be destroyed while a coroutine is suspended on it except through cancel() (the destructor does that).
class timer {
public:
  using clock_type = chrono::steady_clock;
  using time_point = clock_type::time_point;
  using duration   = clock_type::duration;

private:
  __reactor_service* __svc_;
  time_point __expiry_{}; // the epoch: already expired
  __op_list __waiters_;

  _LIBCPP_HIDE_FROM_ABI static time_point __after(time_point __now, duration __d) noexcept {
    if (__d > duration::zero() && __now > time_point::max() - __d)
      return time_point::max();
    return __now + __d;
  }

public:
  class __wait_awaiter : public __timer_op {
    timer* __t_;

  public:
    _LIBCPP_HIDE_FROM_ABI explicit __wait_awaiter(timer& __t) noexcept : __timer_op(__t.__svc_), __t_(&__t) {}
    _LIBCPP_HIDE_FROM_ABI __wait_awaiter(__wait_awaiter&& __o) noexcept
        : __timer_op(std::move(__o)), __t_(__o.__t_) {} // NOLINT(bugprone-use-after-move)

    _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

    _LIBCPP_HIDE_FROM_ABI bool await_suspend(coroutine_handle<> __h, const io_env* __env) {
      __bind(__h, __env);
      if (__env->stop_token.stop_requested()) {
        __ec_ = make_error_code(error::canceled);
        return false;
      }
      __deadline_ = __t_->__expiry_;
      if (__deadline_ <= clock_type::now()) {
        __ec_ = error_code();
        return false;
      }
      __arm_stop(__env);
      return __svc_->__start_timer(*this, __t_->__waiters_);
    }

    _LIBCPP_HIDE_FROM_ABI io_result<> await_resume() noexcept {
      __finish();
      return io_result<>(__ec_);
    }
  };

  _LIBCPP_HIDE_FROM_ABI explicit timer(execution_context& __ctx) : __svc_(&__ctx.use_service<__reactor_service>()) {}
  _LIBCPP_HIDE_FROM_ABI timer(execution_context& __ctx, time_point __t) : timer(__ctx) { __expiry_ = __t; }
  _LIBCPP_HIDE_FROM_ABI timer(execution_context& __ctx, duration __d) : timer(__ctx) {
    __expiry_ = __after(clock_type::now(), __d);
  }

  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, timer>)
  _LIBCPP_HIDE_FROM_ABI explicit timer(const _Ex& __ex) : timer(__ex.context()) {}
  template <Executor _Ex>
  _LIBCPP_HIDE_FROM_ABI timer(const _Ex& __ex, time_point __t) : timer(__ex.context(), __t) {}
  template <Executor _Ex>
  _LIBCPP_HIDE_FROM_ABI timer(const _Ex& __ex, duration __d) : timer(__ex.context(), __d) {}

  timer(const timer&)            = delete;
  timer& operator=(const timer&) = delete;

  // Precondition: no wait is pending on __o.
  _LIBCPP_HIDE_FROM_ABI timer(timer&& __o) noexcept : __svc_(__o.__svc_), __expiry_(__o.__expiry_) {}
  // Cancels the waits of *this; precondition: no wait is pending on __o.
  _LIBCPP_HIDE_FROM_ABI timer& operator=(timer&& __o) noexcept {
    if (this != &__o) {
      cancel();
      __svc_    = __o.__svc_;
      __expiry_ = __o.__expiry_;
    }
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI ~timer() { cancel(); }

  _LIBCPP_HIDE_FROM_ABI time_point expiry() const noexcept { return __expiry_; }

  // Cancels the pending waits (they complete with error::canceled), sets the expiry and returns the number of waits
  // that were cancelled.
  _LIBCPP_HIDE_FROM_ABI size_t expires_at(time_point __t) noexcept {
    size_t __n = cancel();
    __expiry_  = __t;
    return __n;
  }
  _LIBCPP_HIDE_FROM_ABI size_t expires_after(duration __d) noexcept {
    return expires_at(__after(clock_type::now(), __d));
  }

  // Cancels all pending waits; returns their number.
  _LIBCPP_HIDE_FROM_ABI size_t cancel() noexcept { return __svc_->__cancel_timers(__waiters_, false); }
  // Cancels the oldest pending wait, if any; returns 0 or 1.
  _LIBCPP_HIDE_FROM_ABI size_t cancel_one() noexcept { return __svc_->__cancel_timers(__waiters_, true); }

  // An IoAwaitable that completes, with io_result<> (an error_code), when the expiry has passed. The timer must outlive
  // the awaitable.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI __wait_awaiter wait() noexcept { return __wait_awaiter(*this); }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_REACTOR

#endif // _LIBCPP___IO_TIMER_TIMER_H
