//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <meta>
#include <variant>
#include <vector>

int main(int, char**) {
  std::vector<std::meta::info> v(3);
  assert(v.size() == 3);
  auto copy = v;
  copy.resize(64);
  copy.resize(2);
  v = copy;
  v.push_back(std::meta::info{});
  assert(v.size() == 3);
  for (auto r : v)
    assert(r == std::meta::info{});

  std::allocator<std::meta::info> allocator;
  auto p = allocator.allocate(2);
  assert(p != nullptr);
  std::construct_at(p);
  std::construct_at(p + 1);
  assert(p[0] == std::meta::info{} && p[1] == std::meta::info{});
  std::destroy_at(p + 1);
  std::destroy_at(p);
  allocator.deallocate(p, 2);

  // Count comparisons to ensure the runtime sort dispatch actually runs.
  v.resize(64);
  int comparisons = 0;
  auto less = [&](std::meta::info a, std::meta::info b) {
    ++comparisons;
    assert(a == std::meta::info{} && b == std::meta::info{});
    return false;
  };
  std::sort(v.begin(), v.end(), less);
  assert(comparisons > 0);
  comparisons = 0;
  std::stable_sort(v.begin(), v.end(), less);
  assert(comparisons > 0);
  for (auto r : v)
    assert(r == std::meta::info{});

  std::array<std::meta::info, 2> array{};
  assert(array[0] == std::meta::info{} && array[1] == std::meta::info{});
  std::variant<std::meta::info, int> variant;
  assert(variant.index() == 0);
  assert(std::get<std::meta::info>(variant) == std::meta::info{});
  variant = 42;
  assert(std::get<int>(variant) == 42);
  variant.emplace<std::meta::info>();
  assert(std::get<std::meta::info>(variant) == std::meta::info{});
  return 0;
}
