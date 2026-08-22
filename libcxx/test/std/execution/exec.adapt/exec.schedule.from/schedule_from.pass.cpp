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
//   struct schedule_from_t { ... };
//   inline constexpr schedule_from_t schedule_from{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using namespace std::execution;

int main(int, char**) {
  // Identity in value: same result as the wrapped sender, round-tripped through sync_wait.
  {
    auto r = std::this_thread::sync_wait(schedule_from(just(42)));
    assert(r.has_value());
    assert(std::get<0>(*r) == 42);
  }
  // Identity in error: connect/start directly (a pure-error sender doesn't satisfy sync_wait's
  // own Mandates, which require a value-completion signature -- same reason
  // exec.adapt/exec.let/let_value.pass.cpp's maybe_errors_sndr exists) and confirm the wrapped
  // sender's error datum passes through unchanged.
  {
    struct err_rcvr {
      using receiver_concept = receiver_tag;
      bool* caught;
      void set_value() && noexcept { assert(false); }
      void set_error(std::runtime_error e) && noexcept {
        *caught = true;
        assert(std::string(e.what()) == "boom");
      }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return env<>{}; }
    };
    bool caught = false;
    auto op = connect(schedule_from(just_error(std::runtime_error("boom"))), err_rcvr{&caught});
    start(op);
    assert(caught);
  }
  // Identity in stopped.
  {
    struct stopped_rcvr {
      using receiver_concept = receiver_tag;
      bool* stopped;
      void set_value() && noexcept { assert(false); }
      void set_stopped() && noexcept { *stopped = true; }
      auto get_env() const noexcept { return env<>{}; }
    };
    bool stopped = false;
    auto op = connect(schedule_from(just_stopped()), stopped_rcvr{&stopped});
    start(op);
    assert(stopped);
  }
  // Identity completion signatures.
  {
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(schedule_from(just(1))), env<>>,
                                  completion_signatures<set_value_t(int)>>);
    static_assert(
        std::is_same_v<completion_signatures_of_t<decltype(schedule_from(just_error(std::runtime_error("x")))), env<>>,
                       completion_signatures<set_error_t(std::runtime_error)>>);
  }
  return 0;
}
