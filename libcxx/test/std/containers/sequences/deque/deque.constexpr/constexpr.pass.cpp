//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <deque>
//
// P3372R3: constexpr containers and adaptors -- deque's full member surface
// (constructors, observers, modifiers, comparisons, swap) is constexpr. Its
// backing storage (__split_buffer) was already constexpr since C++20.

#include <deque>
#include <utility>

constexpr bool test_deque() {
  std::deque<int> d;
  if (!d.empty() || d.size() != 0)
    return false;
  d.push_back(1);
  d.push_front(0);
  d.emplace_back(2);
  d.emplace_front(-1);
  if (d.size() != 4 || d[0] != -1 || d[1] != 0 || d[2] != 1 || d[3] != 2)
    return false;
  d.pop_front();
  if (d.front() != 0 || d.back() != 2)
    return false;

  std::deque<int> copy(d);
  if (!(copy == d) || !(copy <= d))
    return false;
  copy.push_back(99);
  if (!(copy != d) || !(copy > d))
    return false;

  copy.insert(copy.begin(), -5);
  if (copy.front() != -5)
    return false;
  copy.erase(copy.begin());
  if (copy.front() != 0)
    return false;

  copy.resize(2);
  if (copy.size() != 2)
    return false;

  d.swap(copy);
  if (d.size() != 2)
    return false;

  d.clear();
  return d.empty();
}
static_assert(test_deque());

constexpr bool test_deque_constructors() {
  std::deque<int> from_size(3, 7);
  std::deque<int> moved(std::move(from_size));
  if (moved.size() != 3 || moved[0] != 7 || moved[1] != 7 || moved[2] != 7)
    return false;

  std::deque<int> from_il{1, 2, 3, 4};
  return from_il.size() == 4 && from_il[3] == 4;
}
static_assert(test_deque_constructors());

int main(int, char**) { return 0; }
