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
//   class inline_scheduler { ... };
//   class task_scheduler { ... };
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <utility>

using namespace std::execution;

struct capture_rcvr {
  using receiver_concept = receiver_tag;
  bool* flag;

  void set_value() && noexcept { *flag = true; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return env<>{}; }
};

int main(int, char**) {
  // inline_scheduler's schedule() sender completes set_value() synchronously, on the calling
  // agent, from within start() -- there is no separate execution agent to wait for.
  {
    bool ran = false;
    auto op = connect(inline_scheduler{}.schedule(), capture_rcvr{&ran});
    start(op);
    assert(ran);
  }

  // task_scheduler type-erases any scheduler; two instances wrapping the same underlying
  // scheduler value compare equal, and .schedule() on the erased handle still runs the
  // wrapped scheduler's own schedule() sender to completion.
  {
    task_scheduler sch(inline_scheduler{});
    task_scheduler sch2(inline_scheduler{});
    assert(sch == sch2);

    bool ran = false;
    auto op = connect(sch.schedule(), capture_rcvr{&ran});
    start(op);
    assert(ran);
  }

  return 0;
}
