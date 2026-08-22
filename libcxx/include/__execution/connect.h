//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_CONNECT_H
#define _LIBCPP___EXECUTION_CONNECT_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__execution/awaitable.h>
#include <__execution/completion_functions.h>
#include <__execution/domain.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__type_traits/is_void.h>
#include <__utility/declval.h>
#include <__utility/exchange.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__utility/unreachable.h>
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

template <class _Sndr, class _Rcvr>
_LIBCPP_HIDE_FROM_ABI constexpr auto __connect_impl(_Sndr&& __sndr, _Rcvr&& __rcvr)
    -> decltype(execution::transform_sender(std::forward<_Sndr>(__sndr), execution::get_env(__rcvr))
                    .connect(std::forward<_Rcvr>(__rcvr))) {
  return execution::transform_sender(std::forward<_Sndr>(__sndr), execution::get_env(__rcvr))
      .connect(std::forward<_Rcvr>(__rcvr));
}

// [exec.connect] (6.2): connect-awaitable(sndr, rcvr) -- wraps an awaitable sender in a
// coroutine-backed operation state, used only when (6.1)'s member `.connect()` isn't
// viable. `_DS`/`_DR` are the standard's DS/DR (decayed new_sndr/Rcvr).

// Forward-declared: connect-awaitable-promise::get_return_object names operation-state-task
// before the latter's full definition; the mutual reference resolves because neither
// template is actually instantiated until __connect_awaitable (defined last in this file)
// is itself instantiated, by which point both are complete.
template <class _DS, class _DR>
struct __operation_state_task;

// [exec.connect]p3: connect-awaitable-promise. Bodies of final_suspend/unhandled_exception/
// return_void are unreachable in practice -- the coroutine body below only ever suspends at
// __suspend_complete's awaiter, whose await_suspend never resumes the handle -- but are
// spelled out (and marked [[noreturn]]) to match the standard's own defensive text exactly.
template <class _DS, class _DR>
struct __connect_awaitable_promise : __with_await_transform<__connect_awaitable_promise<_DS, _DR>> {
  _LIBCPP_HIDE_FROM_ABI __connect_awaitable_promise(_DS&, _DR& __rcvr) noexcept : __rcvr_(__rcvr) {}

  _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() noexcept { return {}; }
  [[noreturn]] _LIBCPP_HIDE_FROM_ABI suspend_always final_suspend() noexcept { std::terminate(); }
  [[noreturn]] _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept { std::terminate(); }
  [[noreturn]] _LIBCPP_HIDE_FROM_ABI void return_void() noexcept { std::terminate(); }

  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> unhandled_stopped() noexcept {
    execution::set_stopped(std::move(__rcvr_));
    return noop_coroutine();
  }

  _LIBCPP_HIDE_FROM_ABI __operation_state_task<_DS, _DR> get_return_object() noexcept {
    return __operation_state_task<_DS, _DR>{coroutine_handle<__connect_awaitable_promise>::from_promise(*this)};
  }

  _LIBCPP_HIDE_FROM_ABI env_of_t<_DR> get_env() const noexcept { return execution::get_env(__rcvr_); }

private:
  _DR& __rcvr_;
};

// [exec.connect]p4: operation-state-task.
template <class _DS, class _DR>
struct __operation_state_task {
  using operation_state_concept = operation_state_tag;
  using promise_type            = __connect_awaitable_promise<_DS, _DR>;

  _LIBCPP_HIDE_FROM_ABI explicit __operation_state_task(coroutine_handle<> __h) noexcept : __coro_(__h) {}
  _LIBCPP_HIDE_FROM_ABI __operation_state_task(__operation_state_task&& __o) noexcept
      : __coro_(std::exchange(__o.__coro_, {})) {}
  _LIBCPP_HIDE_FROM_ABI ~__operation_state_task() {
    if (__coro_)
      __coro_.destroy();
  }

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { __coro_.resume(); }

private:
  coroutine_handle<> __coro_;
};

// [exec.connect]p5: V and Sigs, needed both by connect-awaitable's own requires-clause and
// by its body.
template <class _DS, class _DR>
using __connect_awaitable_value_t = __await_result_type<_DS, __connect_awaitable_promise<_DS, _DR>>;

template <class _DS, class _DR>
using __connect_awaitable_sigs =
    completion_signatures<__set_value_sig_t<__connect_awaitable_value_t<_DS, _DR>>, set_error_t(exception_ptr),
                           set_stopped_t()>;

// [exec.connect]p5: suspend-complete(fun, as...). The lambda's explicit `__fun` capture
// (redundant with the by-reference default) matches the standard's own `[&, fun]` --
// forcing `fun` to be captured by value while `as...` stays by reference is deliberate: the
// awaiter is only ever used as the immediate operand of `co_await` within the same full
// expression as this call, so the by-reference captures of `as...` remain valid for the
// awaiter's entire lifetime even though they refer to this function's own (otherwise
// short-lived) parameters.
template <class _Fun, class... _Ts>
_LIBCPP_HIDE_FROM_ABI auto __suspend_complete(_Fun __fun, _Ts&&... __as) noexcept {
  auto __fn = [&, __fun]() noexcept { __fun(std::forward<_Ts>(__as)...); };

  struct __awaiter {
    decltype(__fn) __fn_;

    _LIBCPP_HIDE_FROM_ABI static constexpr bool await_ready() noexcept { return false; }
    _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<>) noexcept { __fn_(); }
    [[noreturn]] _LIBCPP_HIDE_FROM_ABI void await_resume() noexcept { std::unreachable(); }
  };
  return __awaiter{__fn};
}

