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
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Tp&& __t) const
      noexcept(noexcept(std::as_const(__t).get_env())) -> decltype(std::as_const(__t).get_env())
    requires requires { { std::as_const(__t).get_env() } -> __queryable; }
  {
    return std::as_const(__t).get_env();
  }

  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI constexpr env<> operator()(_Tp&&) const noexcept
    requires(!requires(const _Tp& __t) { { __t.get_env() } -> __queryable; })
  {
    return env<>{};
  }
};

inline constexpr get_env_t get_env{};

template <class _Tp>
using env_of_t = decltype(execution::get_env(std::declval<_Tp>()));

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_ENV_H
