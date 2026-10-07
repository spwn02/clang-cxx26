//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <utility>

// C++26 [const.wrap.class]: the template argument of constant_wrapper is a cw-fixed-value, so a string literal or any array
// can be wrapped (P4206R0 removes this in C++29).

#include <cassert>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#ifndef __cpp_lib_constant_wrapper
#  error __cpp_lib_constant_wrapper should be defined
#endif
#if __cpp_lib_constant_wrapper != 202603L
#  error __cpp_lib_constant_wrapper should have the value 202603L
#endif

constexpr auto s = std::cw<"abc">;
static_assert(sizeof(s.value) == 4 && s.value[0] == 'a' && s.value[2] == 'c');
static_assert(std::string_view(s) == "abc");
static_assert(std::is_same_v<std::remove_reference_t<decltype(s.value)>, const char[4]>);
static_assert(std::is_same_v<decltype(s)::value_type, const char[4]>);

// the same literal gives the same type
static_assert(std::is_same_v<decltype(std::cw<"abc">), decltype(std::cw<"abc">)>);
static_assert(!std::is_same_v<decltype(std::cw<"abc">), decltype(std::cw<"abd">)>);

// an array of another element type
constexpr int arr[3] = {1, 2, 3};
static_assert(std::cw<arr>[std::cw<1>] == 2);
constexpr auto ints = std::cw<arr>;
static_assert(ints.value[2] == 3);

// the unwrapping subscript works on the wrapped array
static_assert(std::cw<"xyz">[std::cw<2>] == 'z');

int main(int, char**) { return 0; }
