//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_ENV_H
#define _LIBCPP___EXECUTION_ENV_H

#include <__config>
#include <__execution/queryable.h>
#include <__tuple/tuple_element.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/unwrap_ref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.prop]
// A queryable object that answers exactly one query, `_Query{}`, with a fixed `_Value`.
template <class _Query, class _Value>
class prop {
public:
  // `const`-qualified so that `prop` remains an aggregate (no user-declared constructors,
  // matching [exec.prop]'s synopsis) while implicitly deleting copy/move assignment (per
  // [exec.prop]: "prop is not assignable") without an explicit `operator=` declaration —
  // the latter would make the implicit copy constructor deprecated ([depr.impldec]).
  _LIBCPP_NO_UNIQUE_ADDRESS const _Query __query_;
  _LIBCPP_NO_UNIQUE_ADDRESS const _Value __value_;

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr const _Value& query(_Query, _Args&&...) const noexcept {
    return __value_;
  }
};

template <class _Query, class _Value>
prop(_Query, _Value) -> prop<_Query, unwrap_reference_t<_Value>>;

template <class _Env, class _Query, class... _Args>
concept __has_query =
    requires(const _Env& __e, _Query __q, _Args&&... __args) { __e.query(__q, std::forward<_Args>(__args)...); };

// [exec.env]
// A queryable object that combines several queryable objects; `query(q, args...)` is
// forwarded to the *first* element of the pack for which that call is well-formed.
template <__queryable... _Envs>
class env {
private:
  // Both helpers below guard the `_Idx == sizeof...(_Envs)` (no match) case explicitly,
  // rather than relying on the class's `query()` requires-clause to prevent that recursive
  // instantiation from ever happening: a function's noexcept-specifier is evaluated as part
  // of forming its type for overload resolution, which is *not* protected by SFINAE the way
  // a trailing requires-clause substitution failure is (an error there is not confined to
  // the "immediate context") — so `__query_is_noexcept` must stay well-formed even when
  // instantiated for a query no element answers, or checking `requires { e.query(q) }` from
  // outside (as the tests below do) hard-errors instead of quietly evaluating to `false`.
  template <size_t _Idx, class _Query, class... _Args>
  static consteval bool __query_is_noexcept() {
    if constexpr (_Idx == sizeof...(_Envs)) {
      return true; // unreachable when `query()` actually participates in overload resolution
    } else if constexpr (__has_query<tuple_element_t<_Idx, tuple<_Envs...>>, _Query, _Args...>) {
      using _Env = tuple_element_t<_Idx, tuple<_Envs...>>;
      return noexcept(std::declval<const _Env&>().query(std::declval<_Query>(), std::declval<_Args>()...));
    } else {
      return __query_is_noexcept<_Idx + 1, _Query, _Args...>();
    }
  }

  template <size_t _Idx, class _Query, class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __query_at(_Query __q, _Args&&... __args) const {
    if constexpr (_Idx == sizeof...(_Envs)) {
      static_assert(_Idx < sizeof...(_Envs), "unreachable: query() requires that some element matches");
    } else if constexpr (__has_query<tuple_element_t<_Idx, tuple<_Envs...>>, _Query, _Args...>) {
      return std::get<_Idx>(__envs_).query(__q, std::forward<_Args>(__args)...);
    } else {
      return __query_at<_Idx + 1>(__q, std::forward<_Args>(__args)...);
    }
  }

  tuple<_Envs...> __envs_;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr env() = default;

  // Copy/move constructors declared explicitly (rather than left implicit) so that
  // `env` stays movable: since `env` already has other user-declared constructors below
  // (so, unlike `prop`, it was never an aggregate to begin with), there's no reason to lean
  // on `const`-qualifying `__envs_` the way `prop` does, and doing so would make every move
  // silently degrade into a copy. Declaring these explicitly, alongside the deleted
  // assignment operators, also sidesteps `-Wdeprecated-copy` (relying on the *implicit* copy
  // constructor while a copy-assignment operator is user-declared, even deleted, is
  // deprecated per [depr.impldec]).
  _LIBCPP_HIDE_FROM_ABI constexpr env(const env&) = default;
  _LIBCPP_HIDE_FROM_ABI constexpr env(env&&)      = default;

  // [exec.env]: "env is not assignable".
  env& operator=(const env&) = delete;
  env& operator=(env&&)      = delete;

  template <class... _Vs>
    requires(sizeof...(_Vs) == sizeof...(_Envs)) && (sizeof...(_Vs) > 0) &&
            (is_constructible_v<_Envs, _Vs> && ...)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit env(_Vs&&... __vs) noexcept((is_nothrow_constructible_v<_Envs, _Vs> && ...))
      : __envs_(std::forward<_Vs>(__vs)...) {}

  template <class _Query, class... _Args>
    requires(__has_query<_Envs, _Query, _Args...> || ...)
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Query __q, _Args&&... __args) const
      noexcept(__query_is_noexcept<0, _Query, _Args...>()) {
    return __query_at<0>(__q, std::forward<_Args>(__args)...);
  }
};

template <class... _Envs>
env(_Envs...) -> env<unwrap_reference_t<_Envs>...>;

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_ENV_H
