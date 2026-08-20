// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STOP_TOKEN_NEVER_STOP_TOKEN_H
#define _LIBCPP___STOP_TOKEN_NEVER_STOP_TOKEN_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

// [stoptoken.never]
// A stop token that models `unstoppable_token`: it provides the stop-token interface, but
// statically reports (via `static constexpr` member functions) that a stop is never possible
// nor requested, so callers can skip callback registration entirely at compile time.
class never_stop_token {
  struct __callback_type {
    _LIBCPP_HIDE_FROM_ABI explicit __callback_type(never_stop_token, auto&&) noexcept {}
  };

public:
  template <class>
  using callback_type = __callback_type;

  _LIBCPP_HIDE_FROM_ABI static constexpr bool stop_requested() noexcept { return false; }
  _LIBCPP_HIDE_FROM_ABI static constexpr bool stop_possible() noexcept { return false; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend bool
  operator==(const never_stop_token&, const never_stop_token&) noexcept = default;
};

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___STOP_TOKEN_NEVER_STOP_TOKEN_H
