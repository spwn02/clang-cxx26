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
  assert(std::strstr(buffer, "1.23456789012345678899") != nullptr);

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
  return 0;
}
