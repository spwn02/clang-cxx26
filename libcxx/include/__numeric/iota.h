// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___NUMERIC_IOTA_H
#define _LIBCPP___NUMERIC_IOTA_H

#include <__config>
#include <__iterator/iterator_traits.h>
#include <__type_traits/is_convertible.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _ForwardIterator, class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 void
iota(_ForwardIterator __first, _ForwardIterator __last, _Tp __value) {
#if _LIBCPP_STD_VER >= 20
  static_assert(is_convertible<_Tp, typename iterator_traits<_ForwardIterator>::value_type>::value,
                "Mandates: T is convertible to ForwardIterator's value type");
#endif
  for (; __first != __last; ++__first, (void)++__value)
    *__first = __value;
}

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___NUMERIC_IOTA_H
