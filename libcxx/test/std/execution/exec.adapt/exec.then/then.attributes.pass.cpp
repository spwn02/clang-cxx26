//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.snd.general]p9-10 and [exec.adapt.general]p3.2: the completion scheduler and completion domain attributes of
// then, upon_error and upon_stopped. The completion operations of the adaptor with the tag of the intercepted
// completion happen where the child completed with it; a potentially throwing function adds an error completion that
// happens where the invocation did; the completion domain of an error completion with two possible places is the
// COMMON-DOMAIN of their domains. Other queries are forwarded as by FWD-ENV.

#include <cassert>
#include <concepts>
#include <exception>
#include <execution>
#include <stop_token>
#include <type_traits>

namespace ex = std::execution;

struct Sch {
  using scheduler_concept = ex::scheduler_tag;
  struct Sender {
    using sender_concept = ex::sender_tag;
    ex::env<> get_env() const noexcept { return {}; }
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>{};
    }
  };
  int id;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(Sch, Sch) = default;
};
static_assert(ex::scheduler<Sch>);

struct DValue {};
struct DError {};
struct DStopped {};

// attributes of a child that completes on a different scheduler and in a different domain per completion tag
struct ChildAttrs {
  Sch query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {1}; }
  Sch query(ex::get_completion_scheduler_t<ex::set_error_t>) const noexcept { return {2}; }
  Sch query(ex::get_completion_scheduler_t<ex::set_stopped_t>) const noexcept { return {3}; }
  DValue query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  DError query(ex::get_completion_domain_t<ex::set_error_t>) const noexcept { return {}; }
  DStopped query(ex::get_completion_domain_t<ex::set_stopped_t>) const noexcept { return {}; }
  std::never_stop_token query(std::get_stop_token_t) const noexcept { return {}; } // a forwarding query, not a completion one
};

template <class... Sigs>
struct Child {
  using sender_concept = ex::sender_tag;
  ChildAttrs get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<Sigs...>{};
  }
};

using AllSigs = Child<ex::set_value_t(int), ex::set_error_t(int), ex::set_stopped_t()>;
using NoErrorSigs = Child<ex::set_value_t(int), ex::set_stopped_t()>;
using NoValueSigs = Child<ex::set_error_t(int), ex::set_stopped_t()>;
using NoStoppedSigs = Child<ex::set_value_t(int), ex::set_error_t(int)>;

inline constexpr auto nothrow_fn = [](int) noexcept { return 1; };
inline constexpr auto throwing_fn = [](int) { return 1; };
inline constexpr auto nothrow_stopped_fn = []() noexcept { return 1; };
inline constexpr auto throwing_stopped_fn = []() { return 1; };

template <class Tag, class Attrs, class... Envs>
concept has_cs = requires(const Attrs& a, const Envs&... e) { ex::get_completion_scheduler<Tag>(a, e...); };
template <class Tag, class Attrs, class... Envs>
concept has_cd = requires(const Attrs& a, const Envs&... e) { ex::get_completion_domain<Tag>(a, e...); };

template <class Tag, class Attrs, class... Envs>
int sched_id(const Attrs& a, const Envs&... e) {
  return ex::get_completion_scheduler<Tag>(a, e...).id;
}

using V = ex::set_value_t;
using E = ex::set_error_t;
using S = ex::set_stopped_t;
using Env0 = ex::env<>;

// ---------------------------------------------------------------------------------------------------------------
// then: values complete where the child completed with a value, stopped where it stopped
template <class Fn, class Sndr>
using then_attrs_t = decltype(ex::get_env(ex::then(std::declval<Sndr>(), std::declval<Fn>())));

using ThenNothrow = then_attrs_t<decltype(nothrow_fn), AllSigs>;
static_assert(has_cs<V, ThenNothrow, Env0>);
static_assert(std::same_as<decltype(ex::get_completion_scheduler<V>(std::declval<ThenNothrow>(), Env0{})), Sch>);
// the signatures of this child do not depend on an environment, so none is needed
static_assert(has_cs<V, ThenNothrow>);

