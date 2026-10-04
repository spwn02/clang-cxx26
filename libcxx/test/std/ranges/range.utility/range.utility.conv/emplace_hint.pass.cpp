//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-ranges

// <ranges>

// [range.utility.conv.general]: container-append / container-appendable use
// c.emplace_hint(c.end(), ref) instead of c.emplace(c.end(), ref).

#include <cassert>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <set>
#include <vector>

// A container that can only be appended to through emplace_hint(const_iterator, T).
struct HintOnly {
  using value_type      = int;
  using reference       = int&;
  using const_reference = const int&;
  using iterator        = int*;
  using const_iterator  = const int*;
  using difference_type = std::ptrdiff_t;
  using size_type       = std::size_t;

  int data[8];
  int n     = 0;
  int hints = 0;

  iterator begin() { return data; }
  iterator end() { return data + n; }
  const_iterator begin() const { return data; }
  const_iterator end() const { return data + n; }
  iterator emplace_hint(const_iterator, int v) {
    ++hints;
    data[n++] = v;
    return data + n - 1;
  }
};

int main(int, char**) {
  std::vector<int> v{3, 1, 2};
  auto h = std::ranges::to<HintOnly>(v);
  assert(h.n == 3 && h.hints == 3 && h.data[0] == 3 && h.data[2] == 2);

  auto s = std::ranges::to<std::set<int>>(v);
  assert(s.size() == 3 && *s.begin() == 1);
  return 0;
}
