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
#include <forward_list>
struct X { int v; };
template<class P> void test(P p) { std::array<X, 4> a{{{1},{2},{3},{4}}}; assert(std::ranges::find(p,a.begin(),a.end(),3,&X::v)==a.begin()+2); assert(std::ranges::find(p,a,4,&X::v)==a.begin()+3); }
template<class P> concept Has = requires(P p, std::forward_list<int>& r) { std::ranges::find(p,r,1); };
static_assert(!Has<decltype(std::execution::seq)>);
int main(int,char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
