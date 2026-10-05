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

// [exec.task.scheduler]: explicit task_scheduler(Sch&& sch, Allocator alloc = {})
//   Mandates: Sch satisfies infallible-scheduler<env<>>.

#include <exception>
#include <execution>

namespace ex = std::execution;

struct Sender {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>{};
  }
  template <class Rcvr>
  struct Op {
    using operation_state_concept = ex::operation_state_tag;
    Rcvr rcvr;
    void start() & noexcept { ex::set_value(std::move(rcvr)); }
  };
  template <class Rcvr>
  Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
    return {std::forward<Rcvr>(r)};
  }
};

// a scheduler whose schedule operation can fail
struct FallibleScheduler {
  using scheduler_concept = ex::scheduler_tag;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(FallibleScheduler, FallibleScheduler) = default;
};
static_assert(ex::scheduler<FallibleScheduler>);

// expected-note@*:* 0+ {{in instantiation of}}

void test() {
  ex::task_scheduler ok{ex::inline_scheduler{}};
  ex::task_scheduler bad{FallibleScheduler{}}; // expected-error@*:* {{Mandates: Sch satisfies infallible-scheduler<env<>>}}
  (void)ok;
  (void)bad;
}
