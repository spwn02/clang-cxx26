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

// <net>

// cancel() and close() of a socket, and cancel() of an acceptor, with an executor whose post() resumes the coroutine at
// once, inside the call: the completion of the cancelled operation unregisters its wait from the socket, which must not
// find the socket locked by the cancellation (it deadlocked), and several operations pending at the same time are all
// cancelled.

#include <net>

#include <atomic>
#include <cassert>
#include <coroutine>
#include <thread>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

struct inline_executor {
  io::io_context* c;
  bool operator==(const inline_executor&) const noexcept { return true; }
  io::io_context& context() const noexcept { return *c; }
  void on_work_started() const noexcept {}
  void on_work_finished() const noexcept {}
  std::coroutine_handle<> dispatch(io::continuation& k) const { return k.h; }
  void post(io::continuation& k) const { k.h.resume(); }
};

struct result_t {
  std::atomic<int> finished{0};
  std::error_code ec[4];
};

io::task<void> reader(io::tcp_socket* s, result_t* r, int i) {
  char b[4];
  auto [ec, n] = co_await s->read_some(io::make_buffer(b, 4));
  r->ec[i]     = ec;
  ++r->finished;
}
io::task<void> acceptor_waiter(io::tcp_acceptor* a, result_t* r, int i) {
  auto [ec, s] = co_await a->accept();
  r->ec[i]     = ec;
  ++r->finished;
}
io::task<void> udp_reader(io::udp_socket* s, result_t* r, int i) {
  char b[4];
  io::endpoint from;
  auto [ec, n] = co_await s->receive_from(io::make_buffer(b, 4), from);
  r->ec[i]     = ec;
  ++r->finished;
}

static void connect_pair(io::io_context& ctx, io::tcp_acceptor& acc, io::tcp_socket& a, io::tcp_socket& b) {
  run(ctx, [](io::tcp_acceptor& x, io::tcp_socket* c, io::tcp_socket* s) -> io::task<void> {
    auto [r1, r2] = co_await io::when_all(
        [](io::tcp_acceptor& y) -> io::task<io::tcp_socket> {
          auto [ec, sock] = co_await y.accept();
          assert(!ec);
          co_return std::move(sock);
        }(x),
        c->connect(x.local_endpoint()));
    *s = std::move(r1);
  }(acc, &a, &b));
  ctx.restart();
}

static void expect_canceled(const result_t& r, int n) {
  assert(r.finished == n);
  for (int i = 0; i != n; ++i)
    assert(r.ec[i] == io::error::canceled);
}

static void test_cancel_and_close() {
  for (int what = 0; what != 2; ++what) {
    io::io_context ctx;
    io::tcp_acceptor acc(ctx, loopback());
    io::tcp_socket a(ctx), b(ctx);
    connect_pair(ctx, acc, a, b);
    result_t r;
    // Three reads pending at the same time.
    for (int i = 0; i != 3; ++i)
      io::run_async(inline_executor{&ctx})(reader(&b, &r, i));
    assert(r.finished == 0 && b.__has_waits());
    if (what == 0)
      b.cancel();
    else
      b.close();
    expect_canceled(r, 3);
    assert(!b.__has_waits());
    // The operations that are started later are not affected: a read after cancel() completes with data.
    if (what == 0) {
      io::run_async(inline_executor{&ctx})(reader(&b, &r, 3));
      assert(r.finished == 3);
      std::string msg = "ping";
      run(ctx, [](io::tcp_socket* s, std::string* m) -> io::task<void> {
        (void)co_await s->write_some(cbuf(*m));
      }(&a, &msg));
      ctx.restart();
      ctx.poll();
      assert(r.finished == 4 && !r.ec[3]);
    }
  }
}

static void test_cancel_acceptor() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  result_t r;
  io::run_async(inline_executor{&ctx})(acceptor_waiter(&acc, &r, 0));
  io::run_async(inline_executor{&ctx})(acceptor_waiter(&acc, &r, 1));
  assert(r.finished == 0);
  acc.cancel();
  expect_canceled(r, 2);
  assert(!acc.__has_waits());
}

static void test_cancel_udp() {
  io::io_context ctx;
  io::udp_socket s(ctx);
  assert(!s.open() && !s.bind(loopback()));
  result_t r;
  io::run_async(inline_executor{&ctx})(udp_reader(&s, &r, 0));
  io::run_async(inline_executor{&ctx})(udp_reader(&s, &r, 1));
  assert(r.finished == 0);
  s.cancel();
  expect_canceled(r, 2);
  assert(!s.__has_waits());
}

// A cancel() in one thread while the awaiting coroutines are resumed (and destroyed) by another: the unregistration
// waits for the call of the cancellation that reaches its operation.
static void test_cancel_concurrent() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  io::tcp_socket a(ctx), b(ctx);
  connect_pair(ctx, acc, a, b);
  for (int round = 0; round != 200; ++round) {
    result_t r;
    for (int i = 0; i != 3; ++i)
      io::run_async(inline_executor{&ctx})(reader(&b, &r, i));
    std::thread t([&] { b.cancel(); });
    b.cancel();
    t.join();
    expect_canceled(r, 3);
  }
}

int main(int, char**) {
  test_cancel_and_close();
  test_cancel_acceptor();
  test_cancel_udp();
  test_cancel_concurrent();
  return 0;
}
