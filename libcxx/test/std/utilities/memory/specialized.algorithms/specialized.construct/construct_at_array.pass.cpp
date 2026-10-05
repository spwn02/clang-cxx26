//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <memory>

// [specialized.construct]: construct_at (and ranges::construct_at) accept bounded array types: the array is
// value-initialized, which takes no arguments; unbounded arrays are excluded from overload resolution.

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "test_macros.h"

template <class T, class... Args>
concept can_construct_at = requires(T* p, Args&&... args) { std::construct_at(p, std::forward<Args>(args)...); };
template <class T, class... Args>
concept can_ranges_construct_at = requires(T* p, Args&&... args) { std::ranges::construct_at(p, std::forward<Args>(args)...); };

static_assert(can_construct_at<int[2]>);
static_assert(can_construct_at<int[2][3]>);
static_assert(!can_construct_at<int[]>);
static_assert(!can_construct_at<int[][3]>);
static_assert(can_construct_at<int, int>);
static_assert(can_ranges_construct_at<int[2]>);
static_assert(!can_ranges_construct_at<int[]>);

struct Counted {
  static inline int live = 0;
  int value = 5;
  Counted() { ++live; }
  ~Counted() { --live; }
};

void test_runtime() {
  int a[2] = {7, 7};
  auto* p  = std::construct_at(&a);
  static_assert(std::is_same_v<decltype(p), int (*)[2]>);
  assert(p == &a && a[0] == 0 && a[1] == 0);

  int b[2][3];
  std::construct_at(&b);
  for (auto& row : b)
    for (int v : row)
      assert(v == 0);

  alignas(Counted) unsigned char storage[sizeof(Counted) * 3];
  auto* arr = reinterpret_cast<Counted(*)[3]>(storage);
  std::ranges::construct_at(arr);
  assert(Counted::live == 3);
  for (auto& e : *std::launder(arr))
    assert(e.value == 5);
  std::destroy_at(std::launder(arr));
  assert(Counted::live == 0);
}

constexpr bool test_constexpr() {
  std::allocator<int[2]> alloc;
  auto* p = alloc.allocate(1);
  auto* q = std::construct_at(p);
  bool ok = q == p && (*p)[0] == 0 && (*p)[1] == 0;
  (*p)[1] = 3;
  ok      = ok && (*p)[1] == 3;
  std::destroy_at(p);
  alloc.deallocate(p, 1);

  std::allocator<int[2][3]> alloc2;
  auto* r = alloc2.allocate(1);
  std::ranges::construct_at(r);
  ok = ok && (*r)[1][2] == 0;
  std::destroy_at(r);
  alloc2.deallocate(r, 1);
  return ok;
}

int main(int, char**) {
  test_runtime();
  assert(test_constexpr());
#if TEST_STD_VER >= 26
  static_assert(test_constexpr());
#endif
  return 0;
}
