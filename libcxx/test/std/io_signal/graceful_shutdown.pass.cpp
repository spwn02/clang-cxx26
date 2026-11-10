//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_signal>

// The graceful-shutdown pattern of P4100R1 [8.8]: co_await when_any(accept_loop(), signals.wait()) ends when the signal
// arrives; here the accept loop is a long timer wait. The loser is cancelled through the stop token and the whole
// thing finishes without waiting for the timer.

#include <io_signal>
#include <io_timer>
#include <io_context>

#include <cassert>
#include <csignal>
#include <thread>
#include <unistd.h>
#include <variant>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

io::task<std::size_t> server(io::timer& accept_loop, io::signal_set& signals, int& signo) {
  auto r = co_await io::when_any(accept_loop.wait(), signals.wait());
  // r is a variant<io_result<>, io_result<int>> in the index of the winner
  if (r.index() == 1)
    signo = std::get<1>(std::get<1>(r));
  co_return r.index();
}

int main(int, char**) {
  // The signal wins.
  {
    io::io_context ctx;
    io::timer accept_loop(ctx, ms(30000));
    io::signal_set signals(ctx, SIGUSR1, SIGUSR2);
    int signo         = 0;
    std::size_t index = 99;
    io::run_async(ctx.get_executor(), [&](std::size_t i) { index = i; }, [](std::exception_ptr) {})(
        server(accept_loop, signals, signo));
    std::thread([] {
      std::this_thread::sleep_for(ms(30));
      ::kill(::getpid(), SIGUSR2);
    }).detach();
    auto start = clk::now();
    watchdog wd(ctx);
    ctx.run();
    assert(index == 1);
    assert(signo == SIGUSR2);
    assert(clk::now() - start < ms(5000));
  }
  // The timer wins; the signal wait is cancelled and the set can be used again.
  {
    io::io_context ctx;
    io::timer accept_loop(ctx, ms(20));
    io::signal_set signals(ctx, SIGUSR1);
    int signo         = 0;
    std::size_t index = 99;
    io::run_async(ctx.get_executor(), [&](std::size_t i) { index = i; }, [](std::exception_ptr) {})(
        server(accept_loop, signals, signo));
    watchdog wd(ctx);
    ctx.run();
    assert(index == 0);
    std::error_code ec;
    int n = 0;
    ctx.restart();
    io::run_async(ctx.get_executor())([](io::signal_set& s, std::error_code& ec, int& n) -> io::task<void> {
      auto [e, v] = co_await s.wait();
      ec          = e;
      n           = v;
    }(signals, ec, n));
    std::thread([] {
      std::this_thread::sleep_for(ms(20));
      ::kill(::getpid(), SIGUSR1);
    }).detach();
    ctx.run();
    assert(!ec && n == SIGUSR1);
  }
  return 0;
}
