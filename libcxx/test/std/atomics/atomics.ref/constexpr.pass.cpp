//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// P3309R3: constexpr atomic_ref<T>.
//
// store/load/exchange/operator= are constexpr for any trivially copyable T
// (atomic_ref stores a plain T*, with no _Atomic-qualification to bypass, so
// this doesn't need the scalar-only carve-out that atomic<T> does -- see
// support/c11.h's comment for that one). compare_exchange_weak/strong and
// wait compare via `==`, which -- unlike the bytewise
// __atomic_compare_exchange/__clear_padding/memcmp machinery normally used
// for arbitrary trivially-copyable T -- requires an equality comparison to
// exist, so those two stay scalar-only.

#include <atomic>

struct TrivialPoint {
  int x;
  int y;
  friend constexpr bool operator==(const TrivialPoint&, const TrivialPoint&) = default;
};

constexpr bool test_scalar() {
  int obj = 1;
  std::atomic_ref<int> a(obj);
  if (a.load() != 1)
    return false;

  a.store(2);
  if (a.load() != 2 || obj != 2)
    return false;

  int old = a.exchange(3);
  if (old != 2 || a.load() != 3)
    return false;

  int expected = 3;
  if (!a.compare_exchange_strong(expected, 4))
    return false;
  if (a.load() != 4)
    return false;

  expected = 3; // stale: a holds 4
  if (a.compare_exchange_weak(expected, 5))
    return false;
  if (expected != 4)
    return false;

  a.wait(1); // returns immediately: current value (4) != 1
  a.notify_one();
  a.notify_all();

  a = 10;
  if (a.load() != 10)
    return false;

  if (a.fetch_add(5) != 10 || a.load() != 15)
    return false;
  if (a.fetch_sub(5) != 15 || a.load() != 10)
    return false;
  if (a.fetch_and(0b1100) != 10 || a.load() != 0b1000)
    return false;
  if (a.fetch_or(0b0011) != 0b1000 || a.load() != 0b1011)
    return false;
  if (a.fetch_xor(0b1111) != 0b1011 || a.load() != 0b0100)
    return false;
  if (a.fetch_max(100) != 0b0100 || a.load() != 100)
    return false;
  if (a.fetch_min(3) != 100 || a.load() != 3)
    return false;

  ++a;
  if (a.load() != 4)
    return false;
  --a;
  a += 6;
  a -= 2;
  if (a.load() != 7)
    return false;

  return true;
}
static_assert(test_scalar());

constexpr bool test_pointer() {
  int arr[3] = {0, 0, 0};
  int* obj   = &arr[0];
  std::atomic_ref<int*> a(obj);
  if (a.load() != &arr[0])
    return false;

  if (a.fetch_add(2) != &arr[0] || a.load() != &arr[2])
    return false;
  if (a.fetch_sub(1) != &arr[2] || a.load() != &arr[1])
    return false;

  return true;
}
static_assert(test_pointer());

// The store/load/exchange/operator= path (but not compare_exchange_*/wait, which
// need a scalar `==`) is constexpr for an arbitrary trivially copyable class type.
constexpr bool test_class_type() {
  TrivialPoint obj{1, 2};
  std::atomic_ref<TrivialPoint> a(obj);
  if (a.load().x != 1 || a.load().y != 2)
    return false;

  a.store(TrivialPoint{3, 4});
  if (a.load().x != 3 || a.load().y != 4)
    return false;

  TrivialPoint old = a.exchange(TrivialPoint{5, 6});
  if (old.x != 3 || old.y != 4 || a.load().x != 5 || a.load().y != 6)
    return false;

  a = TrivialPoint{7, 8};
  if (a.load().x != 7 || a.load().y != 8)
    return false;

  return true;
}
static_assert(test_class_type());

int main(int, char**) { return 0; }
