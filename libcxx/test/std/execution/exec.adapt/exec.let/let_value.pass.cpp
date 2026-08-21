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
//   struct let_value_t { ... };
//   inline constexpr let_value_t let_value{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace std::execution;

// Declares both a value and an error completion, but always errors at runtime -- needed
// because sync_wait's own Mandates require a value-completion signature to be present
// (mirrors exec.sync.wait/sync_wait.pass.cpp's own maybe_errors_sndr; without this, a sender
// whose overall completions are error-only, like let_value(sndr, fn-that-always-errors),
// can't be sync_wait'd at all).
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
  // The payoff case: the child's value datum feeds a *new* sender, and it's that sender's
  // completion the overall operation waits on -- not a synchronous transform like `then`.
  {
    auto r = std::this_thread::sync_wait(let_value(just(1), [](int& i) { return just(i + 1); }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 2);
  }
  // Call-syntax form is equivalent to the pipe form.
  {
    auto r = std::this_thread::sync_wait(let_value(just(1), [](int& i) { return just(i + 1); }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 2);
  }
  {
    auto r = std::this_thread::sync_wait(just(1) | let_value([](int& i) { return just(i + 1); }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 2);
  }
  // The continuation's own completion (error, not value) reaches the outer receiver --
  // confirms this is genuinely "connect and start a new operation", not "invoke and
  // synchronously complete" the way then/upon_error/upon_stopped work.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(let_value(just(1), [](int&) { return maybe_errors_sndr{}; }));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // Absent completion: a sender with no value completion (only reachable through
  // let_error/let_stopped's own tests below, since a childless just_error/just_stopped can't
  // itself be sync_wait'd) is exercised via let_error/let_stopped instead. Here: the
  // passthrough side -- a sender with only a value completion is unaffected by let_error,
  // fn never invoked.
  {
    bool called = false;
    auto r      = std::this_thread::sync_wait(let_error(just(1), [&](int&) {
                                            called = true;
                                            return just(2);
                                          }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 1);
    assert(!called);
    static_assert(
        std::is_same_v<completion_signatures_of_t<decltype(let_error(just(1), [](int&) { return just(2); })), env<>>,
                        completion_signatures<set_value_t(int)>>);
  }
  // A throwing fn completes with the exception via sync_wait's error path (args-construction/
  // fn-invocation nothrow check drives whether set_error_t(exception_ptr) is advertised; here
  // fn can throw, so it must be).
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(
          let_value(just(1), [](int&) -> decltype(just(1)) { throw std::runtime_error("boom"); }));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // let_value's completion signatures: the continuation's own signatures replace the
  // intercepted set_value_t(...), plus set_error_t(exception_ptr) since the fn here can
  // throw (its return type isn't noexcept-invocable).
  {
    auto sndr = let_value(just(1), [](int& i) { return just(i + 1); });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(sndr), env<>>,
                                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
