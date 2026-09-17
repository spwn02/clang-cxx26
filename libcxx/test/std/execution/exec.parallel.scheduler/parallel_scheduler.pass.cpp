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
//   class parallel_scheduler { ... };
//   parallel_scheduler get_parallel_scheduler() noexcept;
// }
//
// Pass 1 only (see docs/design/parallel_scheduler_p2079.md): the pool, parallel_scheduler,
// get_parallel_scheduler(), and schedule(). bulk() customization and the
// system_context_replaceability ABI are follow-up work, not tested here.

#include <atomic>
#include <cassert>
#include <execution>
#include <thread>
#include <type_traits>
#include <utility>

using namespace std::execution;

static_assert(scheduler<parallel_scheduler>);
static_assert(!std::is_default_constructible_v<parallel_scheduler>);
static_assert(std::is_copy_constructible_v<parallel_scheduler>);

int main(int, char**) {
  // Handle-identity semantics: every call to get_parallel_scheduler() returns a handle to the
  // same process-wide pool.
  {
    auto a = get_parallel_scheduler();
    auto b = get_parallel_scheduler();
    assert(a == b);
  }

  // [exec.parallel.scheduler]p2: forward progress guarantee is parallel.
  {
    assert(get_parallel_scheduler().query(get_forward_progress_guarantee) == forward_progress_guarantee::parallel);
  }

  // get_completion_scheduler<set_value_t> round-trips through schedule()'s attributes back to
  // the scheduler it came from, matching run_loop's own [exec.run.loop.types]p5 test.
  {
    auto sch  = get_parallel_scheduler();
    auto sndr = schedule(sch);
    assert((get_completion_scheduler<set_value_t>(get_env(sndr)) == sch));
  }

  // Basic scheduling: unlike run_loop (single draining thread, driven by an explicit run()
  // call), start() here pushes onto the pool's queue and set_value() fires on whichever worker
  // thread pops it -- genuinely asynchronous, so the test waits via atomic<bool>::wait rather
  // than asserting immediately after start() the way exec.ctx/run_loop.pass.cpp's
  // synchronous-completion cases do.
  {
    std::atomic<bool> ran{false};

    struct rcvr {
      using receiver_concept = receiver_tag;
      std::atomic<bool>* ran;
      void set_value() && noexcept {
        ran->store(true);
        ran->notify_all();
      }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return env<>{}; }
    };

    auto sndr = schedule(get_parallel_scheduler());
    auto op   = connect(std::move(sndr), rcvr{&ran});
    start(op);
    ran.wait(false);
    assert(ran.load());
  }

  // task_scheduler must be able to type-erase parallel_scheduler (it satisfies `scheduler`
  // exactly the way inline_scheduler does), and the erased handle's schedule() must still
  // actually run the wrapped scheduler's own schedule() sender to completion.
  {
    task_scheduler ts(get_parallel_scheduler());
    std::atomic<bool> ran{false};

    struct rcvr {
      using receiver_concept = receiver_tag;
      std::atomic<bool>* ran;
      void set_value() && noexcept {
        ran->store(true);
        ran->notify_all();
      }
      void set_error(std::exception_ptr) && noexcept { assert(false); }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return env<>{}; }
    };

    auto op = connect(ts.schedule(), rcvr{&ran});
    start(op);
    ran.wait(false);
    assert(ran.load());
  }

  // Concurrency stress: many concurrent schedule()s incrementing a shared atomic, joined via
  // async_scope's spawn/join -- the first real exercise of spawn()/join() against genuinely
  // outstanding, genuinely parallel work rather than run_loop's single-threaded stand-in. Run
  // repeatedly (not just once) to surface any race rather than trusting a single clean pass,
  // matching this project's established discipline for concurrency-sensitive code.
  {
    constexpr int kReps = 20;
    constexpr int kN    = 200;
    for (int rep = 0; rep < kReps; ++rep) {
      std::atomic<int> counter{0};
      auto sch = get_parallel_scheduler();

      simple_counting_scope scope;
      auto token = scope.get_token();
      for (int i = 0; i < kN; ++i) {
        spawn(then(schedule(sch), [&counter]() noexcept { counter.fetch_add(1, std::memory_order_relaxed); }),
              token);
      }
      std::this_thread::sync_wait(scope.join());
      assert(counter.load() == kN);
    }
  }

  return 0;
}
