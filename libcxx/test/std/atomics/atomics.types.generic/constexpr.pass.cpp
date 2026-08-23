//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// P3309R3: constexpr atomic<T> for scalar T.
//
// constexpr T load(memory_order = memory_order::seq_cst) const noexcept;
// constexpr void store(T, memory_order = memory_order::seq_cst) noexcept;
// constexpr T exchange(T, memory_order = memory_order::seq_cst) noexcept;
// constexpr bool compare_exchange_weak(T&, T, memory_order, memory_order) noexcept;
// constexpr bool compare_exchange_strong(T&, T, memory_order, memory_order) noexcept;
// constexpr T fetch_add(T, memory_order = memory_order::seq_cst) noexcept;         // integral/pointer/float
// constexpr T fetch_sub(T, memory_order = memory_order::seq_cst) noexcept;         // integral/pointer/float
// constexpr T fetch_and/or/xor(T, memory_order = memory_order::seq_cst) noexcept;  // integral
// constexpr T fetch_max/fetch_min(T, memory_order = memory_order::seq_cst) noexcept;
// constexpr void wait(T, memory_order = memory_order::seq_cst) const noexcept;
// constexpr void notify_one() noexcept;
// constexpr void notify_all() noexcept;
//
// This fork's compiler has no constexpr support for any of the underlying
// `__c11_atomic_*`/`__atomic_*` builtins, so libc++'s implementation reads/writes
// the underlying storage directly during constant evaluation, bypassing them --
// which only type-checks for scalar element types (see support/c11.h). Scoped
// accordingly: exercised here only for scalar T (int, bool, pointer, float).

#include <atomic>
#include <cassert>
#include <cstdint>
#include <limits>

template <class T>
constexpr bool test_integral_like(T v1, T v2) {
  std::atomic<T> a(v1);
  if (a.load() != v1)
    return false;

  a.store(v2);
  if (a.load() != v2)
    return false;

  T old = a.exchange(v1);
  if (old != v2 || a.load() != v1)
    return false;

  T expected = v1;
  if (!a.compare_exchange_strong(expected, v2))
    return false;
  if (a.load() != v2)
    return false;

  expected = v1; // now stale: a holds v2
  if (a.compare_exchange_weak(expected, v1))
    return false;
  if (expected != v2)
    return false;

  a.wait(v1); // returns immediately: current value (v2) != v1
  a.notify_one();
  a.notify_all();

  return true;
}

constexpr bool test_int() {
  if (!test_integral_like<int>(1, 2))
    return false;

  std::atomic<int> a(10);
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
  if (a.load() != 3)
    return false;
  a += 7;
  if (a.load() != 10)
    return false;
  a -= 2;
  if (a.load() != 8)
    return false;
  a &= 0b1100;
  if (a.load() != 0b1000)
    return false;
  a |= 0b0011;
  if (a.load() != 0b1011)
    return false;
  a ^= 0b1111;
  if (a.load() != 0b0100)
    return false;

  return true;
}
static_assert(test_int());

constexpr bool test_bool() { return test_integral_like<bool>(true, false); }
static_assert(test_bool());

constexpr bool test_pointer() {
  int arr[3] = {0, 0, 0};
  if (!test_integral_like<int*>(&arr[0], &arr[1]))
    return false;

  std::atomic<int*> a(&arr[0]);
  if (a.fetch_add(2) != &arr[0] || a.load() != &arr[2])
    return false;
  if (a.fetch_sub(1) != &arr[2] || a.load() != &arr[1])
    return false;
  if (a.fetch_max(&arr[2]) != &arr[1] || a.load() != &arr[2])
    return false;
  if (a.fetch_min(&arr[1]) != &arr[2] || a.load() != &arr[1])
    return false;
  ++a;
  if (a.load() != &arr[2])
    return false;
  --a;
  --a;
  if (a.load() != &arr[0])
    return false;

  return true;
}
static_assert(test_pointer());

constexpr bool test_float() {
  if (!test_integral_like<double>(1.5, 2.5))
    return false;

  std::atomic<double> a(1.0);
  if (a.fetch_add(0.5) != 1.0 || a.load() != 1.5)
    return false;
  if (a.fetch_sub(0.5) != 1.5 || a.load() != 1.0)
    return false;
  if (a.fetch_max(2.0) != 1.0 || a.load() != 2.0)
    return false;
  if (a.fetch_min(0.5) != 2.0 || a.load() != 0.5)
    return false;

  a += 1.5;
  if (a.load() != 2.0)
    return false;
  a -= 1.5;
  if (a.load() != 0.5)
    return false;

  return true;
}
static_assert(test_float());

// This fork's target (x86-64) has an 80-bit extended `long double`, on which
// atomic<long double>'s runtime fetch_add/sub/max/min fall back to a CAS loop
// (see atomic.h's __has_rmw_builtin) -- so this specifically exercises that
// CAS-loop's own consteval branch, distinct from the direct-builtin path
// exercised by `double` above.
constexpr bool test_long_double() { return test_integral_like<long double>(1.5L, 2.5L); }
static_assert(test_long_double());

