//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// XFAIL: !has-64-bit-atomics

// <atomic>

// template<class T>
//     T
//     atomic_fetch_min_explicit(volatile atomic<T>*, atomic<T>::value_type,
//                               memory_order) noexcept;
//
// template<class T>
//     T
//     atomic_fetch_min_explicit(atomic<T>*, atomic<T>::value_type,
//                               memory_order) noexcept;

#include <atomic>
#include <type_traits>
#include <cassert>

#include "test_macros.h"
#include "atomic_helpers.h"

template <class T>
struct TestFn {
  void operator()() const {
    {
      typedef std::atomic<T> A;
      A t(T(5));
      assert(std::atomic_fetch_min_explicit(&t, T(9), std::memory_order_seq_cst) == T(5));
      assert(t == T(5));
      assert(std::atomic_fetch_min_explicit(&t, T(3), std::memory_order_seq_cst) == T(5));
      assert(t == T(3));
      ASSERT_NOEXCEPT(std::atomic_fetch_min_explicit(&t, T(0), std::memory_order_relaxed));
    }
    {
      typedef std::atomic<T> A;
      volatile A t(T(5));
      assert(std::atomic_fetch_min_explicit(&t, T(9), std::memory_order_seq_cst) == T(5));
      assert(t == T(5));
      assert(std::atomic_fetch_min_explicit(&t, T(3), std::memory_order_seq_cst) == T(5));
      assert(t == T(3));
      ASSERT_NOEXCEPT(std::atomic_fetch_min_explicit(&t, T(0), std::memory_order_relaxed));
    }
  }
};

template <class T>
void testp() {
  {
    typedef std::atomic<T> A;
    typedef typename std::remove_pointer<T>::type X;
    X a[3] = {};
    A t(&a[2]);
    assert(std::atomic_fetch_min_explicit(&t, &a[0], std::memory_order_seq_cst) == &a[2]);
    assert(t == &a[0]);
    assert(std::atomic_fetch_min_explicit(&t, &a[1], std::memory_order_seq_cst) == &a[0]);
    assert(t == &a[0]);
    ASSERT_NOEXCEPT(std::atomic_fetch_min_explicit(&t, &a[0], std::memory_order_relaxed));
  }
  {
    typedef std::atomic<T> A;
    typedef typename std::remove_pointer<T>::type X;
    X a[3] = {};
    volatile A t(&a[2]);
    assert(std::atomic_fetch_min_explicit(&t, &a[0], std::memory_order_seq_cst) == &a[2]);
    assert(t == &a[0]);
    assert(std::atomic_fetch_min_explicit(&t, &a[1], std::memory_order_seq_cst) == &a[0]);
    assert(t == &a[0]);
    ASSERT_NOEXCEPT(std::atomic_fetch_min_explicit(&t, &a[0], std::memory_order_relaxed));
  }
}

int main(int, char**) {
  TestEachIntegralType<TestFn>()();
  TestFn<float>()();
  TestFn<double>()();

  testp<int*>();
  testp<const int*>();

  return 0;
}
