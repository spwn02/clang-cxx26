//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_H
#define _LIBCPP___TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_H

#include <__config>
#include <__type_traits/integral_constant.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

template <class _Base, class _Derived>
struct _LIBCPP_NO_SPECIALIZATIONS is_pointer_interconvertible_base_of : bool_constant<__is_pointer_interconvertible_base_of(_Base, _Derived)> {};

template <class _Base, class _Derived>
_LIBCPP_NO_SPECIALIZATIONS inline constexpr bool is_pointer_interconvertible_base_of_v = __is_pointer_interconvertible_base_of(_Base, _Derived);

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_H