// [atomics.syn]'s free `atomic_*` functions are constexpr-marked separately from the member
// functions they forward to; make sure that marking actually took, for each family (the
// `atomic_fetch_and/or/xor(_explicit)` overloads are `__enable_if_t`-constrained on integral T,
// a different shape from the others, so don't assume they follow along for free).
constexpr bool test_free_functions() {
  std::atomic<int> a(1);

  std::atomic_store(&a, 2);
  if (std::atomic_load(&a) != 2)
    return false;
  std::atomic_store_explicit(&a, 3, std::memory_order_relaxed);
  if (std::atomic_load_explicit(&a, std::memory_order_relaxed) != 3)
    return false;

  if (std::atomic_exchange(&a, 4) != 3 || std::atomic_load(&a) != 4)
    return false;
  if (std::atomic_exchange_explicit(&a, 5, std::memory_order_relaxed) != 4 || std::atomic_load(&a) != 5)
    return false;

  int expected = 5;
  if (!std::atomic_compare_exchange_weak(&a, &expected, 6) || std::atomic_load(&a) != 6)
    return false;
  expected = 6;
  if (!std::atomic_compare_exchange_strong(&a, &expected, 7) || std::atomic_load(&a) != 7)
    return false;
  expected = 6; // stale
  if (std::atomic_compare_exchange_weak_explicit(
          &a, &expected, 8, std::memory_order_relaxed, std::memory_order_relaxed) ||
      expected != 7)
    return false;
  expected = 7;
  if (!std::atomic_compare_exchange_strong_explicit(
          &a, &expected, 8, std::memory_order_relaxed, std::memory_order_relaxed) ||
      std::atomic_load(&a) != 8)
    return false;

  if (std::atomic_fetch_add(&a, 2) != 8 || std::atomic_load(&a) != 10)
    return false;
  if (std::atomic_fetch_add_explicit(&a, 2, std::memory_order_relaxed) != 10 || std::atomic_load(&a) != 12)
    return false;
  if (std::atomic_fetch_sub(&a, 2) != 12 || std::atomic_load(&a) != 10)
    return false;
  if (std::atomic_fetch_sub_explicit(&a, 2, std::memory_order_relaxed) != 10 || std::atomic_load(&a) != 8)
    return false;
  if (std::atomic_fetch_and(&a, 0b1100) != 8 || std::atomic_load(&a) != 0b1000)
    return false;
  if (std::atomic_fetch_and_explicit(&a, 0b1001, std::memory_order_relaxed) != 0b1000 || std::atomic_load(&a) != 0b1000)
    return false;
  if (std::atomic_fetch_or(&a, 0b0011) != 0b1000 || std::atomic_load(&a) != 0b1011)
    return false;
  if (std::atomic_fetch_or_explicit(&a, 0b0100, std::memory_order_relaxed) != 0b1011 || std::atomic_load(&a) != 0b1111)
    return false;
  if (std::atomic_fetch_xor(&a, 0b1111) != 0b1111 || std::atomic_load(&a) != 0)
    return false;
  if (std::atomic_fetch_xor_explicit(&a, 0b1010, std::memory_order_relaxed) != 0 || std::atomic_load(&a) != 0b1010)
    return false;
  if (std::atomic_fetch_max(&a, 100) != 0b1010 || std::atomic_load(&a) != 100)
    return false;
  if (std::atomic_fetch_max_explicit(&a, 50, std::memory_order_relaxed) != 100 || std::atomic_load(&a) != 100)
    return false;
  if (std::atomic_fetch_min(&a, 3) != 100 || std::atomic_load(&a) != 3)
    return false;
  if (std::atomic_fetch_min_explicit(&a, 50, std::memory_order_relaxed) != 3 || std::atomic_load(&a) != 3)
    return false;

  std::atomic_wait(&a, 0); // returns immediately: current value (3) != 0
  std::atomic_wait_explicit(&a, 0, std::memory_order_relaxed);
  std::atomic_notify_one(&a);
  std::atomic_notify_all(&a);

  return true;
}
static_assert(test_free_functions());

// [atomics.types.float]/[atomics.ref.float]: NaN never propagates into the stored value for
// fetch_max/fetch_min ("as if by fmaximum_num/fminimum_num"). c11.h's consteval branch
// duplicates this NaN handling from <atomic>'s own __maximum_num/__minimum_num (the two aren't
// otherwise linked), so exercise it directly at compile time to catch divergence.
constexpr bool test_fetch_max_min_nan() {
  constexpr double nan = std::numeric_limits<double>::quiet_NaN();

  std::atomic<double> a(1.0);
  double old = a.fetch_max(nan); // NaN operand: keep the non-NaN value
  if (old != 1.0 || a.load() != 1.0)
    return false;

  a.store(1.0);
  old = a.fetch_min(nan);
  if (old != 1.0 || a.load() != 1.0)
    return false;

  a.store(nan);
  old = a.fetch_max(2.0); // current value is NaN: prior return value must be NaN, ...
  if (!__builtin_isnan(old))
    return false;
  if (a.load() != 2.0) // ... and the stored value becomes the non-NaN operand
    return false;

  return true;
}
static_assert(test_fetch_max_min_nan());

// A regression check, not a constexpr test: atomic<T> for a non-scalar trivially copyable class
// type T stays non-constexpr (a real compiler gap, not a libc++ choice -- see support/c11.h's
// comment), but adding the `if constexpr (is_scalar_v<_Tp>)` consteval branches must not break
// ordinary *runtime* compilation/use of atomic<T> for such T.
struct TrivialPoint {
  int x;
  int y;
};

void test_class_type_still_compiles() {
  std::atomic<TrivialPoint> a(TrivialPoint{1, 2});
  TrivialPoint v = a.load();
  assert(v.x == 1 && v.y == 2);
  a.store(TrivialPoint{3, 4});
  TrivialPoint old = a.exchange(TrivialPoint{5, 6});
  assert(old.x == 3 && old.y == 4);
  assert(a.load().x == 5 && a.load().y == 6);
}

int main(int, char**) {
  test_class_type_still_compiles();
  return 0;
}
