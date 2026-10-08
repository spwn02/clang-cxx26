//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_FWD_ENV_H
#define _LIBCPP___EXECUTION_FWD_ENV_H

#include <__config>
#include <__execution/env.h>
#include <__execution/forwarding_query.h>
#include <__execution/queryable.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// get_completion_scheduler_t and get_completion_domain_t are defined in <__execution/get_scheduler.h> and
// <__execution/domain.h>, which include this header's users; the sender attributes below only need to recognize them.
template <class _Cpo>
struct get_completion_scheduler_t;
template <class _Tag = void>
struct get_completion_domain_t;

template <class _Query>
inline constexpr bool __is_completion_query_v = false;
template <class _Cpo>
inline constexpr bool __is_completion_query_v<get_completion_scheduler_t<_Cpo>> = true;
template <class _Tag>
inline constexpr bool __is_completion_query_v<get_completion_domain_t<_Tag>> = true;

// [exec.snd.expos]p4: FWD-ENV(env). A queryable adaptor over env that only forwards queries
// q for which forwarding_query(q) is true -- everything else is ill-formed. Per
// [exec.adapt.general]p3.2/3.4, this is the environment a single-child sender adaptor
// exposes as its own attributes (from its child's attributes), and the one it connects its
// child through (from its receiver's environment) -- the first consumer is
// <__execution/then.h> and the other single-child adaptors use the same shape.
//
// Stores env *by value* (a copy), not by reference: __fwd_env_fn below is meant to be called
// directly in a `return` statement (e.g. `return __fwd_env_fn(get_env(rcvr_));`), where
// get_env's own result is a temporary -- a reference member would dangle the moment that
// full-expression ends, since nothing else keeps the temporary alive past it. These environment
// objects are small value types, so copying them is inexpensive.
// [exec.snd.expos] basic-sender::get_completion_signatures: the type of the environment of the receiver that the
// completion signatures are computed for is the first type in the list `Env..., env<>`.
template <class _Default, class... _Env>
struct __first_env_or {
  using type = _Default;
};
template <class _Default, class _First, class... _Rest>
struct __first_env_or<_Default, _First, _Rest...> {
  using type = _First;
};

template <class _Env>
class __fwd_env {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __fwd_env(_Env __env) noexcept(is_nothrow_move_constructible_v<_Env>)
      : __env_(std::move(__env)) {}

  // The leading call is deliberately parenthesized: `requires std::forwarding_query(_Tag())
  // && requires(...) {...}` (no parens) mis-parses on this fork's Clang -- the atomic
  // constraint ends up being just the id-expression `std::forwarding_query` (type
  // `const forwarding_query_t`, not `bool`), with `(_Tag())` treated separately. Empirically
  // confirmed in isolation: a bare call expression immediately followed by `&&` as the first
  // operand of a `requires`-clause needs parenthesizing here to parse as a call.
  template <class _Tag, class... _Args>
    requires(std::forwarding_query(_Tag())) && requires(const _Env& __env, _Tag __tag, _Args&&... __args) {
      __env.query(__tag, std::forward<_Args>(__args)...);
    }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Tag __tag, _Args&&... __args) const
      noexcept(noexcept(__env_.query(__tag, std::forward<_Args>(__args)...))) {
    return __env_.query(__tag, std::forward<_Args>(__args)...);
  }

private:
  _Env __env_;
};

template <class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __fwd_env_fn(_Env&& __env) noexcept(
    is_nothrow_constructible_v<__fwd_env<remove_cvref_t<_Env>>, _Env>) {
  return __fwd_env<remove_cvref_t<_Env>>(std::forward<_Env>(__env));
}

// FWD-ENV-T(E) for the environment E of the receiver completion signatures are computed for: the first type of
// `Env..., env<>` ([exec.snd.expos] basic-sender::get_completion_signatures), forwarded.
template <class... _Env>
using __fwd_env_of_first_t = __fwd_env<typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type>;

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_FWD_ENV_H
