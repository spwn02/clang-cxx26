//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// template<class T, class Allocator>
//   void swap(hive<T, Allocator>& x, hive<T, Allocator>& y) noexcept(noexcept(x.swap(y)));  // [hive.special]

// A qualified call std::swap(x, y) must select the container's overload (which calls the member swap and so swaps
// propagating allocators), not the generic move-based std::swap.

#include <cassert>
#include <hive>
#include <memory>
#include <utility>

#include "test_macros.h"

static int allocator_swaps = 0;

template <class T>
struct counting_allocator {
  using value_type                             = T;
  using propagate_on_container_swap            = std::true_type;
  using propagate_on_container_move_assignment = std::false_type;

  int id;
  constexpr explicit counting_allocator(int i = 0) noexcept : id(i) {}
  template <class U>
  constexpr counting_allocator(const counting_allocator<U>& other) noexcept : id(other.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>().allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>().deallocate(p, n); }
  friend void swap(counting_allocator& a, counting_allocator& b) noexcept {
    ++allocator_swaps;
    std::swap(a.id, b.id);
  }
  friend bool operator==(const counting_allocator& a, const counting_allocator& b) noexcept { return a.id == b.id; }
};

int main(int, char**) {
  using H = std::hive<int, counting_allocator<int>>;
  H a(counting_allocator<int>(1));
  H b(counting_allocator<int>(2));
  a.insert(10);
  b.insert(20);
  b.insert(30);

  allocator_swaps = 0;
  std::swap(a, b);
  assert(allocator_swaps == 1);
  assert(a.size() == 2 && b.size() == 1);
  assert(a.get_allocator().id == 2 && b.get_allocator().id == 1);

  static_assert(noexcept(std::swap(a, b)) == noexcept(a.swap(b)));
  return 0;
}
