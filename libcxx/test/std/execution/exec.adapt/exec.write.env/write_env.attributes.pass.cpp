//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.write.env]: the child of write_env(sndr, env) is connected through JOIN-ENV(env, FWD-ENV(rcvr_env)). The
// completion operations of write_env are those of the child, so its completion scheduler and domain queries are the
// ones of the child asked with the joined environment, and the other forwarding queries are forwarded.

#include <cassert>
#include <concepts>
#include <execution>
#include <stop_token>

namespace ex = std::execution;

struct Domain {};
struct OtherDomain {};

struct Other {
  using scheduler_concept = ex::scheduler_tag;
  int id;
  decltype(ex::schedule(ex::inline_scheduler{})) schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(Other, Other) = default;
};

// the environment the sender is started in: the scheduler is an inline_scheduler
struct OuterEnv {
  ex::inline_scheduler query(ex::get_scheduler_t) const noexcept { return {}; }
  Domain query(ex::get_domain_t) const noexcept { return {}; }
};

void test_joined_environment() {
  // the inline scheduler completes on the scheduler of the environment its child is connected with: the written one
  auto sndr = ex::write_env(ex::schedule(ex::inline_scheduler{}), ex::prop(ex::get_scheduler, Other{17}));
  auto attrs = ex::get_env(sndr);
  static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_value_t>(attrs, OuterEnv{})), Other>);
  assert((ex::get_completion_scheduler<ex::set_value_t>(attrs, OuterEnv{}) == Other{17}));

  // without the write the scheduler of the outer environment is the one
  auto plain = ex::get_env(ex::write_env(ex::schedule(ex::inline_scheduler{}), ex::prop(std::get_stop_token, std::never_stop_token{})));
  static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_value_t>(plain, OuterEnv{})),
                             ex::inline_scheduler>);

  // the domain is the one of the written environment as well
  auto domain_attrs = ex::get_env(ex::write_env(ex::schedule(ex::inline_scheduler{}), ex::prop(ex::get_domain, OtherDomain{})));
  static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(domain_attrs, OuterEnv{})), OtherDomain>);
  auto outer_domain = ex::get_env(ex::write_env(ex::schedule(ex::inline_scheduler{}), ex::prop(std::get_stop_token, std::never_stop_token{})));
  static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(outer_domain, OuterEnv{})), Domain>);
}

void test_unstoppable() {
  auto attrs = ex::get_env(ex::unstoppable(ex::schedule(ex::inline_scheduler{})));
  static_assert(std::same_as<decltype(ex::get_completion_scheduler<ex::set_value_t>(attrs, OuterEnv{})),
                             ex::inline_scheduler>);
}

template <class Tag, class Attrs, class... Envs>
concept has_cs = requires(const Attrs& a, const Envs&... e) { ex::get_completion_scheduler<Tag>(a, e...); };

// a child that has attributes of its own
struct ChildAttrs {
  std::never_stop_token query(std::get_stop_token_t) const noexcept { return {}; }
  template <class Env>
    requires requires(const Env& env) { ex::get_scheduler(env); }
  decltype(auto) query(ex::get_completion_scheduler_t<ex::set_value_t>, const Env& env) const noexcept {
    return ex::get_scheduler(env);
  }
};
struct Child {
  using sender_concept = ex::sender_tag;
  ChildAttrs get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
};

void test_forwarding() {
  auto attrs = ex::get_env(ex::write_env(Child{}, ex::prop(ex::get_scheduler, Other{1})));
  // other forwarding queries are forwarded to the attributes of the child
  static_assert(std::same_as<decltype(std::get_stop_token(attrs)), std::never_stop_token>);
  // the child asks the joined environment for its scheduler, which is the written one
  assert((ex::get_completion_scheduler<ex::set_value_t>(attrs, OuterEnv{}) == Other{1}));
  assert((ex::get_completion_scheduler<ex::set_value_t>(attrs, ex::env<>{}) == Other{1}));
  // nothing known for the other tags
  static_assert(!has_cs<ex::set_error_t, decltype(attrs), OuterEnv>);
}

int main(int, char**) {
  test_joined_environment();
  test_unstoppable();
  test_forwarding();
  return 0;
}
