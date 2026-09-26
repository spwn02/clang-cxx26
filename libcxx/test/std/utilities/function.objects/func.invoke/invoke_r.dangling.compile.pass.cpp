//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <functional>

// P2255R2: INVOKE<R>(f, args...) is ill-formed if reference_converts_from_temporary_v<R, decltype(INVOKE(f, args...))>,
// so is_invocable_r is false and std::function refuses such targets.

#include <functional>
#include <string>
#include <type_traits>

int returns_int();
const char* returns_cstr();
const std::string& returns_string_ref();

static_assert(!std::is_invocable_r_v<const long&, decltype(&returns_int)>);
static_assert(std::is_invocable_r_v<long, decltype(&returns_int)>);
static_assert(std::is_invocable_r_v<void, decltype(&returns_int)>);
static_assert(!std::is_invocable_r_v<const std::string&, decltype(&returns_cstr)>);
static_assert(std::is_invocable_r_v<const std::string&, decltype(&returns_string_ref)>);
static_assert(!std::is_nothrow_invocable_r_v<const long&, decltype(&returns_int)>);

static_assert(!std::is_constructible_v<std::function<const long&()>, decltype(&returns_int)>);
static_assert(std::is_constructible_v<std::function<long()>, decltype(&returns_int)>);
static_assert(!std::is_constructible_v<std::function<const std::string&()>, decltype(&returns_cstr)>);
