// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <charconv>

// std::from_chars for std::float128_t (P1467R9). Results are correctly rounded (ties to even) for decimal and
// hexadecimal input; the expected bit patterns were cross-checked with glibc's strtof128.

#include <bit>
#include <cassert>
#include <charconv>
#include <cstring>
#include <stdfloat>

#include "test_macros.h"

#if defined(__STDCPP_FLOAT128_T__)

using F = std::float128_t;

struct Case {
  const char* text;
  std::chars_format fmt;
  std::errc ec;
  long consumed;
  unsigned long long hi, lo; // the parsed value (only meaningful when ec == std::errc{})
};

static F make(unsigned long long hi, unsigned long long lo) {
  return std::bit_cast<F>((static_cast<unsigned __int128>(hi) << 64) | lo);
}

static unsigned __int128 bits(F v) { return std::bit_cast<unsigned __int128>(v); }

// clang-format off
static const Case cases[] = {
  {"1", std::chars_format::general, std::errc{}, 1, 0x3fff000000000000ull, 0x0000000000000000ull},
  {"-1", std::chars_format::general, std::errc{}, 2, 0xbfff000000000000ull, 0x0000000000000000ull},
  {"1.5", std::chars_format::general, std::errc{}, 3, 0x3fff800000000000ull, 0x0000000000000000ull},
  {"+1", std::chars_format::general, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {".5", std::chars_format::general, std::errc{}, 2, 0x3ffe000000000000ull, 0x0000000000000000ull},
  {"5.", std::chars_format::general, std::errc{}, 2, 0x4001400000000000ull, 0x0000000000000000ull},
  {".", std::chars_format::general, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"-", std::chars_format::general, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"", std::chars_format::general, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"abc", std::chars_format::general, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1e", std::chars_format::general, std::errc{}, 1, 0x3fff000000000000ull, 0x0000000000000000ull},
  {"1e+", std::chars_format::general, std::errc{}, 1, 0x3fff000000000000ull, 0x0000000000000000ull},
  {"1e5", std::chars_format::general, std::errc{}, 3, 0x400f86a000000000ull, 0x0000000000000000ull},
  {"1E5", std::chars_format::general, std::errc{}, 3, 0x400f86a000000000ull, 0x0000000000000000ull},
  {"1e+5", std::chars_format::general, std::errc{}, 4, 0x400f86a000000000ull, 0x0000000000000000ull},
  {"1e-5", std::chars_format::general, std::errc{}, 4, 0x3fee4f8b588e368full, 0x08461f9f01b866e4ull},
  {"1.5abc", std::chars_format::general, std::errc{}, 3, 0x3fff800000000000ull, 0x0000000000000000ull},
  {"1.5e3x", std::chars_format::general, std::errc{}, 5, 0x4009770000000000ull, 0x0000000000000000ull},
  {"1.2.3", std::chars_format::general, std::errc{}, 3, 0x3fff333333333333ull, 0x3333333333333333ull},
  {"1e5", std::chars_format::fixed, std::errc{}, 1, 0x3fff000000000000ull, 0x0000000000000000ull},
  {"1.25", std::chars_format::fixed, std::errc{}, 4, 0x3fff400000000000ull, 0x0000000000000000ull},
  {"1e5", std::chars_format::scientific, std::errc{}, 3, 0x400f86a000000000ull, 0x0000000000000000ull},
  {"1", std::chars_format::scientific, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1.5", std::chars_format::scientific, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1.5e0", std::chars_format::scientific, std::errc{}, 5, 0x3fff800000000000ull, 0x0000000000000000ull},
  {"inf", std::chars_format::general, std::errc{}, 3, 0x7fff000000000000ull, 0x0000000000000000ull},
  {"-inf", std::chars_format::general, std::errc{}, 4, 0xffff000000000000ull, 0x0000000000000000ull},
  {"INF", std::chars_format::general, std::errc{}, 3, 0x7fff000000000000ull, 0x0000000000000000ull},
  {"infinity", std::chars_format::general, std::errc{}, 8, 0x7fff000000000000ull, 0x0000000000000000ull},
  {"InFiNiTy", std::chars_format::general, std::errc{}, 8, 0x7fff000000000000ull, 0x0000000000000000ull},
  {"infinit", std::chars_format::general, std::errc{}, 3, 0x7fff000000000000ull, 0x0000000000000000ull},
  {"nan", std::chars_format::general, std::errc{}, 3, 0x7fff800000000000ull, 0x0000000000000000ull},
  {"-nan", std::chars_format::general, std::errc{}, 4, 0xffff800000000000ull, 0x0000000000000000ull},
  {"NAN(abc_1)", std::chars_format::general, std::errc{}, 10, 0x7fff800000000000ull, 0x0000000000000000ull},
  {"nan(", std::chars_format::general, std::errc{}, 3, 0x7fff800000000000ull, 0x0000000000000000ull},
  {"nan(a b)", std::chars_format::general, std::errc{}, 3, 0x7fff800000000000ull, 0x0000000000000000ull},
  {"in", std::chars_format::general, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"0", std::chars_format::general, std::errc{}, 1, 0x0000000000000000ull, 0x0000000000000000ull},
  {"-0", std::chars_format::general, std::errc{}, 2, 0x8000000000000000ull, 0x0000000000000000ull},
  {"0.0e100", std::chars_format::general, std::errc{}, 7, 0x0000000000000000ull, 0x0000000000000000ull},
  {"000123", std::chars_format::general, std::errc{}, 6, 0x4005ec0000000000ull, 0x0000000000000000ull},
  {"0.000000000000000000000000000001", std::chars_format::general, std::errc{}, 32, 0x3f9b4484bfeebc29ull, 0xf863424b06f3529aull},
  {"1e4932", std::chars_format::general, std::errc{}, 6, 0x7ffeae596552b8fdull, 0xed99d037e3d04b75ull},
  {"1.18973149535723176508575932662800702e4932", std::chars_format::general, std::errc{}, 42, 0x7ffeffffffffffffull, 0xffffffffffffffffull},
  {"1.2e4932", std::chars_format::general, std::errc::result_out_of_range, 8, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1e5000", std::chars_format::general, std::errc::result_out_of_range, 6, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"-1e5000", std::chars_format::general, std::errc::result_out_of_range, 7, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"3.36210314311209350626267781732175260e-4932", std::chars_format::general, std::errc{}, 43, 0x0001000000000000ull, 0x0000000000000000ull},
  {"6.47517511943802511092443895822764655e-4966", std::chars_format::general, std::errc{}, 43, 0x0000000000000000ull, 0x0000000000000001ull},
  {"3.2375875597190125554622194791138232e-4966", std::chars_format::general, std::errc::result_out_of_range, 42, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"3.23758755971901255546221947911382328e-4966", std::chars_format::general, std::errc{}, 43, 0x0000000000000000ull, 0x0000000000000001ull},
  {"1e-4966", std::chars_format::general, std::errc::result_out_of_range, 7, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1e-6000", std::chars_format::general, std::errc::result_out_of_range, 7, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"123456789012345678901234567890123456789012345678901234567890e-30", std::chars_format::general, std::errc{}, 64, 0x405f8ee90ff6c373ull, 0xe0ee4e3f0ad21f9bull},
  {"0.1", std::chars_format::general, std::errc{}, 3, 0x3ffb999999999999ull, 0x999999999999999aull},
  {"1.0000000000000000000000000000000001", std::chars_format::general, std::errc{}, 36, 0x3fff000000000000ull, 0x0000000000000001ull},
  {"1.00000000000000000000000000000000019252", std::chars_format::general, std::errc{}, 40, 0x3fff000000000000ull, 0x0000000000000001ull},
  {"1", std::chars_format::hex, std::errc{}, 1, 0x3fff000000000000ull, 0x0000000000000000ull},
  {"1.8p0", std::chars_format::hex, std::errc{}, 5, 0x3fff800000000000ull, 0x0000000000000000ull},
  {"1.8p+3", std::chars_format::hex, std::errc{}, 6, 0x4002800000000000ull, 0x0000000000000000ull},
  {"-1.8p-3", std::chars_format::hex, std::errc{}, 7, 0xbffc800000000000ull, 0x0000000000000000ull},
  {"0.0000000000000000000000000001p-16382", std::chars_format::hex, std::errc{}, 37, 0x0000000000000000ull, 0x0000000000000001ull},
  {"ffff.8", std::chars_format::hex, std::errc{}, 6, 0x400effff00000000ull, 0x0000000000000000ull},
  {"1p16384", std::chars_format::hex, std::errc::result_out_of_range, 7, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1p-16495", std::chars_format::hex, std::errc::result_out_of_range, 8, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1p-16496", std::chars_format::hex, std::errc::result_out_of_range, 8, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"0x1p0", std::chars_format::hex, std::errc{}, 1, 0x0000000000000000ull, 0x0000000000000000ull},
  {"1.199999999999999999999999999ap+0", std::chars_format::hex, std::errc{}, 33, 0x3fff199999999999ull, 0x999999999999999aull},
  {"p3", std::chars_format::hex, std::errc::invalid_argument, 0, 0x400c81c800000000ull, 0x0000000000000000ull},
  {"1p", std::chars_format::hex, std::errc{}, 1, 0x3fff000000000000ull, 0x0000000000000000ull},
};
static void test_table() {
  for (const Case& c : cases) {
    const F sentinel = make(0x400c81c800000000ull, 0); // 12345
    F value          = sentinel;
    std::from_chars_result r = std::from_chars(c.text, c.text + std::strlen(c.text), value, c.fmt);
    assert(r.ec == c.ec);
    assert(r.ptr == c.text + c.consumed || c.ec == std::errc::invalid_argument);
    if (c.ec == std::errc{}) {
      unsigned __int128 expected = (static_cast<unsigned __int128>(c.hi) << 64) | c.lo;
      bool is_nan = ((expected >> 112) & 0x7fff) == 0x7fff && (expected & ((static_cast<unsigned __int128>(1) << 112) - 1)) != 0;
      if (is_nan) {
        assert(value != value);
        assert((bits(value) >> 127) == (expected >> 127));
      } else {
        assert(bits(value) == expected);
      }
    } else {
      // value is unmodified on errors
      assert(bits(value) == bits(sentinel));
      if (c.ec == std::errc::invalid_argument)
        assert(r.ptr == c.text);
    }
  }
}

static std::errc parse_hex(const char* text, F& value) {
  return std::from_chars(text, text + std::strlen(text), value, std::chars_format::hex).ec;
}

// Exact halfway cases are rounded to even; values just above or below them are rounded to the nearest.
static void test_limits() {
  F value = 0;
  const unsigned __int128 largest = (static_cast<unsigned __int128>(0x7ffeffffffffffffull) << 64) | 0xffffffffffffffffull;
  assert(parse_hex("1.ffffffffffffffffffffffffffffp+16383", value) == std::errc{});
  assert(bits(value) == largest);
  // exactly half way between the largest finite value and 2^16384: the even neighbour is 2^16384, an overflow
  value = 1;
  assert(parse_hex("1.ffffffffffffffffffffffffffff8p+16383", value) == std::errc::result_out_of_range);
  assert(bits(value) == bits(F(1)));
  assert(parse_hex("1.ffffffffffffffffffffffffffff7fp+16383", value) == std::errc{});
  assert(bits(value) == largest);
  // half of the smallest subnormal: ties to even, i.e. zero, which is out of range
  value = 1;
  assert(parse_hex("1p-16495", value) == std::errc::result_out_of_range);
  assert(bits(value) == bits(F(1)));
  assert(parse_hex("1.0000000000000000000000000001p-16495", value) == std::errc{});
  assert(bits(value) == 1);
  // 1 + 2^-113 is half way between 1 and the next value: ties to even (1); anything above rounds up
  assert(parse_hex("1.00000000000000000000000000008p+0", value) == std::errc{});
  assert(bits(value) == bits(F(1)));
  assert(parse_hex("1.00000000000000000000000000008000001p+0", value) == std::errc{});
  assert(bits(value) == bits(F(1)) + 1);
}

int main(int, char**) {
  test_table();
  test_limits();
  return 0;
}

#else
int main(int, char**) { return 0; }
#endif
