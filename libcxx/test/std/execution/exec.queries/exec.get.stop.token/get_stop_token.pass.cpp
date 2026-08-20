//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <execution>

// struct get_stop_token_t : forwarding_query_t {
//   template<class Env>
//   constexpr auto operator()(const Env&) const noexcept;
// };
// inline constexpr get_stop_token_t get_stop_token{};

#include <cassert>
#include <execution>
#include <stop_token>
#include <type_traits>

struct HasStopToken {
  std::inplace_stop_token query(std::get_stop_token_t) const noexcept { return __token_; }
  std::inplace_stop_token __token_;
};

struct NoStopToken {};

int main(int, char**) {
  using namespace std;

  static_assert(forwarding_query(get_stop_token));

  // falls back to `never_stop_token` when the env doesn't answer the query
  NoStopToken no_token;
  static_assert(std::is_same_v<decltype(get_stop_token(no_token)), std::never_stop_token>);
  static_assert(noexcept(get_stop_token(no_token)));
  assert(!get_stop_token(no_token).stop_possible());

  // otherwise forwards to the env's own answer
  std::inplace_stop_source src;
  HasStopToken has_token{src.get_token()};
  static_assert(std::is_same_v<decltype(get_stop_token(has_token)), std::inplace_stop_token>);
  assert(get_stop_token(has_token).stop_possible());
  assert(!get_stop_token(has_token).stop_requested());

  src.request_stop();
  assert(get_stop_token(has_token).stop_requested());

  return 0;
}
