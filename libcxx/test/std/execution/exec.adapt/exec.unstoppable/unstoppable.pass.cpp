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

// inline constexpr unspecified unstoppable{};

#include <cassert>
#include <execution>
#include <stop_token>
#include <type_traits>

using namespace std::execution;

// [exec.unstoppable]p2: "unstoppable(sndr) is expression-equivalent to
// write_env(sndr, prop(get_stop_token, never_stop_token{}))." Unlike write_env, unstoppable
// takes only a single (sender) argument, so no separate "no partial-application form" static
// check is needed the way <__execution/write_env.h>'s test has one -- there's nothing to
// partially apply.

struct rcvr {
  using receiver_concept = receiver_tag;
  bool* saw_never_stop_token;
  std::inplace_stop_token tok;

  // The child's completion datum is whatever get_stop_token(child's env) returned -- see
  // read_env(std::get_stop_token) below.
  template <class _StopToken>
  void set_value(_StopToken&&) && noexcept {
    *saw_never_stop_token = std::is_same_v<std::decay_t<_StopToken>, std::never_stop_token>;
  }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }

  // The *outer* receiver's own environment answers get_stop_token with a genuinely stoppable
  // token -- the point of the test is that unstoppable's child does not see this.
  struct env_t {
    std::inplace_stop_token tok;
    auto query(std::get_stop_token_t) const noexcept { return tok; }
  };
  auto get_env() const noexcept { return env_t{tok}; }
};

int main(int, char**) {
  {
    std::inplace_stop_source src;
    bool saw_never_stop_token = false;
    auto op = connect(unstoppable(read_env(std::get_stop_token)), rcvr{&saw_never_stop_token, src.get_token()});
    start(op);
    assert(saw_never_stop_token);
  }
  // Cross-check against write_env directly, tying [exec.unstoppable]p2's expression-equivalence
  // to <__execution/write_env.h>'s own implementation: same completion signatures either way.
  {
    static_assert(std::is_same_v<
                   completion_signatures_of_t<decltype(unstoppable(read_env(std::get_stop_token))), env<>>,
                   completion_signatures_of_t<
                       decltype(write_env(read_env(std::get_stop_token), prop(std::get_stop_token, std::never_stop_token{}))),
                       env<>>>);
  }
  return 0;
}
