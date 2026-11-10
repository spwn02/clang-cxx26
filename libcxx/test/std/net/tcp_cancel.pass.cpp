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

// <net>

// Cancellation of pending socket operations: cancel() of the object, close() of the object and a stop request on the
// stop token of the awaiting coroutine. All of them complete with io::error::canceled (io::cond::canceled), later
// operations are not affected.

#include <net>

#include <stop_token>
#include <string>
#include <vector>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

// A connected pair of sockets on the context.
struct pair_t {
  io::tcp_socket client;
  io::tcp_socket server;
  explicit pair_t(io::io_context& ctx) : client(ctx), server(ctx) {}
};

io::task<void> make_pair(io::io_context& ctx, io::tcp_acceptor& acc, pair_t* p) {
  auto [r1, r2] = co_await io::when_all(
      [](io::tcp_acceptor& a) -> io::task<io::tcp_socket> {
        auto [ec, s] = co_await a.accept();
        assert(!ec);
        co_return std::move(s);
      }(acc),
      p->client.connect(acc.local_endpoint()));
  assert(!std::get<0>(r2));
  p->server = std::move(r1);
}

// --- cancel() of a pending accept; the acceptor works afterwards

io::task<void> accept_waiter(io::tcp_acceptor& acc, std::error_code* out, bool* finished) {
  auto [ec, s] = co_await acc.accept();
  *out         = ec;
  assert(!s.is_open());
  *finished = true;
}

io::task<void> cancel_accept_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  std::error_code ec;
  bool finished = false;
  io::run_async(ctx.get_executor())(accept_waiter(acc, &ec, &finished));
  co_await sleep_for(ctx, ms(30));
  assert(!finished);
  acc.cancel();
  co_await sleep_for(ctx, ms(30));
  assert(finished);
  assert(ec == io::error::canceled && ec == io::cond::canceled);
  assert(acc.is_open()); // cancel() does not close

  // The next accept is not affected.
  io::tcp_socket c(ctx);
  auto [r1, r2] = co_await io::when_all(acc.accept(), c.connect(acc.local_endpoint()));
  assert(!std::get<0>(r1) && !std::get<0>(r2));
}

static void test_cancel_accept() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, cancel_accept_body(ctx, acc));
}

// --- close() of the acceptor completes the pending accept

io::task<void> close_accept_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  std::error_code ec;
  bool finished = false;
  io::run_async(ctx.get_executor())(accept_waiter(acc, &ec, &finished));
  co_await sleep_for(ctx, ms(30));
  acc.close();
  assert(!acc.is_open());
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled);
}

static void test_close_accept() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, close_accept_body(ctx, acc));
}

// --- cancel() and close() of a pending read; the socket stays usable after cancel()

io::task<void> read_waiter(io::tcp_socket& s, std::error_code* out, std::size_t* n, bool* finished) {
  char buf[8];
  auto [ec, got] = co_await s.read_some(io::make_buffer(buf, sizeof buf));
  *out           = ec;
  *n             = got;
  *finished      = true;
}

io::task<void> cancel_read_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  pair_t p(ctx);
  co_await make_pair(ctx, acc, &p);
  std::error_code ec;
  std::size_t n = 99;
  bool finished = false;
  io::run_async(ctx.get_executor())(read_waiter(p.server, &ec, &n, &finished));
  co_await sleep_for(ctx, ms(30));
  assert(!finished);
  p.server.cancel();
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled && n == 0);
  assert(p.server.is_open());

  // Still usable: data written by the peer is read.
  auto [wec, wn] = co_await io::write(p.client, cbuf("ok"));
  assert(!wec);
  char buf[2];
  auto [rec, rn] = co_await io::read(p.server, io::make_buffer(buf, 2));
  assert(!rec && rn == 2);

  // close() cancels as well
  finished = false;
  io::run_async(ctx.get_executor())(read_waiter(p.server, &ec, &n, &finished));
  co_await sleep_for(ctx, ms(30));
  assert(!finished);
  p.server.close();
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled);
}

static void test_cancel_read() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, cancel_read_body(ctx, acc));
}

// --- cancel() of a pending write: fill the buffers of both sides until the write blocks

io::task<void> write_waiter(io::tcp_socket& s, std::error_code* out, std::size_t* total, bool* finished) {
  std::vector<char> chunk(256 * 1024, 'w');
  for (;;) {
    auto [ec, n] = co_await s.write_some(io::make_buffer(static_cast<const void*>(chunk.data()), chunk.size()));
    if (ec) {
      *out = ec;
      break;
    }
    *total += n;
  }
  *finished = true;
}

io::task<void> cancel_write_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  pair_t p(ctx);
  co_await make_pair(ctx, acc, &p);
  std::error_code ec;
  std::size_t total = 0;
  bool finished     = false;
  io::run_async(ctx.get_executor())(write_waiter(p.client, &ec, &total, &finished));
  co_await sleep_for(ctx, ms(100));
  assert(!finished && total > 0);
  p.client.cancel();
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled);
}

static void test_cancel_write() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, cancel_write_body(ctx, acc));
}

// --- stop token of the awaiting coroutine

io::task<void> stoppable_read(io::tcp_socket& s, std::error_code* out, bool* finished) {
  char buf[8];
  auto [ec, got] = co_await s.read_some(io::make_buffer(buf, sizeof buf));
  assert(got == 0);
  *out      = ec;
  *finished = true;
}

io::task<void> stop_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  pair_t p(ctx);
  co_await make_pair(ctx, acc, &p);
  std::stop_source src;
  std::error_code ec;
  bool finished = false;
  io::run_async(ctx.get_executor(), src.get_token())(stoppable_read(p.server, &ec, &finished));
  co_await sleep_for(ctx, ms(30));
  assert(!finished);
  src.request_stop();
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled);

  // An already stopped token: the read that would have to wait completes at once with canceled.
  std::stop_source stopped;
  stopped.request_stop();
  finished = false;
  ec.clear();
  io::run_async(ctx.get_executor(), stopped.get_token())(stoppable_read(p.server, &ec, &finished));
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled);

  // ... and an accept
  std::stop_source src2;
  io::tcp_acceptor acc2(ctx, loopback());
  std::error_code aec;
  bool afinished = false;
  io::run_async(ctx.get_executor(), src2.get_token())(accept_waiter(acc2, &aec, &afinished));
  co_await sleep_for(ctx, ms(20));
  src2.request_stop();
  co_await sleep_for(ctx, ms(30));
  assert(afinished && aec == io::cond::canceled);
}

static void test_stop_token() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, stop_body(ctx, acc));
}

// --- the io_context is not kept alive by cancelled operations: run() returns

int main(int, char**) {
  test_cancel_accept();
  test_close_accept();
  test_cancel_read();
  test_cancel_write();
  test_stop_token();
  return 0;
}
