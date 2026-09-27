// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [ranges.syn]: every <ranges> entity except the stream views is freestanding.

#include <ranges>
#include <algorithm>
#include <array>
#include <execution>
#include <vector>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif
#if !defined(__cpp_lib_freestanding_ranges) || __cpp_lib_freestanding_ranges != 202306L
#  error "missing or wrong __cpp_lib_freestanding_ranges"
#endif

void test_ranges() {
  int values[] = {1, 2, 3, 4};
  auto first = std::ranges::begin(values);
  auto last = std::ranges::end(values);
  auto count = std::ranges::size(values);
  auto is_empty = std::ranges::empty(values);
  auto pointer = std::ranges::data(values);
  auto filtered = std::views::filter(values, [](int value) { return value > 1; });
  auto transformed = std::views::transform(values, [](int value) { return value + 1; });
  auto taken = std::views::take(values, 2);
  auto dropped = std::views::drop(values, 1);
  auto reversed = std::views::reverse(values);
  auto owned = std::views::all(std::array{1, 2});
  auto referenced = std::views::all(values);
  auto empty = std::ranges::empty_view<int>{};
  auto single = std::ranges::single_view<int>{1};
  auto integers = std::views::iota(0, 4);
  auto common = std::views::common(integers);
  auto zipped = std::views::zip(values, values);
  auto zip_transformed = std::views::zip_transform([](int a, int b) { return a + b; }, values, values);
  auto adjacent = std::views::adjacent<2>(values);
  auto adjacent_transformed = std::views::adjacent_transform<2>(values, [](int a, int b) { return a + b; });
  auto chunked = std::views::chunk(values, 2);
  auto slid = std::views::slide(values, 2);
  auto chunked_by = std::views::chunk_by(values, [](int a, int b) { return a < b; });
  auto joined = std::views::join_with(std::views::single(std::views::single(1)), 0);
  auto converted = std::ranges::to<std::vector<int>>(values);
  (void)first; (void)last; (void)count; (void)is_empty; (void)pointer; (void)filtered; (void)transformed;
  (void)taken; (void)dropped; (void)reversed; (void)owned; (void)referenced; (void)empty; (void)single;
  (void)common; (void)zipped; (void)zip_transformed; (void)adjacent; (void)adjacent_transformed;
  (void)chunked; (void)slid; (void)chunked_by; (void)joined; (void)converted;
}
