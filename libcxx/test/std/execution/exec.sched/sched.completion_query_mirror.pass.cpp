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

// [exec.sched]p6: for a scheduler sch, get_completion_scheduler<T>(sch, envs...) and
// get_completion_scheduler<T>(get_env(schedule(sch)), envs...) are both ill-formed or both well-formed with the same
// type and value, and likewise get_completion_domain<T>. This checks the library schedulers in every case where the
// queries of the scheduler and of its schedule sender can agree. They cannot for:
//  - the error tag: the draft's second bullet makes the scheduler query always well-formed for a non-empty envs, while
//    the schedule senders never complete with set_error ([exec.task.scheduler] and [exec.run.loop.types] say so; the
//    parallel_scheduler sender's signatures are not in the draft: P2079R10's exposition-only sender has
//    set_error(exception_ptr), but then task_scheduler(parallel_scheduler) would violate infallible-scheduler);
//  - set_stopped for an environment whose stop token is unstoppable (the sender has no such completion, so answering
//    the query would make the program ill-formed, [exec.get.compl.sched]p7);
//  - inline_scheduler with an environment without a scheduler (inline-attrs answers get_scheduler(env)).
// These are LWG candidates, see #263.

#include <cassert>
#include <concepts>
#include <execution>
#include <stop_token>
#include <type_traits>

namespace ex = std::execution;

template <class T, class Sch, class... Envs>
concept cs_sch = requires(Sch s, const Envs&... e) { ex::get_completion_scheduler<T>(s, e...); };
template <class T, class Sch, class... Envs>
concept cs_snd = requires(Sch s, const Envs&... e) {
  ex::get_completion_scheduler<T>(ex::get_env(ex::schedule(s)), e...);
};
template <class T, class Sch, class... Envs>
concept cd_sch = requires(Sch s, const Envs&... e) { ex::get_completion_domain<T>(s, e...); };
template <class T, class Sch, class... Envs>
concept cd_snd = requires(Sch s, const Envs&... e) {
  ex::get_completion_domain<T>(ex::get_env(ex::schedule(s)), e...);
};

template <class T, class Sch, class... Envs>
constexpr bool mirrored_cs() {
  if constexpr (cs_sch<T, Sch, Envs...> && cs_snd<T, Sch, Envs...>) {
    using A = decltype(ex::get_completion_scheduler<T>(std::declval<Sch&>(), std::declval<const Envs&>()...));
    using B = decltype(ex::get_completion_scheduler<T>(ex::get_env(ex::schedule(std::declval<Sch&>())),
                                                       std::declval<const Envs&>()...));
    return std::same_as<A, B>;
  } else {
    return !cs_sch<T, Sch, Envs...> && !cs_snd<T, Sch, Envs...>;
  }
}
template <class T, class Sch, class... Envs>
constexpr bool mirrored_cd() {
  if constexpr (cd_sch<T, Sch, Envs...> && cd_snd<T, Sch, Envs...>) {
    using A = decltype(ex::get_completion_domain<T>(std::declval<Sch&>(), std::declval<const Envs&>()...));
    using B = decltype(ex::get_completion_domain<T>(ex::get_env(ex::schedule(std::declval<Sch&>())),
                                                    std::declval<const Envs&>()...));
    return std::same_as<A, B>;
  } else {
    return !cd_sch<T, Sch, Envs...> && !cd_snd<T, Sch, Envs...>;
  }
}

using env_with_sched = decltype(ex::env(ex::prop(ex::get_scheduler, ex::inline_scheduler{})));
using env_stoppable  = decltype(ex::env(ex::prop(std::get_stop_token, std::inplace_stop_token{})));
using env_stoppable_with_sched =
    decltype(ex::env(ex::prop(std::get_stop_token, std::inplace_stop_token{}), ex::prop(ex::get_scheduler, ex::inline_scheduler{})));
using loop_sched = decltype(std::declval<ex::run_loop&>().get_scheduler());

