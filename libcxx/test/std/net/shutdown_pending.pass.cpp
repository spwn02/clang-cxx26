//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux || darwin || freebsd || windows
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <net>

// A context that is destroyed while name lookups are pending (queued for a helper thread, running, or finished with
// the completion still queued) destroys the chains of the coroutines that wait and leaves nothing behind.

#include <net>

#include <atomic>
#include <cassert>
#include <utility>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};

io::task<void> lookup(io::io_context& ctx, sentinel) {
  io::resolver r(ctx);
  (void)co_await r.resolve("127.0.0.1", "80", io::resolve_flags::numeric_host | io::resolve_flags::numeric_service);
}
io::task<void> reverse_lookup(io::io_context& ctx, sentinel) {
  io::resolver r(ctx);
  (void)co_await r.resolve(
      io::endpoint(io::ip_address(io::ipv4_address::loopback()), 80), io::reverse_flags::numeric_host);
}
io::task<void> nested(io::io_context& ctx, std::atomic<int>* d, sentinel) { co_await lookup(ctx, sentinel(d)); }

int main(int, char**) {
  for (int round = 0; round != 100; ++round) {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      // more requests than helper threads
      for (int i = 0; i != 8; ++i)
        io::run_async(ctx.get_executor())(lookup(ctx, sentinel(&d)));
      io::run_async(ctx.get_executor())(reverse_lookup(ctx, sentinel(&d)));
      io::run_async(ctx.get_executor())(nested(ctx, &d, sentinel(&d)));
      ctx.poll(); // the lookups are started; each is queued, running or finished with its completion queued
    }
    // 8 + 1 + 2 chains; a lookup that completed and was resumed by poll() has finished (its sentinel is gone as well)
    assert(d == 8 + 1 + 2);
  }

  // The context destroyed before anything ran.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      io::run_async(ctx.get_executor())(lookup(ctx, sentinel(&d)));
    }
    assert(d == 1);
  }
  return 0;
}
