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

// <io_context>

// The internal reactor_wait(ctx, fd, events) awaitable that the descriptor-based facilities of the series (TCP, UDP, files)
// are built on: readiness of a pipe or socket, readiness that exists before the wait starts, error and hang-up
// conditions, two waits on one descriptor, cancellation, and a descriptor that epoll cannot watch.

#include <io_context>

#include <cassert>
#include <fcntl.h>
#include <stop_token>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

struct result {
  std::error_code ec = std::make_error_code(std::errc::invalid_argument);
  unsigned events    = 0;
  bool done          = false;
};

io::task<void> wait_fd(io::io_context& ctx, int fd, unsigned events, result& r) {
  auto [ec, ev] = co_await io::__reactor_wait(ctx, fd, events);
  r.ec          = ec;
  r.events      = ev;
  r.done        = true;
}

int main(int, char**) {
  // A pipe becomes readable after another thread writes to it.
  {
    int p[2];
    assert(::pipe(p) == 0);
    io::io_context ctx;
    result r;
    io::run_async(ctx.get_executor())(wait_fd(ctx, p[0], io::__poll_read, r));
    std::thread w([&] {
      std::this_thread::sleep_for(ms(30));
      char c = 'x';
      assert(::write(p[1], &c, 1) == 1);
    });
    watchdog wd(ctx);
    ctx.run();
    w.join();
    assert(r.done && !r.ec);
    assert(r.events == io::__poll_read);
    ::close(p[0]);
    ::close(p[1]);
  }

  // Readiness that exists before the wait starts is not lost (level-triggered).
  {
    int p[2];
    assert(::pipe(p) == 0);
    char c = 'x';
    assert(::write(p[1], &c, 1) == 1);
    io::io_context ctx;
    result r;
    io::run_async(ctx.get_executor())(wait_fd(ctx, p[0], io::__poll_read, r));
    watchdog wd(ctx);
    ctx.run();
    assert(r.done && !r.ec && r.events == io::__poll_read);
    // Not consumed: waiting again completes again.
    result r2;
    ctx.restart();
    io::run_async(ctx.get_executor())(wait_fd(ctx, p[0], io::__poll_read, r2));
    ctx.run();
    assert(r2.done && !r2.ec);
    ::close(p[0]);
    ::close(p[1]);
  }

  // The write end of an empty pipe is writable at once.
  {
    int p[2];
    assert(::pipe(p) == 0);
    io::io_context ctx;
    result r;
    io::run_async(ctx.get_executor())(wait_fd(ctx, p[1], io::__poll_write, r));
    watchdog wd(ctx);
    ctx.run();
    assert(r.done && !r.ec && r.events == io::__poll_write);
    ::close(p[0]);
    ::close(p[1]);
  }

  // A hang-up completes the wait with the error bit.
  {
    int p[2];
    assert(::pipe(p) == 0);
    io::io_context ctx;
    result r;
    io::run_async(ctx.get_executor())(wait_fd(ctx, p[0], io::__poll_read, r));
    ctx.poll();
    ::close(p[1]);
    watchdog wd(ctx);
    ctx.restart();
    ctx.run();
    assert(r.done && !r.ec);
    assert(r.events & io::__poll_error);
    ::close(p[0]);
  }

  // A read wait and a write wait on the same socket: the write one completes at once, the read one when data arrives.
  {
    int sv[2];
    assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
    io::io_context ctx;
    result rr, rw;
    io::run_async(ctx.get_executor())(wait_fd(ctx, sv[0], io::__poll_read, rr));
    io::run_async(ctx.get_executor())(wait_fd(ctx, sv[0], io::__poll_write, rw));
    std::thread w([&] {
      std::this_thread::sleep_for(ms(50));
      char c = 'y';
      assert(::write(sv[1], &c, 1) == 1);
    });
    watchdog wd(ctx);
    ctx.run();
    w.join();
    assert(rw.done && !rw.ec && (rw.events & io::__poll_write));
    assert(rr.done && !rr.ec && (rr.events & io::__poll_read));
    ::close(sv[0]);
    ::close(sv[1]);
  }

  // Cancellation by the stop token while waiting; the descriptor can be waited for again afterwards.
  {
    int p[2];
    assert(::pipe(p) == 0);
    io::io_context ctx;
    std::stop_source src;
    result r;
    io::run_async(ctx.get_executor(), src.get_token())(wait_fd(ctx, p[0], io::__poll_read, r));
    std::thread t([&] {
      std::this_thread::sleep_for(ms(30));
      src.request_stop();
    });
    watchdog wd(ctx);
    ctx.run();
    t.join();
    assert(r.done && r.ec == io::cond::canceled);
    assert(r.events == 0);
    result r2;
    char c = 'z';
    assert(::write(p[1], &c, 1) == 1);
    ctx.restart();
    io::run_async(ctx.get_executor())(wait_fd(ctx, p[0], io::__poll_read, r2));
    ctx.run();
    assert(r2.done && !r2.ec);
    ::close(p[0]);
    ::close(p[1]);
  }

  // A stop request before the wait: canceled without suspending.
  {
    int p[2];
    assert(::pipe(p) == 0);
    io::io_context ctx;
    std::stop_source src;
    src.request_stop();
    result r;
    io::run_async(ctx.get_executor(), src.get_token())(wait_fd(ctx, p[0], io::__poll_read, r));
    watchdog wd(ctx);
    ctx.run();
    assert(r.done && r.ec == io::cond::canceled);
    ::close(p[0]);
    ::close(p[1]);
  }

  // A regular file cannot be watched by epoll: the wait fails at once.
  {
    int fd = ::open("/dev/null", O_RDONLY);
    assert(fd >= 0);
    int rf = ::open("/proc/self/exe", O_RDONLY);
    assert(rf >= 0);
    io::io_context ctx;
    result r;
    io::run_async(ctx.get_executor())(wait_fd(ctx, rf, io::__poll_read, r));
    watchdog wd(ctx);
    ctx.run();
    assert(r.done && r.ec == std::errc::operation_not_permitted);
    ::close(fd);
    ::close(rf);
  }

  // An invalid descriptor fails at once.
  {
    io::io_context ctx;
    result r;
    io::run_async(ctx.get_executor())(wait_fd(ctx, -1, io::__poll_read, r));
    watchdog wd(ctx);
    ctx.run();
    assert(r.done && r.ec == std::errc::bad_file_descriptor);
  }

  return 0;
}
