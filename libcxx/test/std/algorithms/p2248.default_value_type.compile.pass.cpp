//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// P2248R8: the algorithms that take a value argument have a default template argument for its type, so that a
// braced-init-list can be passed. Each function below has to compile, for the algorithms of <algorithm>, <numeric>
// and the uninitialized algorithms of <memory>, for the classic and the std::ranges versions.

#include <algorithm>
#include <memory>
#include <numeric>
#include <vector>
#include <ranges>
struct P { int a = 0; int b = 0; constexpr P() = default; constexpr P(int x, int y) : a(x), b(y) {} constexpr bool operator==(const P&) const = default; constexpr auto operator<=>(const P&) const = default; };
// each case is a function that must compile; failing ones are reported by line
void c_fill(std::vector<P>& v)                { std::fill(v.begin(), v.end(), {}); }
void c_fill_n(std::vector<P>& v)              { std::fill_n(v.begin(), 2, {}); }
void c_replace(std::vector<P>& v)             { std::replace(v.begin(), v.end(), {}, {1, 2}); }
void c_replace_if(std::vector<P>& v)          { std::replace_if(v.begin(), v.end(), [](const P&){ return true; }, {1, 2}); }
void c_replace_copy_if(std::vector<P>& v)     { std::replace_copy_if(v.begin(), v.end(), v.begin(), [](const P&){ return true; }, {1, 2}); }
void c_remove(std::vector<P>& v)              { (void)std::remove(v.begin(), v.end(), {}); }
void c_remove_copy(std::vector<P>& v)         { (void)std::remove_copy(v.begin(), v.end(), v.begin(), {}); }
void c_count(std::vector<P>& v)               { (void)std::count(v.begin(), v.end(), {}); }
void c_find(std::vector<P>& v)                { (void)std::find(v.begin(), v.end(), {}); }
void c_search_n(std::vector<P>& v)            { (void)std::search_n(v.begin(), v.end(), 2, {}); }
void c_lower_bound(std::vector<P>& v)         { (void)std::lower_bound(v.begin(), v.end(), {}); }
void c_upper_bound(std::vector<P>& v)         { (void)std::upper_bound(v.begin(), v.end(), {}); }
void c_equal_range(std::vector<P>& v)         { (void)std::equal_range(v.begin(), v.end(), {}); }
void c_binary_search(std::vector<P>& v)       { (void)std::binary_search(v.begin(), v.end(), {}); }
void c_iota(std::vector<int>& v)              { std::iota(v.begin(), v.end(), 0); }
void c_accumulate(std::vector<P>& v)          { (void)std::accumulate(v.begin(), v.end(), P{}, [](P a, const P&){ return a; }); }
void c_uninit_fill(P* p)                      { std::uninitialized_fill(p, p + 2, {}); }
void c_uninit_fill_n(P* p)                    { std::uninitialized_fill_n(p, 2, {}); }
void r_fill(std::vector<P>& v)                { std::ranges::fill(v, {}); std::ranges::fill(v.begin(), v.end(), {}); }
void r_fill_n(std::vector<P>& v)              { std::ranges::fill_n(v.begin(), 2, {}); }
void r_replace(std::vector<P>& v)             { std::ranges::replace(v, {}, {1, 2}); }
void r_replace_if(std::vector<P>& v)          { std::ranges::replace_if(v, [](const P&){ return true; }, {1, 2}); }
void r_replace_copy(std::vector<P>& v)        { std::ranges::replace_copy(v, v.begin(), {}, {1, 2}); }
void r_replace_copy_if(std::vector<P>& v)     { std::ranges::replace_copy_if(v, v.begin(), [](const P&){ return true; }, {1, 2}); }
void r_remove(std::vector<P>& v)              { (void)std::ranges::remove(v, {}); }
void r_remove_copy(std::vector<P>& v)         { (void)std::ranges::remove_copy(v, v.begin(), {}); }
void r_count(std::vector<P>& v)               { (void)std::ranges::count(v, {}); }
void r_find(std::vector<P>& v)                { (void)std::ranges::find(v, {}); }
void r_find_last(std::vector<P>& v)           { (void)std::ranges::find_last(v, {}); }
void r_contains(std::vector<P>& v)            { (void)std::ranges::contains(v, {}); }
void r_search_n(std::vector<P>& v)            { (void)std::ranges::search_n(v, 2, {}); }
void r_lower_bound(std::vector<P>& v)         { (void)std::ranges::lower_bound(v, {}); }
void r_upper_bound(std::vector<P>& v)         { (void)std::ranges::upper_bound(v, {}); }
void r_equal_range(std::vector<P>& v)         { (void)std::ranges::equal_range(v, {}); }
void r_binary_search(std::vector<P>& v)       { (void)std::ranges::binary_search(v, {}); }
void r_uninit_fill(P* p)                      { std::ranges::uninitialized_fill(p, p + 2, {}); }
void r_uninit_fill_n(P* p)                    { std::ranges::uninitialized_fill_n(p, 2, {}); }
void r_iota(std::vector<int>& v)              { std::ranges::iota(v, 0); }

