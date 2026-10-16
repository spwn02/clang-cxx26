//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <execution>

// check-types of the adaptors ([exec.then]p5, [exec.bulk]p6/p8, [exec.continues.on]p6, [exec.into.variant]): the function
// must be callable with the datums of the completions it receives (bulk: with an lvalue of each datum, preceded by the
// index, or the begin and end of a chunk), and the datums of the completions that are moved to another place must be
// decay-copyable. A sender that does not pass is not a sender_in (not a hard error).

#include <concepts>
#include <execution>
#include <memory>
#include <type_traits>

namespace ex = std::execution;

// bulk: the function takes an lvalue reference to the value datum
inline constexpr auto takes_unique = [](int, std::unique_ptr<int>&) noexcept {};
inline constexpr auto takes_int = [](int, int&) noexcept {};
inline constexpr auto takes_chunk_int = [](int, int, int&) noexcept {};
inline constexpr auto takes_chunk_unique = [](int, int, std::unique_ptr<int>&) noexcept {};

static_assert(ex::sender_in<decltype(ex::bulk(ex::just(1), ex::seq, 2, takes_int))>);
static_assert(!ex::sender_in<decltype(ex::bulk(ex::just(1), ex::seq, 2, takes_unique))>);
static_assert(ex::sender_in<decltype(ex::bulk_unchunked(ex::just(1), ex::seq, 2, takes_int))>);
static_assert(!ex::sender_in<decltype(ex::bulk_unchunked(ex::just(1), ex::seq, 2, takes_unique))>);
static_assert(ex::sender_in<decltype(ex::bulk_chunked(ex::just(1), ex::seq, 2, takes_chunk_int))>);
static_assert(!ex::sender_in<decltype(ex::bulk_chunked(ex::just(1), ex::seq, 2, takes_chunk_unique))>);
// a chunk function is not callable as an unchunked one and the other way round
static_assert(!ex::sender_in<decltype(ex::bulk_chunked(ex::just(1), ex::seq, 2, takes_int))>);
static_assert(!ex::sender_in<decltype(ex::bulk_unchunked(ex::just(1), ex::seq, 2, takes_chunk_int))>);
// without value completions there is nothing to call it with
static_assert(ex::sender_in<decltype(ex::bulk(ex::just_stopped(), ex::seq, 2, takes_unique))>);
// the same with an environment
static_assert(!ex::sender_in<decltype(ex::bulk(ex::just(1), ex::seq, 2, takes_unique)), ex::env<>>);

// then, upon_error, upon_stopped
inline constexpr auto takes_string = [](const char*) noexcept {};
inline constexpr auto takes_nothing = []() noexcept {};
static_assert(ex::sender_in<decltype(ex::then(ex::just(1), [](int) noexcept {}))>);
static_assert(!ex::sender_in<decltype(ex::then(ex::just(1), takes_string))>);
static_assert(!ex::sender_in<decltype(ex::then(ex::just(1), takes_nothing))>);
static_assert(ex::sender_in<decltype(ex::then(ex::just(), takes_nothing))>);
static_assert(!ex::sender_in<decltype(ex::upon_error(ex::just_error(1), takes_string))>);
static_assert(ex::sender_in<decltype(ex::upon_error(ex::just_error(1), [](int) noexcept {}))>);
static_assert(!ex::sender_in<decltype(ex::upon_stopped(ex::just_stopped(), [](int) noexcept {}))>);
static_assert(ex::sender_in<decltype(ex::upon_stopped(ex::just_stopped(), takes_nothing))>);
// only the intercepted completion is checked
static_assert(ex::sender_in<decltype(ex::then(ex::just_error(1), takes_string))>);

// continues_on and into_variant decay-copy the datums
struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
};
static_assert(ex::sender_in<decltype(ex::continues_on(ex::just(MoveOnly{}), ex::inline_scheduler{}))>);
static_assert(ex::sender_in<decltype(ex::into_variant(ex::just(MoveOnly{})))>);

int main(int, char**) { return 0; }
