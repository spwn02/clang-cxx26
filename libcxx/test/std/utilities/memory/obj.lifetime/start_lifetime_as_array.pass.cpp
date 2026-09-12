//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// #include <memory>

// template<class T> T* start_lifetime_as_array(void* p, size_t n) noexcept;
// template<class T> const T* start_lifetime_as_array(const void* p, size_t n) noexcept;
// template<class T> volatile T* start_lifetime_as_array(volatile void* p, size_t n) noexcept;
// template<class T> const volatile T* start_lifetime_as_array(const volatile void* p, size_t n) noexcept;

#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>

#include "test_macros.h"

static void test_basic() {
  alignas(int) unsigned char buffer[4 * sizeof(int)];
  int src[4] = {10, 20, 30, 40};
  std::memcpy(buffer, src, sizeof(src));

  int* p = std::start_lifetime_as_array<int>(static_cast<void*>(buffer), 4);
  ASSERT_SAME_TYPE(int*, decltype(p));
  assert(static_cast<void*>(p) == static_cast<void*>(buffer));
  for (int i = 0; i < 4; ++i)
    assert(p[i] == src[i]);
}

// P2679R2's fix: n == 0 is well-defined (including for a null pointer), and
// returns a pointer comparing equal to the argument.
static void test_n_zero() {
  int dummy = 0;
  void* p = &dummy;
  assert(std::start_lifetime_as_array<int>(p, 0) == p);
  assert(std::start_lifetime_as_array<int>(static_cast<void*>(nullptr), 0) == nullptr);
}

static void test_cv_overloads() {
  alignas(int) unsigned char buffer[2 * sizeof(int)];
  int src[2] = {7, 8};
  std::memcpy(buffer, src, sizeof(src));

  ASSERT_SAME_TYPE(int*, decltype(std::start_lifetime_as_array<int>(static_cast<void*>(buffer), 2)));
  ASSERT_SAME_TYPE(const int*, decltype(std::start_lifetime_as_array<int>(static_cast<const void*>(buffer), 2)));
  ASSERT_SAME_TYPE(
      volatile int*, decltype(std::start_lifetime_as_array<int>(static_cast<volatile void*>(buffer), 2)));
  ASSERT_SAME_TYPE(
      const volatile int*,
      decltype(std::start_lifetime_as_array<int>(static_cast<const volatile void*>(buffer), 2)));

  assert(std::start_lifetime_as_array<int>(static_cast<void*>(buffer), 2)[1] == 8);
}

static_assert(noexcept(std::start_lifetime_as_array<int>(std::declval<void*>(), std::size_t(0))));

int main(int, char**) {
  test_basic();
  test_n_zero();
  test_cv_overloads();

  return 0;
}
