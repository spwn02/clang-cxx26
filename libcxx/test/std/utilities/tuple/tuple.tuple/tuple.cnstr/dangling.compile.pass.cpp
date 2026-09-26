//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <tuple>

// P2255R2: the tuple constructors that would bind a reference element to a temporary are defined as deleted.

#include <tuple>
#include <string>
#include <utility>
#include <array>
#include <memory>
#include <type_traits>
using S = std::string;
using T1 = std::tuple<const S&>;
static_assert(!std::is_constructible_v<std::tuple<const int&>, long>);
static_assert(!std::is_constructible_v<T1, const char*>);
static_assert(!std::is_constructible_v<T1, const char(&)[4]>);
static_assert(std::is_constructible_v<T1, S&>);
static_assert(std::is_constructible_v<T1, const S&>);
using T2 = std::tuple<const S&, int>;
static_assert(!std::is_constructible_v<T2, const char*, int>);
static_assert(std::is_constructible_v<T2, S&, int>);
static_assert(!std::is_constructible_v<T2, std::allocator_arg_t, std::allocator<int>, const char*, int>);
static_assert(std::is_constructible_v<T2, std::allocator_arg_t, std::allocator<int>, S&, int>);
// tuple<U...>
static_assert(!std::is_constructible_v<T2, std::tuple<const char*, int>>);
static_assert(!std::is_constructible_v<T2, std::tuple<const char*, int>&>);
static_assert(!std::is_constructible_v<T2, const std::tuple<const char*, int>&>);
static_assert(!std::is_constructible_v<T2, const std::tuple<const char*, int>&&>);
static_assert(!std::is_constructible_v<T2, std::allocator_arg_t, std::allocator<int>, std::tuple<const char*, int>>);
static_assert(std::is_constructible_v<T2, std::tuple<S, int>&>);
static_assert(std::is_constructible_v<T2, std::tuple<S&, int>>);
// pair
static_assert(!std::is_constructible_v<T2, std::pair<const char*, int>>);
static_assert(!std::is_constructible_v<T2, std::pair<const char*, int>&>);
static_assert(!std::is_constructible_v<T2, const std::pair<const char*, int>&>);
static_assert(!std::is_constructible_v<T2, const std::pair<const char*, int>&&>);
static_assert(!std::is_constructible_v<T2, std::allocator_arg_t, std::allocator<int>, std::pair<const char*, int>>);
static_assert(std::is_constructible_v<T2, std::pair<S, int>&>);
// tuple-like
static_assert(!std::is_constructible_v<T2, std::array<const char*, 2>> );  // element types differ -> not constructible either way
static_assert(!std::is_constructible_v<std::tuple<const S&, const S&>, std::array<const char*, 2>>);
static_assert(std::is_constructible_v<std::tuple<const S&, const S&>, std::array<S, 2>&>);
static_assert(!std::is_constructible_v<std::tuple<const S&, const S&>, std::allocator_arg_t, std::allocator<int>, std::array<const char*, 2>>);
// other conversions still fine
static_assert(std::is_constructible_v<std::tuple<S, int>, const char*, int>);
static_assert(std::is_convertible_v<const char*, std::tuple<S>> );
static_assert(std::is_constructible_v<std::tuple<S, S>, std::pair<const char*, const char*>>);
static_assert(std::is_constructible_v<std::tuple<int>, long>);
