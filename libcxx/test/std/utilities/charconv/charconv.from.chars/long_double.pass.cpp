//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// <charconv>

#include <charconv>
#include <cassert>
#include <cstring>

int main(int, char**) {
  constexpr long double expected = 1.234567890123456789L;
  char buffer[128];

  std::to_chars_result out = std::to_chars(buffer, buffer + sizeof(buffer), expected);
  assert(out.ec == std::errc{});
  *out.ptr = '\0';
  // Shortest round-trip representation, not a fixed number of digits.
  assert(std::strcmp(buffer, "1.234567890123456789") == 0);

  long double actual = 0;
  std::from_chars_result in = std::from_chars(buffer, out.ptr, actual);
  assert(in.ec == std::errc{});
  assert(in.ptr == out.ptr);
  assert(actual == expected);

  out = std::to_chars(buffer, buffer + sizeof(buffer), expected, std::chars_format::fixed, 18);
  assert(out.ec == std::errc{});
  assert(std::strcmp(buffer, "1.234567890123456789") == 0);

  out = std::to_chars(buffer, buffer + sizeof(buffer), expected, std::chars_format::hex);
  assert(out.ec == std::errc{});
  actual = 0;
  in = std::from_chars(buffer, out.ptr, actual, std::chars_format::hex);
  assert(in.ec == std::errc{});
  assert(in.ptr == out.ptr);
  assert(actual == expected);

  // Hexadecimal output is normalized (a single leading 1), not glibc's x87 "d.5ep-3" form.
  out = std::to_chars(buffer, buffer + sizeof(buffer), 0x1.abcp+0L, std::chars_format::hex);
  assert(out.ec == std::errc{});
  *out.ptr = '\0';
  assert(std::strcmp(buffer, "1.abcp+0") == 0);
  out = std::to_chars(buffer, buffer + sizeof(buffer), 0x1.abcp+0L, std::chars_format::hex, 2);
  *out.ptr = '\0';
  assert(std::strcmp(buffer, "1.ac" "p+0") == 0);
  out = std::to_chars(buffer, buffer + sizeof(buffer), 1.5L, std::chars_format::hex, 20);
  *out.ptr = '\0';
  assert(std::strcmp(buffer, "1.80000000000000000000p+0") == 0);
  out = std::to_chars(buffer, buffer + sizeof(buffer), 0.0L, std::chars_format::hex);
  *out.ptr = '\0';
  assert(std::strcmp(buffer, "0p+0") == 0);

  // Shortest representations of values that are not exactly representable.
  out = std::to_chars(buffer, buffer + sizeof(buffer), 0.1L);
  *out.ptr = '\0';
  assert(std::strcmp(buffer, "0.1") == 0);
  out = std::to_chars(buffer, buffer + sizeof(buffer), 1e30L, std::chars_format::scientific);
  *out.ptr = '\0';
  assert(std::strcmp(buffer, "1e+30") == 0);
  return 0;
}
