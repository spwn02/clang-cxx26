//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.get.domain]: get_domain(env) is MANDATE-NOTHROW(D()), where D is the type of the first well-formed of
//   auto(AS-CONST(env).query(get_domain)),
//   get_completion_domain<set_value_t>(get_scheduler(env), HIDE-SCHED(env)),
//   default_domain() (env is evaluated).

#include <cassert>
#include <concepts>
#include <execution>
#include <stop_token>
#include <type_traits>

namespace ex = std::execution;

// The domain is default constructed, the query result only determines its type.
struct Domain {
  int value = 0;
  constexpr Domain() = default;
  constexpr Domain(int v) : value(v) {}
};
struct QueryEnv {
  constexpr Domain query(ex::get_domain_t) const noexcept { return Domain{7}; }
};
static_assert(std::same_as<decltype(ex::get_domain(QueryEnv{})), Domain>);
static_assert(ex::get_domain(QueryEnv{}).value == 0);
static_assert(noexcept(ex::get_domain(QueryEnv{})));

// Only D() has to be noexcept, the query itself may throw.
struct ThrowingQueryEnv {
  Domain query(ex::get_domain_t) const { return {}; }
};
static_assert(std::same_as<decltype(ex::get_domain(ThrowingQueryEnv{})), Domain>);

// No query, no scheduler: default_domain.
static_assert(std::same_as<decltype(ex::get_domain(ex::env<>{})), ex::default_domain>);

// No get_domain query, but a scheduler: the completion domain of the scheduler for set_value_t.
struct SchedDomain {};
struct SchedSender {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
};
struct DomainScheduler {
  using scheduler_concept = ex::scheduler_tag;
  SchedSender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  SchedDomain query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  friend bool operator==(DomainScheduler, DomainScheduler) = default;
};
static_assert(ex::scheduler<DomainScheduler>);

struct SchedulerEnv {
  DomainScheduler query(ex::get_scheduler_t) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_domain(SchedulerEnv{})), SchedDomain>);

// A scheduler without a domain query: the domain of a scheduler given an environment is default_domain.
struct PlainScheduler {
  using scheduler_concept = ex::scheduler_tag;
  SchedSender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(PlainScheduler, PlainScheduler) = default;
};
struct PlainSchedulerEnv {
  PlainScheduler query(ex::get_scheduler_t) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_domain(PlainSchedulerEnv{})), ex::default_domain>);

// The get_domain query takes precedence over the scheduler.
struct BothEnv {
  Domain query(ex::get_domain_t) const noexcept { return {}; }
  DomainScheduler query(ex::get_scheduler_t) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_domain(BothEnv{})), Domain>);

// The scheduler is not asked for its domain through the environment it came from: HIDE-SCHED makes the get_scheduler
// and get_domain queries of that environment ill-formed for the completion domain query (otherwise the answer would
// loop back into get_domain).
struct HidingScheduler {
  using scheduler_concept = ex::scheduler_tag;
  SchedSender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  template <class Env>
    requires(!requires(const Env& env) { env.query(ex::get_scheduler); }) &&
            (!requires(const Env& env) { env.query(ex::get_domain); }) && requires(const Env& env) { env.query(std::get_stop_token); }
  Domain query(ex::get_completion_domain_t<ex::set_value_t>, const Env&) const noexcept {
    return {};
  }
  friend bool operator==(HidingScheduler, HidingScheduler) = default;
};
struct HidingEnv {
  HidingScheduler query(ex::get_scheduler_t) const noexcept { return {}; }
  std::never_stop_token query(std::get_stop_token_t) const noexcept { return {}; }
};
// other queries still reach the scheduler's query, get_scheduler and get_domain do not
static_assert(std::same_as<decltype(ex::get_domain(HidingEnv{})), Domain>);

static_assert(std::forwarding_query(ex::get_domain));

int main(int, char**) {
  assert(ex::get_domain(QueryEnv{}).value == 0);
  return 0;
}