// [exec.connect]p5: connect-awaitable.
template <class _DS, class _DR>
  requires receiver_of<_DR, __connect_awaitable_sigs<_DS, _DR>>
_LIBCPP_HIDE_FROM_ABI __operation_state_task<_DS, _DR> __connect_awaitable(_DS __sndr, _DR __rcvr) {
  using _V = __connect_awaitable_value_t<_DS, _DR>;
  exception_ptr __ep;
  try {
    if constexpr (is_void_v<_V>) {
      co_await std::move(__sndr);
      co_await execution::__suspend_complete(execution::set_value, std::move(__rcvr));
    } else {
      co_await execution::__suspend_complete(execution::set_value, std::move(__rcvr), co_await std::move(__sndr));
    }
  } catch (...) {
    __ep = std::current_exception();
  }
  co_await execution::__suspend_complete(execution::set_error, std::move(__rcvr), std::move(__ep));
}

// [exec.connect]p6.2: connect-awaitable(new_sndr, rcvr), computing new_sndr/DS/DR the same
// way __connect_impl does for (6.1) -- see that function's own comment on recomputing
// transform_sender per-branch rather than sharing a single new_sndr, a pre-existing
// deviation this mirrors rather than fixes.
template <class _Sndr, class _Rcvr>
_LIBCPP_HIDE_FROM_ABI constexpr auto __connect_awaitable_impl(_Sndr&& __sndr, _Rcvr&& __rcvr) -> decltype(execution::__connect_awaitable(
    execution::transform_sender(std::forward<_Sndr>(__sndr), execution::get_env(__rcvr)), std::forward<_Rcvr>(__rcvr))) {
  return execution::__connect_awaitable(
      execution::transform_sender(std::forward<_Sndr>(__sndr), execution::get_env(__rcvr)), std::forward<_Rcvr>(__rcvr));
}

// [exec.connect]p6: dispatch between (6.1) member-connect and (6.2) connect-awaitable, via
// two overloads rather than if-constexpr so each branch's own noexcept-specifier is only
// ever substituted for the branch that's actually viable -- connect_t::operator()'s
// noexcept(noexcept(...)) below would otherwise need to name whichever branch's expression,
// which isn't always well-formed for the branch not taken.
template <class _Sndr, class _Rcvr>
  requires(!requires(_Sndr&& __sndr, _Rcvr&& __rcvr) {
    { execution::__connect_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr)) } -> operation_state;
  })
_LIBCPP_HIDE_FROM_ABI constexpr auto __connect_dispatch(_Sndr&& __sndr, _Rcvr&& __rcvr)
    -> decltype(execution::__connect_awaitable_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr))) {
  return execution::__connect_awaitable_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr));
}

template <class _Sndr, class _Rcvr>
_LIBCPP_HIDE_FROM_ABI constexpr auto __connect_dispatch(_Sndr&& __sndr, _Rcvr&& __rcvr)
    -> decltype(execution::__connect_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr))) {
  return execution::__connect_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr));
}

struct connect_t {
  template <class _Sndr, class _Rcvr>
    requires sender<_Sndr> && receiver<_Rcvr> && requires(_Sndr&& __sndr, _Rcvr&& __rcvr) {
      { execution::__connect_dispatch(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr)) } -> operation_state;
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Rcvr&& __rcvr) const
      noexcept(noexcept(execution::__connect_dispatch(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr)))) {
    static_assert(sender_in<_Sndr, env_of_t<_Rcvr>>, "Mandates: sender_in<Sndr, env_of_t<Rcvr>>.");
    static_assert(receiver_of<_Rcvr, completion_signatures_of_t<_Sndr, env_of_t<_Rcvr>>>,
                  "Mandates: receiver_of<Rcvr, completion_signatures_of_t<Sndr, env_of_t<Rcvr>>>.");
    return execution::__connect_dispatch(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr));
  }
};

inline constexpr connect_t connect{};

template <class _Sndr, class _Rcvr>
using connect_result_t = decltype(execution::connect(std::declval<_Sndr>(), std::declval<_Rcvr>()));

// [exec.snd.concepts]: sender-to.
template <class _Sndr, class _Rcvr>
concept __sender_to = sender_in<_Sndr, env_of_t<_Rcvr>> && receiver_of<_Rcvr, completion_signatures_of_t<_Sndr, env_of_t<_Rcvr>>> &&
                       requires(_Sndr&& __sndr, _Rcvr&& __rcvr) {
                         execution::connect(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr));
                       };

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_CONNECT_H
