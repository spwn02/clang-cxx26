//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

#include <concepts>
#include <coroutine>
#include <execution>
#include <utility>

namespace ex = std::execution;

struct Env {
};

struct Receiver {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  void set_error(std::exception_ptr) && noexcept {}
  void set_stopped() && noexcept {}
  Env get_env() const noexcept { return {}; }
};

ex::task<void> completed_task() { co_return; }

int main(int, char**) {
  static_assert(!std::default_initializable<ex::task<void>::start_scheduler_type>);
  auto op = ex::connect(completed_task(), Receiver{});
  ex::start(op); // expected-error@*:* {{task start_scheduler_type is not constructible from its environment}}
}
