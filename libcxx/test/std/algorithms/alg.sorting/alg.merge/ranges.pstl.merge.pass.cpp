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
template <class P> void test(P&& p) { std::array<int, 3> a{1, 3, 5}, b{2, 4, 6}, out{}; auto r = std::ranges::merge(p, a.begin(), a.end(), b.begin(), b.end(), out.begin()); assert(r.in1 == a.end() && r.in2 == b.end() && r.out == out.end() && out[2] == 3); auto q = std::ranges::merge(p, a, b, out.begin()); assert(q.in1 == a.end() && q.in2 == b.end() && q.out == out.end()); }
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
