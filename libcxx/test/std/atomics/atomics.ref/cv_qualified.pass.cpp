//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// XFAIL: !has-64-bit-atomics

// P3323R1: cv-qualified types in atomic_ref
//
// template <class T>
// struct atomic_ref {
//   using value_type = remove_cv_t<T>;
//   ...
//   void store(value_type, memory_order = memory_order::seq_cst) const noexcept;     // Constraints: !is_const_v<T>
//   value_type load(memory_order = memory_order::seq_cst) const noexcept;            // no constraint
//   value_type exchange(value_type, memory_order = memory_order::seq_cst) const noexcept; // Constraints: !is_const_v<T>
//   bool compare_exchange_weak/strong(...) const noexcept;                           // Constraints: !is_const_v<T>
//   void wait(value_type, memory_order = memory_order::seq_cst) const noexcept;       // no constraint
//   void notify_one/all() const noexcept;                                            // Constraints: !is_const_v<T>
// };
//
// Also: the program is ill-formed if is_always_lock_free is false and is_volatile_v<T> is true, so this
// test only ever names atomic_ref<volatile T> for T known (via LockFreeStatusInfo) to always be lock-free.

#include <atomic>
#include <cassert>
#include <concepts>
#include <type_traits>

#include "atomic_helpers.h"
#include "test_macros.h"

template <class T>
concept can_store = requires(std::atomic_ref<T> a, typename std::atomic_ref<T>::value_type v) { a.store(v); };
template <class T>
concept can_assign = requires(std::atomic_ref<T> a, typename std::atomic_ref<T>::value_type v) { a = v; };
template <class T>
concept can_exchange = requires(std::atomic_ref<T> a, typename std::atomic_ref<T>::value_type v) { a.exchange(v); };
template <class T>
concept can_compare_exchange = requires(
    std::atomic_ref<T> a, typename std::atomic_ref<T>::value_type& e, typename std::atomic_ref<T>::value_type d) {
  a.compare_exchange_weak(e, d);
  a.compare_exchange_strong(e, d);
};
template <class T>
concept can_notify = requires(std::atomic_ref<T> a) {
  a.notify_one();
  a.notify_all();
};

// value_type is always the cv-unqualified T, regardless of T's own cv-qualification.
template <class T>
void test_value_type() {
  static_assert(std::same_as<typename std::atomic_ref<T>::value_type, std::remove_cv_t<T>>);
  static_assert(std::same_as<typename std::atomic_ref<const T>::value_type, std::remove_cv_t<T>>);
  static_assert(std::same_as<typename std::atomic_ref<volatile T>::value_type, std::remove_cv_t<T>>);
  static_assert(std::same_as<typename std::atomic_ref<const volatile T>::value_type, std::remove_cv_t<T>>);
}

// A const-qualified T only supports the read-only subset of the API: load, the conversion operator, and wait.
// store/operator=/exchange/compare_exchange_weak/compare_exchange_strong/notify_one/notify_all must not
// participate in overload resolution at all (not just fail to compile if called).
template <class T>
void test_const() {
  static_assert(!can_store<const T>);
  static_assert(!can_assign<const T>);
  static_assert(!can_exchange<const T>);
  static_assert(!can_compare_exchange<const T>);
  static_assert(!can_notify<const T>);

  const T x = T(1);
  std::atomic_ref<const T> const a(x);
  assert(a.load() == T(1));
  assert(static_cast<T>(a) == T(1));
  // load() != T(0), so wait must return immediately without blocking.
  a.wait(T(0));
}

