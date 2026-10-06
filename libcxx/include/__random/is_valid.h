//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANDOM_IS_VALID_H
#define _LIBCPP___RANDOM_IS_VALID_H

#include <__config>
#include <__type_traits/enable_if.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_unsigned.h>
#include <__utility/declval.h>
#include <cstdint>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

// [rand.req.genl]/1.4 (P4037R1):
// If a template argument corresponding to a template parameter named RealType
// is neither a standard floating-point type nor a member of an
// implementation-defined subset of the extended floating-point types, the
// program is ill-formed. (This implementation accepts float, double and long double.)

template <class>
struct __libcpp_random_is_valid_realtype : false_type {};
template <>
struct __libcpp_random_is_valid_realtype<float> : true_type {};
template <>
struct __libcpp_random_is_valid_realtype<double> : true_type {};
template <>
struct __libcpp_random_is_valid_realtype<long double> : true_type {};

// [rand.req.genl]/1.5 (P4037R1):
// If a template argument corresponding to a template parameter named IntType is
// neither a standard signed nor a standard unsigned integer type, nor an
// extended integer type whose width is at least that of char and at most that
// of long long, nor a member of an implementation-defined subset of the integer
// types, the program is ill-formed. So signed char and unsigned char (int8_t and
// uint8_t below) are required; __int128 is an extension.

template <class>
struct __libcpp_random_is_valid_inttype : false_type {};
template <>
struct __libcpp_random_is_valid_inttype<int8_t> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<short> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<int> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<long> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<long long> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<uint8_t> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned short> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned int> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned long> : true_type {};
template <>
struct __libcpp_random_is_valid_inttype<unsigned long long> : true_type {};

#if _LIBCPP_HAS_INT128
template <>
struct __libcpp_random_is_valid_inttype<__int128_t> : true_type {}; // extension
template <>
struct __libcpp_random_is_valid_inttype<__uint128_t> : true_type {}; // extension
#endif                                                               // _LIBCPP_HAS_INT128

// [rand.req.urng]/3:
// A class G meets the uniform random bit generator requirements if G models
// uniform_random_bit_generator, invoke_result_t<G&> is an unsigned integer type,
// and G provides a nested typedef-name result_type that denotes the same type
// as invoke_result_t<G&>.
// (In particular, reject URNGs with signed result_types; our distributions cannot
// handle such generator types.)

template <class, class = void>
struct __libcpp_random_is_valid_urng : false_type {};
template <class _Gp>
struct __libcpp_random_is_valid_urng<
    _Gp,
    __enable_if_t< is_unsigned<typename _Gp::result_type>::value &&
                   _IsSame<decltype(std::declval<_Gp&>()()), typename _Gp::result_type>::value > > : true_type {};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___RANDOM_IS_VALID_H
