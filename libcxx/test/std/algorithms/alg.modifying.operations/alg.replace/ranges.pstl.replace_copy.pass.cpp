//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl
#include <algorithm>
#include <array>
#include <cassert>
#include <execution>
template <class P> void test(P&& p) { std::array<int, 4> a{1, 2, 1, 3}, b{}; auto r = std::ranges::replace_copy(p, a.begin(), a.end(), b.begin(), 1, 9); assert(r.in == a.end() && r.out == b.end() && b[0] == 9 && b[2] == 9); auto q = std::ranges::replace_copy(p, a, b.begin(), 3, 7); assert(q.in == a.end() && q.out == b.end()); }
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
