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

// Only the on(sch, sndr) form ([exec.on]p1.1) is implemented -- see <__execution/on.h>'s own
// top comment and docs/CXX26_GAPS.md's session log for the pipeable-closure form
// (on(sndr, sch, closure), [exec.on]p1.2), deferred to a future session.

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
  return 0;
}