// without an environment
static_assert(mirrored_cs<ex::set_value_t, ex::parallel_scheduler>() && cs_sch<ex::set_value_t, ex::parallel_scheduler>);
static_assert(mirrored_cs<ex::set_value_t, ex::task_scheduler>() && cs_sch<ex::set_value_t, ex::task_scheduler>);
static_assert(mirrored_cs<ex::set_value_t, loop_sched>() && cs_sch<ex::set_value_t, loop_sched>);
static_assert(mirrored_cs<ex::set_stopped_t, loop_sched>() && cs_sch<ex::set_stopped_t, loop_sched>);
static_assert(mirrored_cd<ex::set_value_t, ex::parallel_scheduler>() && cd_sch<ex::set_value_t, ex::parallel_scheduler>);
static_assert(mirrored_cd<ex::set_value_t, loop_sched>() && cd_sch<ex::set_value_t, loop_sched>);
static_assert(mirrored_cd<ex::set_stopped_t, loop_sched>());
static_assert(mirrored_cd<ex::set_value_t, ex::task_scheduler>());
static_assert(mirrored_cs<ex::set_value_t, ex::inline_scheduler>() && !cs_sch<ex::set_value_t, ex::inline_scheduler>);

// with an environment: the value completion
static_assert(mirrored_cs<ex::set_value_t, ex::parallel_scheduler, ex::env<>>());
static_assert(mirrored_cs<ex::set_value_t, ex::task_scheduler, ex::env<>>());
static_assert(mirrored_cs<ex::set_value_t, loop_sched, ex::env<>>());
static_assert(mirrored_cs<ex::set_value_t, ex::inline_scheduler, env_with_sched>());
static_assert(mirrored_cs<ex::set_stopped_t, loop_sched, ex::env<>>());
static_assert(mirrored_cs<ex::set_stopped_t, loop_sched, env_with_sched>());
static_assert(mirrored_cd<ex::set_value_t, ex::parallel_scheduler, ex::env<>>());
static_assert(mirrored_cd<ex::set_value_t, ex::parallel_scheduler, env_with_sched>());
static_assert(mirrored_cd<ex::set_value_t, loop_sched, ex::env<>>());
static_assert(mirrored_cd<ex::set_value_t, loop_sched, env_with_sched>());
static_assert(mirrored_cd<ex::set_stopped_t, loop_sched, ex::env<>>());
static_assert(mirrored_cd<ex::set_value_t, ex::task_scheduler, ex::env<>>());
static_assert(mirrored_cd<ex::set_value_t, ex::inline_scheduler, env_with_sched>());

// with an environment with a stoppable token: the sender can complete with set_stopped
static_assert(mirrored_cs<ex::set_stopped_t, ex::parallel_scheduler, env_stoppable>());
static_assert(mirrored_cs<ex::set_stopped_t, ex::parallel_scheduler, env_stoppable_with_sched>());
static_assert(mirrored_cs<ex::set_stopped_t, ex::task_scheduler, env_stoppable>());
static_assert(mirrored_cs<ex::set_stopped_t, ex::task_scheduler, env_stoppable_with_sched>());
static_assert(mirrored_cs<ex::set_stopped_t, loop_sched, env_stoppable>());
static_assert(mirrored_cd<ex::set_stopped_t, ex::parallel_scheduler, env_stoppable>());
static_assert(mirrored_cd<ex::set_stopped_t, ex::task_scheduler, env_stoppable>());
static_assert(mirrored_cd<ex::set_stopped_t, loop_sched, env_stoppable>());
static_assert(mirrored_cs<ex::set_value_t, ex::parallel_scheduler, env_stoppable>());
static_assert(mirrored_cs<ex::set_value_t, ex::task_scheduler, env_stoppable>());

// [exec.sched]p5 and the values: the scheduler's answer is the scheduler itself
int main(int, char**) {
  ex::run_loop loop;
  auto rl = loop.get_scheduler();
  assert(ex::get_completion_scheduler<ex::set_value_t>(rl) == rl);
  assert(ex::get_completion_scheduler<ex::set_stopped_t>(rl) == rl);
  assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(ex::schedule(rl))) == rl);
  ex::parallel_scheduler ps = ex::get_parallel_scheduler();
  assert(ex::get_completion_scheduler<ex::set_value_t>(ps) == ps);
  assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(ex::schedule(ps))) == ps);
  ex::task_scheduler ts{ex::inline_scheduler{}};
  assert(ex::get_completion_scheduler<ex::set_value_t>(ts) == ts);
  assert(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(ex::schedule(ts))) == ts);
  return 0;
}
