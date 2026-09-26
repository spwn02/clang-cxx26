// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_H
#define _LIBCPP___TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

template <class _S, class _M>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_pointer_interconvertible_with_class(_M _S::* __m) noexcept {
  return __builtin_is_pointer_interconvertible_with_class(__m);
}

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_H
