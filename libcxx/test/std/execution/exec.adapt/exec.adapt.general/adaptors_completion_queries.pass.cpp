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

// [exec.snd.general]: the completion scheduler and domain of an adaptor are those of the places its completions happen
// in. Adaptors that intercept some completion of the child (into_variant, stopped_as_error, stopped_as_optional, bulk)
// answer from the completion queries of the child for the completions that are forwarded or produced where the child
// completed.

#include <cassert>
#include <concepts>
#include <execution>
#include <stop_token>
#include <utility>

namespace ex = std::execution;

template <class Tag, class Sndr, class... Envs>
concept has_cs = requires(const Sndr& s, const Envs&... e) { ex::get_completion_scheduler<Tag>(ex::get_env(s), e...); };
template <class Tag, class Sndr, class... Envs>
concept has_cd = requires(const Sndr& s, const Envs&... e) { ex::get_completion_domain<Tag>(ex::get_env(s), e...); };

using env_plain     = ex::env<>;
using env_stoppable = decltype(ex::env(ex::prop(std::get_stop_token, std::inplace_stop_token{})));

int main(int, char**) {
  ex::run_loop loop;
  auto rl    = loop.get_scheduler();
  auto child = ex::schedule(rl); // completes with set_value (and with set_stopped for a stoppable token), on rl
  env_plain e0;
  env_stoppable e1;

  { // into_variant: the value completion is a value completion, the others are forwarded
    auto s = ex::into_variant(child);
    static_assert(has_cs<ex::set_value_t, decltype(s), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), e0) == rl);
    static_assert(has_cs<ex::set_stopped_t, decltype(s), env_stoppable>);
    assert(ex::get_completion_scheduler<ex::set_stopped_t>(ex::get_env(s), e1) == rl);
    static_assert(has_cd<ex::set_value_t, decltype(s), env_plain>);
  }
  { // stopped_as_error: the stopped completion becomes an error completion in the same place; no stopped completion
    auto s = ex::stopped_as_error(child, 5);
    static_assert(has_cs<ex::set_value_t, decltype(s), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), e0) == rl);
    static_assert(has_cs<ex::set_error_t, decltype(s), env_stoppable>);
    assert(ex::get_completion_scheduler<ex::set_error_t>(ex::get_env(s), e1) == rl);
    static_assert(!has_cs<ex::set_stopped_t, decltype(s), env_stoppable>);
    static_assert(!has_cs<ex::set_error_t, decltype(s), env_plain>); // the child cannot be stopped, so no error
  }
  { // stopped_as_optional: the stopped completion becomes a value completion (an empty optional)
    auto s = ex::stopped_as_optional(ex::then(child, []() noexcept { return 1; }));
    // without a stopped completion of the child the value completions all come from the child's value completion
    static_assert(has_cs<ex::set_value_t, decltype(s), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), e0) == rl);
    // with one they happen in two places (the child's value and stopped completions): no single scheduler is known,
    // but the completion domain is the common domain of the two
    static_assert(!has_cs<ex::set_value_t, decltype(s), env_stoppable>);
    static_assert(has_cd<ex::set_value_t, decltype(s), env_stoppable>);
    static_assert(!has_cs<ex::set_stopped_t, decltype(s), env_stoppable>);
  }
  { // bulk: the function runs where the child completed with set_value
    auto s = ex::bulk(child, ex::seq, 3, []([[maybe_unused]] int i) noexcept {});
    static_assert(has_cs<ex::set_value_t, decltype(s), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), e0) == rl);
    auto c = ex::bulk_chunked(child, ex::seq, 3, []([[maybe_unused]] int b, [[maybe_unused]] int e) noexcept {});
    static_assert(has_cs<ex::set_value_t, decltype(c), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(c), e0) == rl);
    // a function that can throw adds an error completion where the value completion is
    auto t = ex::bulk(child, ex::seq, 3, []([[maybe_unused]] int i) {});
    static_assert(has_cs<ex::set_error_t, decltype(t), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_error_t>(ex::get_env(t), e0) == rl);
    static_assert(!has_cs<ex::set_error_t, decltype(s), env_plain>);
  }
  { // continues_on: the value and stopped completions happen on the scheduler it continues on; errors are not answered
    ex::run_loop other;
    auto os = other.get_scheduler();
    auto s  = ex::continues_on(child, os);
    static_assert(has_cs<ex::set_value_t, decltype(s), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), e0) == os);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), e0) != rl);
    static_assert(!has_cs<ex::set_stopped_t, decltype(s), env_plain>); // nothing can be stopped
    static_assert(has_cs<ex::set_stopped_t, decltype(s), env_stoppable>);
    assert(ex::get_completion_scheduler<ex::set_stopped_t>(ex::get_env(s), e1) == os);
    static_assert(!has_cs<ex::set_error_t, decltype(s), env_stoppable>);
    static_assert(has_cd<ex::set_value_t, decltype(s), env_plain>);
    static_assert(has_cd<ex::set_stopped_t, decltype(s), env_stoppable>);
    other.finish();
  }
  { // starts_on: the child completes where it is started when it asks for the scheduler it was started on
    ex::run_loop other;
    auto os = other.get_scheduler();
    auto s  = ex::starts_on(os, ex::read_env(ex::get_start_scheduler) | ex::let_value([](auto sch) noexcept {
                                    return ex::schedule(sch);
                                  }));
    static_assert(!has_cs<ex::set_value_t, decltype(s), env_plain>); // the continuation is only known at run time
    auto t = ex::starts_on(os, ex::schedule(rl));
    static_assert(has_cs<ex::set_value_t, decltype(t), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(t), e0) == rl);
    static_assert(has_cd<ex::set_value_t, decltype(t), env_plain>);
    static_assert(!has_cs<ex::set_error_t, decltype(t), env_plain>); // the scheduling failure: unspecified agent
    // the stopped completions come from the child and from the scheduling operation: no scheduler, a common domain
    static_assert(!has_cs<ex::set_stopped_t, decltype(t), env_stoppable>);
    static_assert(has_cd<ex::set_stopped_t, decltype(t), env_stoppable>);
    static_assert(!has_cd<ex::set_stopped_t, decltype(t), env_plain>); // nothing can be stopped
    other.finish();
  }
  { // on: both forms complete on a scheduler determined by the environment or by the child
    ex::run_loop other;
    auto os = other.get_scheduler();
    auto s1 = ex::on(os, ex::just(1)); // completes on the scheduler it was started on
    static_assert(!has_cs<ex::set_value_t, decltype(s1), env_plain>); // env<> has no start scheduler
    auto env_rl = ex::env(ex::prop(ex::get_start_scheduler, rl));
    static_assert(has_cs<ex::set_value_t, decltype(s1), decltype(env_rl)>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s1), env_rl) == rl);
    static_assert(has_cd<ex::set_value_t, decltype(s1), decltype(env_rl)>);
    auto s2 = ex::on(child, os, ex::then([]() noexcept { return 1; })); // completes where child completes
    static_assert(has_cs<ex::set_value_t, decltype(s2), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s2), e0) == rl);
    // the stopped completions happen on the same final scheduler (for an environment where there can be some)
    auto env_rl_stoppable = ex::env(ex::prop(ex::get_start_scheduler, rl), ex::prop(std::get_stop_token, std::inplace_stop_token{}));
    static_assert(has_cs<ex::set_stopped_t, decltype(s1), decltype(env_rl_stoppable)>);
    assert(ex::get_completion_scheduler<ex::set_stopped_t>(ex::get_env(s1), env_rl_stoppable) == rl);
    static_assert(!has_cs<ex::set_stopped_t, decltype(s1), decltype(env_rl)>); // nothing can be stopped
    static_assert(has_cs<ex::set_stopped_t, decltype(s2), env_stoppable>);
    assert(ex::get_completion_scheduler<ex::set_stopped_t>(ex::get_env(s2), e1) == rl);
    static_assert(!has_cs<ex::set_error_t, decltype(s1), decltype(env_rl_stoppable)>);
    other.finish();
  }
  { // affine: every completion happens on an agent of the start scheduler of the environment
    ex::run_loop other;
    auto os = other.get_scheduler();
    auto s  = ex::affine(child);
    static_assert(!has_cs<ex::set_value_t, decltype(s), env_plain>); // no start scheduler
    auto env_os = ex::env(ex::prop(ex::get_start_scheduler, os));
    static_assert(has_cs<ex::set_value_t, decltype(s), decltype(env_os)>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), env_os) == os);
    static_assert(has_cd<ex::set_value_t, decltype(s), decltype(env_os)>);
    static_assert(!has_cs<ex::set_error_t, decltype(s), decltype(env_os)>); // nothing fails
    auto env_os_stoppable = ex::env(ex::prop(ex::get_start_scheduler, os), ex::prop(std::get_stop_token, std::inplace_stop_token{}));
    static_assert(has_cs<ex::set_stopped_t, decltype(s), decltype(env_os_stoppable)>);
    assert(ex::get_completion_scheduler<ex::set_stopped_t>(ex::get_env(s), env_os_stoppable) == os);
    other.finish();
  }
  { // let_value: no completion scheduler (the continuation is only known at run time), but a completion domain when the
    // places the completions happen in know theirs
    auto s = ex::let_value(ex::just(1), [rl](int) noexcept { return ex::schedule(rl); });
    static_assert(!has_cs<ex::set_value_t, decltype(s), env_plain>);
    static_assert(has_cd<ex::set_value_t, decltype(s), env_plain>);
    static_assert(!has_cd<ex::set_error_t, decltype(s), env_plain>); // nothing can fail
    // the stopped completions of the child are forwarded, those of the continuation happen on rl
    auto t = ex::let_value(child, [rl](auto&&...) noexcept { return ex::schedule(rl); });
    static_assert(has_cd<ex::set_stopped_t, decltype(t), env_stoppable>);
    static_assert(has_cd<ex::set_value_t, decltype(t), env_stoppable>);
    // a continuation sender that does not tell its domain makes it unknown: the draft does not specify the attributes of
    // just (LWG candidate, #263), so there is no domain to report for those completions
    auto u = ex::let_value(ex::just(1), [](int x) noexcept { return ex::just(x); });
    static_assert(!has_cd<ex::set_value_t, decltype(u), env_plain>);
    static_assert(!has_cs<ex::set_value_t, decltype(u), env_plain>);
  }
  { // schedule_from completes where its child completes
    auto s = ex::schedule_from(child);
    static_assert(has_cs<ex::set_value_t, decltype(s), env_plain>);
    assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), e0) == rl);
    static_assert(has_cs<ex::set_stopped_t, decltype(s), env_stoppable>);
    static_assert(!has_cs<ex::set_error_t, decltype(s), env_stoppable>);
  }
  { // when_all: the common domain of the children (which child completes last is only known at run time)
    auto s = ex::when_all(child, child);
    static_assert(!has_cs<ex::set_value_t, decltype(s), env_plain>);
    static_assert(has_cd<ex::set_value_t, decltype(s), env_plain>);
    static_assert(has_cd<ex::set_stopped_t, decltype(s), env_stoppable>);
    static_assert(!has_cd<ex::set_error_t, decltype(s), env_plain>);
    auto t = ex::when_all_with_variant(child);
    static_assert(has_cd<ex::set_value_t, decltype(t), env_plain>);
    // a child that does not tell its domain makes it unknown (same draft hole as for let_value above)
    auto u = ex::when_all(child, ex::just(1));
    static_assert(!has_cd<ex::set_value_t, decltype(u), env_plain>);
  }
  loop.finish();
  return 0;
}
