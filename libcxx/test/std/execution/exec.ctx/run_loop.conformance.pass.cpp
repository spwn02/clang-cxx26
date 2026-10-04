//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

#include <execution>
#include <cassert>
#include <type_traits>
namespace ex = std::execution;
struct receiver {
  using receiver_concept = ex::receiver_tag;
  int* completion;
  std::inplace_stop_token token;
  auto get_env() const noexcept { return ex::env<>{}; }
  auto query(std::get_stop_token_t) const noexcept { return token; }
  void set_value() && noexcept { *completion = 1; }
  void set_stopped() && noexcept { *completion = 2; }
};
struct throwing_receiver : receiver {
  throwing_receiver(const throwing_receiver&) noexcept(false);
  throwing_receiver(throwing_receiver&&) noexcept = default;
};
int main(int, char**) {
  ex::run_loop loop;
  auto sender = ex::schedule(loop.get_scheduler());
  int completion = 0;
  std::inplace_stop_source source;
  source.request_stop();
  receiver r{&completion, source.get_token()};
  // [exec.run.loop.types]: "connect(sndr, rcvr) ... is potentially-throwing if and only if
  // (void(sndr), auto(rcvr)) is potentially-throwing."
  // "initialized with the expression rcvr passed to the invocation of connect"
  static_assert(noexcept(ex::connect(sender, r)));
  static_assert(!noexcept(ex::connect(sender, std::declval<throwing_receiver&>())));
  auto op = ex::connect(sender, r);
  ex::start(op);
  assert(completion == 0);
  loop.finish();
  loop.run();
  // [exec.run.loop.types]: "if (get_stop_token(REC(o)).stop_requested()) {
  // set_stopped(std::move(REC(o))); } else { set_value(std::move(REC(o))); }"
  assert(completion == 2);
  // The wording is unchanged from P2300R10 and checks the receiver itself; the stop token of the
  // receiver's environment is honoured as well, so cancellation through get_env is not lost.
  struct env_receiver : receiver {
    auto get_env() const noexcept { return ex::prop{std::get_stop_token, token}; }
    auto query(std::get_stop_token_t) const noexcept { return std::never_stop_token{}; }
  };
  ex::run_loop other;
  completion = 0;
  auto op2 = ex::connect(ex::schedule(other.get_scheduler()), env_receiver{{&completion, source.get_token()}});
  ex::start(op2);
  other.finish(); other.run();
  assert(completion == 2);
  return 0;
}
