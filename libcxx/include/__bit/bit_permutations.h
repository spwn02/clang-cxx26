// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___BIT_BIT_PERMUTATIONS_H
#define _LIBCPP___BIT_BIT_PERMUTATIONS_H

#include <__assert>
#include <__config>
#include <__type_traits/integer_traits.h>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [bit.permute]: N is numeric_limits<T>::digits.

// reverse(x) = sum_{n=0}^{N-1} x_n 2^(N-n-1)
template <__unsigned_integer _Tp>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp bit_reverse(_Tp __x) noexcept {
  if constexpr (sizeof(_Tp) == 1)
    return static_cast<_Tp>(__builtin_bitreverse8(__x));
  else if constexpr (sizeof(_Tp) == 2)
    return static_cast<_Tp>(__builtin_bitreverse16(__x));
  else if constexpr (sizeof(_Tp) == 4)
    return static_cast<_Tp>(__builtin_bitreverse32(__x));
  else if constexpr (sizeof(_Tp) == 8)
    return static_cast<_Tp>(__builtin_bitreverse64(__x));
  else {
    // wider than 64 bits (unsigned __int128): reverse the halves and swap them
    static_assert(sizeof(_Tp) == 16);
    return static_cast<_Tp>((static_cast<_Tp>(__builtin_bitreverse64(static_cast<unsigned long long>(__x))) << 64) |
                            static_cast<_Tp>(__builtin_bitreverse64(static_cast<unsigned long long>(__x >> 64))));
  }
}

// repeat(x, l) = sum_{n=0}^{N-1} x_(n mod l) 2^n
template <__unsigned_integer _Tp>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp bit_repeat(_Tp __x, int __l) {
  // A call that violates the precondition is not a core constant expression ([bit.permute]).
  if (__l <= 0) {
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(false, "bit_repeat requires the length to be greater than zero");
    __builtin_unreachable();
  }
  constexpr int __n = numeric_limits<_Tp>::digits;
  if (__l >= __n)
    return __x;
  const _Tp __unit = static_cast<_Tp>(__x & static_cast<_Tp>((static_cast<_Tp>(1) << __l) - 1));
  _Tp __result     = 0;
  for (int __shift = 0; __shift < __n; __shift += __l)
    __result = static_cast<_Tp>(__result | static_cast<_Tp>(__unit << __shift));
  return __result;
}

// compress(x, m) = sum_{n=0}^{N-1} m_n x_n 2^sigma(m, n): the bits of x selected by m, packed to the least significant end
template <__unsigned_integer _Tp>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp bit_compress(_Tp __x, _Tp __m) noexcept {
  _Tp __result = 0;
  _Tp __out    = 1;
  while (__m != 0) {
    const _Tp __lowest = static_cast<_Tp>(__m & static_cast<_Tp>(-__m)); // the lowest set bit of the mask
    if ((__x & __lowest) != 0)
      __result = static_cast<_Tp>(__result | __out);
    __out = static_cast<_Tp>(__out << 1);
    __m   = static_cast<_Tp>(__m & static_cast<_Tp>(__m - 1));
  }
  return __result;
}

// expand(x, m) = sum_{n=0}^{N-1} m_n x_sigma(m, n) 2^n: the low bits of x spread to the positions selected by m
template <__unsigned_integer _Tp>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp bit_expand(_Tp __x, _Tp __m) noexcept {
  _Tp __result = 0;
  _Tp __in     = 1;
  while (__m != 0) {
    const _Tp __lowest = static_cast<_Tp>(__m & static_cast<_Tp>(-__m)); // the lowest set bit of the mask
    if ((__x & __in) != 0)
      __result = static_cast<_Tp>(__result | __lowest);
    __in = static_cast<_Tp>(__in << 1);
    __m  = static_cast<_Tp>(__m & static_cast<_Tp>(__m - 1));
  }
  return __result;
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___BIT_BIT_PERMUTATIONS_H
