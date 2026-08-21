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
//   struct then_t { ... };
//   inline constexpr then_t then{};
// }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <tuple>
#include <type_traits>

using namespace std::execution;

int main(int, char**) {
  // M4 vertical-slice checkpoint: just(42) | then(f) | sync_wait().
  {
    auto r = std::this_thread::sync_wait(just(42) | then([](int i) { return i + 1; }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 43);
  }
  // Call-syntax form (then(sndr, f)) is equivalent to the pipe form.
  {
    auto r = std::this_thread::sync_wait(then(just(42), [](int i) { return i + 1; }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 43);
  }
  // Multiple value datums, and a void-returning fn (no value datum in the result).
  {
    bool called = false;
    auto r      = std::this_thread::sync_wait(just(1, 2) | then([&](int a, int b) {
                                              called = true;
                                              assert(a == 1 && b == 2);
                                            }));
    assert(r.has_value());
    assert(called);
    static_assert(std::is_same_v<decltype(*r), std::tuple<>&>);
  }
  // Chaining multiple `then`s via the pipe.
  {
    auto r = std::this_thread::sync_wait(just(10) | then([](int i) { return i * 2; }) |
                                          then([](int i) { return i + 1; }));
    assert(r.has_value());
    assert(std::get<0>(*r) == 21);
  }
  // A throwing `fn` completes with the exception via sync_wait's error path.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(just(1) | then([](int) -> int { throw std::runtime_error("boom"); }));
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // then's completion signatures: a nothrow fn adds no error completion; a potentially-
  // throwing fn adds set_error_t(exception_ptr).
  {
    auto nothrow_sndr = just(1) | then([](int i) noexcept { return i; });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(nothrow_sndr), env<>>,
                                  completion_signatures<set_value_t(int)>>);

    auto throwing_sndr = just(1) | then([](int i) { return i; });
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(throwing_sndr), env<>>,
                                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
