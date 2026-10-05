// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_AWAIT_COMPLETION_ADAPTOR_H
#define _LIBCPP___EXECUTION_GET_AWAIT_COMPLETION_ADAPTOR_H

#include <__config>
#include <__execution/forwarding_query.h>
#include <__utility/as_const.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.get.await.adapt]: get_await_completion_adaptor asks a queryable object for its associated awaitable completion
// adaptor. get_await_completion_adaptor(env) is MANDATE-NOTHROW(AS-CONST(env).query(get_await_completion_adaptor)).
struct get_await_completion_adaptor_t : forwarding_query_t {
  template <class _Env>
    requires requires(const _Env& __env, const get_await_completion_adaptor_t& __self) {
      std::as_const(__env).query(__self);
    }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator()(const _Env& __env) const noexcept {
    static_assert(noexcept(std::as_const(__env).query(*this)),
                  "Mandates: the expression env.query(get_await_completion_adaptor) is noexcept.");
    return std::as_const(__env).query(*this);
  }
};

inline constexpr get_await_completion_adaptor_t get_await_completion_adaptor{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_AWAIT_COMPLETION_ADAPTOR_H
