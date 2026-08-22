//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads
// XFAIL: availability-synchronization_library-missing

// <stop_token>

// template<class Token>
//   concept stoppable_token = ...;
// template<class Token>
//   concept unstoppable_token = ...;
// template<class T, class CallbackFn>
//   using stop_callback_for_t = T::template callback_type<CallbackFn>;

#include <stop_token>
#include <type_traits>

struct NotAToken {};

struct MinimalToken {
  template <class>
  struct callback_type {
    explicit callback_type(MinimalToken, auto&&) noexcept {}
  };

  bool stop_requested() const noexcept { return false; }
  bool stop_possible() const noexcept { return false; }

  friend bool operator==(const MinimalToken&, const MinimalToken&) = default;
};

static_assert(std::stoppable_token<std::stop_token>);
static_assert(std::stoppable_token<std::inplace_stop_token>);
static_assert(std::stoppable_token<std::never_stop_token>);
static_assert(std::stoppable_token<MinimalToken>);

static_assert(!std::stoppable_token<NotAToken>);
static_assert(!std::stoppable_token<int>);
static_assert(!std::stoppable_token<void>);

// only `never_stop_token` can statically prove, at compile time, that a stop is never possible
static_assert(std::unstoppable_token<std::never_stop_token>);
static_assert(!std::unstoppable_token<std::stop_token>);
static_assert(!std::unstoppable_token<std::inplace_stop_token>);
static_assert(!std::unstoppable_token<NotAToken>);

static_assert(std::is_same_v<std::stop_callback_for_t<std::inplace_stop_token, int>,
                              std::inplace_stop_callback<int>>);
static_assert(std::is_same_v<std::stop_callback_for_t<std::stop_token, int>, std::stop_callback<int>>);
static_assert(std::is_same_v<std::stop_callback_for_t<MinimalToken, int>, MinimalToken::callback_type<int>>);

int main(int, char**) { return 0; }
