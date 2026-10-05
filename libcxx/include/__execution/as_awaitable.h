//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_AS_AWAITABLE_H
#define _LIBCPP___EXECUTION_AS_AWAITABLE_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__execution/awaitable.h>
#include <__execution/completion_functions.h>
#include <__execution/connect.h>
#include <__execution/domain.h>
#include <__execution/fwd_env.h>
#include <__execution/get_await_completion_adaptor.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_reference.h>
#include <__type_traits/remove_cvref.h>
#include <__memory/addressof.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <exception>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.as.awaitable]p2-6: sender-awaitable and its nested awaitable-receiver. Wraps a
// single-valued sender in an awaiter so it can be `co_await`ed from a coroutine whose
// promise is `Promise`.
template <class _Sndr, class _Promise>
class __sender_awaitable {
  struct __unit {};
  using __value_type  = __single_sender_value_type<_Sndr, env_of_t<_Promise>>;
  using __result_type = conditional_t<is_void_v<__value_type>, __unit, __value_type>;

  struct __awaitable_receiver {
    using receiver_concept = receiver_tag;

    variant<monostate, __result_type, exception_ptr>* __result_;
    coroutine_handle<_Promise> __continuation_;

    template <class... _Args>
    _LIBCPP_HIDE_FROM_ABI void set_value(_Args&&... __args) && noexcept {
      try {
        __result_->template emplace<1>(std::forward<_Args>(__args)...);
      } catch (...) {
        __result_->template emplace<2>(std::current_exception());
      }
      __continuation_.resume();
    }

    template <class _Err>
    _LIBCPP_HIDE_FROM_ABI void set_error(_Err&& __err) && noexcept {
      try {
        __result_->template emplace<2>(execution::__as_except_ptr(std::forward<_Err>(__err)));
      } catch (...) {
        __result_->template emplace<2>(std::current_exception());
      }
      __continuation_.resume();
    }

    // [exec.as.awaitable]p4.3: treated as if an uncatchable "stopped" exception were thrown
    // from the await-expression -- the coroutine is never resumed; the caller's
    // unhandled_stopped() is invoked instead. Requires Promise to provide unhandled_stopped()
    // (only instantiated -- see __awaitable_receiver::set_stopped's callers -- if the
    // connected sender's completions actually include set_stopped_t; the standard's
    // `awaitable-sender` concept would SFINAE this earlier, but that concept isn't
    // implemented here (see as_awaitable_t's dispatch comment below), so a Promise lacking
    // unhandled_stopped() surfaces as a hard error only when a sender that can actually
    // complete with set_stopped is awaited).
    _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept {
      static_cast<coroutine_handle<>>(__continuation_.promise().unhandled_stopped()).resume();
    }

    _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept {
      return execution::__fwd_env_fn(execution::get_env(__continuation_.promise()));
    }
  };

  variant<monostate, __result_type, exception_ptr> __result_{};
  connect_result_t<_Sndr, __awaitable_receiver> __state_;

public:
  _LIBCPP_HIDE_FROM_ABI __sender_awaitable(_Sndr&& __sndr, _Promise& __p)
      : __state_(execution::connect(
            std::forward<_Sndr>(__sndr),
            __awaitable_receiver{std::addressof(__result_), coroutine_handle<_Promise>::from_promise(__p)})) {}

  _LIBCPP_HIDE_FROM_ABI static constexpr bool await_ready() noexcept { return false; }
  _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<_Promise>) noexcept { execution::start(__state_); }

  _LIBCPP_HIDE_FROM_ABI __value_type await_resume() {
    if (__result_.index() == 2) {
      std::rethrow_exception(std::get<2>(std::move(__result_)));
    }
    if constexpr (!is_void_v<__value_type>) {
      return std::forward<__value_type>(std::get<1>(__result_));
    }
  }
};

