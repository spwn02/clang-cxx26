//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_ENV_H
#define _LIBCPP___EXECUTION_GET_ENV_H

#include <__config>
#include <__execution/env.h>
#include <__execution/queryable.h>
#include <__utility/as_const.h>
#include <__utility/declval.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.get.env]
struct get_env_t {
  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator()(_Tp&& __t) const noexcept {
    // [exec.get.env]p1: MANDATE-NOTHROW(AS-CONST(o).get_env()) if that expression is well-formed, otherwise env<>{}.
    // Mandates: the type of the expression satisfies queryable.
    if constexpr (requires { std::as_const(__t).get_env(); }) {
      static_assert(noexcept(std::as_const(__t).get_env()), "Mandates: the expression o.get_env() is noexcept.");
      static_assert(__queryable<decltype(std::as_const(__t).get_env())>,
                    "Mandates: the type of o.get_env() satisfies queryable.");
      return std::as_const(__t).get_env();
    } else {
      return env<>{};
    }
  }
};

inline constexpr get_env_t get_env{};

template <class _Tp>
using env_of_t = decltype(execution::get_env(std::declval<_Tp>()));

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_ENV_H
