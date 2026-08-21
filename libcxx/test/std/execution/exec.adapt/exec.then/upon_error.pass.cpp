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
//   struct upon_error_t { ... };
//   inline constexpr upon_error_t upon_error{};
// }

#include <cassert>
#include <execution>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace std::execution;

int main(int, char**) {
  // The payoff case: upon_error turns an error completion into a value completion --
  // no hand-written sender needed, unlike M4's sync_wait tests.
  {
    auto r = std::this_thread::sync_wait(just_error(5) | upon_error([](int e) { return e * 2; }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 10);
  }
  // Call-syntax form is equivalent to the pipe form.
  {
    auto r = std::this_thread::sync_wait(upon_error(just_error(5), [](int e) { return e * 2; }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 10);
  }
  // A void-returning fn produces a datum-less value completion.
  {
    bool called = false;
    auto r      = std::this_thread::sync_wait(just_error(7) | upon_error([&](int e) {
                                              called = true;
                                              assert(e == 7);
                                            }));
    assert(r.has_value());
    assert(called);
    static_assert(std::is_same_v<decltype(*r), std::tuple<>&>);
  }
  // Absent completion: a sender with no error completion is an unaffected no-op -- upon_error's
  // fn is never invoked, and the value completion passes through untouched.
  {
    bool called = false;
    auto r = std::this_thread::sync_wait(just(1) | upon_error([&](int) {
                                            called = true;
                                            return 2;
                                          }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 1);
    assert(!called);
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(just(1) | upon_error([](int) { return 2; })),
                                                              env<>>,
                                  completion_signatures<set_value_t(int)>>);
  }
  // A throwing fn completes with the exception via sync_wait's error path.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(just_error(1) | upon_error([](int) -> int { throw std::runtime_error("boom"); }));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // Dedup-collision case: the child contributes set_value_t(int) + set_error_t(exception_ptr)
  // (from a throwing `then`); upon_error consumes the error into set_value_t(int) and re-adds
  // set_error_t(exception_ptr) because its own fn can throw too -- exercises intercept +
  // passthrough + dedup simultaneously.
  {
    auto sndr = just(1) | then([](int i) -> int {
                  if (i < 0)
                    throw std::runtime_error("negative");
                  return i;
                }) |
                upon_error([](std::exception_ptr) { return 0; });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(sndr), env<>>,
                                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
    auto r = std::this_thread::sync_wait(std::move(sndr));
    assert(r.has_value());
    assert(std::get<0>(*r) == 1);
  }
  // upon_error's completion signatures: a nothrow fn adds no error completion; a potentially-
  // throwing fn adds set_error_t(exception_ptr).
  {
    auto nothrow_sndr = just_error(1) | upon_error([](int e) noexcept { return e; });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(nothrow_sndr), env<>>,
                                  completion_signatures<set_value_t(int)>>);

    auto throwing_sndr = just_error(1) | upon_error([](int e) { return e; });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(throwing_sndr), env<>>,
                                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
