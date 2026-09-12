//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// A minimal test to validate import works.

// MODULE_DEPENDENCIES: std

import std;

int main(int, char**) {
  std::println("Hello modular world");

  // Regression test: libcxx/modules/std/ranges.inc left views::chunk,
  // views::slide, and views::stride under `#if 0` (never actually exported)
  // even after chunk_view/slide_view/stride_view were implemented and wired
  // into <ranges> -- `import std;` couldn't see them even though
  // `#include <ranges>` could.
  auto values  = std::views::iota(0, 16);
  auto strided = values | std::views::stride(2);
  auto chunked = values | std::views::chunk(4);
  auto windowed = values | std::views::slide(3);
  auto cartesian = std::views::cartesian_product(std::array{1, 2}, std::array{3, 4});
  if (*strided.begin() + *chunked.front().begin() + *windowed.front().begin() + std::get<0>(*cartesian.begin()) != 1)
    return 1;

  // Regression test: ranges.inc's `views::enumerate` was hand-written as a
  // stale zip+iota proxy directly inside the module partition (predating
  // issue #85's real enumerate_view rewrite), and enumerate_view.h itself
  // was excluded from the std module build entirely (`#if
  // !defined(_LIBCPP_BUILDING_STD_MODULE)`), so `std::ranges::enumerate_view`
  // wasn't even visible, let alone re-exported with the right implementation.
  static_assert(std::same_as<decltype(std::views::iota(0, 1) | std::views::enumerate),
                              std::ranges::enumerate_view<std::ranges::iota_view<int, int>>>);
  for (auto [i, v] : std::views::iota(0, 3) | std::views::enumerate) {
    if (i != v)
      return 1;
  }

  // Regression test: libcxx/modules/std/utility.inc never exported
  // std::constant_wrapper/std::cw (P2781R9) at all.
  static_assert(std::cw<5> + std::cw<3> == std::cw<8>);
  static_assert(std::same_as<decltype(std::cw<5>), const std::constant_wrapper<5>>);

  // Regression test: libcxx/modules/std/execution.inc exported
  // std::this_thread::sync_wait but not its CPO tag type sync_wait_t.
  static_assert(std::same_as<decltype(std::this_thread::sync_wait), const std::this_thread::sync_wait_t>);

  return 0;
}
