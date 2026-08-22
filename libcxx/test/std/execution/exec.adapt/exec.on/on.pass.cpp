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
//   struct on_t { ... };
//   inline constexpr on_t on{};
// }

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

using namespace std::execution;

int main(int, char**) {
  // on(sch, sndr) starts sndr on sch, then (on completion) transfers back to whatever scheduler
  // was in effect when the on-sender itself was started. Using *two distinct* run_loops here
  // (rather than one loop for both) is the discriminating part of this test: it's the only way
  // to actually observe [exec.on]p7.3's "transfer back to the remembered scheduler" hop, rather
  // than merely observing that *some* scheduling happened. Draining loop_a alone must cascade
  // sndr's completion through starts_on/continues_on but must *not* yet reach the outer
  // receiver -- the second hop is still queued on loop_b, untouched -- confirming the result
  // genuinely lands on orig_sch (loop_b) rather than staying on sch (loop_a).
  {
    run_loop loop_a, loop_b;
    int value      = -1;
    bool completed = false;

    struct on_env {
      run_loop* loop;
      auto query(get_start_scheduler_t) const noexcept { return loop->get_scheduler(); }
    };

    struct rcvr {
      using receiver_concept = receiver_tag;
      int* value;
      bool* completed;
      run_loop* loop;
      void set_value(int v) && noexcept {
        *value     = v;
        *completed = true;
      }
      void set_error(std::exception_ptr) && noexcept { assert(false); }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return on_env{loop}; }
    };

    auto op = connect(on(loop_a.get_scheduler(), just(42)), rcvr{&value, &completed, &loop_b});
    start(op);
    assert(!completed);
    loop_a.finish();
    loop_a.run();
    assert(!completed); // sndr ran on loop_a, but the transfer-back hop is still queued on loop_b.
    loop_b.finish();
    loop_b.run();
    assert(completed);
    assert(value == 42);
  }
  // on(sndr, sch, closure): upon sndr's completion, transfer to sch, run closure there, then
  // transfer back to wherever sndr completed. Deliberately uses two distinct run_loops for the
  // same reason as the on(sch, sndr) test above -- loop_orig (where sndr itself completes, via
  // schedule_from(sndr)) and loop_b (sch, where closure runs) must stay observably distinct.
  // sndr must be schedule(some_scheduler) specifically (not e.g. just(1)): [exec.on]p8.1's
  // get_completion_scheduler<set_value_t>(get_env(sndr), get_env(rcvr)) needs sndr's own
  // attributes to actually answer that query -- run-loop-scheduler's sender is the one sender in
  // this fork's execution/ subsystem that does ([exec.run.loop.types]p5).
  {
    run_loop loop_orig, loop_b;
    int value      = -1;
    bool completed = false;

    struct rcvr {
      using receiver_concept = receiver_tag;
      int* value;
      bool* completed;
      void set_value(int v) && noexcept {
        *value     = v;
        *completed = true;
      }
      void set_error(std::exception_ptr) && noexcept { assert(false); }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return env<>{}; }
    };

    auto op = connect(on(schedule(loop_orig.get_scheduler()), loop_b.get_scheduler(), then([] { return 42; })),
                       rcvr{&value, &completed});
    start(op);
    assert(!completed);
    loop_orig.finish();
    loop_orig.run();
    assert(!completed); // sndr completed, but the "run closure on sch" hop is queued on loop_b.
    loop_b.finish();
    loop_b.run();
    assert(!completed); // closure ran on loop_b, but the transfer-back hop is queued on loop_orig.
    // A drained-to-empty run_loop's __pop_front() only unblocks on state == finishing, and run()
    // downgrades that to a *different* enum value (finished) the moment the queue empties -- so a
    // second run() call needs its own finish() first, even though the queue already holds the item
    // pushed by loop_b's cascade above (an empirically-caught deadlock, not a hypothetical one: the
    // first draft of this test hung here without the second finish()).
    loop_orig.finish();
    loop_orig.run();
    assert(completed);
    assert(value == 42);
  }
  // on(sch, closure) (2 args, closure not a sender) is the partial-application form of the above:
  // sndr | on(sch, closure) must equal on(sndr, sch, closure). Reuses the same two-loop
  // discrimination, condensed into a single drain-until-done loop since the exact number of hops
  // isn't the point of this particular test -- call-vs-pipe equivalence is.
  {
    run_loop loop_orig, loop_b;
    int value      = -1;
    bool completed = false;

    struct rcvr {
      using receiver_concept = receiver_tag;
      int* value;
      bool* completed;
      void set_value(int v) && noexcept {
        *value     = v;
        *completed = true;
      }
      void set_error(std::exception_ptr) && noexcept { assert(false); }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return env<>{}; }
    };

    auto op = connect(schedule(loop_orig.get_scheduler()) | on(loop_b.get_scheduler(), then([] { return 42; })),
                       rcvr{&value, &completed});
    start(op);
    // finish() must be called again before *every* run(), not just once up front: once a loop's
    // queue empties, __pop_front() downgrades its state from finishing to a distinct finished
    // value, and only finishing (not finished) unblocks the "is there more work" wait -- a stale
    // finished state hangs the next run() call forever even when new work has since been queued.
    // Calling finish() on an already-empty, already-finished loop is a harmless no-op, so doing it
    // unconditionally on every iteration is always safe here, regardless of which loop (if either)
    // actually has pending work this time around.
    while (!completed) {
      loop_orig.finish();
      loop_orig.run();
      loop_b.finish();
      loop_b.run();
    }
    assert(value == 42);
  }
  return 0;
}
