//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++26

// constexpr COPYCV(T, void)* address() const noexcept; // P3936R1

#include <atomic>
#include <cassert>
#include <concepts>
#include <memory>
#include <type_traits>

#include "atomic_helpers.h"
#include "test_macros.h"

template <typename T>
struct TestAddress {
  void operator()() const {
    T x(T(1));
    const std::atomic_ref<T> a(x);

    std::same_as<void*> decltype(auto) p = a.address();
    assert(static_cast<void*>(std::addressof(x)) == p);

    static_assert(noexcept((a.address())));
  }
};

// the cv-qualifiers of T are copied to void
template <class T>
using address_t = decltype(std::declval<const std::atomic_ref<T>&>().address());
static_assert(std::is_same_v<address_t<int>, void*>);
static_assert(std::is_same_v<address_t<const int>, const void*>);
static_assert(std::is_same_v<address_t<volatile int>, volatile void*>);
static_assert(std::is_same_v<address_t<const volatile int>, const volatile void*>);
static_assert(std::is_same_v<address_t<int*>, void*>);
static_assert(std::is_same_v<address_t<float>, void*>);
static_assert(std::is_same_v<address_t<const float>, const void*>);

int main(int, char**) {
  TestEachAtomicType<TestAddress>()();

  {
    int i = 3;
    const std::atomic_ref<const int> a(i);
    const void* p = a.address();
    assert(p == &i);
    std::atomic_ref<int> b(i);
    std::atomic_ref<const int> c(b); // the converting constructor goes through address()
    assert(c.load() == 3 && c.address() == &i);
  }

  return 0;
}
