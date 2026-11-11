//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux || darwin || freebsd
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_timer>

// Stress: four threads run the context; 1000 timers of 0 to 5 ms, every wait completes exactly once and no wait is lost.

#include <io_timer>
#include <io_context>

#include <atomic>
#include <cassert>
#include <memory>
#include <thread>
#include <vector>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

io::task<void> one(io::timer& t, std::atomic<int>& fired, std::atomic<int>& errors) {
  auto [ec] = co_await t.wait();
  if (ec)
    ++errors;
  ++fired;
}

int main(int, char**) {
  constexpr int N = 1000;
  io::io_context ctx;
  std::vector<std::unique_ptr<io::timer>> timers;
  std::atomic<int> fired{0}, errors{0};
  for (int i = 0; i != N; ++i) {
    timers.push_back(std::make_unique<io::timer>(ctx, ms(i % 6)));
    io::run_async(ctx.get_executor())(one(*timers.back(), fired, errors));
  }
  watchdog wd(ctx, ms(20000));
  std::vector<std::thread> ts;
  for (int i = 0; i != 4; ++i)
    ts.emplace_back([&] { ctx.run(); });
  for (auto& t : ts)
    t.join();
  assert(fired == N);
  assert(errors == 0);
  return 0;
}
