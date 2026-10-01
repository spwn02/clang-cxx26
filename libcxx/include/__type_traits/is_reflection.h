//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_REFLECTION_H
#define _LIBCPP___TYPE_TRAITS_IS_REFLECTION_H

#include <__config>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cv.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && __has_feature(reflection)

template <class _Tp>
struct _LIBCPP_NO_SPECIALIZATIONS is_reflection
    : integral_constant<bool, is_same<__remove_cv_t<_Tp>, decltype(^^int)>::value> {};

template <class _Tp>
_LIBCPP_NO_SPECIALIZATIONS inline constexpr bool is_reflection_v = is_reflection<_Tp>::value;

#endif // _LIBCPP_STD_VER >= 26 && __has_feature(reflection)

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_REFLECTION_H
