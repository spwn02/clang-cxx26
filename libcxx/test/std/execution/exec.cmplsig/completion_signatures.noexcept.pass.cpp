//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.cmplsig]: the function types a completion_signatures can have are set_value_t(Vs...), set_error_t(Err) and
// set_stopped_t(), without a noexcept specifier (a pointer to a noexcept function converts to a pointer to the permitted
// type, which the concept would accept).

#include <concepts>
#include <execution>

namespace ex = std::execution;

template <class... Fns>
concept valid_completion_signatures = requires { typename ex::completion_signatures<Fns...>; };

static_assert(valid_completion_signatures<>);
static_assert(valid_completion_signatures<ex::set_value_t(int), ex::set_value_t(int, double), ex::set_error_t(int),
                                          ex::set_stopped_t()>);
static_assert(valid_completion_signatures<ex::set_value_t(int&), ex::set_value_t(const int&&), ex::set_error_t(int&)>);
static_assert(!valid_completion_signatures<ex::set_value_t(int) noexcept>);
static_assert(!valid_completion_signatures<ex::set_value_t() noexcept>);
static_assert(!valid_completion_signatures<ex::set_error_t(int) noexcept>);
static_assert(!valid_completion_signatures<ex::set_stopped_t() noexcept>);
static_assert(!valid_completion_signatures<ex::set_value_t(int), ex::set_stopped_t() noexcept>);
static_assert(!valid_completion_signatures<ex::set_error_t()>);
static_assert(!valid_completion_signatures<ex::set_error_t(int, int)>);
static_assert(!valid_completion_signatures<ex::set_stopped_t(int)>);
static_assert(!valid_completion_signatures<int>);
static_assert(!valid_completion_signatures<void(int)>);

int main(int, char**) { return 0; }
