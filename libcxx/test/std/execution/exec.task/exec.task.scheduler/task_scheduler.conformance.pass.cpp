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
  int* out;
  void set_value() && noexcept { ++*out; }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return ex::env<>{}; }
};
struct throwing_receiver : receiver { throwing_receiver(const throwing_receiver&) noexcept(false);
  throwing_receiver(throwing_receiver&&) noexcept = default; };
using inline_sender = decltype(ex::schedule(ex::inline_scheduler{}));
// [exec.inline.scheduler]: "completion_signatures_of_t<inline-sender> ... completion_signatures<set_value_t()>"
static_assert(std::same_as<ex::completion_signatures_of_t<inline_sender>, ex::completion_signatures<ex::set_value_t()>>);
// [exec.inline.scheduler]: "is potentially-throwing if and only if ((void)sndr, auto(rcvr)) is potentially-throwing"
static_assert(noexcept(ex::connect(ex::schedule(ex::inline_scheduler{}), std::declval<receiver&>())));
static_assert(!noexcept(ex::connect(ex::schedule(ex::inline_scheduler{}), std::declval<throwing_receiver&>())));
using erased_sender = decltype(ex::schedule(std::declval<ex::task_scheduler>()));
// [exec.task.scheduler]: "completion_signatures<set_value_t()> if unstoppable_token<stop_token_of_t<E>> is true,
// and otherwise completion_signatures<set_value_t(), set_stopped_t()>"
static_assert(std::same_as<ex::completion_signatures_of_t<erased_sender, ex::env<>>, ex::completion_signatures<ex::set_value_t()>>);
using stopped_env = ex::prop<std::get_stop_token_t, std::inplace_stop_token>;
static_assert(std::same_as<ex::completion_signatures_of_t<erased_sender, stopped_env>, ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>>);
int main(int, char**) {
  ex::run_loop loop;
  auto scheduler = loop.get_scheduler();
  // [exec.task.scheduler]: "Constructs an operation state os with connect(schedule(sched_), WRAP-RCVR(r))"
  ex::task_scheduler erased(scheduler);
  // [exec.task.scheduler]: "false if the type of SCHED(lhs) is not Sch, otherwise SCHED(lhs) == rhs."
  assert(erased == scheduler);
  assert(scheduler == erased);
  assert(!(erased == ex::inline_scheduler{}));
  int count = 0;
  auto op = ex::connect(ex::schedule(erased), receiver{&count});
  ex::start(op);
  assert(count == 0);
  loop.finish(); loop.run();
  assert(count == 1);
  // [exec.task.scheduler]: "get_completion_scheduler<set_value_t>(get_env(ts-sndr)) is equal to *this."
  assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(ex::schedule(erased))) == erased);
  // [exec.task.scheduler]: "get_completion_domain<set_value_t>(get_env(ts-sndr)) ... ts-domain()"
  // #220: the public domain CPO currently has no operator(); check its prescribed query hooks.
  static_assert(std::same_as<decltype(erased.query(ex::get_completion_domain<ex::set_value_t>)),
                            decltype(ex::get_env(ex::schedule(erased)).query(ex::get_completion_domain<ex::set_value_t>))>);
  return 0;
}
