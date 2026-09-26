// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_CORRESPONDING_MEMBER_H
#define _LIBCPP___TYPE_TRAITS_IS_CORRESPONDING_MEMBER_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

template <class _S1, class _S2, class _M1, class _M2>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_corresponding_member(_M1 _S1::* __m1, _M2 _S2::* __m2) noexcept {
  return __builtin_is_corresponding_member(__m1, __m2);
}

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_CORRESPONDING_MEMBER_H
