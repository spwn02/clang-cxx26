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

// <io_bridge>

// The two coroutine models do not mix by accident. An io::task cannot co_await a sender directly (it must go through
// await_sender), and a std::execution::task cannot co_await an IoAwaitable directly (the two-argument await_suspend is
// not callable without the io_env, and the awaitable has no as_sender wrapper applied).

#include <io_bridge>

#include <coroutine>

namespace io = std::io;
namespace ex = std::execution;

struct io_awaitable {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>, const io::io_env*) noexcept {}
  void await_resume() const noexcept {}
};

io::task<void> io_task_awaits_sender() {
  co_await ex::just(1); // expected-error {{call to deleted member function 'await_transform'}}
}

io::task<void> io_task_awaits_sender_ok() {
  co_await io::await_sender(ex::just(1)); // fine
}

ex::task<void> execution_task_awaits_io_awaitable() {
  co_await io_awaitable{}; // expected-error {{too few arguments to function call, expected 2, have 1}}
}

ex::task<void> execution_task_awaits_await_sender() {
  co_await io::await_sender(ex::just(1)); // expected-error {{too few arguments to function call, expected 2, have 1}}
}
