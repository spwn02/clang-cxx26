//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.get.compl.domain]: get_completion_domain<Tag>(attrs, envs...) is MANDATE-NOTHROW(D()), where D is the type of
// the first well-formed of
//   TRY-QUERY(attrs, get_completion_domain<Tag>, envs...),
//   get_completion_domain<set_value_t>(attrs, envs...) if Tag is void,
//   TRY-QUERY(get_completion_scheduler<Tag>(attrs, envs...), get_completion_domain<set_value_t>, envs...),
//   default_domain if attrs is a scheduler and envs is not empty;
// otherwise the expression is ill-formed. Tag is void or a completion tag.
// forwarding_query(get_completion_domain<Tag>) is true.

#include <cassert>
#include <concepts>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

template <class Tag, class Q, class... Envs>
concept has_completion_domain = requires(const Q& q, const Envs&... envs) {
  ex::get_completion_domain<Tag>(q, envs...);
};

struct Sender {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
};

struct DomainA {};
struct DomainB {};

// 1. the attributes answer the query for the tag
struct TagAttrs {
  DomainA query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  DomainB query(ex::get_completion_domain_t<ex::set_error_t>) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(TagAttrs{})), DomainA>);
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_error_t>(TagAttrs{})), DomainB>);
static_assert(!has_completion_domain<ex::set_stopped_t, TagAttrs>);
// 2. the void tag asks for the one of set_value_t
static_assert(std::same_as<decltype(ex::get_completion_domain<>(TagAttrs{})), DomainA>);
static_assert(std::same_as<decltype(ex::get_completion_domain<void>(TagAttrs{})), DomainA>);

// the query may take the environments, and is preferred to TRY-QUERY without them
struct EnvAttrs {
  DomainA query(ex::get_completion_domain_t<ex::set_value_t>, const ex::env<>&) const noexcept { return {}; }
  DomainB query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(EnvAttrs{}, ex::env<>{})), DomainA>);
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(EnvAttrs{})), DomainB>);

// the domain is default constructed, the query result only determines its type
struct ValueDomain {
  int value = 0;
  ValueDomain() = default;
  constexpr ValueDomain(int v) : value(v) {}
};
struct ValueAttrs {
  constexpr ValueDomain query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {5}; }
};
static_assert(ex::get_completion_domain<ex::set_value_t>(ValueAttrs{}).value == 0);

// 3. the domain of the completion scheduler
struct SchedulerDomain {};
struct DomainScheduler {
  using scheduler_concept = ex::scheduler_tag;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  SchedulerDomain query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
  friend bool operator==(DomainScheduler, DomainScheduler) = default;
};
struct SchedulerAttrs {
  DomainScheduler query(ex::get_completion_scheduler_t<ex::set_error_t>) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_error_t>(SchedulerAttrs{})), SchedulerDomain>);
static_assert(!has_completion_domain<ex::set_value_t, SchedulerAttrs>);

// 4. a scheduler (given an environment) that does not say otherwise is in default_domain
struct PlainScheduler {
  using scheduler_concept = ex::scheduler_tag;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(PlainScheduler, PlainScheduler) = default;
};
static_assert(!has_completion_domain<ex::set_value_t, PlainScheduler>);
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(PlainScheduler{}, ex::env<>{})),
                           ex::default_domain>);
static_assert(std::same_as<decltype(ex::get_completion_domain<>(PlainScheduler{}, ex::env<>{})), ex::default_domain>);
// a scheduler's own query wins
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(DomainScheduler{}, ex::env<>{})),
                           SchedulerDomain>);

// otherwise ill-formed
static_assert(!has_completion_domain<ex::set_value_t, ex::env<>>);
static_assert(!has_completion_domain<ex::set_value_t, ex::env<>, ex::env<>>);
static_assert(!has_completion_domain<ex::set_value_t, int, ex::env<>>);
// only void and the completion tags are allowed
static_assert(!has_completion_domain<int, TagAttrs>);
static_assert(!has_completion_domain<ex::get_domain_t, TagAttrs>);

// MANDATE-NOTHROW(D()): the domain type is nothrow default constructible, the query may throw
struct ThrowingQueryAttrs {
  DomainA query(ex::get_completion_domain_t<ex::set_value_t>) const { return {}; }
};
static_assert(noexcept(ex::get_completion_domain<ex::set_value_t>(ThrowingQueryAttrs{})));

// the template argument defaults to void ([execution.syn])
static_assert(std::same_as<ex::get_completion_domain_t<>, ex::get_completion_domain_t<void>>);
static_assert(std::same_as<std::remove_cvref_t<decltype(ex::get_completion_domain<>)>, ex::get_completion_domain_t<void>>);

// [exec.get.compl.domain]p6: forwarding queries.
static_assert(std::forwarding_query(ex::get_completion_domain<>));
static_assert(std::forwarding_query(ex::get_completion_domain<ex::set_value_t>));
static_assert(std::forwarding_query(ex::get_completion_domain<ex::set_error_t>));
static_assert(std::forwarding_query(ex::get_completion_domain<ex::set_stopped_t>));

int main(int, char**) { return 0; }
