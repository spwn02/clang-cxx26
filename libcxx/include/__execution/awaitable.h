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
#include <__type_traits/is_class.h>
#include <__type_traits/is_final.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_void.h>
#include <__utility/declval.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

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

// [exec.awaitable]p2: the operator co_await half of GET-AWAITER -- the operator co_await picked by overload
// resolution among the member and the non-member candidates, if any, otherwise the operand itself ([expr.await]).
// Overload resolution between a member and a non-member candidate is not reproduced: if both are viable the
// expression is treated as ambiguous (ill-formed), which is what happens whenever their conversion sequences for the
// operand are equal. A candidate that is better for the operand's value category (e.g. a `&&`-qualified member against
// a non-member taking `const X&`) is therefore wrongly rejected.
template <class _Awaiter>
concept __has_member_co_await = requires(_Awaiter&& __a) { std::forward<_Awaiter>(__a).operator co_await(); };
template <class _Awaiter>
concept __has_non_member_co_await = requires(_Awaiter&& __a) { operator co_await(std::forward<_Awaiter>(__a)); };

template <class _Awaiter>
  requires __has_member_co_await<_Awaiter> && (!__has_non_member_co_await<_Awaiter>)
_LIBCPP_HIDE_FROM_ABI auto __exec_co_await_transform(_Awaiter&& __a)
    -> decltype(std::forward<_Awaiter>(__a).operator co_await()) {
  return std::forward<_Awaiter>(__a).operator co_await();
}

template <class _Awaiter>
  requires(!__has_member_co_await<_Awaiter>) && __has_non_member_co_await<_Awaiter>
_LIBCPP_HIDE_FROM_ABI auto __exec_co_await_transform(_Awaiter&& __a)
    -> decltype(operator co_await(std::forward<_Awaiter>(__a))) {
  return operator co_await(std::forward<_Awaiter>(__a));
}

template <class _Awaiter>
  requires(!__has_member_co_await<_Awaiter>) && (!__has_non_member_co_await<_Awaiter>)
_LIBCPP_HIDE_FROM_ABI _Awaiter&& __exec_co_await_transform(_Awaiter&& __a) noexcept {
  return std::forward<_Awaiter>(__a);
}

// Whether member lookup of `await_transform` in a promise type finds a declaration: a name that is declared in both a
// base class and the promise is ambiguous in a class derived from the two, whatever it declares.
struct __await_transform_probe {
  void await_transform();
};
template <class _Promise>
struct __await_transform_probe_derived : _Promise, __await_transform_probe {};
template <class _Promise>
concept __has_await_transform_member =
    is_class_v<_Promise> && (!is_final_v<_Promise>) &&
    (!requires { &__await_transform_probe_derived<_Promise>::await_transform; });

// [exec.awaitable]p2: GET-AWAITER(c, p) -- the series of transformations applied to `c` as the operand of an
// await-expression in a coroutine whose promise `p` has type Promise.
//
// [expr.await]p3: if the promise type has an await_transform member (a search that finds at least one declaration),
// the operand is p.await_transform(c), and the expression is ill-formed if that is not valid for `c`; there is no
// fallback to `c`. Otherwise the operand is `c`.
template <class _Cp, class _Promise>
  requires __has_await_transform_member<_Promise> &&
           requires(_Cp&& __c, _Promise& __p) {
             execution::__exec_co_await_transform(__p.await_transform(std::forward<_Cp>(__c)));
           }
_LIBCPP_HIDE_FROM_ABI auto __get_awaiter(_Cp&& __c, _Promise& __p)
    -> decltype(execution::__exec_co_await_transform(__p.await_transform(std::forward<_Cp>(__c)))) {
  return execution::__exec_co_await_transform(__p.await_transform(std::forward<_Cp>(__c)));
}

template <class _Cp, class _Promise>
  requires(!__has_await_transform_member<_Promise>) && requires(_Cp&& __c) {
    execution::__exec_co_await_transform(std::forward<_Cp>(__c));
  }
_LIBCPP_HIDE_FROM_ABI auto __get_awaiter(_Cp&& __c, _Promise&)
    -> decltype(execution::__exec_co_await_transform(std::forward<_Cp>(__c))) {
  return execution::__exec_co_await_transform(std::forward<_Cp>(__c));
}

// [exec.awaitable]p2: GET-AWAITER(c) == GET-AWAITER(c, q) for an unspecified none-such q.
template <class _Cp>
  requires requires(_Cp&& __c) { execution::__exec_co_await_transform(std::forward<_Cp>(__c)); }
_LIBCPP_HIDE_FROM_ABI auto __get_awaiter(_Cp&& __c)
    -> decltype(execution::__exec_co_await_transform(std::forward<_Cp>(__c))) {
  return execution::__exec_co_await_transform(std::forward<_Cp>(__c));
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

// [exec.awaitable]p4: await-result-type<C, Promise> and await-result-type<C>, unified as a
// variadic alias (rather than a defaulted second parameter) so callers can pack-expand a
// possibly-empty Env pack directly into the Promise slot, matching __is_awaitable's own
// shape (e.g. get_completion_signatures.h's `__await_result_type<Sndr, __env_promise<Env>...>`
// for a zero-or-one-element Env pack) -- a pack expansion cannot target a non-pack (even
// defaulted) template parameter. Selects between __get_awaiter's one- and two-argument
// overloads based on whether the Promise pack is empty, exactly like __is_awaitable.
template <class _Cp, class... _Promise>
using __await_result_type =
    decltype(execution::__get_awaiter(std::declval<_Cp>(), std::declval<_Promise&>()...).await_resume());

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

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_AWAITABLE_H
