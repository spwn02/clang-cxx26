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
//   struct affine_on_t { ... };
//   inline constexpr affine_on_t affine_on{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <utility>

using namespace std::execution;

struct value_rcvr {
  using receiver_concept = receiver_tag;
  int* out;

  void set_value(int v) && noexcept { *out = v; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return env<>{}; }
};

struct stopped_rcvr {
  using receiver_concept = receiver_tag;
  bool* stopped;

  void set_value() && noexcept { assert(false); }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { *stopped = true; }
  auto get_env() const noexcept { return env<>{}; }
};

int main(int, char**) {
  // affine_on(just(42), inline_scheduler) completes on the given scheduler with the child
  // sender's own value -- the only observable behavior [exec.affine.on] mandates (the
  // same-resource fast path is a permitted "may", not a requirement).
  {
    int out = 0;
    auto op = connect(affine_on(just(42), inline_scheduler{}), value_rcvr{&out});
    start(op);
    assert(out == 42);
  }
  // A stopped child sender still completes with set_stopped after the scheduling hop.
  {
    bool stopped = false;
    auto op = connect(affine_on(just_stopped(), inline_scheduler{}), stopped_rcvr{&stopped});
    start(op);
    assert(stopped);
  }

  return 0;
}
