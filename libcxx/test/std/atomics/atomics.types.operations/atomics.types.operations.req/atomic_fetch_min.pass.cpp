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
//     atomic_fetch_min(volatile atomic<T>* obj, atomic<T>::value_type) noexcept;
//
// template<class T>
//     T
//     atomic_fetch_min(atomic<T>* obj, atomic<T>::value_type) noexcept;

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
      assert(std::atomic_fetch_min(&t, T(9)) == T(5));
      assert(t == T(5));
      assert(std::atomic_fetch_min(&t, T(3)) == T(5));
      assert(t == T(3));
      ASSERT_NOEXCEPT(std::atomic_fetch_min(&t, T(0)));
    }
    {
      typedef std::atomic<T> A;
      volatile A t(T(5));
      assert(std::atomic_fetch_min(&t, T(9)) == T(5));
      assert(t == T(5));
      assert(std::atomic_fetch_min(&t, T(3)) == T(5));
      assert(t == T(3));
      ASSERT_NOEXCEPT(std::atomic_fetch_min(&t, T(0)));
    }
  }
};

int main(int, char**) {
  TestEachIntegralType<TestFn>()();
  TestFn<float>()();
  TestFn<double>()();

  return 0;
}
