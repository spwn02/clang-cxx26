//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// Regression test for ranges::distance on a join_view whose outer range
// produces xvalues, as views::as_rvalue is specified to do.

#include <array>
#include <cassert>
#include <ranges>
#include <vector>

int main(int, char**) {
  std::vector<std::array<int, 2>> values{{1, 2}, {3, 4}};

  auto range = std::views::all(std::move(values)) | std::views::as_rvalue | std::views::join;

  // Keep plain iteration as a control case for the segmented-iterator path.
  int expected = 1;
  for (int value : range)
    assert(value == expected++);

  std::vector<std::array<int, 2>> distance_values{{1, 2}, {3, 4}};
  auto distance_range =
      std::views::all(std::move(distance_values)) | std::views::as_rvalue | std::views::join;
  assert(std::ranges::distance(distance_range) == 4);

  return 0;
}
