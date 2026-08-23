//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// XFAIL: !has-64-bit-atomics

// integral-type fetch_max(integral-type, memory_order = memory_order::seq_cst) const noexcept;
// floating-point-type fetch_max(floating-point-type, memory_order = memory_order::seq_cst) const noexcept;
// T* fetch_max(T*, memory_order = memory_order::seq_cst) const noexcept;

#include <atomic>
#include <cassert>
#include <cmath>
#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

#include "atomic_helpers.h"
#include "test_helper.h"
#include "test_macros.h"

template <typename T>
concept has_fetch_max = requires {
  std::declval<T const>().fetch_max(std::declval<T>());
  std::declval<T const>().fetch_max(std::declval<T>(), std::declval<std::memory_order>());
};

template <typename T>
struct TestDoesNotHaveFetchMax {
  void operator()() const { static_assert(!has_fetch_max<std::atomic_ref<T>>); }
};

template <typename T>
struct TestFetchMax {
  void operator()() const {
    if constexpr (std::is_arithmetic_v<T>) {
      T x(T(5));
      std::atomic_ref<T> const a(x);

      {
        std::same_as<T> decltype(auto) y = a.fetch_max(T(3));
        assert(y == T(5));
        assert(x == T(5));
        ASSERT_NOEXCEPT(a.fetch_max(T(0)));
      }

      {
        std::same_as<T> decltype(auto) y = a.fetch_max(T(9), std::memory_order_relaxed);
        assert(y == T(5));
        assert(x == T(9));
        ASSERT_NOEXCEPT(a.fetch_max(T(0), std::memory_order_relaxed));
      }

      if constexpr (std::is_floating_point_v<T>) {
        // NaN: never propagates into the stored value if the other operand isn't NaN.
        const T nan = std::numeric_limits<T>::quiet_NaN();
        {
          T y(T(3.1));
          std::atomic_ref<T> const b(y);
          assert(b.fetch_max(nan) == T(3.1));
          assert(y == T(3.1));
        }
        {
          T y(nan);
          std::atomic_ref<T> const b(y);
          assert(std::isnan(b.fetch_max(T(3.1))));
          assert(y == T(3.1));
        }
      }
    } else {
      static_assert(std::is_pointer_v<T>);
      using U = std::remove_pointer_t<T>;
      U t[9] = {};
      T p{&t[5]};
      std::atomic_ref<T> const a(p);

      {
        std::same_as<T> decltype(auto) y = a.fetch_max(&t[3]);
        assert(y == &t[5]);
        assert(a == &t[5]);
        ASSERT_NOEXCEPT(a.fetch_max(&t[0]));
      }

      {
        std::same_as<T> decltype(auto) y = a.fetch_max(&t[7], std::memory_order_relaxed);
        assert(y == &t[5]);
        assert(a == &t[7]);
        ASSERT_NOEXCEPT(a.fetch_max(&t[0], std::memory_order_relaxed));
      }
    }

    // memory_order::release
    {
      auto fetch_max = [](std::atomic_ref<T> const& x, T old_val, T new_val) {
        (void)old_val;
        x.fetch_max(new_val, std::memory_order::release);
      };
      auto load = [](std::atomic_ref<T> const& x) { return x.load(std::memory_order::acquire); };
      test_acquire_release<T>(fetch_max, load);
    }

    // memory_order::seq_cst
    {
      auto fetch_max_no_arg = [](std::atomic_ref<T> const& x, T old_val, T new_val) {
        (void)old_val;
        x.fetch_max(new_val);
      };
      auto fetch_max_with_order = [](std::atomic_ref<T> const& x, T old_val, T new_val) {
        (void)old_val;
        x.fetch_max(new_val, std::memory_order::seq_cst);
      };
      auto load = [](std::atomic_ref<T> const& x) { return x.load(); };
      test_seq_cst<T>(fetch_max_no_arg, load);
      test_seq_cst<T>(fetch_max_with_order, load);
    }
  }
};

int main(int, char**) {
  TestEachIntegralType<TestFetchMax>()();

  TestFetchMax<float>()();
  TestFetchMax<double>()();

  TestEachPointerType<TestFetchMax>()();

  TestDoesNotHaveFetchMax<bool>()();
  TestDoesNotHaveFetchMax<UserAtomicType>()();
  TestDoesNotHaveFetchMax<LargeUserAtomicType>()();

  return 0;
}
