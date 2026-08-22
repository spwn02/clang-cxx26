//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// XFAIL: !has-64-bit-atomics

// Older versions of clang have a bug with atomic builtins affecting double and long double.
// Fixed by 5fdd0948.
// XFAIL: target=powerpc-ibm-{{.*}} && clang-18

// https://github.com/llvm/llvm-project/issues/72893
// XFAIL: target={{x86_64-.*}} && tsan

// floating-point-type fetch_max(floating-point-type,
//                               memory_order = memory_order::seq_cst) volatile noexcept;
// floating-point-type fetch_max(floating-point-type,
//                               memory_order = memory_order::seq_cst) noexcept;

#include <atomic>
#include <cassert>
#include <cmath>
#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

#include "test_helper.h"
#include "test_macros.h"

template <class T>
concept HasVolatileFetchMax = requires(volatile std::atomic<T>& a, T t) { a.fetch_max(t); };

template <class T, template <class> class MaybeVolatile = std::type_identity_t>
void test_impl() {
  static_assert(HasVolatileFetchMax<T> == std::atomic<T>::is_always_lock_free);
  static_assert(noexcept(std::declval<MaybeVolatile<std::atomic<T>>&>().fetch_max(T(0))));

  // fetch_max
  {
    MaybeVolatile<std::atomic<T>> a(T(3.1));
    std::same_as<T> decltype(auto) r = a.fetch_max(T(1.2), std::memory_order::relaxed);
    assert(r == T(3.1));
    assert(a.load() == T(3.1));
  }
  {
    MaybeVolatile<std::atomic<T>> a(T(3.1));
    assert(a.fetch_max(T(9.5)) == T(3.1));
    assert(a.load() == T(9.5));
  }

  // NaN: never propagates into the stored value if the other operand isn't NaN.
  {
    const T nan = std::numeric_limits<T>::quiet_NaN();
    MaybeVolatile<std::atomic<T>> a(T(3.1));
    assert(a.fetch_max(nan) == T(3.1));
    assert(a.load() == T(3.1));
  }
  {
    const T nan = std::numeric_limits<T>::quiet_NaN();
    MaybeVolatile<std::atomic<T>> a(nan);
    assert(std::isnan(a.fetch_max(T(3.1))));
    assert(a.load() == T(3.1));
  }

  // memory_order::release
  {
    auto store = [](MaybeVolatile<std::atomic<T>>& x, T old_val, T new_val) {
      (void)old_val;
      x.fetch_max(new_val, std::memory_order::release);
    };
    auto load = [](MaybeVolatile<std::atomic<T>>& x) { return x.load(std::memory_order::acquire); };
    test_acquire_release<T, MaybeVolatile>(store, load);
  }

  // memory_order::seq_cst
  {
    auto fetch_max = [](MaybeVolatile<std::atomic<T>>& x, T old_value, T new_val) {
      (void)old_value;
      x.fetch_max(new_val);
    };
    auto fetch_max_with_order = [](MaybeVolatile<std::atomic<T>>& x, T old_value, T new_val) {
      x.fetch_max(new_val, std::memory_order::seq_cst);
      (void)old_value;
    };
    auto load = [](MaybeVolatile<std::atomic<T>>& x) { return x.load(); };
    test_seq_cst<T, MaybeVolatile>(fetch_max, load);
    test_seq_cst<T, MaybeVolatile>(fetch_max_with_order, load);
  }
}

template <class T>
void test() {
  test_impl<T>();
  if constexpr (std::atomic<T>::is_always_lock_free) {
    test_impl<T, std::add_volatile_t>();
  }
}

int main(int, char**) {
  test<float>();
  test<double>();
  // TODO https://github.com/llvm/llvm-project/issues/47978
  // test<long double>();

  return 0;
}
