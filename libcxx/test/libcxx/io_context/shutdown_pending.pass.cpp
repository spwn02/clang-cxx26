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

// <io_context>

// An io_context that is destroyed while a coroutine is suspended on a descriptor wait of the reactor destroys the whole
// chain (the waiting task, its callers and the run_async root) and leaves no operation behind.

#include <io_context>

#include <atomic>
#include <cassert>
#include <unistd.h>
#include <utility>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};

io::task<void> wait_fd(io::io_context& ctx, int fd, unsigned events, sentinel) {
  (void)co_await io::__reactor_wait(ctx, fd, events);
}
io::task<void> caller(io::io_context& ctx, int fd, std::atomic<int>* d, sentinel) {
  co_await wait_fd(ctx, fd, io::__poll_read, sentinel(d));
}
io::task<void> top(io::io_context& ctx, int fd, std::atomic<int>* d, sentinel) {
  co_await caller(ctx, fd, d, sentinel(d));
}

int main(int, char**) {
  // A pipe that never becomes readable, the wait three tasks deep.
  {
    int p[2];
    assert(::pipe(p) == 0);
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      io::run_async(ctx.get_executor())(top(ctx, p[0], &d, sentinel(&d)));
      ctx.poll();
      assert(d == 0);
    }
    assert(d == 3);
    ::close(p[0]);
    ::close(p[1]);
  }

  // Several waits on the same descriptor and on different ones.
  {
    int p[2], q[2];
    assert(::pipe(p) == 0 && ::pipe(q) == 0);
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      for (int i = 0; i != 3; ++i)
        io::run_async(ctx.get_executor())(wait_fd(ctx, p[0], io::__poll_read, sentinel(&d)));
      io::run_async(ctx.get_executor())(wait_fd(ctx, q[0], io::__poll_read, sentinel(&d)));
      io::run_async(ctx.get_executor())(wait_fd(ctx, q[1], io::__poll_write | io::__poll_read, sentinel(&d)));
      ctx.poll();
    }
    assert(d == 5);
    for (int fd : {p[0], p[1], q[0], q[1]})
      ::close(fd);
  }

  return 0;
}
