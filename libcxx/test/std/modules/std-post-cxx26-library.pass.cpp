//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: clang-modules-build, gcc, no-threads
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

// The names of the papers merged into the draft after the C++26 DIS are exported in C++29:
// P3104R6 (bit permutations), P3242R4 (copy and fill for mdspan), P2019R9 (thread attributes).

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

  // P3242R4
  int src[6] = {1, 2, 3, 4, 5, 6}, dst[6] = {};
  std::mdspan ms(src, 2, 3), md(dst, 2, 3);
  std::copy(ms, md);
  if (dst[0] != 1 || dst[5] != 6)
    return 5;
  std::fill(md, 4);
  if (dst[0] != 4 || dst[5] != 4)
    return 6;

  // P2019R9: the attributes and the jthread aliases
  int x = 0;
  std::thread t(std::thread::name_hint("x"), std::thread::stack_size_hint(1 << 20), [&x] { x = 1; });
  t.join();
  if (x != 1)
    return 7;
  std::jthread::stack_size_hint hint(1 << 20);
  (void)hint;
  return 0;
}
