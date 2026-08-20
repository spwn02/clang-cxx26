// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STOP_TOKEN_INPLACE_STOP_TOKEN_H
#define _LIBCPP___STOP_TOKEN_INPLACE_STOP_TOKEN_H

#include <__config>
#include <__utility/swap.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

class inplace_stop_source;

template <class _Callback>
class inplace_stop_callback;

// [stoptoken.inplace]
// Unlike `stop_token`, `inplace_stop_token` does not own (or share ownership of) the state it
// refers to: it is a non-owning reference to an `inplace_stop_source` that the caller must keep
// alive for as long as the token (and any `inplace_stop_callback` registered through it) is used.
// In exchange for that stricter lifetime contract, no allocation or reference counting is needed.
class inplace_stop_token {
public:
  template <class _Fn>
  using callback_type = inplace_stop_callback<_Fn>;

  _LIBCPP_HIDE_FROM_ABI constexpr inplace_stop_token() noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool stop_requested() const noexcept;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool stop_possible() const noexcept { return __source_ != nullptr; }

  _LIBCPP_HIDE_FROM_ABI constexpr void swap(inplace_stop_token& __other) noexcept {
    std::swap(__source_, __other.__source_);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend bool
  operator==(const inplace_stop_token&, const inplace_stop_token&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI friend constexpr void swap(inplace_stop_token& __lhs, inplace_stop_token& __rhs) noexcept {
    __lhs.swap(__rhs);
  }

private:
  const inplace_stop_source* __source_ = nullptr;

  friend class inplace_stop_source;
  template <class>
  friend class inplace_stop_callback;

  _LIBCPP_HIDE_FROM_ABI explicit constexpr inplace_stop_token(const inplace_stop_source* __source) noexcept
      : __source_(__source) {}
};

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___STOP_TOKEN_INPLACE_STOP_TOKEN_H
