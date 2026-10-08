//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_PROMOTE_H
#define _LIBCPP___TYPE_TRAITS_PROMOTE_H

#include <__config>
#include <__type_traits/enable_if.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_arithmetic.h>
#include <__type_traits/is_floating_point.h>
#include <__type_traits/is_same.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

float __promote_impl(float);
double __promote_impl(char);
double __promote_impl(int);
double __promote_impl(unsigned);
double __promote_impl(long);
double __promote_impl(unsigned long);
double __promote_impl(long long);
double __promote_impl(unsigned long long);
#if _LIBCPP_HAS_INT128
double __promote_impl(__int128_t);
double __promote_impl(__uint128_t);
#endif
double __promote_impl(double);
long double __promote_impl(long double);

#if _LIBCPP_STD_VER >= 14
template <class _Tp>
struct __is_extended_floating_point
    : public integral_constant<bool,
                               is_floating_point<_Tp>::value && !is_same<_Tp, float>::value &&
                                   !is_same<_Tp, double>::value && !is_same<_Tp, long double>::value> {};

template <class _Tp>
inline constexpr bool __is_extended_floating_point_v = __is_extended_floating_point<_Tp>::value;

template <class... _Args, __enable_if_t<!(__is_extended_floating_point_v<_Args> || ...), int> = 0>
auto __promote_result(_Args...) -> decltype((std::__promote_impl(_Args()) + ...));

// Extended floating-point types use the core language's conversion ranks and
// subranks. This return type also makes unordered pairs fail by substitution.
template <class... _Args, __enable_if_t<(__is_extended_floating_point_v<_Args> || ...), int> = 0>
auto __promote_result(_Args...) -> decltype((_Args() + ...));

template <class... _Args>
using __promote_t _LIBCPP_NODEBUG =
    decltype((__enable_if_t<(is_arithmetic<_Args>::value && ...)>)0, std::__promote_result(_Args()...));
#else
// (compiled as C++03/C++11 too: no extended floating-point types, no variable templates)
template <class... _Args>
using __promote_t _LIBCPP_NODEBUG =
    decltype((__enable_if_t<(is_arithmetic<_Args>::value && ...)>)0, (std::__promote_impl(_Args()) + ...));
#endif

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_PROMOTE_H
