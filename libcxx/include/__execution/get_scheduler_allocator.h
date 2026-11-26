//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_SCHEDULER_ALLOCATOR_H
#define _LIBCPP___EXECUTION_GET_SCHEDULER_ALLOCATOR_H

#include <__config>

#if _LIBCPP_STD_VER >= 26
#include <__execution/forwarding_query.h>
#include <__execution/get_allocator.h>
#include <__type_traits/remove_cvref.h>
#endif // _LIBCPP_STD_VER >= 26

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [exec.get.scheduler.allocator]
// Declared directly in namespace std (not std::execution) per [execution.syn].
struct get_scheduler_allocator_t : forwarding_query_t {
  template <class _Env>
    requires requires(const _Env& __env, const get_scheduler_allocator_t& __self) {
      __env.query(__self);
    } || requires(const _Env& __env) { get_allocator(__env); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept {
    if constexpr (requires { __env.query(*this); }) {
      // [exec.get.scheduler.allocator]p2-3: MANDATE-NOTHROW and simple-allocator.
      static_assert(
          noexcept(__env.query(*this)), "Mandates: the expression env.query(get_scheduler_allocator) is noexcept.");
      static_assert(__simple_allocator<remove_cvref_t<decltype(__env.query(*this))>>,
                    "Mandates: the type of env.query(get_scheduler_allocator) satisfies simple-allocator.");
      return __env.query(*this);
    } else {
      return get_allocator(__env);
    }
  }
};

inline constexpr get_scheduler_allocator_t get_scheduler_allocator{};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_SCHEDULER_ALLOCATOR_H
