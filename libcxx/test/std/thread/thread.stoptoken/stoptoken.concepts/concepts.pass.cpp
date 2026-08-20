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

#include <stop_token>

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

int main(int, char**) { return 0; }
