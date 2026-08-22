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
//   struct continues_on_t { ... };
//   inline constexpr continues_on_t continues_on{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using namespace std::execution;

// Mirrors exec.adapt/exec.let/let_value.pass.cpp's own maybe_errors_sndr: a sender that
// declares both a value and an error completion but always errors at runtime, used to exercise
// continues_on's error-capture-and-replay path without a real async producer.
template <class Rcvr>
struct maybe_errors_opstate {
  using operation_state_concept = operation_state_tag;
  Rcvr rcvr;
  void start() & noexcept { set_error(std::move(rcvr), std::runtime_error("boom")); }
};

struct maybe_errors_sndr {
  using sender_concept = sender_tag;

  template <class Rcvr>
  auto connect(Rcvr&& rcvr) && -> maybe_errors_opstate<std::remove_cvref_t<Rcvr>> {
    return {std::forward<Rcvr>(rcvr)};
  }

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_error_t(std::runtime_error)>{};
  }
};

int main(int, char**) {
  // Value passthrough with a real scheduling hop: starting the operation only starts the child
  // (synchronously, since just(1) completes immediately) and queues the scheduling hop -- the
  // outer receiver isn't invoked until the target run_loop is actually driven, confirming the
  // "start sndr on the current agent, transition to sch's resource only once it completes"
  // contract ([exec.continues.on]p12), not a synchronous passthrough.
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

    auto op = connect(continues_on(just(1), loop.get_scheduler()), rcvr{&value, &completed});
    start(op);
    assert(!completed);
    loop.finish();
    loop.run();
    assert(completed);
    assert(value == 1);
  }
  // Error passthrough: the child's error completion is captured and replayed the same way a
  // value completion is, confirming __on_child_complete's tag-dispatch isn't value-only.
  {
    run_loop loop;
    bool caught = false;

    struct err_rcvr {
      using receiver_concept = receiver_tag;
      bool* caught;
      void set_value(int) && noexcept { assert(false); }
      void set_error(std::runtime_error e) && noexcept {
        *caught = true;
        assert(std::string(e.what()) == "boom");
      }
      void set_stopped() && noexcept { assert(false); }
      auto get_env() const noexcept { return env<>{}; }
    };

    auto op = connect(continues_on(maybe_errors_sndr{}, loop.get_scheduler()), err_rcvr{&caught});
    start(op);
    assert(!caught);
    loop.finish();
    loop.run();
    assert(caught);
  }
  // Completion signatures: the child's own signatures, decay-copied; schedule(sch)'s own
  // set_value_t() contributes nothing extra (it only triggers the internal redispatch), and
  // with env<> (an unstoppable token) run-loop-scheduler's own sender has no set_stopped_t
  // completion to union in either.
  {
    run_loop loop;
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(continues_on(just(1), loop.get_scheduler())), env<>>,
                                  completion_signatures<set_value_t(int)>>);
  }
  return 0;
}
