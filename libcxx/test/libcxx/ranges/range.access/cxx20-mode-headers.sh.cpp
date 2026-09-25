//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// C++23/26 facilities (P2278R4 basic_const_iterator, P3235R3 print opt-ins in
// <chrono>) must not leak into C++20 mode: <ranges> (and everything that includes __ranges/access.h) failed to
// compile at -std=c++20 with "use of undeclared identifier 'constant_iterator'"
// while every lit run used the default (newest) standard. Compile the public
// headers and use ranges::cbegin/cend/crbegin/crend at both standards.

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17

// RUN: %{cxx} %{flags} %{compile_flags} -std=c++20 -fsyntax-only %s
// RUN: %{cxx} %{flags} %{compile_flags} -std=c++23 -fsyntax-only %s

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <iterator>
#include <mdspan>
#include <ranges>
#include <regex>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

void test() {
  std::vector<int> v{1, 2, 3};
  auto b  = std::ranges::cbegin(v);
  auto e  = std::ranges::cend(v);
  auto rb = std::ranges::crbegin(v);
  auto re = std::ranges::crend(v);
  (void)b, (void)e, (void)rb, (void)re;
}
