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

// std::to_chars and std::from_chars for std::float128_t (P1467R9, [charconv.syn]: every cv-unqualified
// floating-point type). The expected strings of the table were produced with glibc's strfromf128 (correctly rounded
// %e, %f, %g and %a conversions) and, for the shortest forms, checked to be the shortest decimal strings that
// round-trip.

#include <bit>
#include <cassert>
#include <charconv>
#include <cstring>
#include <stdfloat>
#include <string>
#include <string_view>

#include "test_macros.h"

#if defined(__STDCPP_FLOAT128_T__)

using F = std::float128_t;

struct Case {
  unsigned long long hi, lo;
  const char* plain;
  const char* scientific;
  const char* fixed;
  const char* general;
  const char* hex;
  const char* e0;
  const char* e5;
  const char* e35;
  const char* g1;
  const char* g10;
  const char* g36;
  const char* f0;
  const char* f3;
  const char* h0;
  const char* h3;
  const char* h28;
  const char* h30;
};

static F make(unsigned long long hi, unsigned long long lo) {
  return std::bit_cast<F>((static_cast<unsigned __int128>(hi) << 64) | lo);
}

static unsigned __int128 bits(F v) { return std::bit_cast<unsigned __int128>(v); }

static std::string run(F v) {
  char buffer[70000];
  std::to_chars_result r = std::to_chars(buffer, buffer + sizeof(buffer), v);
  assert(r.ec == std::errc{});
  return std::string(buffer, r.ptr);
}

static std::string run(F v, std::chars_format fmt) {
  char buffer[70000];
  std::to_chars_result r = std::to_chars(buffer, buffer + sizeof(buffer), v, fmt);
  assert(r.ec == std::errc{});
  return std::string(buffer, r.ptr);
}

static std::string run(F v, std::chars_format fmt, int precision) {
  char buffer[70000];
  std::to_chars_result r = std::to_chars(buffer, buffer + sizeof(buffer), v, fmt, precision);
  assert(r.ec == std::errc{});
  return std::string(buffer, r.ptr);
}

