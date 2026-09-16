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
struct X { int v; };
template<class P> void test(P p) { std::array<X, 4> a{{{1},{2},{3},{4}}}; auto f=[](int x){return x<3;}; assert(std::ranges::find_if_not(p,a.begin(),a.end(),f,&X::v)==a.begin()+2); assert(std::ranges::find_if_not(p,a,f,&X::v)==a.begin()+2); }
int main(int,char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
