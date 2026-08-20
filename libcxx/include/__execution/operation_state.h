//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_OPERATION_STATE_H
#define _LIBCPP___EXECUTION_OPERATION_STATE_H

#include <__concepts/derived_from.h>
#include <__config>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.opstate.general]
struct operation_state_tag {};

// [exec.opstate.start]
// For a subexpression op, the expression start(op) is ill-formed if op is an rvalue;
// otherwise it is expression-equivalent to MANDATE-NOTHROW(op.start()).
struct start_t {
  template <class _Op>
    requires requires(_Op& __op) { __op.start(); }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator()(_Op& __op) const noexcept {
    static_assert(noexcept(__op.start()), "Mandates: the expression op.start() is noexcept.");
    __op.start();
  }
};

inline constexpr start_t start{};

template <class _Op>
concept operation_state = derived_from<typename _Op::operation_state_concept, operation_state_tag> &&
                           requires(_Op& __op) { execution::start(__op); };

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_OPERATION_STATE_H
