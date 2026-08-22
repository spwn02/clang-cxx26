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
  // was in effect when the on-sender itself was started -- here both are the same run_loop, so
  // the whole chain drains from a single finish()+run() with no threads involved: starting the
  // operation queues *one* item (starts_on's own scheduling hop); draining it cascades through
  // continues_on's internal redispatch, which queues a *second* item (the "transfer back" hop)
  // on the very same loop mid-drain, which the same run() call picks up before returning.
  {
    run_loop loop;
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

    auto op = connect(on(loop.get_scheduler(), just(42)), rcvr{&value, &completed, &loop});
    start(op);
    assert(!completed);
    loop.finish();
    loop.run();
    assert(completed);
    assert(value == 42);
  }
  return 0;
}
