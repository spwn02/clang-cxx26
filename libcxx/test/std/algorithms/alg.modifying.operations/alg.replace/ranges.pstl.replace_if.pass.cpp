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
template <class P> void test(P&& p) { std::array<int, 4> a{1, 2, 3, 4}; auto i = std::ranges::replace_if(p, a.begin(), a.end(), [](int x) { return x % 2 == 0; }, 0); assert(i == a.end() && a[1] == 0 && a[3] == 0); i = std::ranges::replace_if(p, a, [](int x) { return x == 0; }, 8); assert(i == a.end()); }
int main(int, char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
