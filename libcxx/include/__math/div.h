//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <stdlib.h> declares div_t, ldiv_t and lldiv_t and includes this header after the system header. Including it
// before the include guard makes either header usable first.
#include <__config>

#if _LIBCPP_STD_VER >= 23
#  include <stdlib.h>
#endif

#ifndef _LIBCPP___MATH_DIV_H
#define _LIBCPP___MATH_DIV_H

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 23

_LIBCPP_BEGIN_NAMESPACE_STD

namespace __math {

// [c.math.abs] / [cstdlib.syn] (P0533R9): div, ldiv and lldiv are constexpr. The template parameter keeps these
// distinct from the C library's declarations at global scope.

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr div_t div(int __x, int __y) _NOEXCEPT {
  div_t __r{};
  __r.quot = __x / __y;
  __r.rem  = __x % __y;
  return __r;
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr ldiv_t div(long __x, long __y) _NOEXCEPT {
  ldiv_t __r{};
  __r.quot = __x / __y;
  __r.rem  = __x % __y;
  return __r;
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr lldiv_t div(long long __x, long long __y) _NOEXCEPT {
  lldiv_t __r{};
  __r.quot = __x / __y;
  __r.rem  = __x % __y;
  return __r;
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr ldiv_t ldiv(long __x, long __y) _NOEXCEPT {
  return __math::div(__x, __y);
}

template <class = int>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI constexpr lldiv_t lldiv(long long __x, long long __y) _NOEXCEPT {
  return __math::div(__x, __y);
}

} // namespace __math

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 23

#endif // _LIBCPP___MATH_DIV_H
