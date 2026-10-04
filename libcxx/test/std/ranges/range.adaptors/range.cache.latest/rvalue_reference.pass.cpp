//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-ranges

// <ranges>

// [range.cache.latest.iterator]: operator* caches addressof(as-lvalue(*current_)), so a range whose
// reference type is an rvalue reference (as_rvalue) can be wrapped.

#include <cassert>
#include <concepts>
#include <ranges>

namespace r = std::ranges;
namespace v = std::views;

int main(int, char**) {
  int a[] = {1, 2};
  auto x  = a | v::as_rvalue | v::cache_latest;
  static_assert(std::same_as<r::range_reference_t<decltype(x)>, int&>);
  auto it = x.begin();
  assert(*it == 1);
  assert(&*it == &a[0]);
  ++it;
  assert(*it == 2);
  return 0;
}
