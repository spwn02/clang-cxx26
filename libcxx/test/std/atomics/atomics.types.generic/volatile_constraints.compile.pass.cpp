//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <atomic>

// P1831R1: the volatile overloads of the member functions of atomic<T> are only available
// if atomic<T>::is_always_lock_free is true.

#include <atomic>

template <class T>
concept has_volatile_load = requires(volatile std::atomic<T>& a) { a.load(); };
template <class T>
concept has_volatile_store = requires(volatile std::atomic<T>& a, T t) { a.store(t); };
template <class T>
concept has_volatile_exchange = requires(volatile std::atomic<T>& a, T t) { a.exchange(t); };
template <class T>
concept has_volatile_cas_strong = requires(volatile std::atomic<T>& a, T t) { a.compare_exchange_strong(t, t); };
template <class T>
concept has_volatile_cas_weak = requires(volatile std::atomic<T>& a, T t) { a.compare_exchange_weak(t, t); };
template <class T>
concept has_volatile_wait = requires(volatile std::atomic<T>& a, T t) { a.wait(t); };
template <class T>
concept has_volatile_notify_one = requires(volatile std::atomic<T>& a) { a.notify_one(); };
template <class T>
concept has_volatile_notify_all = requires(volatile std::atomic<T>& a) { a.notify_all(); };
template <class T>
concept has_volatile_conversion = requires(volatile std::atomic<T>& a) { T(a); };
template <class T>
concept has_volatile_assignment = requires(volatile std::atomic<T>& a, T t) { a = t; };
// Only a query: not constrained.
template <class T>
concept has_volatile_is_lock_free = requires(volatile std::atomic<T>& a) { a.is_lock_free(); };

struct Small {
  int a;
};
struct Big {
  char data[64];
};

template <class T>
constexpr bool check() {
  constexpr bool lock_free = std::atomic<T>::is_always_lock_free;
  static_assert(has_volatile_load<T> == lock_free);
  static_assert(has_volatile_store<T> == lock_free);
  static_assert(has_volatile_exchange<T> == lock_free);
  static_assert(has_volatile_cas_strong<T> == lock_free);
  static_assert(has_volatile_cas_weak<T> == lock_free);
  static_assert(has_volatile_wait<T> == lock_free);
  static_assert(has_volatile_notify_one<T> == lock_free);
  static_assert(has_volatile_notify_all<T> == lock_free);
  static_assert(has_volatile_conversion<T> == lock_free);
  static_assert(has_volatile_assignment<T> == lock_free);
  static_assert(has_volatile_is_lock_free<T>);
  // The non-volatile overloads are always there.
  static_assert(requires(std::atomic<T>& a, T t) {
    a.load();
    a.store(t);
    a.exchange(t);
  });
  return lock_free;
}

static_assert(check<int>());
static_assert(check<long>());
static_assert(check<bool>());
static_assert(check<float>());
static_assert(check<double>());
static_assert(check<int*>());
static_assert(check<Small>());
static_assert(!check<Big>()); // not lock-free: no volatile overloads

// Integral and pointer read-modify-write operations.
template <class T>
concept has_volatile_fetch_add = requires(volatile std::atomic<T>& a, T t) { a.fetch_add(t); };
template <class T>
concept has_volatile_increment = requires(volatile std::atomic<T>& a) { ++a; a++; };
template <class T>
concept has_volatile_plus_assign = requires(volatile std::atomic<T>& a, T t) { a += t; };
static_assert(has_volatile_fetch_add<int> && has_volatile_increment<int> && has_volatile_plus_assign<int>);
static_assert(has_volatile_fetch_add<float> && has_volatile_plus_assign<float>);
static_assert(requires(volatile std::atomic<int*>& a) { a.fetch_add(1); ++a; a -= 1; });

int main(int, char**) { return 0; }
