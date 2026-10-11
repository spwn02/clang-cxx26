// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <format>

// std::formatter<std::float128_t, charT> ([format.formatter.spec]: one for every cv-unqualified floating-point
// type). The expected strings of the table were produced with glibc's strfromf128 (%e, %g, %a, %f).

#include <cassert>
#include <format>
#include <stdfloat>
#include <string>

#include "test_macros.h"

#if defined(__STDCPP_FLOAT128_T__)

using F = std::float128_t;

static_assert(std::formattable<F, char>);
static_assert(std::formattable<F, wchar_t>);

struct Case {
  unsigned long long hi, lo;
  const char* dflt;
  const char* e;
  const char* e20;
  const char* g;
  const char* g20;
  const char* a;
  const char* a5;
  const char* f10;
  const char* f40;
};

static F make(unsigned long long hi, unsigned long long lo) {
  return __builtin_bit_cast(F, (static_cast<unsigned __int128>(hi) << 64) | lo);
}

// clang-format off
static const Case cases[] = {
  {0x3fff800000000000ull, 0x0000000000000000ull, "1.5",
   "1.500000e+00", "1.50000000000000000000e+00", "1.5", "1.5", "1.8p+0", "1.80000p+0", "1.5000000000", "1.5000000000000000000000000000000000000000"},
  {0xbffb999999999999ull, 0x999999999999999aull, "-0.1",
   "-1.000000e-01", "-1.00000000000000000000e-01", "-0.1", "-0.1", "-1.999999999999999999999999999ap-4", "-1.9999ap-4", "-0.1000000000", "-0.1000000000000000000000000000000000048148"},
  {0x4000921fb54442d1ull, 0x8469898cc51701b8ull, "3.1415926535897932384626433832795028",
   "3.141593e+00", "3.14159265358979323846e+00", "3.14159", "3.1415926535897932385", "1.921fb54442d18469898cc51701b8p+1", "1.921fbp+1", "3.1415926536", "3.1415926535897932384626433832795027974791"},
  {0x414b249ad2594c37ull, 0xceb0b2784c4ce0bfull, "1e+100",
   "1.000000e+100", "1.00000000000000000000e+100", "1e+100", "1e+100", "1.249ad2594c37ceb0b2784c4ce0bfp+332", "1.249adp+332", "", ""},
  {0x3eb2bff2ee48e052ull, 0xfd7ab2f0fc572779ull, "1e-100",
   "1.000000e-100", "1.00000000000000000000e-100", "1e-100", "1e-100", "1.bff2ee48e052fd7ab2f0fc572779p-333", "1.bff2fp-333", "", ""},
  {0x4019d6f34547e6b7ull, 0x4df048579890dc7cull, "123456789.12345678912345678912345678",
   "1.234568e+08", "1.23456789123456789123e+08", "1.23457e+08", "123456789.12345678912", "1.d6f34547e6b74df048579890dc7cp+26", "1.d6f34p+26", "123456789.1234567891", "123456789.1234567891234567891234567847874802902405"},
  {0x7ffeffffffffffffull, 0xffffffffffffffffull, "1.189731495357231765085759326628007e+4932",
   "1.189731e+4932", "1.18973149535723176509e+4932", "1.18973e+4932", "1.1897314953572317651e+4932", "1.ffffffffffffffffffffffffffffp+16383", "2.00000p+16383", "", ""},
  {0x0000000000000000ull, 0x0000000000000001ull, "6e-4966",
   "6.475175e-4966", "6.47517511943802511092e-4966", "6.47518e-4966", "6.4751751194380251109e-4966", "0.0000000000000000000000000001p-16382", "0.00000p-16382", "", ""},
  {0x3ffe000000000000ull, 0x0000000000000000ull, "0.5",
   "5.000000e-01", "5.00000000000000000000e-01", "0.5", "0.5", "1p-1", "1.00000p-1", "0.5000000000", "0.5000000000000000000000000000000000000000"},
  {0x40023ffcb923a29cull, 0x779a6b50b0f27bb3ull, "9.9996",
   "9.999600e+00", "9.99960000000000000000e+00", "9.9996", "9.9996", "1.3ffcb923a29c779a6b50b0f27bb3p+3", "1.3ffccp+3", "9.9996000000", "9.9996000000000000000000000000000000073956"},
};
static void test_table() {
  for (const Case& c : cases) {
    F v = make(c.hi, c.lo);
    assert(std::format("{}", v) == c.dflt);
    assert(std::format("{:e}", v) == c.e);
    assert(std::format("{:.20e}", v) == c.e20);
    assert(std::format("{:g}", v) == c.g);
    assert(std::format("{:.20g}", v) == c.g20);
    assert(std::format("{:a}", v) == c.a);
    assert(std::format("{:.5a}", v) == c.a5);
    if (*c.f10) {
      assert(std::format("{:.10f}", v) == c.f10);
      assert(std::format("{:.40f}", v) == c.f40);
    }
#  ifndef TEST_HAS_NO_WIDE_CHARACTERS
    std::wstring w = std::format(L"{:.20e}", v);
    std::string n  = c.e20;
    assert(w == std::wstring(n.begin(), n.end()));
#  endif
  }
}

