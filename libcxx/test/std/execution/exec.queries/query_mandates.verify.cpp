//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// The Mandates of the query objects and completion functions of [exec]: MANDATE-NOTHROW(expr) requires
// noexcept(expr), and the result types of get_allocator, get_env, the scheduler queries, forwarding_query,
// set_value/set_error/set_stopped and start are constrained by Mandates elements. A violation makes the program
// ill-formed, it does not silently select another overload or fall back to a default.

#include <execution>
#include <memory>

namespace ex = std::execution;

// expected-note@*:* 0+ {{in instantiation of}}

struct ForwardingNonBool {
  constexpr int query(std::forwarding_query_t) const noexcept { return 1; }
};
struct ForwardingThrowing {
  constexpr bool query(std::forwarding_query_t) const { return true; }
};
struct ThrowingAllocator {
  std::allocator<int> query(std::get_allocator_t) const { return {}; }
};
struct NotAnAllocator {
  int query(std::get_allocator_t) const noexcept { return 0; }
};
struct ThrowingGetEnv {
  ex::env<> get_env() const { return {}; }
};
struct VoidGetEnv {
  void get_env() const noexcept {}
};
struct IntCompletionScheduler {
  int query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return 7; }
};
struct ThrowingCompletionScheduler {
  ex::inline_scheduler query(ex::get_completion_scheduler_t<ex::set_value_t>) const { return {}; }
};
struct ThrowingStartScheduler {
  ex::inline_scheduler query(ex::get_start_scheduler_t) const { return {}; }
};
struct IntStartScheduler {
  int query(ex::get_start_scheduler_t) const noexcept { return 0; }
};
struct ThrowingDelegationScheduler {
  ex::inline_scheduler query(ex::get_delegation_scheduler_t) const { return {}; }
};
struct IntDelegationScheduler {
  int query(ex::get_delegation_scheduler_t) const noexcept { return 0; }
};
struct ThrowingScheduler {
  ex::inline_scheduler query(ex::get_scheduler_t) const { return {}; }
};
struct NonVoidCompletions {
  int set_value() && noexcept { return 1; }
  int set_error(int) && noexcept { return 1; }
  int set_stopped() && noexcept { return 1; }
};
struct NonVoidStart {
  using operation_state_concept = ex::operation_state_tag;
  int start() & noexcept { return 7; }
};

void test() {
  (void)std::forwarding_query(ForwardingNonBool{});        // expected-error@*:* {{Mandates: the expression q.query(forwarding_query) has type bool}}
  (void)std::forwarding_query(ForwardingThrowing{});       // expected-error@*:* {{Mandates: the expression q.query(forwarding_query) is noexcept}}
  (void)std::get_allocator(ThrowingAllocator{});           // expected-error@*:* {{Mandates: the expression env.query(get_allocator) is noexcept}}
  (void)std::get_allocator(NotAnAllocator{});              // expected-error@*:* {{Mandates: the type of env.query(get_allocator) satisfies simple-allocator}}
  (void)ex::get_env(ThrowingGetEnv{});                     // expected-error@*:* {{Mandates: the expression o.get_env() is noexcept}}
  (void)ex::get_env(VoidGetEnv{});                         // expected-error@*:* {{Mandates: the type of o.get_env() satisfies queryable}}
  (void)ex::get_completion_scheduler<ex::set_value_t>(IntCompletionScheduler{});
  // expected-error@*:* {{Mandates: the type of the expression get_completion_scheduler<tag>(q, envs...) satisfies scheduler}}
  (void)ex::get_completion_scheduler<ex::set_value_t>(ThrowingCompletionScheduler{});
  // expected-error@*:* {{Mandates: the query expression of get_completion_scheduler is noexcept}}
  (void)ex::get_start_scheduler(ThrowingStartScheduler{}); // expected-error@*:* {{Mandates: the expression env.query(get_start_scheduler) is noexcept}}
  (void)ex::get_start_scheduler(IntStartScheduler{});      // expected-error@*:* {{Mandates: the type of env.query(get_start_scheduler) satisfies scheduler}}
  (void)ex::get_delegation_scheduler(ThrowingDelegationScheduler{}); // expected-error@*:* {{Mandates: the expression env.query(get_delegation_scheduler) is noexcept}}
  (void)ex::get_delegation_scheduler(IntDelegationScheduler{});      // expected-error@*:* {{Mandates: the type of env.query(get_delegation_scheduler) satisfies scheduler}}
  (void)ex::get_scheduler(ThrowingScheduler{});            // expected-error@*:* {{Mandates: the expression env.query(get_scheduler) is noexcept}}
  ex::set_value(NonVoidCompletions{});                     // expected-error@*:* {{Mandates: the type of the expression rcvr.set_value(...) is void}}
  ex::set_error(NonVoidCompletions{}, 1);                  // expected-error@*:* {{Mandates: the type of the expression rcvr.set_error(...) is void}}
  ex::set_stopped(NonVoidCompletions{});                   // expected-error@*:* {{Mandates: the type of the expression rcvr.set_stopped(...) is void}}
  NonVoidStart op;
  ex::start(op);                                           // expected-error@*:* {{Mandates: the type of the expression op.start() is void}}
}