// [exec.as.awaitable]p8: adapt-for-await-completion(s) is get_await_completion_adaptor(get_env(s))(s) if that is
// well-formed, except that s is evaluated only once, and s otherwise.
template <class _Sndr>
  requires requires(_Sndr&& __s) { execution::get_await_completion_adaptor(execution::get_env(__s))(__s); }
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __adapt_for_await_completion(_Sndr&& __s) {
  return execution::get_await_completion_adaptor(execution::get_env(__s))(__s);
}

template <class _Sndr>
  requires(!requires(_Sndr&& __s) { execution::get_await_completion_adaptor(execution::get_env(__s))(__s); })
_LIBCPP_HIDE_FROM_ABI constexpr _Sndr&& __adapt_for_await_completion(_Sndr&& __s) noexcept {
  return std::forward<_Sndr>(__s);
}

// [exec.as.awaitable]p7: as_awaitable(expr, p) is expr.as_awaitable(p) if that is well-formed (7.1); otherwise
// adapt-for-await-completion(transform_sender(expr, get_env(p))).as_awaitable(p) if that is well-formed and expr is a
// sender in env_of_t<Promise> with a single-sender-value-type (7.2); otherwise (void(p), expr) if GET-AWAITER(expr) is an
// awaiter for Promise (7.3); otherwise sender-awaitable{adapt-for-await-completion(transform_sender(expr, get_env(p))), p}
// for such a sender (7.4), and (void(p), expr) (7.5). The awaitable-sender concept of the exposition is not needed by
// the dispatch below.
template <class _Expr, class _Promise>
concept __has_adapted_as_awaitable = sender_in<_Expr, env_of_t<_Promise>> &&
                                      requires { typename __single_sender_value_type<_Expr, env_of_t<_Promise>>; } &&
                                      requires(_Expr&& __expr, _Promise& __p) {
                                        execution::__adapt_for_await_completion(
                                            execution::transform_sender(std::forward<_Expr>(__expr), execution::get_env(__p)))
                                            .as_awaitable(__p);
                                      };

struct as_awaitable_t {
  template <class _Expr, class _Promise>
  _LIBCPP_HIDE_FROM_ABI auto operator()(_Expr&& __expr, _Promise& __p) const -> decltype(auto) {
    if constexpr (requires { std::forward<_Expr>(__expr).as_awaitable(__p); }) {
      // (7.1)
      using __a_t = decltype(std::forward<_Expr>(__expr).as_awaitable(__p));
      static_assert(__is_awaitable<__a_t, _Promise>, "Mandates: is-awaitable<A, Promise>.");
      return std::forward<_Expr>(__expr).as_awaitable(__p);
    } else if constexpr (__has_adapted_as_awaitable<_Expr, _Promise>) {
      // (7.2)
      return execution::__adapt_for_await_completion(
                 execution::transform_sender(std::forward<_Expr>(__expr), execution::get_env(__p)))
          .as_awaitable(__p);
    } else if constexpr (requires {
                            { execution::__get_awaiter(std::forward<_Expr>(__expr)) } -> __is_awaiter<_Promise>;
                          }) {
      // (7.3): already directly awaitable, without going through Promise's await_transform.
      return static_cast<_Expr&&>(__expr);
    } else if constexpr (sender_in<_Expr, env_of_t<_Promise>> &&
                          requires { typename __single_sender_value_type<_Expr, env_of_t<_Promise>>; }) {
      // (7.4)
      using _AdaptedRef = decltype(execution::__adapt_for_await_completion(
          execution::transform_sender(std::forward<_Expr>(__expr), execution::get_env(__p))));
      // the sender is moved into the awaitable: name the object type of an rvalue, as for the transformed sender itself
      using _Adapted = conditional_t<is_rvalue_reference_v<_AdaptedRef>, remove_reference_t<_AdaptedRef>, _AdaptedRef>;
      return __sender_awaitable<_Adapted, _Promise>{
          execution::__adapt_for_await_completion(
              execution::transform_sender(std::forward<_Expr>(__expr), execution::get_env(__p))),
          __p};
    } else {
      // (7.5)
      return static_cast<_Expr&&>(__expr);
    }
  }
};

inline constexpr as_awaitable_t as_awaitable{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_AS_AWAITABLE_H
