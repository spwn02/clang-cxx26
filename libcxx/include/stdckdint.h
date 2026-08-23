// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_STDCKDINT_H
#define _LIBCPP_STDCKDINT_H

/*
    stdckdint.h synopsis // since C++26

Macros:

    __STDC_VERSION_STDCKDINT_H__

Functions (declared at global scope):

    template<class type1, class type2, class type3>
      bool ckd_add(type1* result, type2 a, type3 b);
    template<class type1, class type2, class type3>
      bool ckd_sub(type1* result, type2 a, type3 b);
    template<class type1, class type2, class type3>
      bool ckd_mul(type1* result, type2 a, type3 b);

*/

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

#  include <__type_traits/integer_traits.h>

#  define __STDC_VERSION_STDCKDINT_H__ 202311L

// Computes *result = a op b (in infinite signed precision, then converted to
// type1) and returns true if that conversion is not value-preserving, i.e. if
// overflow occurred.
template <std::__signed_or_unsigned_integer _Tp1, std::__signed_or_unsigned_integer _Tp2,
          std::__signed_or_unsigned_integer _Tp3>
_LIBCPP_HIDE_FROM_ABI inline bool ckd_add(_Tp1* __result, _Tp2 __a, _Tp3 __b) {
  return __builtin_add_overflow(__a, __b, __result);
}

template <std::__signed_or_unsigned_integer _Tp1, std::__signed_or_unsigned_integer _Tp2,
          std::__signed_or_unsigned_integer _Tp3>
_LIBCPP_HIDE_FROM_ABI inline bool ckd_sub(_Tp1* __result, _Tp2 __a, _Tp3 __b) {
  return __builtin_sub_overflow(__a, __b, __result);
}

template <std::__signed_or_unsigned_integer _Tp1, std::__signed_or_unsigned_integer _Tp2,
          std::__signed_or_unsigned_integer _Tp3>
_LIBCPP_HIDE_FROM_ABI inline bool ckd_mul(_Tp1* __result, _Tp2 __a, _Tp3 __b) {
  return __builtin_mul_overflow(__a, __b, __result);
}

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP_STDCKDINT_H
