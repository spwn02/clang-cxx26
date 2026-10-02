//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// UNSUPPORTED: gcc-15

// <compare>

// template<class T, class U>
//   struct type_order;
// template<class T, class U>
//   constexpr strong_ordering type_order_v = type_order<T, U>::value;

#include <compare>
#include <concepts>
#include <array>
#include "test_macros.h"

template <class T, class U>
constexpr bool test_order(std::strong_ordering expected) {
  return std::type_order<T, U>()() == expected && std::type_order<T, U>::value == expected &&
         std::type_order_v<T, U> == expected && static_cast<std::strong_ordering>(std::type_order<T, U>()) == expected;
}

template <class T, class U>
constexpr bool eq = test_order<T, U>(std::strong_ordering::equal);
template <class T, class U>
constexpr bool ne =
    std::type_order<T, U>()() != std::strong_ordering::equal &&
    std::type_order<T, U>::value != std::strong_ordering::equal &&
    std::type_order_v<T, U> != std::strong_ordering::equal &&
    static_cast<std::strong_ordering>(std::type_order<T, U>()) != std::strong_ordering::equal;
template <class T, class U>
constexpr bool lt = test_order<T, U>(std::strong_ordering::less);
template <class T, class U>
constexpr bool gt = test_order<T, U>(std::strong_ordering::greater);

struct A {};
struct B {};
struct C {};

static_assert(std::same_as<std::type_order<A, A>::value_type, std::strong_ordering>);
static_assert(std::same_as<decltype(std::type_order<A, A>::value), const std::strong_ordering>);
static_assert(std::same_as<decltype(std::type_order_v<A, A>), const std::strong_ordering>);

ASSERT_NOEXCEPT((std::type_order<int, int>()()));
ASSERT_NOEXCEPT((static_cast<std::strong_ordering>(std::type_order<int, int>())));

static_assert(ne<int, const int>);
static_assert(ne<int, int&>);
static_assert(ne<int&, int&&>);

static_assert(eq<A, A>);
static_assert(ne<A, B> && ne<B, A>);

// since we do lexicographical compare of the mangled names and both A and B
// are class types, A must compare less than B regardless of ABI
static_assert(lt<A, B>);
static_assert(gt<B, A>);
static_assert(!(lt<A, B> && lt<B, C>) || lt<A, C>);

struct incomplete;
static_assert(eq<incomplete, incomplete>);
static_assert(ne<incomplete, A>);

template <auto>
constexpr bool test_template_arg = true;
static_assert(test_template_arg<std::type_order<A, incomplete>{}>);

// Check the complete ordering relation, without assuming a particular order.

struct Inc;
template <class> struct Box {};
enum class Enum {};
union Union { int n; };
using Closure = decltype([] {});

template <class... T>
struct type_list {};

template <class T, class... U>
constexpr auto order_row(type_list<U...>) {
  return std::array{(std::type_order_v<T, U> < 0 ? -1 : std::type_order_v<T, U> > 0 ? 1 : 0)...};
}

template <class... T>
constexpr bool check_order(type_list<T...> types) {
  const std::array table{order_row<T>(types)...};
  for (unsigned i = 0; i != sizeof...(T); ++i) {
    if (table[i][i] != 0)
      return false;
    for (unsigned j = 0; j != sizeof...(T); ++j) {
      if (table[i][j] != -table[j][i] || (i != j && table[i][j] == 0))
        return false;
      for (unsigned k = 0; k != sizeof...(T); ++k)
        if (table[i][j] < 0 && table[j][k] < 0 && table[i][k] >= 0)
          return false;
    }
  }
  return true;
}

constexpr bool test_local_type() {
  struct Local {};
  return check_order(type_list<int, char, const int, int&, int&&, int*, int[3], void,
                              void(), A, Inc, Box<int>, Closure, Local, Enum, Union, int A::*>{});
}
static_assert(test_local_type());
static_assert(std::type_order_v<int, char> != 0);
