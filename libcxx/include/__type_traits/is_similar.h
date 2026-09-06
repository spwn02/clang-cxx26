//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_SIMILAR_H
#define _LIBCPP___TYPE_TRAITS_IS_SIMILAR_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Tp, class _Up>
struct __is_similar_impl {
  static inline const bool value = __is_same(_Tp, _Up);
};

template <class _Tp, class _Up>
inline const bool __is_similar_v = __is_similar_impl<__remove_cv(_Tp), __remove_cv(_Up)>::value;

template <class _Tp, class _Up>
struct __is_similar_impl<_Tp*, _Up*> {
  static inline const bool value = __is_similar_v<_Tp, _Up>;
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_SIMILAR_H
