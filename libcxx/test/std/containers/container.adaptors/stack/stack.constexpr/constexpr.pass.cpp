//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stack>
//
// P3372R3: constexpr containers and adaptors -- stack's full member surface
// (constructors, observers, modifiers, comparisons, swap, deduction guides)
// is constexpr. std::vector, itself fully constexpr, is used as the
// underlying container -- the default (std::deque) is not yet constexpr in
// this fork.

#include <array>
#include <stack>
#include <utility>
#include <vector>

constexpr bool test_stack() {
  std::stack<int, std::vector<int>> s;
  if (!s.empty() || s.size() != 0)
    return false;
  s.push(1);
  s.emplace(2);
  std::array<int, 2> more{3, 4};
  s.push_range(more);
  if (s.top() != 4 || s.size() != 4)
    return false;
  std::stack<int, std::vector<int>> copy(s);
  if (!(copy == s) || !(copy >= s))
    return false;
  copy.pop();
  if (!(copy < s))
    return false;
  copy.swap(s);
  return copy.top() == 4 && s.top() == 3;
}
static_assert(test_stack());

constexpr bool test_stack_constructors() {
  std::stack deduced(std::vector<int>{1, 2});
  std::stack<int, std::vector<int>> moved(std::move(deduced));
  return moved.top() == 2 && moved.size() == 2;
}
static_assert(test_stack_constructors());

int main(int, char**) { return 0; }
