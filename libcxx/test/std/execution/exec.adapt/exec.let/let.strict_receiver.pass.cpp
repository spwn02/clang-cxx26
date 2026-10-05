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

// [exec.let]: the error completion of the let-family operation state is only produced (and only advertised) when
// decay-copying the datums, invoking the function or connecting the continuation sender can throw. A receiver that
// handles exactly the advertised completions must therefore work.

#include <cassert>
#include <exception>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

struct strict_rcvr {
  using receiver_concept = ex::receiver_tag;
  int* value;
  bool* stopped;
  void set_value(int v) && noexcept { *value = v; }
  void set_stopped() && noexcept { *stopped = true; }
  auto get_env() const noexcept { return ex::env<>{}; }
};

int main(int, char**) {
  {
    auto sndr = ex::just(1) | ex::let_value([](int x) noexcept { return ex::just(x + 1); });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(sndr), ex::env<>>,
                                 ex::completion_signatures<ex::set_value_t(int)>>);
    int value = 0;
    bool stopped = false;
    auto op = ex::connect(std::move(sndr), strict_rcvr{&value, &stopped});
    ex::start(op);
    assert(value == 2 && !stopped);
  }
  {
    auto sndr = ex::just_error(3) | ex::let_error([](int e) noexcept { return ex::just(e + 1); });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(sndr), ex::env<>>,
                                 ex::completion_signatures<ex::set_value_t(int)>>);
    int value = 0;
    bool stopped = false;
    auto op = ex::connect(std::move(sndr), strict_rcvr{&value, &stopped});
    ex::start(op);
    assert(value == 4);
  }
  {
    auto sndr = ex::just_stopped() | ex::let_stopped([]() noexcept { return ex::just(9); });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(sndr), ex::env<>>,
                                 ex::completion_signatures<ex::set_value_t(int)>>);
    int value = 0;
    bool stopped = false;
    auto op = ex::connect(std::move(sndr), strict_rcvr{&value, &stopped});
    ex::start(op);
    assert(value == 9);
  }
  { // a potentially throwing function does advertise (and need) set_error(exception_ptr)
    auto sndr = ex::just(1) | ex::let_value([](int x) { return ex::just(x); });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(sndr), ex::env<>>,
                                 ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
