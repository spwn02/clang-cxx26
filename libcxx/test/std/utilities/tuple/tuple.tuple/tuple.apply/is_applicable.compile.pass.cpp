//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <tuple>

// template <class Fn, class Tuple> struct is_applicable;
// template <class Fn, class Tuple> struct is_nothrow_applicable;
// template <class Fn, class Tuple> struct apply_result;

// [meta.rel]: is_applicable requires tuple-like<Tuple>, so it is false (not a hard error) for a
// Tuple that is not tuple-like, and apply_result then has no member 'type'.

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

struct F {
  int operator()(int, double) const;
};
struct G {
  int operator()(int, double) const noexcept;
};
struct H {
  int operator()(int) const;
};
struct NotTuple {
  int a;
};

template <class T, class = void>
struct has_type : std::false_type {};
template <class T>
struct has_type<T, std::void_t<typename T::type>> : std::true_type {};

// Not tuple-like: false, no hard error.
static_assert(!std::is_applicable_v<F, int>);
static_assert(!std::is_applicable_v<F, void*>);
static_assert(!std::is_applicable_v<F, NotTuple>);
static_assert(!std::is_applicable_v<F, int&>);
static_assert(!std::is_nothrow_applicable_v<F, int>);
static_assert(!has_type<std::apply_result<F, int>>::value);

// Tuple-like (tuple, pair, array; references and cv-qualification).
static_assert(std::is_applicable_v<F, std::tuple<int, double>>);
static_assert(std::is_applicable_v<F, std::tuple<int, double>&>);
static_assert(std::is_applicable_v<F, const std::tuple<int, double>&>);
static_assert(std::is_applicable_v<F, std::pair<int, double>>);
static_assert(std::is_applicable_v<H, std::array<int, 1>>);
static_assert(!std::is_applicable_v<F, std::tuple<int>>);
static_assert(!std::is_applicable_v<F, std::tuple<int, double, char>>);
static_assert(!std::is_nothrow_applicable_v<F, std::tuple<int, double>>);
static_assert(std::is_nothrow_applicable_v<G, std::tuple<int, double>&>);
static_assert(has_type<std::apply_result<F, std::tuple<int, double>&>>::value);
static_assert(std::is_same_v<std::apply_result_t<F, std::tuple<int, double>&>, int>);