// A volatile-qualified T (only ever named here for T known to always be lock-free -- otherwise the whole
// specialization is ill-formed per [atomics.ref.generic.general]) supports the full read/write API, operating
// through the referenced volatile object.
//
// Values are restricted to T(0)/T(1) throughout (rather than an increasing 1..N sequence) since T can be bool,
// which only has two distinct values.
template <class T>
void test_volatile() {
  static_assert(std::atomic_ref<volatile T>::is_always_lock_free);
  static_assert(can_store<volatile T>);
  static_assert(can_assign<volatile T>);
  static_assert(can_exchange<volatile T>);
  static_assert(can_compare_exchange<volatile T>);
  static_assert(can_notify<volatile T>);

  const T zero = T(0);
  const T one  = T(1);

  T x = zero;
  std::atomic_ref<volatile T> const a(x);
  assert(a.load() == zero);

  a.store(one);
  assert(x == one);

  assert(a.exchange(zero) == one);
  assert(x == zero);

  T expected = zero;
  assert(a.compare_exchange_strong(expected, one));
  assert(x == one);
  // expected is untouched by a successful compare_exchange, so it's still `zero` here while the referenced
  // object now holds `one` -- the next call must therefore fail and refresh expected to the current value.
  assert(!a.compare_exchange_strong(expected, zero));
  assert(expected == one);
  assert(x == one);

  a = zero;
  assert(x == zero);

  // load() == zero == old, so this would block if wait() didn't compile/dispatch correctly through
  // __atomic_waitable_traits<__atomic_ref_base<volatile T>> -- store a different value from another
  // "vantage point" first so wait() returns immediately without actually blocking.
  a.store(one);
  a.wait(zero);

  a.notify_one();
  a.notify_all();
}

template <class T>
void test_type() {
  test_value_type<T>();
  test_const<T>();
  if constexpr (LockFreeStatusInfo<T>::status_known && LockFreeStatusInfo<T>::value == LockFreeStatus::always) {
    test_volatile<T>();
  }
}

void test() {
  test_type<bool>();
  test_type<int>();
  test_type<float>();
  test_type<double>();

  // Pointer specialization: cv-qualification of the *pointee* (e.g. atomic_ref<const int*>, an atomic_ref to
  // an ordinary, non-cv-qualified `const int*` variable) is a completely ordinary, unrelated case that
  // already worked and leaves value_type untouched.
  static_assert(std::same_as<std::atomic_ref<int*>::value_type, int*>);
  static_assert(std::same_as<std::atomic_ref<const int*>::value_type, const int*>);

  int i         = 42;
  int* p        = &i;
  const int* cp = &i;
  std::atomic_ref<int*> const ap(p);
  assert(ap.load() == &i);
  std::atomic_ref<const int*> const acp(cp);
  assert(acp.load() == &i);

  // cv-qualification of the pointer *itself* (e.g. atomic_ref<int* const>) is the case this paper actually
  // adds: [atomics.ref.pointer] dropped its deduced "template<class T> struct atomic_ref<T*>" notation in
  // favor of the same placeholder-type convention used for [atomics.ref.int]/[atomics.ref.float] (each paired
  // with value_type = remove_cv_t<placeholder>), so a cv-qualified pointer type must reach this specialization
  // too, exactly like atomic_ref<const int> reaches [atomics.ref.int] (unlike a naive `atomic_ref<T*>` partial
  // specialization pattern, which can never match a top-level cv-qualified argument like `int* const` at all).
  static_assert(std::same_as<std::atomic_ref<int* const>::value_type, int*>);
  static_assert(std::same_as<std::atomic_ref<int* volatile>::value_type, int*>);
  static_assert(std::same_as<std::atomic_ref<int* const>::difference_type, std::ptrdiff_t>);

  static_assert(!can_store<int* const>);
  static_assert(!can_exchange<int* const>);
  static_assert(!can_compare_exchange<int* const>);
  static_assert(can_store<int* volatile>);
  static_assert(can_exchange<int* volatile>);
  static_assert(can_compare_exchange<int* volatile>);

  int* pcp = &i;
  std::atomic_ref<int* const> const apc(pcp);
  assert(apc.load() == &i);

  int* pv = &i;
  std::atomic_ref<int* volatile> const apv(pv);
  assert(apv.load() == &i);
  apv.store(nullptr);
  assert(pv == nullptr);
  assert(apv.exchange(&i) == nullptr);
  assert(pv == &i);
  assert(apv.fetch_add(1) == &i);
  assert(pv == &i + 1);
  assert(apv.fetch_sub(1) == &i + 1);
  assert(pv == &i);
  int* expected = &i;
  assert(apv.compare_exchange_strong(expected, &i + 1));
  assert(pv == &i + 1);
  --apv;
  assert(pv == &i);
}

int main(int, char**) {
  test();
  return 0;
}
