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

// [exec.on]p2.2/p2.3: on(sch, x) must be ill-formed when x is neither a sender nor a pipeable
// sender adaptor closure object -- confirms the two 2-arg overloads (__on_sndr's sender-constrained
// form and the partial-application closure-constrained form) don't jointly accept more than the
// wording allows, which is exactly what the header comment's "falls out for free from sender and
// __sender_adaptor_closure_object being mutually exclusive" claim depends on.
static_assert(!std::is_invocable_v<on_t, decltype(std::declval<run_loop&>().get_scheduler()), int>);

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
  // Completion signatures for the 3-arg form: __on2_sndr::get_completion_signatures reconstructs
  // the connect()-time composition (continues_on(closure(continues_on(child, sch)), orig_sch)) at
  // the type level via a chain of declval/decltype -- this pins that reconstruction against the
  // runtime path exercised above, rather than leaving it checked only implicitly (any valid-but-
  // wrong signature set here would still satisfy connect()'s Mandates and go unnoticed otherwise).
  // then's fn (`[] { return 42; }`) is a non-noexcept lambda, so set_error_t(exception_ptr) is
  // expected alongside set_value_t(int).
  {
    run_loop loop_orig, loop_b;
    static_assert(std::is_same_v<
                   completion_signatures_of_t<decltype(on(schedule(loop_orig.get_scheduler()),
                                                           loop_b.get_scheduler(),
                                                           then([] { return 42; }))),
                                               env<>>,
                   completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  // on(sch, closure) (2 args, closure not a sender) is the partial-application form of the above:
  // sndr | on(sch, closure) must equal on(sndr, sch, closure) -- argument for argument, not just
  // "eventually completes with the same value". A runtime drain can't tell those apart: every hop,
  // on either loop, gets drained on every iteration of the polling loop below regardless of which
  // scheduler it actually landed on, so a `bind_back` that captured (sch, closure) in the wrong
  // order (e.g. producing on(sndr, closure, sch) or similar) could still complete with the right
  // value. What actually discriminates that is a type identity between the two spellings; the two
  // on(...) calls must reference the very same closure *value* (not two separately-written lambdas,
  // which are distinct closure types) for the comparison to be meaningful, hence hoisting it into a
  // named variable first.
  {
    run_loop loop_orig, loop_b;
    auto closure = then([] { return 42; });
    static_assert(
        std::is_same_v<decltype(schedule(loop_orig.get_scheduler()) | on(loop_b.get_scheduler(), closure)),
                       decltype(on(schedule(loop_orig.get_scheduler()), loop_b.get_scheduler(), closure))>);

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

    auto op = connect(schedule(loop_orig.get_scheduler()) | on(loop_b.get_scheduler(), closure),
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
