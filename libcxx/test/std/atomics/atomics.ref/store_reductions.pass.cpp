//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <atomic>

// atomic_ref<T>::store_add, store_sub, store_max, store_min ([atomics.ref.int], [atomics.ref.float],
// [atomics.ref.pointer]); store_and, store_or, store_xor for integral types

#include <atomic>
#include <cassert>
#include <memory>

#include "test_macros.h"

constexpr bool test_constexpr() {
  int a[4]{};
  int* p = a;
  std::atomic_ref<int*> r(p);
  r.store_add(2);
  r.store_sub(1);
  r.store_max(a + 3);
  r.store_min(a + 2);
  return p == a + 2;
}
static_assert(test_constexpr());

int main(int, char**) {
  {
    int v = 1;
    std::atomic_ref<int> r(v);
    r.store_add(2);
    assert(v == 3);
    r.store_sub(1);
    assert(v == 2);
    r.store_and(3);
    r.store_or(4);
    r.store_xor(1);
    assert(v == 7);
    r.store_max(9);
    assert(v == 9);
    r.store_min(3, std::memory_order_relaxed);
    assert(v == 3);
  }
  {
    double v = 1.5;
    std::atomic_ref<double> r(v);
    r.store_add(1.0);
    assert(v == 2.5);
    r.store_sub(0.5);
    assert(v == 2.0);
    r.store_max(4.0);
    assert(v == 4.0);
    r.store_min(1.0);
    assert(v == 1.0);
  }
  {
    int a[4]{};
    int* p = a;
    std::atomic_ref<int*> r(p);
    r.store_add(2);
    assert(p == a + 2);
    r.store_sub(1);
    assert(p == a + 1);
    r.store_max(a + 3);
    assert(p == a + 3);
    r.store_min(a + 2, std::memory_order_relaxed);
    assert(p == a + 2);
    r.store_max(a);
    assert(p == a + 2);
  }
  return 0;
}