void test_then() {
  auto sndr = ex::then(AllSigs{}, nothrow_fn);
  auto attrs = ex::get_env(sndr);
  assert(sched_id<V>(attrs, Env0{}) == 1); // the invocation happens where the child completed with a value
  assert(sched_id<S>(attrs, Env0{}) == 3); // stopped is forwarded
  // the function is not potentially throwing: the errors are the ones of the child
  assert(sched_id<E>(attrs, Env0{}) == 2);
  static_assert(std::same_as<decltype(ex::get_completion_domain<V>(attrs, Env0{})), DValue>);
  static_assert(std::same_as<decltype(ex::get_completion_domain<E>(attrs, Env0{})), DError>);
  static_assert(std::same_as<decltype(ex::get_completion_domain<S>(attrs, Env0{})), DStopped>);
  // the void tag is the value tag
  static_assert(std::same_as<decltype(ex::get_completion_domain<>(attrs, Env0{})), DValue>);
  // other forwarding queries are forwarded
  static_assert(std::same_as<decltype(std::get_stop_token(attrs)), std::never_stop_token>);
}

void test_then_throwing() {
  // [exec.snd.general]p10 example: with a potentially throwing function the exception is an error completion on the
  // agent of the value completion, so the error completions have two possible schedulers.
  auto attrs = ex::get_env(ex::then(AllSigs{}, throwing_fn));
  assert(sched_id<V>(attrs, Env0{}) == 1);
  assert(sched_id<S>(attrs, Env0{}) == 3);
  static_assert(!has_cs<E, decltype(attrs), Env0>);
  // ... and the completion domain of the errors is the COMMON-DOMAIN of the domains of the places
  static_assert(std::same_as<decltype(ex::get_completion_domain<E>(attrs, Env0{})),
                             ex::indeterminate_domain<DValue, DError>>);
  static_assert(std::same_as<decltype(ex::get_completion_domain<V>(attrs, Env0{})), DValue>);
}

void test_then_without_child_errors() {
  // the child has no error completion: the only errors are the exceptions of the function, on the value agent
  auto attrs = ex::get_env(ex::then(NoErrorSigs{}, throwing_fn));
  assert(sched_id<E>(attrs, Env0{}) == 1);
  static_assert(std::same_as<decltype(ex::get_completion_domain<E>(attrs, Env0{})), DValue>);
  // not potentially throwing and no errors: there are no error completions to ask about
  auto none = ex::get_env(ex::then(NoErrorSigs{}, nothrow_fn));
  static_assert(!has_cs<E, decltype(none), Env0>);
  static_assert(!has_cd<E, decltype(none), Env0>);
  assert(sched_id<V>(none, Env0{}) == 1);
}

void test_then_without_values_or_stopped() {
  // without value completions of the child there are no value completions
  auto no_value = ex::get_env(ex::then(NoValueSigs{}, throwing_fn));
  static_assert(!has_cs<V, decltype(no_value), Env0>);
  static_assert(!has_cd<V, decltype(no_value), Env0>);
  assert(sched_id<E>(no_value, Env0{}) == 2);
  assert(sched_id<S>(no_value, Env0{}) == 3);
  auto no_stopped = ex::get_env(ex::then(NoStoppedSigs{}, nothrow_fn));
  static_assert(!has_cs<S, decltype(no_stopped), Env0>);
  static_assert(has_cs<V, decltype(no_stopped), Env0>);
}

