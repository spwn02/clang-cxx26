//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_STOP_TOKEN_H
#define _LIBCPP___EXECUTION_GET_STOP_TOKEN_H

#include <__config>
#include <__execution/forwarding_query.h>
#include <__stop_token/never_stop_token.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

// [exec.get.stop.token]
// Declared directly in namespace std (not std::execution) per [execution.syn].
struct get_stop_token_t : forwarding_query_t {
  template <class _Env>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept {
    if constexpr (requires(const get_stop_token_t& __self) { __env.query(__self); }) {
      static_assert(stoppable_token<decltype(__env.query(*this))>,
                    "Mandates: the type of env.query(get_stop_token) models stoppable_token.");
      static_assert(noexcept(__env.query(*this)),
                     "Mandates: the expression env.query(get_stop_token) is noexcept.");
      return __env.query(*this);
    } else {
      return never_stop_token{};
    }
  }
};

inline constexpr get_stop_token_t get_stop_token{};

template <class _Tp>
using stop_token_of_t = remove_cvref_t<decltype(get_stop_token(std::declval<_Tp>()))>;

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_STOP_TOKEN_H
