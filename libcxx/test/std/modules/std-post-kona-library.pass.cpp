//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build, gcc, no-threads
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

// The names added by P3104R6 (bit permutations), P4052R0 (saturating_*) and P2019R9 (thread attributes) are exported.

import std;

int main(int, char**) {
  // P3104R6
  if (std::bit_reverse(std::uint8_t{0b0000'0001}) != 0b1000'0000)
    return 1;
  if (std::bit_repeat(std::uint8_t{0b01}, 2) != 0b0101'0101)
    return 2;
  if (std::bit_compress(std::uint8_t{0b1010'1010}, std::uint8_t{0b1100'1100}) != 0b0000'1010)
    return 3;
  if (std::bit_expand(std::uint8_t{0b0000'0011}, std::uint8_t{0b0000'0110}) != 0b0000'0110)
    return 4;

  // P4052R0
  if (std::saturating_add(std::uint8_t{250}, std::uint8_t{10}) != 255)
    return 5;
  if (std::saturating_cast<std::uint8_t>(1000) != 255)
    return 6;
  if (std::saturating_sub(std::uint8_t{3}, std::uint8_t{10}) != 0 || std::saturating_mul(std::int8_t{100}, std::int8_t{2}) != 127 ||
      std::saturating_div(std::int8_t{-128}, std::int8_t{-1}) != 127)
    return 7;

  // P2019R9: the attributes, including the deduction guides, and the jthread aliases
  int x = 0;
  std::thread t(std::thread::name_hint("x"), std::thread::stack_size_hint(1 << 20), [&x] { x = 1; });
  t.join();
  std::jthread j(std::jthread::name_hint("y"), std::jthread::stack_size_hint(1 << 20), [&x](std::stop_token) { x += 1; });
  j.join();
  if (x != 2)
    return 8;
  return 0;
}
