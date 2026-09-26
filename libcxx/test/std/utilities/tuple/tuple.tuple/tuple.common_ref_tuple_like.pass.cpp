//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <tuple>

// P2165R4: a tuple and another tuple-like (pair, array, subrange) have a common type / common reference, compare with
// each other, and can be concatenated.

#include <array>
#include <compare>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

using T = std::tuple<int, int>;
using A = std::array<int, 2>;

static_assert(std::is_same_v<std::common_type_t<std::tuple<int, long>, std::pair<long, int>>, std::tuple<long, long>>);
static_assert(std::is_same_v<std::common_type_t<std::pair<int, long>, std::tuple<int, int>>, std::tuple<int, long>>);
static_assert(std::is_same_v<std::common_type_t<T, A>, T>);
static_assert(std::is_same_v<std::common_type_t<std::tuple<int, int>, std::array<long, 2>>, std::tuple<long, long>>);
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int&, long&>&, std::pair<int&, long&>&>,
                             std::tuple<int&, long&>>);
static_assert(std::is_same_v<std::common_type_t<std::pair<int, int>, std::pair<long, int>>, std::pair<long, int>>);

template <class X, class Y>
concept has_common_type = requires { typename std::common_type_t<X, Y>; };
static_assert(!has_common_type<std::tuple<int>, std::pair<int, int>>); // different sizes

constexpr bool test() {
  T t{1, 2};
  A a{1, 2};
  if (!(t == a) || (t <=> a) != 0 || !(t == std::pair{1, 2}))
    return false;

  int arr[2] = {3, 4};
  std::ranges::subrange<int*, int*> sr(arr, arr + 2);
  auto cat = std::tuple_cat(std::tuple{1}, sr, std::pair{5, 6}, a);
  static_assert(std::is_same_v<decltype(cat), std::tuple<int, int*, int*, int, int, int, int>>);
  if (std::get<1>(cat) != arr || std::get<2>(cat) != arr + 2 || std::get<6>(cat) != 2)
    return false;

  if (!std::apply([](int* b, int* e) { return e - b == 2; }, sr))
    return false;
  return std::make_from_tuple<std::pair<int, int>>(a) == std::pair{1, 2};
}
static_assert(test());

int main(int, char**) { return 0; }
