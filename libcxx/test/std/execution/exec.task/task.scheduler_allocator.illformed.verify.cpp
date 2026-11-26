//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

#include <execution>
#include <coroutine>
#include <utility>

namespace ex = std::execution;

struct NonConstructibleScheduler : ex::inline_scheduler {
  using scheduler_concept                                                      = ex::scheduler_tag;
  NonConstructibleScheduler()                                                  = delete;
  NonConstructibleScheduler(const NonConstructibleScheduler&)                  = default;
  friend bool operator==(NonConstructibleScheduler, NonConstructibleScheduler) = default;
};

struct Env {
  using start_scheduler_type = NonConstructibleScheduler;
  ex::inline_scheduler query(ex::get_start_scheduler_t) const noexcept { return {}; }
};

struct Receiver {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  void set_error(std::exception_ptr) && noexcept {}
  void set_stopped() && noexcept {}
  Env get_env() const noexcept { return {}; }
};

ex::task<void, Env> completed_task() { co_return; }

int main(int, char**) {
  auto __op = ex::connect(completed_task(), Receiver{});
  ex::start(__op); // expected-error@*:* {{task start_scheduler_type is not constructible from its environment}}
}
