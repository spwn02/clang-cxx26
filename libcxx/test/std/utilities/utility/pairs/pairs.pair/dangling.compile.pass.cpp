//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <utility>

// P2255R2: the pair constructors that would bind a reference member to a temporary are defined as deleted.

#include <array>
#include <string>
#include <type_traits>
#include <utility>

using S = std::string;
using P = std::pair<const S&, int>;

static_assert(!std::is_constructible_v<P, const char*, int>);
static_assert(!std::is_constructible_v<P, const char (&)[4], int>);
static_assert(!std::is_constructible_v<std::pair<const int&, int>, long, int>);
static_assert(std::is_constructible_v<P, S&, int>);
static_assert(std::is_constructible_v<P, const S&, int>);
static_assert(std::is_constructible_v<std::pair<S, int>, const char*, int>);

// pair<U1, U2> in all four value categories
static_assert(!std::is_constructible_v<P, std::pair<const char*, int>>);
static_assert(!std::is_constructible_v<P, std::pair<const char*, int>&>);
static_assert(!std::is_constructible_v<P, const std::pair<const char*, int>&>);
static_assert(!std::is_constructible_v<P, const std::pair<const char*, int>&&>);
static_assert(std::is_constructible_v<P, std::pair<S, int>&>);
static_assert(std::is_constructible_v<P, std::pair<S&, int>>);

// pair-like
static_assert(!std::is_constructible_v<std::pair<const S&, const S&>, std::array<const char*, 2>>);
static_assert(std::is_constructible_v<std::pair<const S&, const S&>, std::array<S, 2>&>);

// A deleted constructor is not implicitly usable either.
static_assert(!std::is_convertible_v<std::pair<const char*, int>, P>);
