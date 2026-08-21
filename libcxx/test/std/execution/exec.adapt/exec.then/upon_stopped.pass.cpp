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
//   struct upon_stopped_t { ... };
//   inline constexpr upon_stopped_t upon_stopped{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <tuple>
#include <type_traits>

using namespace std::execution;

int main(int, char**) {
  // The payoff case: upon_stopped turns a stopped completion into a value completion.
  {
    auto r = std::this_thread::sync_wait(just_stopped() | upon_stopped([] { return 7; }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 7);
  }
  // Call-syntax form is equivalent to the pipe form.
  {
    auto r = std::this_thread::sync_wait(upon_stopped(just_stopped(), [] { return 7; }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 7);
  }
  // A void-returning fn produces a datum-less value completion.
  {
    bool called = false;
    auto r      = std::this_thread::sync_wait(just_stopped() | upon_stopped([&] { called = true; }));
    assert(r.has_value());
    assert(called);
    static_assert(std::is_same_v<decltype(*r), std::tuple<>&>);
  }
  // Absent completion: a sender with no stopped completion is an unaffected no-op --
  // upon_stopped's fn is never invoked, and the value completion passes through untouched.
  {
    bool called = false;
    auto r = std::this_thread::sync_wait(just(1) | upon_stopped([&] {
                                            called = true;
                                            return 2;
                                          }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 1);
    assert(!called);
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(just(1) | upon_stopped([] { return 2; })),
                                                              env<>>,
                                  completion_signatures<set_value_t(int)>>);
  }
  // A throwing fn completes with the exception via sync_wait's error path.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(just_stopped() | upon_stopped([]() -> int { throw std::runtime_error("boom"); }));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // upon_stopped's completion signatures: a nothrow fn adds no error completion; a
  // potentially-throwing fn adds set_error_t(exception_ptr).
  {
    auto nothrow_sndr = just_stopped() | upon_stopped([]() noexcept { return 1; });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(nothrow_sndr), env<>>,
                                  completion_signatures<set_value_t(int)>>);

    auto throwing_sndr = just_stopped() | upon_stopped([]() { return 1; });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(throwing_sndr), env<>>,
                                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
