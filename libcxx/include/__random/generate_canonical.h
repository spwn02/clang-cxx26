//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANDOM_GENERATE_CANONICAL_H
#define _LIBCPP___RANDOM_GENERATE_CANONICAL_H

#include <__config>
#include <__random/log2.h>
#include <cstdint>
#include <initializer_list>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

// generate_canonical

#if _LIBCPP_STD_VER >= 26

// P0952R2: fixes the pre-C++26 formula below, which could both return a
// value == 1 and produce non-uniform results when the URNG's range isn't a
// power of two. Follows [rand.util.canonical] literally: k is the smallest
// integer such that R^k >= r^d (r == radix, d == digits), x == floor(R^k /
// r^d), attempts repeat until S < x * r^d, and the result is
// floor(S/x) / r^d.
template <class _RealType, size_t __digits, class _URNG>
_LIBCPP_HIDE_FROM_ABI _RealType generate_canonical(_URNG& __g) {
  static_assert(numeric_limits<_RealType>::radix == 2,
                "std::generate_canonical only supports floating-point types with radix 2");

  constexpr size_t __d = numeric_limits<_RealType>::digits < __digits ? numeric_limits<_RealType>::digits : __digits;

  if constexpr (__d == 0) {
    // r^0 == 1, so k (the smallest integer with R^k >= 1) is 0: no
    // invocations of g are made, and the result is trivially 0.
    return _RealType(0);
  } else {
#  if _LIBCPP_HAS_INT128
    using _UInt = __uint128_t;
#  else
    using _UInt = unsigned long long;
#  endif
    // R = g.max() - g.min() + 1, computed in a type wide enough that the
    // "+ 1" cannot wrap even when the URNG's range spans its entire
    // result_type (e.g. mt19937_64, whose range is exactly 2^64).
    constexpr _UInt __rp = static_cast<_UInt>(_URNG::max()) - static_cast<_UInt>(_URNG::min()) + _UInt(1);
    constexpr _UInt __rd = _UInt(1) << __d;

    // k = smallest integer such that R^k >= r^d; x = floor(R^k / r^d).
    // R and r^d are both compile-time constants, so k and R^k are computed
    // here as compile-time constants too, with overflow of the _UInt
    // intermediate detected directly rather than approximated via a
    // bits(R)+d bound: that bound is loose exactly where R^k is smallest
    // (a large, e.g. full-width-power-of-two R makes k tiny), so it can
    // reject instantiations the arithmetic below handles exactly -- e.g.
    // long double (digits == 64) with a full-range 64-bit engine
    // (R == 2^64, k == 1).
    struct __canon_params {
      size_t __k;
      _UInt __rk;
      bool __fits;
    };
    constexpr __canon_params __params = [] {
      size_t __k  = 0;
      _UInt __rk = _UInt(1);
      while (__rk < __rd) {
        if (__rk > ~_UInt(0) / __rp)
          return __canon_params{__k, __rk, false};
        __rk *= __rp;
        ++__k;
      }
      return __canon_params{__k, __rk, true};
    }();

    static_assert(__params.__fits,
                  "generate_canonical: this URNG's range combined with RealType's precision "
                  "exceeds this implementation's intermediate-precision limit");

    const _UInt __x         = __params.__rk / __rd;
    const _UInt __threshold = __x * __rd;

    while (true) {
      _UInt __s    = _UInt(0);
      _UInt __base = _UInt(1);
      for (size_t __i = 0; __i < __params.__k; ++__i, __base *= __rp)
        __s += static_cast<_UInt>(__g() - _URNG::min()) * __base;
      if (__s < __threshold)
        return static_cast<_RealType>(__s / __x) / static_cast<_RealType>(__rd);
    }
  }
}

#else // _LIBCPP_STD_VER >= 26

template <class _RealType, size_t __bits, class _URNG>
_LIBCPP_HIDE_FROM_ABI _RealType generate_canonical(_URNG& __g) {
  const size_t __dt = numeric_limits<_RealType>::digits;
  const size_t __b  = __dt < __bits ? __dt : __bits;
#  ifdef _LIBCPP_CXX03_LANG
  const size_t __log_r = __log2<uint64_t, _URNG::_Max - _URNG::_Min + uint64_t(1)>::value;
#  else
  const size_t __log_r = __log2<uint64_t, _URNG::max() - _URNG::min() + uint64_t(1)>::value;
#  endif
  const size_t __k     = __b / __log_r + (__b % __log_r != 0) + (__b == 0);
  const _RealType __rp = static_cast<_RealType>(_URNG::max() - _URNG::min()) + _RealType(1);
  _RealType __base     = __rp;
  _RealType __sp       = __g() - _URNG::min();
  for (size_t __i = 1; __i < __k; ++__i, __base *= __rp)
    __sp += (__g() - _URNG::min()) * __base;
  return __sp / __base;
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___RANDOM_GENERATE_CANONICAL_H
