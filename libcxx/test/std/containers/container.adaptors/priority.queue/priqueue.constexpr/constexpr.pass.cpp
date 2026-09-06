//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <queue>
//
// P3372R3: constexpr containers and adaptors -- priority_queue's full member
// surface is constexpr. std::vector, itself fully constexpr, is used as the
// underlying container -- the default is already std::vector, so no
// substitution is needed here (unlike stack/queue, whose default is
// std::deque).

#include <array>
#include <functional>
#include <queue>
#include <vector>

constexpr bool test_priority_queue() {
  std::priority_queue<int, std::vector<int>> q;
  if (!q.empty() || q.size() != 0)
    return false;
  q.push(2);
  q.emplace(4);
  std::array<int, 2> more{1, 3};
  q.push_range(more);
  if (q.top() != 4 || q.size() != 4)
    return false;
  q.pop();
  if (q.top() != 3)
    return false;
  std::priority_queue<int, std::vector<int>> other;
  other.push(9);
  q.swap(other);
  return q.top() == 9 && other.top() == 3;
}
static_assert(test_priority_queue());

constexpr bool test_priority_queue_constructors() {
  std::priority_queue deduced(std::less<int>{}, std::vector<int>{1, 4, 2});
  return deduced.top() == 4 && deduced.size() == 3;
}
static_assert(test_priority_queue_constructors());

int main(int, char**) { return 0; }
