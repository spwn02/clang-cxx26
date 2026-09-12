//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// #include <memory>

// template<class T> T* start_lifetime_as(void* p) noexcept;
// template<class T> const T* start_lifetime_as(const void* p) noexcept;
// template<class T> volatile T* start_lifetime_as(volatile void* p) noexcept;
// template<class T> const volatile T* start_lifetime_as(const volatile void* p) noexcept;

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

#include "test_macros.h"

struct Point {
  int x;
  int y;
};

// [obj.lifetime]/1.3's own motivating example: bytes received "over the
// network" that are a valid object representation of Point, accessed
// without running any constructor.
static void test_deserialization() {
  alignas(Point) unsigned char buffer[sizeof(Point)];
  Point src{3, 4};
  std::memcpy(buffer, &src, sizeof(Point));

  Point* p = std::start_lifetime_as<Point>(static_cast<void*>(buffer));
  ASSERT_SAME_TYPE(Point*, decltype(p));
  assert(static_cast<void*>(p) == static_cast<void*>(buffer));
  assert(p->x == 3);
  assert(p->y == 4);
}

// cv-qualified overloads all return the expected type and address.
static void test_cv_overloads() {
  alignas(Point) unsigned char buffer[sizeof(Point)];
  Point src{1, 2};
  std::memcpy(buffer, &src, sizeof(Point));

  void* vp                = buffer;
  const void* cvp          = buffer;
  volatile void* vvp       = buffer;
  const volatile void* cvvp = buffer;

  Point* p1                = std::start_lifetime_as<Point>(vp);
  const Point* p2           = std::start_lifetime_as<Point>(cvp);
  volatile Point* p3        = std::start_lifetime_as<Point>(vvp);
  const volatile Point* p4  = std::start_lifetime_as<Point>(cvvp);

  ASSERT_SAME_TYPE(Point*, decltype(std::start_lifetime_as<Point>(vp)));
  ASSERT_SAME_TYPE(const Point*, decltype(std::start_lifetime_as<Point>(cvp)));
  ASSERT_SAME_TYPE(volatile Point*, decltype(std::start_lifetime_as<Point>(vvp)));
  ASSERT_SAME_TYPE(const volatile Point*, decltype(std::start_lifetime_as<Point>(cvvp)));

  auto addr = [](const volatile void* p) { return reinterpret_cast<std::uintptr_t>(p); };
  assert(addr(p1) == addr(buffer));
  assert(addr(p2) == addr(buffer));
  assert(addr(p3) == addr(buffer));
  assert(addr(p4) == addr(buffer));
  assert(p1->x == 1 && p1->y == 2);
}

// Scalar and array-of-known-bound types are implicit-lifetime too.
static void test_scalar_and_known_bound_array() {
  alignas(int) unsigned char ibuf[sizeof(int)];
  int src = 42;
  std::memcpy(ibuf, &src, sizeof(int));
  int* ip = std::start_lifetime_as<int>(static_cast<void*>(ibuf));
  assert(*ip == 42);

  alignas(int[4]) unsigned char abuf[sizeof(int[4])];
  int asrc[4] = {1, 2, 3, 4};
  std::memcpy(abuf, asrc, sizeof(asrc));
  // start_lifetime_as<int[4]> returns int(*)[4], per the paper's own
  // documented (and deliberately unfixed) inconsistency with make_unique.
  auto* ap = std::start_lifetime_as<int[4]>(static_cast<void*>(abuf));
  ASSERT_SAME_TYPE(int(*)[4], decltype(ap));
  assert((*ap)[0] == 1 && (*ap)[3] == 4);
}

// noexcept, per the paper's own Design rationale (no code runs, so it can
// never throw).
static_assert(noexcept(std::start_lifetime_as<int>(std::declval<void*>())));
static_assert(noexcept(std::start_lifetime_as<int>(std::declval<const void*>())));
static_assert(noexcept(std::start_lifetime_as<int>(std::declval<volatile void*>())));
static_assert(noexcept(std::start_lifetime_as<int>(std::declval<const volatile void*>())));

int main(int, char**) {
  test_deserialization();
  test_cv_overloads();
  test_scalar_and_known_bound_array();

  return 0;
}
