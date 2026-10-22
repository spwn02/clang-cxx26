//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <inplace_vector>

// template<container-compatible-range<T> R>
//   constexpr inplace_vector(from_range_t, R&& rg);
//
// Mandates: If ranges::size(rg) is a constant expression, then ranges::size(rg) <= N.
// A range whose size is not a constant expression (a std::vector) is accepted and checked at run time.

#include <array>
#include <cassert>
#include <inplace_vector>
#include <ranges>
#include <vector>

int main(int, char**) {
  std::array<int, 3> constant_size{1, 2, 3};
  std::inplace_vector<int, 4> a(std::from_range, constant_size);
  assert(a.size() == 3);

  std::vector<int> runtime_size{1, 2, 3, 4};
  std::inplace_vector<int, 4> b(std::from_range, runtime_size);
  assert(b.size() == 4);

  return 0;
}
