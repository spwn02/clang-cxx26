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
//   struct stopped_as_error_t { ... };
//   inline constexpr stopped_as_error_t stopped_as_error{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using namespace std::execution;

// A sender that, depending on `stop`, completes with either a value or set_stopped() at
// runtime. Used (rather than just_stopped() alone) for the stopped-path tests below: a
// composed `let_stopped(sndr, fn)` whose *only* completion is the continuation's error would
// give sync_wait's own sync-wait-result-type computation (<__execution/sync_wait.h>) nothing
// to gather a set_value_t shape from at all -- unrelated to stopped_as_error itself, but this
// keeps the tests exercising sync_wait meaningfully composable regardless.
struct value_or_stopped_sndr {
  using sender_concept = sender_tag;
  bool stop;

  template <class _Rcvr>
  struct __opstate {
    using operation_state_concept = operation_state_tag;
    bool stop;
    _Rcvr rcvr;
    void start() & noexcept {
      if (stop) {
        std::execution::set_stopped(std::move(rcvr));
      } else {
        std::execution::set_value(std::move(rcvr), 42);
      }
    }
  };

  template <class _Rcvr>
  auto connect(_Rcvr&& __rcvr) && -> __opstate<std::remove_cvref_t<_Rcvr>> {
    return {stop, std::forward<_Rcvr>(__rcvr)};
  }

  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_stopped_t()>{};
  }
};
static_assert(sender<value_or_stopped_sndr>);

int main(int, char**) {
  // Value path: unaffected by stopped_as_error -- passes straight through.
  {
    auto r = std::this_thread::sync_wait(stopped_as_error(just(42), std::string("nope")));
    assert(r.has_value());
    assert(std::get<0>(*r) == 42);
  }
  // Stopped path: the child's stopped completion becomes an error completion via sync_wait's
  // AS-EXCEPT-PTR path -- matching exec.adapt/exec.let/let_stopped.pass.cpp's own
  // throwing-continuation test for the same rethrow mechanism.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(stopped_as_error(value_or_stopped_sndr{true}, std::runtime_error("boom")));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // Call-syntax and pipe-syntax forms are equivalent.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(value_or_stopped_sndr{true} | stopped_as_error(std::runtime_error("boom")));
    } catch (const std::runtime_error&) {
      caught = true;
    }
    assert(caught);
  }
  // Completion signatures: just_stopped()'s only completion (set_stopped_t()) is replaced by
  // the continuation's own (set_error_t(string)) -- no added set_error_t(exception_ptr), since
  // the closure's move-construction of a std::string is statically nothrow.
  {
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(stopped_as_error(just_stopped(), std::string("x"))), env<>>,
                                  completion_signatures<set_error_t(std::string)>>);
    static_assert(!sends_stopped<decltype(stopped_as_error(just_stopped(), std::string("x"))), env<>>);
  }
  return 0;
}
