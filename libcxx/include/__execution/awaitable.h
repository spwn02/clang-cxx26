//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_AWAITABLE_H
#define _LIBCPP___EXECUTION_AWAITABLE_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_void.h>
#include <__utility/declval.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.awaitable]p2: GET-AWAITER(c)'s unspecified empty "none-such" promise. It must lack
// an await_transform member (so __get_awaiter's await_transform-lookup step below correctly
// finds nothing) and coroutine_handle<__exec_none_such> must behave as coroutine_handle<void>
// -- true by construction, since forming coroutine_handle<P> imposes no requirements on P.
struct __exec_none_such {};

template <class _Tp>
inline constexpr bool __is_specialization_of_coroutine_handle = false;
template <class _Promise>
inline constexpr bool __is_specialization_of_coroutine_handle<coroutine_handle<_Promise>> = true;

// [exec.awaitable]p3: await-suspend-result<T>.
template <class _Tp>
concept __await_suspend_result = is_void_v<_Tp> || is_same_v<_Tp, bool> || __is_specialization_of_coroutine_handle<_Tp>;

// [exec.awaitable]p2: the operator co_await half of GET-AWAITER -- member operator co_await,
// then non-member operator co_await, then the identity fallback.
template <class _Awaiter>
_LIBCPP_HIDE_FROM_ABI decltype(auto) __exec_co_await_transform(_Awaiter&& __a) {
  if constexpr (requires { std::forward<_Awaiter>(__a).operator co_await(); }) {
    return std::forward<_Awaiter>(__a).operator co_await();
  } else if constexpr (requires { operator co_await(std::forward<_Awaiter>(__a)); }) {
    return operator co_await(std::forward<_Awaiter>(__a));
  } else {
    return std::forward<_Awaiter>(__a);
  }
}

// [exec.awaitable]p2: GET-AWAITER(c, p) -- the series of transformations applied to `c` as
// the operand of an await-expression in a coroutine whose promise `p` has type Promise.
//
// The standard's rule for the await_transform step is "if this search is performed and
// finds at least one declaration, a = p.await_transform(c)" -- meaning a promise declaring
// *some* await_transform member that is not callable with this particular `c` is a hard
// (non-SFINAE) error, not a fallback to `a = c`. `requires{ p.await_transform(c); }` cannot
// distinguish "no such member" from "member exists but unusable here"; this implementation
// accepts that divergence -- the same class of approximation as __valid_completion_for's
// callable-for-invocable substitution (receiver.h) -- rather than reproducing member-lookup
// fidelity.
template <class _Cp, class _Promise>
_LIBCPP_HIDE_FROM_ABI decltype(auto) __get_awaiter(_Cp&& __c, _Promise& __p) {
  if constexpr (requires { __p.await_transform(std::forward<_Cp>(__c)); }) {
    return execution::__exec_co_await_transform(__p.await_transform(std::forward<_Cp>(__c)));
  } else {
    return execution::__exec_co_await_transform(std::forward<_Cp>(__c));
  }
}

// [exec.awaitable]p2: GET-AWAITER(c) == GET-AWAITER(c, q) for an unspecified none-such q.
template <class _Cp>
_LIBCPP_HIDE_FROM_ABI decltype(auto) __get_awaiter(_Cp&& __c) {
  __exec_none_such __q;
  return execution::__get_awaiter(std::forward<_Cp>(__c), __q);
}

// [exec.awaitable]p3: is-awaiter.
template <class _Ap, class... _Promise>
concept __is_awaiter = requires(_Ap& __a, coroutine_handle<_Promise...> __h) {
  __a.await_ready() ? 1 : 0;
  { __a.await_suspend(__h) } -> __await_suspend_result;
  __a.await_resume();
};

// [exec.awaitable]p3: is-awaitable. Relies on overload resolution over the two __get_awaiter
// overloads above to select GET-AWAITER(fc(), p...)'s one- or two-argument form based on
// whether the _Promise pack is empty.
template <class _Cp, class... _Promise>
concept __is_awaitable = requires(_Cp (*__fc)() noexcept, _Promise&... __p) {
  { execution::__get_awaiter(__fc(), __p...) } -> __is_awaiter<_Promise...>;
};

// [exec.awaitable]p4: await-result-type<C, Promise> and await-result-type<C>, unified via a
// defaulted second parameter -- __get_awaiter(c, __exec_none_such&) and __get_awaiter(c) are
// equivalent by construction (the latter simply supplies its own none-such internally).
template <class _Cp, class _Promise = __exec_none_such>
using __await_result_type = decltype(execution::__get_awaiter(std::declval<_Cp>(), std::declval<_Promise&>()).await_resume());

// [exec.awaitable]p5: with-await-transform.
template <class _Tp, class _Promise>
concept __has_as_awaitable = requires(_Tp&& __t, _Promise& __p) {
  { std::forward<_Tp>(__t).as_awaitable(__p) } -> __is_awaitable<_Promise&>;
};

template <class _Derived>
struct __with_await_transform { // [exec.awaitable]p5: with-await-transform
  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI _Tp&& await_transform(_Tp&& __value) noexcept {
    return std::forward<_Tp>(__value);
  }

  template <class _Tp>
    requires __has_as_awaitable<_Tp, _Derived>
  _LIBCPP_HIDE_FROM_ABI auto await_transform(_Tp&& __value) noexcept(
      noexcept(std::forward<_Tp>(__value).as_awaitable(std::declval<_Derived&>())))
      -> decltype(std::forward<_Tp>(__value).as_awaitable(std::declval<_Derived&>())) {
    return std::forward<_Tp>(__value).as_awaitable(static_cast<_Derived&>(*this));
  }
};

// [exec.awaitable]p6: env-promise. Specializations are used only for type computation; per
// the standard's own note, member bodies need not be defined -- these declarations are only
// ever named in unevaluated (decltype/requires) contexts, never actually driving a real
// coroutine, so leaving get_return_object/initial_suspend/final_suspend/unhandled_exception/
// return_void undefined (with the standard's "unspecified" return types collapsed to `void`,
// since nothing in this fork's scope inspects them) is well-formed: no ODR-use ever occurs.
template <class _Env>
struct __env_promise : __with_await_transform<__env_promise<_Env>> {
  _LIBCPP_HIDE_FROM_ABI void get_return_object() noexcept;
  _LIBCPP_HIDE_FROM_ABI void initial_suspend() noexcept;
  _LIBCPP_HIDE_FROM_ABI void final_suspend() noexcept;
  _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept;
  _LIBCPP_HIDE_FROM_ABI void return_void() noexcept;
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> unhandled_stopped() noexcept;

  _LIBCPP_HIDE_FROM_ABI const _Env& get_env() const noexcept;
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_AWAITABLE_H
