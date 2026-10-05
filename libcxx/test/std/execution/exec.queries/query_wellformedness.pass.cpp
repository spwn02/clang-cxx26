//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// What the query objects of [exec] accept and what makes them ill-formed (as opposed to a Mandates violation, see
// query_mandates.verify.cpp): get_domain only requires D() to be noexcept, get_env falls back to env<>{} only if
// AS-CONST(o).get_env() is ill-formed, get_scheduler and get_completion_scheduler are ill-formed for a result that is
// not a scheduler, start is ill-formed for an rvalue operation state.

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

// [exec.get.domain]: the query itself may throw, only D() is MANDATE-NOTHROW.
struct Domain {};
struct ThrowingQueryEnv {
  Domain query(ex::get_domain_t) const { return {}; }
};
static_assert(std::is_same_v<decltype(ex::get_domain(ThrowingQueryEnv{})), Domain>);
static_assert(noexcept(ex::get_domain(ThrowingQueryEnv{})));

// [exec.get.env]: a get_env that is not callable on a const object is ill-formed, so the fallback applies.
struct NonConstGetEnv {
  struct Env {
    int query(std::get_allocator_t) const;
  };
  Env get_env() noexcept { return {}; }
};
static_assert(std::is_same_v<decltype(ex::get_env(std::declval<NonConstGetEnv&>())), ex::env<>>);
struct ConstGetEnv {
  ex::env<> get_env() const noexcept { return {}; }
};
static_assert(std::is_same_v<decltype(ex::get_env(ConstGetEnv{})), ex::env<>>);
static_assert(noexcept(ex::get_env(ConstGetEnv{})));

// [exec.get.scheduler], [exec.get.compl.sched]: ill-formed, not a hard error, for a non-scheduler result.
struct IntScheduler {
  int query(ex::get_scheduler_t) const noexcept { return 0; }
};
template <class T>
concept has_get_scheduler = requires(const T& t) { ex::get_scheduler(t); };
static_assert(!has_get_scheduler<IntScheduler>);
template <class T>
concept has_get_completion_scheduler = requires(const T& t) { ex::get_completion_scheduler<ex::set_value_t>(t); };
static_assert(!has_get_completion_scheduler<IntScheduler>);
static_assert(!has_get_completion_scheduler<int>);
// the query tag must be a completion tag
template <class Tag, class T>
concept has_get_completion_scheduler_with_env = requires(const T& t) { ex::get_completion_scheduler<Tag>(t, ex::env<>{}); };
static_assert(!has_get_completion_scheduler_with_env<int, ex::inline_scheduler>);
// a scheduler is its own completion scheduler when an environment is passed
static_assert(has_get_completion_scheduler_with_env<ex::set_value_t, ex::inline_scheduler>);
static_assert(!has_get_completion_scheduler<ex::inline_scheduler>);

// [exec.opstate.start]: ill-formed for an rvalue.
struct Op {
  using operation_state_concept = ex::operation_state_tag;
  void start() & noexcept {}
  void start() const&& noexcept {}
};
template <class T>
concept can_start = requires(T&& t) { ex::start(std::forward<T>(t)); };
static_assert(can_start<Op&>);
static_assert(!can_start<Op>);
static_assert(!can_start<const Op>);
static_assert(!can_start<Op&&>);
// A const-callable start must still not be reachable through an rvalue (const Op&& binds to const Op&).
struct ConstOp {
  using operation_state_concept = ex::operation_state_tag;
  void start() const& noexcept {}
};
static_assert(can_start<const ConstOp&>);
static_assert(!can_start<const ConstOp>);
static_assert(!can_start<ConstOp>);

int main(int, char**) {
  Op op;
  ex::start(op);
  return 0;
}