// For a value that is exactly representable in double the output of every format specification except the default
// one (the shortest representation of the type itself) equals the output for the double.
static void test_matches_double() {
  const double values[] = {0.0, -0.0, 1.0, -2.5, 0.1, 1.0 / 3, 3.14159265358979, 1e22, 1.5e-7, 123456.789, 1.7976931348623157e308,
                           0.000123456, 99999.5, 0.5, 1e15, 12345678.0};
  const char* specs[] = {"{:e}",   "{:E}",    "{:f}",       "{:F}",       "{:g}",       "{:G}",      "{:a}",       "{:A}",
                         "{:.0f}", "{:.3e}",  "{:#.3g}",    "{:+}",       "{:>20f}",    "{:*^25.4f}", "{:020.5e}", "{:<15g}",
                         "{:#a}",  "{:.0a}",  "{:+#020.2f}", "{:.10g}",    "{:8.3e}",    "{:.30f}",   "{:.60e}",    "{:+.1a}"};
  for (double d : values) {
    F v = static_cast<F>(d);
    for (const char* spec : specs) {
      // {:+} without a type is the shortest form of the type: compared below only for the exact binary integer values.
      if (std::string(spec) == "{:+}")
        continue;
      assert(std::vformat(spec, std::make_format_args(d)) == std::vformat(spec, std::make_format_args(v)));
    }
  }
}

static void test_special_values() {
  F inf = make(0x7fff000000000000ull, 0);
  F nan = make(0x7fff800000000000ull, 0);
  assert(std::format("{} {} {}", inf, -inf, nan) == "inf -inf nan");
  assert(std::format("{:+} {:010} {:<6}|", inf, inf, nan) == "+inf        inf nan   |");
  assert(std::format("{:e} {:F}", inf, -nan) == "inf -NAN");
  assert(std::format("{:08}", make(0xffff800000000001ull, 0)) == "    -nan");
  assert(std::format("{} {}", make(0, 0), make(0x8000000000000000ull, 0)) == "0 -0");
}

static void test_dynamic_options() {
  F v = make(0x3fff800000000000ull, 0); // 1.5
  assert(std::format("{:{}.{}f}", v, 12, 4) == "      1.5000");
  assert(std::format("{:.{}e}", v, 3) == "1.500e+00");
  // huge values and precisions (the exact decimal expansion)
  F big = make(0x7ffeffffffffffffull, 0xffffffffffffffffull);
  std::string s = std::format("{:.0f}", big);
  assert(s.size() == 4933 && s.compare(0, 20, "11897314953572317650") == 0);
  std::string tiny = std::format("{:.5000f}", make(0, 1));
  assert(tiny.size() == 5002 && tiny.substr(0, 5) == "0.000");
}

int main(int, char**) {
  test_table();
  test_matches_double();
  test_special_values();
  test_dynamic_options();
  return 0;
}

#else
int main(int, char**) { return 0; }
#endif
