//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


// REQUIRES: std-at-least-c++26 && no-threads

// <execution>

// Without thread support (no <stop_token>, no mutex) <execution> must still compile, with the facilities that need
// stop tokens (affine, task, task_scheduler, unstoppable, the scopes, run_loop, the parallel scheduler) absent, and the
// rest working.

#include <cassert>
#include <exception>
#include <execution>

namespace ex = std::execution;

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  int* out;
  void set_value(int v) && noexcept { *out = v; }
  void set_error(std::exception_ptr) && noexcept {}
  void set_stopped() && noexcept {}
  ex::env<> get_env() const noexcept { return {}; }
};

int main(int, char**) {
  int out = 0;
  auto op = ex::connect(ex::just(1) | ex::then([](int x) noexcept { return x + 1; }) |
                            ex::let_value([](int x) noexcept { return ex::just(x * 3); }),
                        rcvr{&out});
  ex::start(op);
  assert(out == 6);
  return 0;
}