// clang-format off
static const Case cases[] = {
  {0x0000000000000000ull, 0x0000000000000000ull,
   "0", "0e+00", "0", "0", "0p+0", "0e+00",
   "0.00000e+00", "0.00000000000000000000000000000000000e+00", "0", "0", "0", "0",
   "0.000", "0p+0", "0.000p+0", "0.0000000000000000000000000000p+0", "0.000000000000000000000000000000p+0"},
  {0x8000000000000000ull, 0x0000000000000000ull,
   "-0", "-0e+00", "-0", "-0", "-0p+0", "-0e+00",
   "-0.00000e+00", "-0.00000000000000000000000000000000000e+00", "-0", "-0", "-0", "-0",
   "-0.000", "-0p+0", "-0.000p+0", "-0.0000000000000000000000000000p+0", "-0.000000000000000000000000000000p+0"},
  {0x3fff000000000000ull, 0x0000000000000000ull,
   "1", "1e+00", "1", "1", "1p+0", "1e+00",
   "1.00000e+00", "1.00000000000000000000000000000000000e+00", "1", "1", "1", "1",
   "1.000", "1p+0", "1.000p+0", "1.0000000000000000000000000000p+0", "1.000000000000000000000000000000p+0"},
  {0xbfff800000000000ull, 0x0000000000000000ull,
   "-1.5", "-1.5e+00", "-1.5", "-1.5", "-1.8p+0", "-2e+00",
   "-1.50000e+00", "-1.50000000000000000000000000000000000e+00", "-2", "-1.5", "-1.5", "-2",
   "-1.500", "-2p+0", "-1.800p+0", "-1.8000000000000000000000000000p+0", "-1.800000000000000000000000000000p+0"},
  {0x3ffb999999999999ull, 0x999999999999999aull,
   "0.1", "1e-01", "0.1", "0.1", "1.999999999999999999999999999ap-4", "1e-01",
   "1.00000e-01", "1.00000000000000000000000000000000005e-01", "0.1", "0.1", "0.100000000000000000000000000000000005", "0",
   "0.100", "2p-4", "1.99ap-4", "1.999999999999999999999999999ap-4", "1.999999999999999999999999999a00p-4"},
  {0x3ffd555555555555ull, 0x5555555555555555ull,
   "0.3333333333333333333333333333333333", "3.333333333333333333333333333333333e-01", "0.3333333333333333333333333333333333", "0.3333333333333333333333333333333333", "1.5555555555555555555555555555p-2", "3e-01",
   "3.33333e-01", "3.33333333333333333333333333333333317e-01", "0.3", "0.3333333333", "0.333333333333333333333333333333333317", "0",
   "0.333", "1p-2", "1.555p-2", "1.5555555555555555555555555555p-2", "1.555555555555555555555555555500p-2"},
  {0x4000921fb54442d1ull, 0x8469898cc51701b8ull,
   "3.1415926535897932384626433832795028", "3.1415926535897932384626433832795028e+00", "3.1415926535897932384626433832795028", "3.1415926535897932384626433832795028", "1.921fb54442d18469898cc51701b8p+1", "3e+00",
   "3.14159e+00", "3.14159265358979323846264338327950280e+00", "3", "3.141592654", "3.1415926535897932384626433832795028", "3",
   "3.142", "2p+1", "1.922p+1", "1.921fb54442d18469898cc51701b8p+1", "1.921fb54442d18469898cc51701b800p+1"},
  {0x40005bf0a8b14576ull, 0x95355fb8ac404e7aull,
   "2.7182818284590452353602874713526623", "2.7182818284590452353602874713526623e+00", "2.7182818284590452353602874713526623", "2.7182818284590452353602874713526623", "1.5bf0a8b1457695355fb8ac404e7ap+1", "3e+00",
   "2.71828e+00", "2.71828182845904523536028747135266231e+00", "3", "2.718281828", "2.71828182845904523536028747135266231", "3",
   "2.718", "1p+1", "1.5bfp+1", "1.5bf0a8b1457695355fb8ac404e7ap+1", "1.5bf0a8b1457695355fb8ac404e7a00p+1"},
  {0x4002400000000000ull, 0x0000000000000000ull,
   "10", "1e+01", "10", "10", "1.4p+3", "1e+01",
   "1.00000e+01", "1.00000000000000000000000000000000000e+01", "1e+01", "10", "10", "10",
   "10.000", "1p+3", "1.400p+3", "1.4000000000000000000000000000p+3", "1.400000000000000000000000000000p+3"},
  {0x40202a05f2000000ull, 0x0000000000000000ull,
   "1e+10", "1e+10", "10000000000", "1e+10", "1.2a05f2p+33", "1e+10",
   "1.00000e+10", "1.00000000000000000000000000000000000e+10", "1e+10", "1e+10", "10000000000", "10000000000",
   "10000000000.000", "1p+33", "1.2a0p+33", "1.2a05f20000000000000000000000p+33", "1.2a05f2000000000000000000000000p+33"},
  {0x4019d6f34547e6b7ull, 0x4df048579890dc7cull,
   "123456789.12345678912345678912345678", "1.2345678912345678912345678912345678e+08", "123456789.12345678912345678912345678", "1.2345678912345678912345678912345678e+08", "1.d6f34547e6b74df048579890dc7cp+26", "1e+08",
   "1.23457e+08", "1.23456789123456789123456789123456785e+08", "1e+08", "123456789.1", "123456789.123456789123456789123456785", "123456789",
   "123456789.123", "2p+26", "1.d6fp+26", "1.d6f34547e6b74df048579890dc7cp+26", "1.d6f34547e6b74df048579890dc7c00p+26"},
  {0x3fddb7cdfd9d7bdbull, 0xab7d6ae6881cb511ull,
   "1e-10", "1e-10", "0.0000000001", "1e-10", "1.b7cdfd9d7bdbab7d6ae6881cb511p-34", "1e-10",
   "1.00000e-10", "1.00000000000000000000000000000000004e-10", "1e-10", "1e-10", "1.00000000000000000000000000000000004e-10", "0",
   "0.000", "2p-34", "1.b7dp-34", "1.b7cdfd9d7bdbab7d6ae6881cb511p-34", "1.b7cdfd9d7bdbab7d6ae6881cb51100p-34"},
  {0x414b249ad2594c37ull, 0xceb0b2784c4ce0bfull,
   "1e+100", "1e+100", "", "1e+100", "1.249ad2594c37ceb0b2784c4ce0bfp+332", "1e+100",
   "1.00000e+100", "9.99999999999999999999999999999999963e+99", "1e+100", "1e+100", "9.99999999999999999999999999999999963e+99", "",
   "", "1p+332", "1.24ap+332", "1.249ad2594c37ceb0b2784c4ce0bfp+332", "1.249ad2594c37ceb0b2784c4ce0bf00p+332"},
  {0x73e6a3750647fcabull, 0x18c21ab905450cc3ull,
   "1e+4000", "1e+4000", "", "1e+4000", "1.a3750647fcab18c21ab905450cc3p+13287", "1e+4000",
   "1.00000e+4000", "1.00000000000000000000000000000000004e+4000", "1e+4000", "1e+4000", "1.00000000000000000000000000000000004e+4000", "",
   "", "2p+13287", "1.a37p+13287", "1.a3750647fcab18c21ab905450cc3p+13287", "1.a3750647fcab18c21ab905450cc300p+13287"},
  {0x8c17387ae70c9e70ull, 0x0b8049732d11a23dull,
   "-1e-4000", "-1e-4000", "", "-1e-4000", "-1.387ae70c9e700b8049732d11a23dp-13288", "-1e-4000",
   "-1.00000e-4000", "-1.00000000000000000000000000000000008e-4000", "-1e-4000", "-1e-4000", "-1.00000000000000000000000000000000008e-4000", "",
   "", "-1p-13288", "-1.388p-13288", "-1.387ae70c9e700b8049732d11a23dp-13288", "-1.387ae70c9e700b8049732d11a23d00p-13288"},
  {0x7ffeffffffffffffull, 0xffffffffffffffffull,
   "1.189731495357231765085759326628007e+4932", "1.189731495357231765085759326628007e+4932", "", "1.189731495357231765085759326628007e+4932", "1.ffffffffffffffffffffffffffffp+16383", "1e+4932",
   "1.18973e+4932", "1.18973149535723176508575932662800702e+4932", "1e+4932", "1.189731495e+4932", "1.18973149535723176508575932662800702e+4932", "",
   "", "2p+16383", "2.000p+16383", "1.ffffffffffffffffffffffffffffp+16383", "1.ffffffffffffffffffffffffffff00p+16383"},
  {0x0001000000000000ull, 0x0000000000000000ull,
   "3.3621031431120935062626778173217526e-4932", "3.3621031431120935062626778173217526e-4932", "", "3.3621031431120935062626778173217526e-4932", "1p-16382", "3e-4932",
   "3.36210e-4932", "3.36210314311209350626267781732175260e-4932", "3e-4932", "3.362103143e-4932", "3.3621031431120935062626778173217526e-4932", "",
   "", "1p-16382", "1.000p-16382", "1.0000000000000000000000000000p-16382", "1.000000000000000000000000000000p-16382"},
  {0x0000000000000000ull, 0x0000000000000001ull,
   "6e-4966", "6e-4966", "", "6e-4966", "0.0000000000000000000000000001p-16382", "6e-4966",
   "6.47518e-4966", "6.47517511943802511092443895822764655e-4966", "6e-4966", "6.475175119e-4966", "6.47517511943802511092443895822764655e-4966", "",
   "", "0p-16382", "0.000p-16382", "0.0000000000000000000000000001p-16382", "0.000000000000000000000000000100p-16382"},
  {0x406f000000000000ull, 0x0000000000000000ull,
   "5192296858534827628530496329220096", "5.192296858534827628530496329220096e+33", "5192296858534827628530496329220096", "5.192296858534827628530496329220096e+33", "1p+112", "5e+33",
   "5.19230e+33", "5.19229685853482762853049632922009600e+33", "5e+33", "5.192296859e+33", "5192296858534827628530496329220096", "5192296858534827628530496329220096",
   "5192296858534827628530496329220096.000", "1p+112", "1.000p+112", "1.0000000000000000000000000000p+112", "1.000000000000000000000000000000p+112"},
  {0x3ffe000000000000ull, 0x0000000000000000ull,
   "0.5", "5e-01", "0.5", "0.5", "1p-1", "5e-01",
   "5.00000e-01", "5.00000000000000000000000000000000000e-01", "0.5", "0.5", "0.5", "0",
   "0.500", "1p-1", "1.000p-1", "1.0000000000000000000000000000p-1", "1.000000000000000000000000000000p-1"},
  {0x3fff800000000000ull, 0x0000000000000000ull,
   "1.5", "1.5e+00", "1.5", "1.5", "1.8p+0", "2e+00",
   "1.50000e+00", "1.50000000000000000000000000000000000e+00", "2", "1.5", "1.5", "2",
   "1.500", "2p+0", "1.800p+0", "1.8000000000000000000000000000p+0", "1.800000000000000000000000000000p+0"},
  {0x4000400000000000ull, 0x0000000000000000ull,
   "2.5", "2.5e+00", "2.5", "2.5", "1.4p+1", "2e+00",
   "2.50000e+00", "2.50000000000000000000000000000000000e+00", "2", "2.5", "2.5", "2",
   "2.500", "1p+1", "1.400p+1", "1.4000000000000000000000000000p+1", "1.400000000000000000000000000000p+1"},
  {0x3ffc000000000000ull, 0x0000000000000000ull,
   "0.125", "1.25e-01", "0.125", "0.125", "1p-3", "1e-01",
   "1.25000e-01", "1.25000000000000000000000000000000000e-01", "0.1", "0.125", "0.125", "0",
   "0.125", "1p-3", "1.000p-3", "1.0000000000000000000000000000p-3", "1.000000000000000000000000000000p-3"},
  {0x40023ffcb923a29cull, 0x779a6b50b0f27bb3ull,
   "9.9996", "9.9996e+00", "9.9996", "9.9996", "1.3ffcb923a29c779a6b50b0f27bb3p+3", "1e+01",
   "9.99960e+00", "9.99960000000000000000000000000000001e+00", "1e+01", "9.9996", "9.99960000000000000000000000000000001", "10",
   "10.000", "1p+3", "1.400p+3", "1.3ffcb923a29c779a6b50b0f27bb3p+3", "1.3ffcb923a29c779a6b50b0f27bb300p+3"},
  {0x400f869f80000000ull, 0x0000000000000000ull,
   "99999.5", "9.99995e+04", "99999.5", "99999.5", "1.869f8p+16", "1e+05",
   "9.99995e+04", "9.99995000000000000000000000000000000e+04", "1e+05", "99999.5", "99999.5", "100000",
   "99999.500", "2p+16", "1.86ap+16", "1.869f800000000000000000000000p+16", "1.869f80000000000000000000000000p+16"},
  {0x3ff1a36d1bd105b0ull, 0x5cee5c585f566b8bull,
   "9.9999e-05", "9.9999e-05", "0.000099999", "9.9999e-05", "1.a36d1bd105b05cee5c585f566b8bp-14", "1e-04",
   "9.99990e-05", "9.99989999999999999999999999999999960e-05", "0.0001", "9.9999e-05", "9.9998999999999999999999999999999996e-05", "0",
   "0.000", "2p-14", "1.a37p-14", "1.a36d1bd105b05cee5c585f566b8bp-14", "1.a36d1bd105b05cee5c585f566b8b00p-14"},
  {0x40480f0cf064dd59ull, 0x2000000000000000ull,
   "1e+22", "1e+22", "", "1e+22", "1.0f0cf064dd592p+73", "1e+22",
   "1.00000e+22", "1.00000000000000000000000000000000000e+22", "1e+22", "1e+22", "10000000000000000000000", "",
   "", "1p+73", "1.0f1p+73", "1.0f0cf064dd592000000000000000p+73", "1.0f0cf064dd59200000000000000000p+73"},
  {0x400fe24000000000ull, 0x0000000000000000ull,
   "123456", "1.23456e+05", "123456", "123456", "1.e24p+16", "1e+05",
   "1.23456e+05", "1.23456000000000000000000000000000000e+05", "1e+05", "123456", "123456", "123456",
   "123456.000", "2p+16", "1.e24p+16", "1.e240000000000000000000000000p+16", "1.e24000000000000000000000000000p+16"},
  {0x400f86a000000000ull, 0x0000000000000000ull,
   "1e+05", "1e+05", "100000", "100000", "1.86ap+16", "1e+05",
   "1.00000e+05", "1.00000000000000000000000000000000000e+05", "1e+05", "100000", "100000", "100000",
   "100000.000", "2p+16", "1.86ap+16", "1.86a0000000000000000000000000p+16", "1.86a000000000000000000000000000p+16"},
  {0x4012e84800000000ull, 0x0000000000000000ull,
   "1e+06", "1e+06", "1000000", "1e+06", "1.e848p+19", "1e+06",
   "1.00000e+06", "1.00000000000000000000000000000000000e+06", "1e+06", "1000000", "1000000", "1000000",
   "1000000.000", "2p+19", "1.e84p+19", "1.e848000000000000000000000000p+19", "1.e84800000000000000000000000000p+19"},
  {0x3ff1a36e2eb1c432ull, 0xca57a786c226809dull,
   "1e-04", "1e-04", "0.0001", "0.0001", "1.a36e2eb1c432ca57a786c226809dp-14", "1e-04",
   "1.00000e-04", "9.99999999999999999999999999999999966e-05", "0.0001", "0.0001", "9.99999999999999999999999999999999966e-05", "0",
   "0.000", "2p-14", "1.a37p-14", "1.a36e2eb1c432ca57a786c226809dp-14", "1.a36e2eb1c432ca57a786c226809d00p-14"},
  {0x3fee4f8b588e368full, 0x08461f9f01b866e4ull,
   "1e-05", "1e-05", "0.00001", "1e-05", "1.4f8b588e368f08461f9f01b866e4p-17", "1e-05",
   "1.00000e-05", "9.99999999999999999999999999999999966e-06", "1e-05", "1e-05", "9.99999999999999999999999999999999966e-06", "0",
   "0.000", "1p-17", "1.4f9p-17", "1.4f8b588e368f08461f9f01b866e4p-17", "1.4f8b588e368f08461f9f01b866e400p-17"},
};
static void test_table() {
  using std::chars_format;
  for (const Case& c : cases) {
    F v = make(c.hi, c.lo);
    assert(run(v) == c.plain);
    assert(run(v, chars_format::scientific) == c.scientific);
    if (*c.fixed)
      assert(run(v, chars_format::fixed) == c.fixed);
    assert(run(v, chars_format::general) == c.general);
    assert(run(v, chars_format::hex) == c.hex);
    assert(run(v, chars_format::scientific, 0) == c.e0);
    assert(run(v, chars_format::scientific, 5) == c.e5);
    assert(run(v, chars_format::scientific, 35) == c.e35);
    assert(run(v, chars_format::general, 1) == c.g1);
    assert(run(v, chars_format::general, 10) == c.g10);
    assert(run(v, chars_format::general, 36) == c.g36);
    if (*c.f0)
      assert(run(v, chars_format::fixed, 0) == c.f0);
    if (*c.f3)
      assert(run(v, chars_format::fixed, 3) == c.f3);
    assert(run(v, chars_format::hex, 0) == c.h0);
    assert(run(v, chars_format::hex, 3) == c.h3);
    assert(run(v, chars_format::hex, 28) == c.h28);
    assert(run(v, chars_format::hex, 30) == c.h30);

    // Every produced form of a finite value parses back to the same value.
    for (std::string s : {run(v), run(v, chars_format::scientific), run(v, chars_format::general)}) {
      F back = 0;
      std::from_chars_result r = std::from_chars(s.data(), s.data() + s.size(), back);
      assert(r.ec == std::errc{} && r.ptr == s.data() + s.size() && bits(back) == bits(v));
    }
    {
      std::string s = run(v, chars_format::hex);
      F back = 0;
      std::from_chars_result r = std::from_chars(s.data(), s.data() + s.size(), back, chars_format::hex);
      assert(r.ec == std::errc{} && r.ptr == s.data() + s.size() && bits(back) == bits(v));
    }
  }
}

