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

// A scheduler whose schedule() returns an awaitable completing with set_value_t(int) (an invalid schedule sender: it has to
// complete with set_value_t()) is diagnosed, and the diagnostics end the compilation. Regression test for the assertion
// "should not see dependent types here" (ASTContext::getTypeInfoImpl) of an assertions build of the compiler, which was
// reached after the diagnostics when a field of an erroneous class template instantiation kept a dependent type (#264).
// (The current headers no longer reach that state; the test crashed an assertions build with the headers before #220, #224
// and #266, and is kept as a smoke test of the invalid-code path.)

#include <coroutine>
#include <execution>
#include <thread>

namespace ex = std::execution;

struct Awaiter {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  int await_resume() { return 42; }
};

struct AwSched {
  using scheduler_concept = ex::scheduler_tag;
  Awaiter schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(AwSched, AwSched) = default;
};

void test() {
  { // expected-error@*:* 0+ {{}}
    ex::task_scheduler ts{AwSched{}};
    auto r = std::this_thread::sync_wait(ex::schedule(ts) | ex::then([] { return 5; }));
    (void)r;
  }
  { // expected-error@*:* 0+ {{}}
    auto r = std::this_thread::sync_wait(ex::starts_on(AwSched{}, ex::just(3)));
    (void)r;
  }
  { // expected-error@*:* 0+ {{}}
    auto r = std::this_thread::sync_wait(ex::continues_on(ex::just(4), AwSched{}));
    (void)r;
  }
}

// expected-error@*:* 0+ {{}}
// expected-note@*:* 0+ {{}}
// expected-warning@*:* 0+ {{}}
