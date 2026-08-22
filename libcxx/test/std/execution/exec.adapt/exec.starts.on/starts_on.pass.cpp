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
//   struct starts_on_t { ... };
//   inline constexpr starts_on_t starts_on{};
// }

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

using namespace std::execution;

int main(int, char**) {
  // Starting the operation only queues sndr's execution onto sch's run_loop -- it doesn't run
  // synchronously on the calling agent -- confirming starts_on genuinely defers to sch rather
  // than being a no-op wrapper.
  {
    run_loop loop;
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

    auto op = connect(starts_on(loop.get_scheduler(), just(42)), rcvr{&value, &completed});
    start(op);
    assert(!completed);
    loop.finish();
    loop.run();
    assert(completed);
    assert(value == 42);
  }
  // Completion signatures: same as the wrapped sender's own (the let_value/continues_on/just
  // composition this is built from doesn't add an exception_ptr alternative here, since moving
  // an already-constructed sender out of the continuation closure can't throw).
  {
    static_assert(std::is_same_v<
                   completion_signatures_of_t<decltype(starts_on(std::declval<run_loop&>().get_scheduler(), just(1))), env<>>,
                   completion_signatures<set_value_t(int)>>);
  }
  return 0;
}
