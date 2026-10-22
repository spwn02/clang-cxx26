//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14
// XFAIL: availability-fp_from_chars-missing

// <charconv>

// from_chars_result from_chars(const char* first, const char* last, long double& value, chars_format fmt);
// to_chars_result to_chars(char* first, char* last, long double value);

// The pattern of [charconv.from.chars] is matched, the value is unmodified unless the conversion succeeds,
// and a subnormal result is not out of range.

#include <charconv>
#include <cassert>
#include <cstring>
#include <limits>

#include "test_macros.h"

static std::from_chars_result
parse(const char* s, long double& v, std::chars_format f = std::chars_format::general) {
  return std::from_chars(s, s + std::strlen(s), v, f);
}

int main(int, char**) {
  using F = std::chars_format;
  long double v = 111;

  assert(parse(" 1", v).ec == std::errc::invalid_argument && v == 111);
  assert(parse("+1", v).ec == std::errc::invalid_argument && v == 111);
  assert(parse("-", v).ec == std::errc::invalid_argument && v == 111);
  assert(parse(".", v).ec == std::errc::invalid_argument && v == 111);

  auto r = parse("1e3", v, F::fixed); // the exponent is not part of the fixed pattern
  assert(r.ec == std::errc{} && r.ptr[0] == 'e' && v == 1);
  assert(parse("1.5", v, F::scientific).ec == std::errc::invalid_argument); // the exponent is required
  r = parse("1.5e2", v, F::scientific);
  assert(r.ec == std::errc{} && *r.ptr == '\0' && v == 150);
  r = parse("ff.8", v, F::hex); // no 0x prefix
  assert(r.ec == std::errc{} && v == 255.5L);
  r = parse("0x1p3", v, F::hex);
  assert(r.ec == std::errc{} && r.ptr[0] == 'x' && v == 0);
  r = parse("-infinity", v);
  assert(r.ec == std::errc{} && v == -std::numeric_limits<long double>::infinity());
  r = parse("nan(abc_1)", v);
  assert(r.ec == std::errc{} && *r.ptr == '\0' && v != v);

  v = 111;
  assert(parse("1e99999", v).ec == std::errc::result_out_of_range && v == 111);
  assert(parse("-1e99999", v).ec == std::errc::result_out_of_range && v == 111);
  assert(parse("1e-99999", v).ec == std::errc::result_out_of_range && v == 111);

  // Subnormal values are representable.
  const long double denorm = std::numeric_limits<long double>::denorm_min();
  char buf[64];
  auto t = std::to_chars(buf, buf + sizeof(buf), denorm);
  assert(t.ec == std::errc{});
  *t.ptr = '\0';
  r      = parse(buf, v);
  assert(r.ec == std::errc{} && v == denorm);

  const long double sub = std::numeric_limits<long double>::min() / 3;
  t                     = std::to_chars(buf, buf + sizeof(buf), sub);
  *t.ptr                = '\0';
  r                     = parse(buf, v);
  assert(r.ec == std::errc{} && v == sub);

  return 0;
}