static void test_special_values() {
  F inf = make(0x7fff000000000000ull, 0);
  F nan = make(0x7fff800000000000ull, 0);
  F snan = make(0x7fff400000000000ull, 0);
  assert(run(inf) == "inf");
  assert(run(-inf) == "-inf");
  assert(run(nan) == "nan");
  assert(run(make(0xffff800000000000ull, 1)) == "-nan");
  assert(run(-nan) == "-nan(ind)"); // sign and quiet bit set, no payload: the "indeterminate" NaN
  assert(run(snan) == "nan(snan)");
  assert(run(inf, std::chars_format::hex, 3) == "inf");
  assert(run(nan, std::chars_format::fixed, 2) == "nan");
}

static void test_buffer_too_small() {
  F v = make(0x3fff000000000000ull, 0); // 1
  char buffer[64];
  std::string expected = run(v, std::chars_format::scientific, 10);
  assert(expected == "1.0000000000e+00");
  std::to_chars_result r = std::to_chars(buffer, buffer + expected.size(), v, std::chars_format::scientific, 10);
  assert(r.ec == std::errc{} && r.ptr == buffer + expected.size());
  r = std::to_chars(buffer, buffer + expected.size() - 1, v, std::chars_format::scientific, 10);
  assert(r.ec == std::errc::value_too_large && r.ptr == buffer + expected.size() - 1);
  r = std::to_chars(buffer, buffer, v);
  assert(r.ec == std::errc::value_too_large && r.ptr == buffer);
  // the minus sign counts
  r = std::to_chars(buffer, buffer + 1, -v);
  assert(r.ec == std::errc::value_too_large);
  // trailing zeros of %g are removed: "1" fits in one character
  r = std::to_chars(buffer, buffer + 1, v, std::chars_format::general, 10);
  assert(r.ec == std::errc{} && r.ptr == buffer + 1 && buffer[0] == '1');
}

