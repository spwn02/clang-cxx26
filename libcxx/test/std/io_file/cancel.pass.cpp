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
// <io_file>

// Cancellation of file operations: an operation that waits for a helper thread is cancelled at once (by a stop request,
// cancel(), close(), or a stop token that is already stopped); an operation whose blocking call is in progress runs
// to its end. The helper threads are saturated with reads of FIFOs that stay blocked until the test writes to them.

#include <io_file>
#include <io_timer>

#include <atomic>
#include <csignal>
#include <fcntl.h>
#include <stop_token>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "test_macros.h"
#include "test_file.h"

using namespace test_file;

struct outcome {
  std::atomic<bool> done{false};
  std::error_code ec;
  std::size_t n = 0;
};

io::task<void> read_one(io::stream_file& f, outcome& o) {
  char c[4];
  auto [ec, n] = co_await f.read_some(io::mutable_buffer(c, 1));
  o.ec         = ec;
  o.n          = n;
  o.done.store(true);
}

io::task<void> read_at_zero(io::random_access_file& f, outcome& o) {
  char c[4];
  auto [ec, n] = co_await f.read_some_at(0, io::mutable_buffer(c, 4));
  o.ec         = ec;
  o.n          = n;
  o.done.store(true);
}

static void spawn(io::io_context& ctx, io::task<void> t, std::stop_token st = {}) {
  io::run_async(ctx.get_executor(), st, [] {}, [](std::exception_ptr) { assert(false); })(std::move(t));
}

io::task<void> pause(io::io_context& ctx, ms d) {
  io::timer t(ctx, d);
  (void)co_await t.wait();
}

int main(int, char**) {
  ::signal(SIGPIPE, SIG_IGN); // a write to a FIFO whose reader is gone must not kill the test
  temp_dir dir;
  io::io_context ctx;

  // the data file for the victims
  {
    io::stream_file w(ctx);
    assert(!w.open(dir / "data", io::file_base::write_only | io::file_base::create));
    run(ctx, [](io::stream_file& w) -> io::task<void> {
      auto [ec, n] = co_await w.write_some(io::const_buffer("abcdefgh", 8));
      assert(!ec && n == 8);
    }(w));
  }

  constexpr int K = 6; // more blocked readers than helper threads
  std::vector<io::stream_file> fifo;
  for (int i = 0; i != K; ++i)
    fifo.emplace_back(ctx);
  outcome rd[K];
  int raw[K];
  for (int i = 0; i != K; ++i) {
    std::string p = dir / ("fifo" + std::to_string(i));
    assert(::mkfifo(p.c_str(), 0600) == 0);
    assert(!fifo[i].open(p, io::file_base::read_write)); // does not block
    raw[i] = ::open(p.c_str(), O_WRONLY | O_NONBLOCK);
    assert(raw[i] >= 0);
  }
  for (int i = 0; i != K; ++i)
    spawn(ctx, read_one(fifo[i], rd[i]));

  io::random_access_file by_stop(ctx), by_cancel(ctx), by_close(ctx), pre_stopped(ctx), running_victim(ctx);
  for (io::random_access_file* f : {&by_stop, &by_cancel, &by_close, &pre_stopped, &running_victim})
    assert(!f->open(dir / "data", io::file_base::read_only));
  outcome o_stop, o_cancel, o_close, o_pre;
  std::stop_source src, src_pre;
  src_pre.request_stop();
  spawn(ctx, read_at_zero(by_stop, o_stop), src.get_token());
  spawn(ctx, read_at_zero(by_cancel, o_cancel));
  spawn(ctx, read_at_zero(by_close, o_close));
  spawn(ctx, read_at_zero(pre_stopped, o_pre), src_pre.get_token());

  bool checked = false;
  spawn(ctx,
        [](io::io_context& ctx,
           outcome* rd,
           int* raw,
           std::stop_source& src,
           io::random_access_file& by_cancel,
           io::random_access_file& by_close,
           outcome& o_stop,
           outcome& o_cancel,
           outcome& o_close,
           outcome& o_pre,
           bool& checked) -> io::task<void> {
          co_await pause(ctx, ms(150)); // everything is started; four threads are blocked in read(), the rest is queued
          assert(o_pre.done && o_pre.ec == io::error::canceled && o_pre.n == 0); // never queued
          assert(!o_stop.done && !o_cancel.done && !o_close.done);
          for (int i = 0; i != K; ++i)
            assert(!rd[i].done);
          src.request_stop();
          by_cancel.cancel();
          by_close.close();
          co_await pause(ctx, ms(50));
          assert(o_stop.done && o_stop.ec == io::error::canceled && o_stop.n == 0);
          assert(o_cancel.done && o_cancel.ec == io::error::canceled);
          assert(o_close.done && o_close.ec == io::error::canceled);
          for (int i = 0; i != K; ++i)
            assert(!rd[i].done); // the calls in progress, and those that wait for a thread, are not affected
          // a later operation on the same file is not affected by cancel()
          assert(by_cancel.is_open() && !by_close.is_open());
          for (int i = 0; i != K; ++i)
            assert(::write(raw[i], "x", 1) == 1);
          checked = true;
        }(ctx, rd, raw, src, by_cancel, by_close, o_stop, o_cancel, o_close, o_pre, checked));

  {
    ctx.restart();
    watchdog wd(ctx, ms(8000));
    ctx.run();
  }
  assert(checked);
  for (int i = 0; i != K; ++i)
    assert(rd[i].done && !rd[i].ec && rd[i].n == 1);

  // after cancel() the file works
  {
    outcome o;
    run(ctx, read_at_zero(by_cancel, o));
    assert(o.done && !o.ec && o.n == 4);
  }

  for (int i = 0; i != K; ++i)
    ::close(raw[i]);
  return 0;
}
