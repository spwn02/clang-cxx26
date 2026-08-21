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
//   struct let_stopped_t { ... };
//   inline constexpr let_stopped_t let_stopped{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace std::execution;

int main(int, char**) {
  // The payoff case: fn (invoked with no arguments, per [exec.let]p3's eager invocable<F>
  // check) returns a new sender that's connected and started.
  {
    auto r = std::this_thread::sync_wait(let_stopped(just_stopped(), [] { return just(7); }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 7);
  }
  // Call-syntax and pipe-syntax forms are equivalent.
  {
    auto r = std::this_thread::sync_wait(just_stopped() | let_stopped([] { return just(7); }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 7);
  }
  // Absent completion: a sender with no stopped completion is an unaffected no-op -- fn is
  // never invoked, and the value completion passes through untouched.
  {
    bool called = false;
    auto r      = std::this_thread::sync_wait(let_stopped(just(1), [&] {
                                            called = true;
                                            return just(2);
                                          }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 1);
    assert(!called);
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(let_stopped(just(1), [] { return just(2); })),
                                                              env<>>,
                                  completion_signatures<set_value_t(int)>>);
  }
  // A throwing fn completes with the exception via sync_wait's error path.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(let_stopped(just_stopped(), []() -> decltype(just(1)) {
        throw std::runtime_error("boom");
      }));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // let_stopped's completion signatures: the continuation's own signatures replace the
  // intercepted set_stopped_t(), plus set_error_t(exception_ptr) since fn can throw.
  {
    auto sndr = let_stopped(just_stopped(), [] { return just(1); });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(sndr), env<>>,
                                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
