//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.inline.scheduler], [exec.snd.expos] inline-attrs: for a subexpression sch of type inline_scheduler,
// sch.query(q, args...) is inline-attrs<set_value_t>().query(q, args...), and
//   inline-attrs<Tag>().query(get_completion_scheduler<Tag>, env) is get_scheduler(env),
//   inline-attrs<Tag>().query(get_completion_domain<Tag>, env) is get_domain(env).
// The sender returned by schedule(sch) has the same attributes.

#include <cassert>
#include <concepts>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

struct Domain {};

// an environment that has a scheduler, and a domain
struct Env {
  ex::inline_scheduler query(ex::get_scheduler_t) const noexcept { return {}; }
  Domain query(ex::get_domain_t) const noexcept { return {}; }
};

template <class Tag, class Q, class... Envs>
concept has_cs = requires(const Q& q, const Envs&... e) { ex::get_completion_scheduler<Tag>(q, e...); };
template <class Tag, class Q, class... Envs>
concept has_cd = requires(const Q& q, const Envs&... e) { ex::get_completion_domain<Tag>(q, e...); };

using Sndr = decltype(ex::schedule(ex::inline_scheduler{}));
using V = ex::set_value_t;

// the scheduler's query
static_assert(std::same_as<decltype(ex::inline_scheduler{}.query(ex::get_completion_scheduler<V>, Env{})),
                           ex::inline_scheduler>);
static_assert(std::same_as<decltype(ex::inline_scheduler{}.query(ex::get_completion_domain<V>, Env{})), Domain>);
// the attributes of the sender
static_assert(std::same_as<decltype(ex::get_env(std::declval<Sndr>()).query(ex::get_completion_scheduler<V>, Env{})),
                           ex::inline_scheduler>);
static_assert(std::same_as<decltype(ex::get_env(std::declval<Sndr>()).query(ex::get_completion_domain<V>, Env{})),
                           Domain>);

// through the query objects: the scheduler of the environment is the completion scheduler, its domain the domain
static_assert(std::same_as<decltype(ex::get_completion_scheduler<V>(ex::inline_scheduler{}, Env{})),
                           ex::inline_scheduler>);
static_assert(std::same_as<decltype(ex::get_completion_domain<V>(ex::inline_scheduler{}, Env{})), Domain>);
static_assert(std::same_as<decltype(ex::get_completion_scheduler<V>(ex::get_env(ex::schedule(ex::inline_scheduler{})), Env{})),
                           ex::inline_scheduler>);
static_assert(std::same_as<decltype(ex::get_completion_domain<V>(ex::get_env(ex::schedule(ex::inline_scheduler{})), Env{})),
                           Domain>);

// the domain of an inline operation also is the one of the environment for the other completion tags
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_error_t>(ex::inline_scheduler{}, Env{})), Domain>);

// an environment with a scheduler of another type: the completion scheduler of an inline operation is that scheduler
struct Other {
  using scheduler_concept = ex::scheduler_tag;
  int id;
  Sndr schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(Other, Other) = default;
};
struct OtherEnv {
  Other query(ex::get_scheduler_t) const noexcept { return {17}; }
};
static_assert(std::same_as<decltype(ex::get_completion_scheduler<V>(ex::get_env(std::declval<Sndr>()), OtherEnv{})), Other>);

// without a scheduler in the environment the attributes of the sender know nothing, as inline-attrs says
static_assert(!has_cs<V, decltype(ex::get_env(std::declval<Sndr>())), ex::env<>>);
static_assert(!has_cs<V, decltype(ex::get_env(std::declval<Sndr>()))>);
// ... and the domain of such an environment is default_domain
static_assert(std::same_as<decltype(ex::get_completion_domain<V>(ex::get_env(std::declval<Sndr>()), ex::env<>{})),
                           ex::default_domain>);

int main(int, char**) {
  assert((ex::get_completion_scheduler<V>(ex::get_env(ex::schedule(ex::inline_scheduler{})), OtherEnv{}) == Other{17}));
  return 0;
}