static unsigned long long state = 0x9e3779b97f4a7c15ull;
static unsigned long long next_random() {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

static void test_round_trips() {
  using std::chars_format;
  for (int i = 0; i < 300; ++i) {
    unsigned long long hi = next_random(), lo = next_random();
    if (i % 3 == 1) // moderate exponents
      hi = (hi & 0x8000ffffffffffffull) | ((16383ull - 80 + next_random() % 160) << 48);
    else if (i % 3 == 2) // subnormals
      hi &= 0x8000ffffffffffffull;
    if (((hi >> 48) & 0x7fff) == 0x7fff)
      continue;
    F v = make(hi, lo);
    for (chars_format fmt : {chars_format::scientific, chars_format::general, chars_format::hex}) {
      std::string s = run(v, fmt);
      F back = 0;
      std::from_chars_result r = std::from_chars(s.data(), s.data() + s.size(), back, fmt);
      assert(r.ec == std::errc{} && r.ptr == s.data() + s.size() && bits(back) == bits(v));
    }
    std::string s = run(v);
    F back = 0;
    std::from_chars_result r = std::from_chars(s.data(), s.data() + s.size(), back);
    assert(r.ec == std::errc{} && bits(back) == bits(v));
    // the shortest form has no digit that can be dropped
    std::string sci = run(v, chars_format::scientific);
    std::size_t e = sci.find('e');
    std::string digits = sci.substr(0, e);
    int count = 0;
    for (char c : digits)
      count += c >= '0' && c <= '9';
    if (count > 1) {
      std::string shorter = run(v, chars_format::scientific, count - 2);
      F parsed = 0;
      std::from_chars(shorter.data(), shorter.data() + shorter.size(), parsed);
      assert(bits(parsed) != bits(v));
    }
  }
}

int main(int, char**) {
  test_table();
  test_special_values();
  test_buffer_too_small();
  test_round_trips();
  return 0;
}

#else
int main(int, char**) { return 0; }
#endif
