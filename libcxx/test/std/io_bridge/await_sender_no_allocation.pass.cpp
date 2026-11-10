//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++29
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_bridge>

// P4092R1: the operation state is stored inline in the awaitable, so awaiting a sender allocates nothing beyond the
// coroutine frame of the awaiting coroutine (which is allocated before the loop starts).

#include <io_bridge>

#include <cassert>
#include <cstdlib>
#include <new>

#include "test_bridge.h"

using namespace test_bridge;
using test_io::test_context;

static int new_calls = 0;

void* operator new(std::size_t n) {
  ++new_calls;
  if (void* p = std::malloc(n ? n : 1))
    return p;
  TEST_THROW(std::bad_alloc());
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

io::task<void> loop_senders(long* sum) {
  for (int i = 0; i != 2000; ++i) {
    *sum += co_await io::await_sender(ex::just(i) | ex::then([](int v) { return v + 1; }));
    auto [ec, v] = co_await io::await_sender(ec_sender{i, {}});
    *sum += v;
  }
}

int main(int, char**) {
  test_context ctx;
  long sum = 0;
  io::run_async(ctx.get_executor())(loop_senders(&sum));
  int before = new_calls;
  ctx.run();
  assert(new_calls == before);
  assert(sum == 2000L * 2001 / 2 + 1999L * 2000 / 2);
  assert(ctx.posted >= 4000);
  return 0;
}
