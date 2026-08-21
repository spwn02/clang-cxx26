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
//   struct let_error_t { ... };
//   inline constexpr let_error_t let_error{};
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
  // The payoff case: unlike upon_error (which just invokes fn and completes synchronously),
  // let_error's fn returns a *new* sender that's connected and started -- here, one that
  // completes asynchronously with a value derived from the error.
  {
    auto r = std::this_thread::sync_wait(let_error(just_error(5), [](int& e) { return just(e * 2); }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 10);
  }
  // Call-syntax and pipe-syntax forms are equivalent.
  {
    auto r = std::this_thread::sync_wait(just_error(5) | let_error([](int& e) { return just(e * 2); }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 10);
  }
  // Absent completion: a sender with no error completion is an unaffected no-op -- fn is
  // never invoked, and the value completion passes through untouched.
  {
    bool called = false;
    auto r = std::this_thread::sync_wait(let_error(just(1), [&](int&) {
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
  // A throwing fn completes with the exception via sync_wait's error path.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(
          let_error(just_error(1), [](int&) -> decltype(just(1)) { throw std::runtime_error("boom"); }));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // let_error's completion signatures: the continuation's own signatures replace the
  // intercepted set_error_t(int), plus set_error_t(exception_ptr) since fn can throw.
  {
    auto sndr = let_error(just_error(1), [](int& e) { return just(e); });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(sndr), env<>>,
                                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
