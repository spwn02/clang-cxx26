// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// The [cpp.predefined] __STDCPP_FLOAT<N>_T__ macros are defined by the frontend only for libc++ users.
// ADDITIONAL_COMPILE_FLAGS: -Xclang -fstdcpp-float-macros

// <charconv>

// Extended floating-point overloads from P1467R9.

#include <charconv>
#include <cassert>
#include <format>
#include <limits>
#include <string>

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

#if TEST_STD_VER >= 23
// binary16 and bfloat16 are parsed with a single correct rounding (ties to even); going through float rounds twice.
template <class _Fp>
void check_parse(const char* text, std::errc ec, unsigned short bits = 0, std::chars_format fmt = std::chars_format::general) {
  _Fp value = static_cast<_Fp>(5);
  std::from_chars_result r = std::from_chars(text, text + __builtin_strlen(text), value, fmt);
  assert(r.ec == ec);
  if (ec == std::errc{})
    assert(__builtin_bit_cast(unsigned short, value) == bits);
  else
    assert(value == static_cast<_Fp>(5)); // unmodified
}

template <class _Fp>
void check_exhaustive() {
  for (unsigned int i = 0; i <= 0xffff; ++i) {
    _Fp value = __builtin_bit_cast(_Fp, static_cast<unsigned short>(i));
    if (value != value || value - value != 0) // NaN and infinities
      continue;
    for (std::chars_format fmt : {std::chars_format::general, std::chars_format::scientific, std::chars_format::hex}) {
      char buffer[64];
      auto out = std::to_chars(buffer, buffer + sizeof(buffer), value, fmt);
      assert(out.ec == std::errc{});
      _Fp parsed = static_cast<_Fp>(5);
      auto in = std::from_chars(buffer, out.ptr, parsed, fmt);
      assert(in.ec == std::errc{} && in.ptr == out.ptr);
      assert(__builtin_bit_cast(unsigned short, parsed) == i);
    }
    char buffer[64];
    auto plain = std::to_chars(buffer, buffer + sizeof(buffer), value);
    assert(plain.ec == std::errc{});
    assert(std::format("{}", value) == std::string(buffer, plain.ptr));
    auto scientific = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::scientific);
    assert(scientific.ec == std::errc{});
    unsigned digits = 0;
    for (char* p = buffer; p != scientific.ptr && *p != 'e'; ++p)
      if (*p >= '0' && *p <= '9')
        ++digits;
    if (digits > 1) {
      auto short_precision = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::scientific, digits - 2);
      assert(short_precision.ec == std::errc{});
      _Fp short_parsed = 0;
      auto short_in = std::from_chars(buffer, short_precision.ptr, short_parsed, std::chars_format::scientific);
      assert(short_in.ec != std::errc{} || short_in.ptr != short_precision.ptr ||
             __builtin_bit_cast(unsigned short, short_parsed) != i);
    }
  }
}
#endif

int main(int, char**) {
#if TEST_STD_VER >= 23
#  if defined(__STDCPP_FLOAT16_T__)
  check_exhaustive<std::float16_t>();
  check_parse<std::float16_t>("65504", std::errc{}, 0x7bff);
  check_parse<std::float16_t>("65519.99999", std::errc{}, 0x7bff);
  check_parse<std::float16_t>("65520", std::errc::result_out_of_range); // rounds to infinity
  check_parse<std::float16_t>("1e10", std::errc::result_out_of_range);
  check_parse<std::float16_t>("1e-10", std::errc::result_out_of_range); // rounds to zero
  check_parse<std::float16_t>("1.00048828125", std::errc{}, 0x3c00);     // exactly half way between 1 and the next value: even
  check_parse<std::float16_t>("1.000488281250000000001", std::errc{}, 0x3c01);
  check_parse<std::float16_t>("1.00048828125001", std::errc{}, 0x3c01);  // rounding twice (via float) gives 1
  check_parse<std::float16_t>("5.9604644775390625e-8", std::errc{}, 0x0001);
  check_parse<std::float16_t>("2.98023223876953125e-08", std::errc::result_out_of_range); // half of the smallest subnormal: ties to zero
  check_parse<std::float16_t>("2.980232238769531251e-08", std::errc{}, 0x0001);
  check_parse<std::float16_t>("1.8p+0", std::errc{}, 0x3e00, std::chars_format::hex);
  check_parse<std::float16_t>("1.ffcp+15", std::errc{}, 0x7bff, std::chars_format::hex);
  check_parse<std::float16_t>("1.ffep+15", std::errc::result_out_of_range, 0, std::chars_format::hex);
  check_narrow(std::float16_t(1.0), "1");
  check_narrow(std::float16_t(0.1), "0.1");
  {
    char buffer[64];
    auto out = std::to_chars(buffer, buffer + sizeof(buffer), 1.1f16, std::chars_format::scientific);
    assert(std::string(buffer, out.ptr) == "1.1e+00");
    out = std::to_chars(buffer, buffer + sizeof(buffer), 1.1f16, std::chars_format::fixed);
    assert(std::string(buffer, out.ptr) == "1.1");
    out = std::to_chars(buffer, buffer + sizeof(buffer), 1.1f16, std::chars_format::general);
    assert(std::string(buffer, out.ptr) == "1.1");
    // 65500 is the shortest decimal that parses back to the largest finite binary16 value (65504).
    out = std::to_chars(buffer, buffer + sizeof(buffer), 65504.0f16);
    assert(std::string(buffer, out.ptr) == "65500");
    out = std::to_chars(buffer, buffer + sizeof(buffer), 65504.0f16, std::chars_format::scientific);
    assert(std::string(buffer, out.ptr) == "6.55e+04");
    out = std::to_chars(buffer, buffer + sizeof(buffer), 5.9604645e-8f16);
    assert(std::string(buffer, out.ptr) == "6e-08");
  }
  check(std::numeric_limits<std::float16_t>::min());
  check(std::numeric_limits<std::float16_t>::max());
  check(std::numeric_limits<std::float16_t>::denorm_min());
  check(std::float16_t(65504.0));
#  endif
#  if defined(__STDCPP_BFLOAT16_T__)
  check_exhaustive<std::bfloat16_t>();
  check_parse<std::bfloat16_t>("1e40", std::errc::result_out_of_range);
  check_parse<std::bfloat16_t>("1e-50", std::errc::result_out_of_range);
  check_parse<std::bfloat16_t>("1.00390625", std::errc{}, 0x3f80);        // half way between 1 and the next value: even
  check_parse<std::bfloat16_t>("1.0039062500001", std::errc{}, 0x3f81);
  check_parse<std::bfloat16_t>("3.3895313892515355e38", std::errc{}, 0x7f7f);
  check_narrow(std::bfloat16_t(1.0), "1");
  check_narrow(std::bfloat16_t(0.1), "0.1");
  {
    char buffer[64];
    auto out = std::to_chars(buffer, buffer + sizeof(buffer), 1.1bf16, std::chars_format::scientific);
    assert(std::string(buffer, out.ptr) == "1.1e+00");
    out = std::to_chars(buffer, buffer + sizeof(buffer), 1.1bf16, std::chars_format::fixed);
    assert(std::string(buffer, out.ptr) == "1.1");
    out = std::to_chars(buffer, buffer + sizeof(buffer), 1.1bf16, std::chars_format::general);
    assert(std::string(buffer, out.ptr) == "1.1");
    out = std::to_chars(buffer, buffer + sizeof(buffer), std::numeric_limits<std::bfloat16_t>::denorm_min());
    assert(std::string(buffer, out.ptr) == "9e-41");
  }
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
