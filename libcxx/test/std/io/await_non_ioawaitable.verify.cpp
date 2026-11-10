//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io>

// A task can only await IoAwaitables: awaiting anything that lacks the two-argument await_suspend is rejected at
// compile time (P4003R3 4.2).

#include <io>

#include <coroutine>

namespace io = std::io;

struct plain_awaitable {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};

io::task<void> awaits_std_awaitable() {
  co_await std::suspend_always{}; // expected-error {{call to deleted member function 'await_transform'}}
}

io::task<void> awaits_plain_awaitable() {
  co_await plain_awaitable{}; // expected-error {{call to deleted member function 'await_transform'}}
}

io::task<void> awaits_int() {
  co_await 42; // expected-error {{call to deleted member function 'await_transform'}}
}
