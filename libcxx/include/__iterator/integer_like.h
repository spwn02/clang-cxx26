// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ITERATOR_INTEGER_LIKE_H
#define _LIBCPP___ITERATOR_INTEGER_LIKE_H

#include <__concepts/arithmetic.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__type_traits/is_enum.h>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

// [iterator.concept.winc]
//
// P2393R1: the integer-like concepts cover the built-in integral types (except bool) and the integer-class types.
// libc++ provides no integer-class types of its own, so a type is recognized as one when it is a non-integral,
// non-enumeration type whose numeric_limits specialization ([numeric.limits], required by the paper for every
// integer-class type) says that it is an integer type; signedness is numeric_limits<I>::is_signed.
template <class _Tp>
concept __integer_class = !integral<_Tp> && !is_enum_v<_Tp> && numeric_limits<_Tp>::is_specialized &&
                          numeric_limits<_Tp>::is_integer;

template <class _Tp>
concept __integer_like = (integral<_Tp> && !same_as<_Tp, bool>) || __integer_class<_Tp>;

template <class _Tp>
concept __signed_integer_like = signed_integral<_Tp> || (__integer_class<_Tp> && numeric_limits<_Tp>::is_signed);

template <class _Tp>
concept __unsigned_integer_like =
    (unsigned_integral<_Tp> && !same_as<_Tp, bool>) || (__integer_class<_Tp> && !numeric_limits<_Tp>::is_signed);

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___ITERATOR_INTEGER_LIKE_H
