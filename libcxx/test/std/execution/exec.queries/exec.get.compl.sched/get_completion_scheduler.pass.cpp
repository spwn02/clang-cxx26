//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.get.compl.sched]: get_completion_scheduler<Tag>(q, envs...) is
//   MANDATE-NOTHROW(RECURSE-QUERY(TRY-QUERY(q, get_completion_scheduler<Tag>, envs...), envs...)) if that is
//   well-formed, otherwise auto(q) if q is a scheduler and envs is not empty, otherwise ill-formed;
//   RECURSE-QUERY(sch1, envs...) is sch1 if TRY-QUERY(sch1, get_completion_scheduler<set_value_t>, envs...) is
//   ill-formed or yields a scheduler of the same type that compares equal to sch1, and otherwise
//   RECURSE-QUERY(sch2, envs...).
// forwarding_query(get_completion_scheduler<Tag>) is true for each completion tag.

#include <cassert>
#include <concepts>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

struct Sender {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
};

// A scheduler whose completion scheduler (for set_value_t) is the next element of a chain, up to 2.
struct Chain {
  using scheduler_concept = ex::scheduler_tag;
  int id = 0;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  constexpr Chain query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {id < 2 ? id + 1 : id}; }
  friend bool operator==(Chain, Chain) = default;
};
static_assert(ex::scheduler<Chain>);

struct ChainAttrs {
  constexpr Chain query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {0}; }
};

// The recursion goes on until two consecutive schedulers of the same type compare equal.
static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_value_t>(ChainAttrs{})), Chain>);

// A scheduler that is its own completion scheduler.
struct Fixed {
  using scheduler_concept = ex::scheduler_tag;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  constexpr Fixed query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
  friend bool operator==(Fixed, Fixed) = default;
};
struct FixedAttrs {
  constexpr Fixed query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
};

// A completion scheduler of another type: the recursion continues with it.
struct Other {
  using scheduler_concept = ex::scheduler_tag;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(Other, Other) = default;
};
struct Hop {
  using scheduler_concept = ex::scheduler_tag;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  Other query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
  friend bool operator==(Hop, Hop) = default;
};
struct HopAttrs {
  Hop query(ex::get_completion_scheduler_t<ex::set_error_t>) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_error_t>(HopAttrs{})), Other>);

// Per tag.
struct PerTagAttrs {
  Fixed query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
  Other query(ex::get_completion_scheduler_t<ex::set_stopped_t>) const noexcept { return {}; }
};
static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_value_t>(PerTagAttrs{})), Fixed>);
static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_stopped_t>(PerTagAttrs{})), Other>);

template <class Tag, class Q, class... Envs>
concept has_completion_scheduler = requires(const Q& q, const Envs&... envs) {
  ex::get_completion_scheduler<Tag>(q, envs...);
};
static_assert(!has_completion_scheduler<ex::set_error_t, PerTagAttrs>);
// a scheduler is its own completion scheduler when an environment is given, and only then
static_assert(!has_completion_scheduler<ex::set_value_t, Other>);
static_assert(has_completion_scheduler<ex::set_value_t, Other, ex::env<>>);
static_assert(has_completion_scheduler<ex::set_error_t, Other, ex::env<>>);
static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_stopped_t>(Other{}, ex::env<>{})), Other>);
// something that is neither
static_assert(!has_completion_scheduler<ex::set_value_t, int>);
static_assert(!has_completion_scheduler<ex::set_value_t, int, ex::env<>>);
static_assert(!has_completion_scheduler<ex::set_value_t, ex::env<>>);
// only the three completion tags are allowed
static_assert(!has_completion_scheduler<int, Other, ex::env<>>);
static_assert(!has_completion_scheduler<void, Other, ex::env<>>);

// [exec.get.compl.sched]p10: forwarding queries.
static_assert(std::forwarding_query(ex::get_completion_scheduler<ex::set_value_t>));
static_assert(std::forwarding_query(ex::get_completion_scheduler<ex::set_error_t>));
static_assert(std::forwarding_query(ex::get_completion_scheduler<ex::set_stopped_t>));

// ... so a forwarding environment forwards them, a non-forwarding one does not.
struct ForwardedAttrs {
  Fixed query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {}; }
};
static_assert(has_completion_scheduler<ex::set_value_t, ForwardedAttrs>);

// The recursion is by value.
constexpr bool test_values() {
  return ex::get_completion_scheduler<ex::set_value_t>(ChainAttrs{}).id == 2 &&
         ex::get_completion_scheduler<ex::set_value_t>(FixedAttrs{}) == Fixed{};
}
static_assert(test_values());

int main(int, char**) {
  assert(test_values());
  assert(ex::get_completion_scheduler<ex::set_value_t>(ChainAttrs{}).id == 2);
  return 0;
}
