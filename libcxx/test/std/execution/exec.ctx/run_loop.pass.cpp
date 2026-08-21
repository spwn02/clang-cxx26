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

// class run_loop;

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

using namespace std::execution;

static_assert(scheduler<decltype(std::declval<run_loop&>().get_scheduler())>);

int main(int, char**) {
  {
    // Basic scheduling: schedule() on the loop's scheduler, connect, start, then run() and
    // observe the work item actually executed.
    run_loop loop;
    bool ran = false;

    struct rcvr {
      using receiver_concept = receiver_tag;
      bool* ran;
      void set_value() && noexcept {
        *ran = true;
      }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return env<>{}; }
    };

    auto sndr = schedule(loop.get_scheduler());
    auto op   = connect(std::move(sndr), rcvr{&ran});
    start(op);
    assert(!ran);
    loop.finish();
    loop.run();
    assert(ran);
  }
  {
    // get_completion_scheduler round-trips through schedule()'s attributes back to the
    // scheduler it came from ([exec.run.loop.types]p5/p8.2).
    run_loop loop;
    auto sch  = loop.get_scheduler();
    auto sndr = schedule(sch);
    assert((get_completion_scheduler<set_value_t>(get_env(sndr)) == sch));
    assert((get_completion_scheduler<set_value_t>(get_env(schedule(sch))) == sch));
    loop.finish();
    loop.run();
  }
  return 0;
}
