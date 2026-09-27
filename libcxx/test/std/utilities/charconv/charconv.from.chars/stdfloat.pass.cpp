// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <charconv>

// Extended floating-point overloads from P1467R9.

#include <charconv>
#include <cassert>
#include <limits>

#include "test_macros.h"

#if TEST_STD_VER >= 23
template <class _Fp>
void check(_Fp __value) {
  char __buffer[128];
  auto __out = std::to_chars(__buffer, __buffer + sizeof(__buffer), __value);
  assert(__out.ec == std::errc{});
  _Fp __parsed = 0;
  auto __in = std::from_chars(__buffer, __out.ptr, __parsed);
  assert(__in.ec == std::errc{} && __in.ptr == __out.ptr);
  assert(__parsed == __value);
  auto __fmt = std::to_chars(__buffer, __buffer + sizeof(__buffer), __value, std::chars_format::general);
  assert(__fmt.ec == std::errc{});
  auto __precision = std::to_chars(__buffer, __buffer + sizeof(__buffer), __value, std::chars_format::general, 7);
  assert(__precision.ec == std::errc{});
}

template <class _Fp>
void check_narrow(_Fp __value, const char* __expected) {
  char __buffer[128];
  auto __out = std::to_chars(__buffer, __buffer + sizeof(__buffer), __value);
  assert(__out.ec == std::errc{});
  assert(__out.ptr - __buffer == static_cast<std::ptrdiff_t>(__builtin_strlen(__expected)));
  for (const char* __p = __expected; *__p; ++__p)
    assert(__buffer[__p - __expected] == *__p);
  check(__value);
}
#endif

int main(int, char**) {
#if TEST_STD_VER >= 23
#  if defined(__STDCPP_FLOAT16_T__)
  check_narrow(std::float16_t(1.0), "1");
  check_narrow(std::float16_t(0.1), "0.1");
  check(std::numeric_limits<std::float16_t>::min());
  check(std::numeric_limits<std::float16_t>::max());
  check(std::numeric_limits<std::float16_t>::denorm_min());
  check(std::float16_t(65504.0));
#  endif
#  if defined(__STDCPP_BFLOAT16_T__)
  check_narrow(std::bfloat16_t(1.0), "1");
  check_narrow(std::bfloat16_t(0.1), "0.1");
  check(std::numeric_limits<std::bfloat16_t>::min());
  check(std::numeric_limits<std::bfloat16_t>::max());
  check(std::numeric_limits<std::bfloat16_t>::denorm_min());
#  endif
#  if defined(__STDCPP_FLOAT32_T__)
  check(std::float32_t(0));
  check(std::float32_t(-0.0));
  check(std::numeric_limits<std::float32_t>::min());
  check(std::numeric_limits<std::float32_t>::max());
  check(std::numeric_limits<std::float32_t>::denorm_min());
  check(std::float32_t(1.00000011920928955078125));
#  endif
#  if defined(__STDCPP_FLOAT64_T__)
  check(std::float64_t(0));
  check(std::float64_t(-0.0));
  check(std::numeric_limits<std::float64_t>::min());
  check(std::numeric_limits<std::float64_t>::max());
  check(std::numeric_limits<std::float64_t>::denorm_min());
  check(std::float64_t(0.10000000000000002));
#  endif
#endif
  return 0;
}
