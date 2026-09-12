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
  if (*strided.begin() + *chunked.front().begin() + *windowed.front().begin() != 0)
    return 1;

  return 0;
}
