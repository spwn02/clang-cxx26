//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_FLOATING_POINT_H
#define _LIBCPP___TYPE_TRAITS_IS_FLOATING_POINT_H

#include <__config>
#include <__type_traits/integral_constant.h>
#include <__type_traits/remove_cv.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

// clang-format off
template <class _Tp> inline const bool __is_floating_point_impl              = false;
template <>          inline const bool __is_floating_point_impl<float>       = true;
template <>          inline const bool __is_floating_point_impl<double>      = true;
template <>          inline const bool __is_floating_point_impl<long double> = true;
#if _LIBCPP_STD_VER >= 23
#  if defined(__STDCPP_FLOAT16_T__)
template <> inline const bool __is_floating_point_impl<_Float16> = true;
#  endif
#  if defined(__STDCPP_FLOAT32_T__)
#    if defined(__clang__)
template <> inline const bool __is_floating_point_impl<__float32> = true;
#    else
template <> inline const bool __is_floating_point_impl<_Float32> = true;
#    endif
#  endif
#  if defined(__STDCPP_FLOAT64_T__)
#    if defined(__clang__)
template <> inline const bool __is_floating_point_impl<__float64> = true;
#    else
template <> inline const bool __is_floating_point_impl<_Float64> = true;
#    endif
#  endif
#  if defined(__STDCPP_FLOAT128_T__)
#    if defined(__clang__)
template <> inline const bool __is_floating_point_impl<__float128> = true;
#    else
template <> inline const bool __is_floating_point_impl<_Float128> = true;
#    endif
#  endif
#  if defined(__STDCPP_BFLOAT16_T__)
template <> inline const bool __is_floating_point_impl<__bf16> = true;
#  endif
#endif
// clang-format on

template <class _Tp>
struct _LIBCPP_NO_SPECIALIZATIONS is_floating_point
    : integral_constant<bool, __is_floating_point_impl<__remove_cv_t<_Tp> > > {};

#if _LIBCPP_STD_VER >= 17
template <class _Tp>
_LIBCPP_NO_SPECIALIZATIONS inline constexpr bool is_floating_point_v = __is_floating_point_impl<__remove_cv_t<_Tp>>;
#endif

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_FLOATING_POINT_H
