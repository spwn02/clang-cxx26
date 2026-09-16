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
template<class P> void test(P&& p) { std::array<X,3> a{{{1},{2},{3}}}, b{{{1},{2},{3}}}; assert(std::ranges::equal(p,a.begin(),a.end(),b.begin(),b.end(),{},&X::v,&X::v)); assert(std::ranges::equal(p,a,b,{},&X::v,&X::v)); }
int main(int,char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
