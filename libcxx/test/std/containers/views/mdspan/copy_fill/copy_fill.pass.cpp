//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl
// ADDITIONAL_COMPILE_FLAGS: -D_LIBCPP_ENABLE_EXPERIMENTAL

// <mdspan>

// [mdspan.copy]: multidimensional copy and fill.
//
// template<class Src, class Dst> constexpr void copy(const Src& src, const Dst& dst);
// template<class ExecutionPolicy, class Src, class Dst> void copy(ExecutionPolicy&& policy, const Src& src, const Dst& dst);
// template<class Dst, class T = Dst::value_type> constexpr void fill(const Dst& dst, const T& value);
// template<class ExecutionPolicy, class Dst, class T = Dst::value_type>
//   void fill(ExecutionPolicy&& policy, const Dst& dst, const T& value);

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <execution>
#include <mdspan>
#include <numeric>
#include <type_traits>
#include <vector>

template <class... Args>
concept can_copy = requires(Args&&... args) { std::copy(static_cast<Args&&>(args)...); };
template <class... Args>
concept can_fill = requires(Args&&... args) { std::fill(static_cast<Args&&>(args)...); };

constexpr bool test_constexpr() {
  int src[6]{1, 2, 3, 4, 5, 6};
  int dst[6]{};
  std::mdspan s(src, std::extents<int, 2, 3>{});
  std::mdspan d(dst, std::extents<int, 2, 3>{});
  std::copy(s, d);
  for (int i = 0; i < 6; ++i)
    if (dst[i] != src[i])
      return false;
  std::fill(d, 9);
  for (int i = 0; i < 6; ++i)
    if (dst[i] != 9)
      return false;
  return true;
}
static_assert(test_constexpr());

void test_copy() {
  // rank 0
  {
    int a = 3, b = 0;
    std::mdspan<int, std::extents<int>> s(&a), d(&b);
    std::copy(s, d);
    assert(b == 3);
  }
  // rank 1 and different element types
  {
    std::array<int, 4> a{1, 2, 3, 4};
    std::array<double, 4> b{};
    std::copy(std::mdspan(a.data(), 4), std::mdspan(b.data(), 4));
    assert((b == std::array<double, 4>{1, 2, 3, 4}));
  }
  // rank 3, different layouts and extents types (static and dynamic)
  {
    std::vector<int> a(24), b(24, -1);
    std::iota(a.begin(), a.end(), 0);
    std::mdspan<int, std::extents<int, 2, 3, 4>> s(a.data());
    std::mdspan<int, std::dextents<unsigned, 3>, std::layout_left> d(b.data(), 2, 3, 4);
    std::copy(s, d);
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 4; ++k)
          assert((d[i, j, k] == s[i, j, k]));
  }
  // strided destination (a column of a matrix)
  {
    std::vector<int> a{1, 2, 3}, b(9, 0);
    std::mdspan<int, std::extents<int, 3, 3>> m(b.data());
    std::copy(std::mdspan(a.data(), 3), std::submdspan(m, std::full_extent, 1));
    assert((m[0, 1] == 1 && m[1, 1] == 2 && m[2, 1] == 3 && m[0, 0] == 0 && m[2, 2] == 0));
  }
  // empty extents
  {
    int a = 0, b = 7;
    std::mdspan<int, std::extents<int, 0, 3>> s(&a), d(&b);
    std::copy(s, d);
    assert(b == 7);
  }
  // execution policy overloads
  {
    std::vector<int> a(6), b(6, 0), c(6, 0);
    std::iota(a.begin(), a.end(), 1);
    std::mdspan s(a.data(), 2, 3), d(b.data(), 2, 3), e(c.data(), 2, 3);
    std::copy(std::execution::seq, s, d);
    std::copy(std::execution::par, s, e);
    assert(a == b && a == c);
    std::fill(std::execution::par_unseq, d, 5);
    assert((b == std::vector<int>(6, 5)));
  }
}

void test_fill() {
  std::vector<int> a(6, 0);
  std::mdspan m(a.data(), 2, 3);
  std::fill(m, 4);
  assert((a == std::vector<int>(6, 4)));
  // the default for T is Dst::value_type, so a braced value deduces nothing and a conversion applies
  std::fill(m, 2.9);
  assert((a == std::vector<int>(6, 2)));
  std::fill(m, {3});
  assert((a == std::vector<int>(6, 3)));
  std::fill(std::execution::seq, m, {5});
  assert((a == std::vector<int>(6, 5)));
  // rank 0 and empty
  int x = 0;
  std::fill(std::mdspan<int, std::extents<int>>(&x), 8);
  assert(x == 8);
  int y = 1;
  std::fill(std::mdspan<int, std::extents<int, 0>>(&y), 8);
  assert(y == 1);
  // const elements are not assignable
  const std::vector<int> c(4, 1);
  std::mdspan<const int, std::extents<int, 4>> cm(c.data());
  static_assert(!can_fill<decltype(cm)&, int>);
  static_assert(!can_copy<decltype(cm)&, decltype(cm)&>);
}

void test_constraints() {
  using M2  = std::mdspan<int, std::extents<int, 2, 3>>;
  using M3  = std::mdspan<int, std::extents<int, 2, 3, 1>>;
  using MD  = std::mdspan<int, std::extents<int, 2, 4>>;
  using MX  = std::mdspan<int, std::dextents<int, 2>>;
  using CM  = std::mdspan<const int, std::extents<int, 2, 3>>;
  static_assert(can_copy<M2&, M2&>);
  static_assert(can_copy<CM&, M2&>);
  static_assert(!can_copy<M2&, CM&>);     // const destination elements
  static_assert(!can_copy<M2&, M3&>);     // different ranks
  static_assert(!can_copy<M2&, MD&>);     // different static extents
  static_assert(can_copy<M2&, MX&>);      // static to dynamic
  static_assert(can_copy<MX&, M2&>);
  static_assert(!can_copy<int&, int&>);   // not mdspans
  static_assert(!can_copy<M2&, int&>);
  static_assert(can_copy<std::execution::sequenced_policy, M2&, M2&>);
  static_assert(!can_copy<int, M2&, M2&>);  // not an execution policy
  static_assert(can_fill<M2&, int>);
  static_assert(can_fill<M2&, double>);
  static_assert(!can_fill<int&, int>);
  static_assert(can_fill<std::execution::parallel_policy, M2&, int>);
  static_assert(!can_fill<int, M2&, int>);
  static_assert(std::same_as<decltype(std::copy(std::declval<M2&>(), std::declval<M2&>())), void>);
  static_assert(std::same_as<decltype(std::fill(std::declval<M2&>(), 1)), void>);
  // the iterator overloads are untouched
  static_assert(can_copy<int*, int*, int*>);
  static_assert(can_copy<std::execution::sequenced_policy, int*, int*, int*>);
  static_assert(can_fill<int*, int*, int>);
}

int main(int, char**) {
  test_copy();
  test_fill();
  test_constraints();
  return 0;
}