// ---------------------------------------------------------------------------------------------------------------
// upon_error: the error completions of the child become value completions, run where the child completed with an error
void test_upon_error() {
  auto nothrow_attrs = ex::get_env(ex::upon_error(AllSigs{}, nothrow_fn));
  // the stopped completion is forwarded
  assert(sched_id<S>(nothrow_attrs, Env0{}) == 3);
  // no error completion is left: the function does not throw, the child's errors were handled
  static_assert(!has_cs<E, decltype(nothrow_attrs), Env0>);
  static_assert(!has_cd<E, decltype(nothrow_attrs), Env0>);
  // the value completions come from two agents, so there is no single scheduler, and the domain is a COMMON-DOMAIN
  static_assert(!has_cs<V, decltype(nothrow_attrs), Env0>);
  static_assert(std::same_as<decltype(ex::get_completion_domain<V>(nothrow_attrs, Env0{})),
                             ex::indeterminate_domain<DValue, DError>>);

  // a throwing function: its exceptions are errors on the agent of the child's errors
  auto throwing_attrs = ex::get_env(ex::upon_error(AllSigs{}, throwing_fn));
  assert(sched_id<E>(throwing_attrs, Env0{}) == 2);
  static_assert(std::same_as<decltype(ex::get_completion_domain<E>(throwing_attrs, Env0{})), DError>);

  // a child without value completions: all the values come from the function
  auto no_value = ex::get_env(ex::upon_error(NoValueSigs{}, nothrow_fn));
  assert(sched_id<V>(no_value, Env0{}) == 2);
  static_assert(std::same_as<decltype(ex::get_completion_domain<V>(no_value, Env0{})), DError>);
}

// ---------------------------------------------------------------------------------------------------------------
// upon_stopped: the stopped completion of the child becomes a value completion
void test_upon_stopped() {
  auto nothrow_attrs = ex::get_env(ex::upon_stopped(AllSigs{}, nothrow_stopped_fn));
  // no stopped completion is left
  static_assert(!has_cs<S, decltype(nothrow_attrs), Env0>);
  static_assert(!has_cd<S, decltype(nothrow_attrs), Env0>);
  // errors are forwarded
  assert(sched_id<E>(nothrow_attrs, Env0{}) == 2);
  auto throwing_attrs = ex::get_env(ex::upon_stopped(AllSigs{}, throwing_stopped_fn));
  // the exception is an error on the agent of the stopped completion: two agents
  static_assert(!has_cs<E, decltype(throwing_attrs), Env0>);
  static_assert(std::same_as<decltype(ex::get_completion_domain<E>(throwing_attrs, Env0{})),
                             ex::indeterminate_domain<DError, DStopped>>);
  // a child that cannot error: the exceptions are the only errors
  auto no_error = ex::get_env(ex::upon_stopped(NoErrorSigs{}, throwing_stopped_fn));
  assert(sched_id<E>(no_error, Env0{}) == 3);
}

// ---------------------------------------------------------------------------------------------------------------
// composed with a real scheduler: the sender of an inline scheduler completes on the scheduler of the environment it is
// started in, and so does then of it (the invocation of the function happens where the value completion did)
struct OtherSch {
  using scheduler_concept = ex::scheduler_tag;
  int id;
  Sch::Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(OtherSch, OtherSch) = default;
};
struct SchedulerEnv {
  OtherSch query(ex::get_scheduler_t) const noexcept { return {17}; }
  DValue query(ex::get_domain_t) const noexcept { return {}; }
};

void test_inline_scheduler() {
  auto sndr = ex::then(ex::schedule(ex::inline_scheduler{}), nothrow_stopped_fn);
  auto attrs = ex::get_env(sndr);
  using A = decltype(attrs);
  static_assert(std::same_as<decltype(ex::get_completion_scheduler<V>(attrs, SchedulerEnv{})), OtherSch>);
  assert((ex::get_completion_scheduler<V>(attrs, SchedulerEnv{}) == OtherSch{17}));
  static_assert(std::same_as<decltype(ex::get_completion_domain<V>(attrs, SchedulerEnv{})), DValue>);
  // the inline scheduler only completes with a value
  static_assert(!has_cs<E, A, SchedulerEnv>);
  static_assert(!has_cs<S, A, SchedulerEnv>);
  // an environment without a scheduler tells nothing about where an inline operation completes
  static_assert(!has_cs<V, A, Env0>);
  static_assert(!has_cs<V, A>);
}

int main(int, char**) {
  test_inline_scheduler();
  test_then();
  test_then_throwing();
  test_then_without_child_errors();
  test_then_without_values_or_stopped();
  test_upon_error();
  test_upon_stopped();
  return 0;
}
