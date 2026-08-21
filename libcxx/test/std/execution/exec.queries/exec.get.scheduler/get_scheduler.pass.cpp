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

// namespace execution {
//   struct get_scheduler_t { ... };
//   inline constexpr get_scheduler_t get_scheduler{};
//   template<class Cpo> struct get_completion_scheduler_t { ... };
//   template<class Cpo> constexpr get_completion_scheduler_t<Cpo> get_completion_scheduler{};
// }

#include <cassert>
#include <execution>

using namespace std::execution;

// get_scheduler(env) is a *current-scheduler* query, answered by an env that explicitly
// carries one via its own query(get_scheduler_t) member (e.g. this_thread's sync-wait-env) --
// unlike get_completion_scheduler, it is not automatically derivable from a sender's own
// attributes just because the sender happens to have a completion scheduler.
struct env_with_sch {
  run_loop* loop;
  auto query(get_scheduler_t) const noexcept { return loop->get_scheduler(); }
};

template <class T>
concept has_get_scheduler = requires(const T& t) { get_scheduler(t); };

int main(int, char**) {
  // get_scheduler is a forwarding query.
  static_assert(std::forwarding_query(get_scheduler));

  run_loop loop;
  env_with_sch cur_env{&loop};
  static_assert(has_get_scheduler<env_with_sch>);
  assert((get_scheduler(cur_env) == loop.get_scheduler()));

  // A plain env<> doesn't answer it.
  static_assert(!has_get_scheduler<env<>>);

  // get_completion_scheduler<set_value_t>/<set_stopped_t> are answered directly by
  // run-loop-sender's own attributes ([exec.run.loop.types]p5) -- this is the mechanism
  // get_scheduler's own definition is built on, exercised here on schedule()'s sender.
  auto sch    = loop.get_scheduler();
  auto sndr   = schedule(sch);
  auto sattrs = get_env(sndr);
  assert((get_completion_scheduler<set_value_t>(sattrs) == sch));
  assert((get_completion_scheduler<set_stopped_t>(sattrs) == sch));

  loop.finish();
  loop.run();
  return 0;
}
