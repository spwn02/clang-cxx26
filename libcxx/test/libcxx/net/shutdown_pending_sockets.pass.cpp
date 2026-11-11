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

// A context that is destroyed while an operation is pending on a socket destroys the chain of the coroutine, and the
// wait that the operation had registered at the socket is gone with it: the socket does not keep a link to the frame
// that was destroyed (the destruction of the socket, or its cancel(), walked freed memory). Checked with the private
// __has_waits() of the socket, for a TCP read, accept and connect, a UDP receive_from and a local stream read, and with
// the socket owned by the destroyed frame itself (the sanitizers find the use after free there; the count of the
// destroyed frames is checked either way).

#include <net>

#include <atomic>
#include <cassert>
#include <fcntl.h>
#include <memory>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "test_macros.h"
#include "../../std/net/test_net.h"

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

io::task<void> make_tcp_pair(io::tcp_acceptor& acc, io::tcp_socket* client, io::tcp_socket* server) {
  auto [r1, r2] = co_await io::when_all(
      [](io::tcp_acceptor& a) -> io::task<io::tcp_socket> {
        auto [ec, s] = co_await a.accept();
        assert(!ec);
        co_return std::move(s);
      }(acc),
      client->connect(acc.local_endpoint()));
  assert(!std::get<0>(r2));
  *server = std::move(r1);
}

io::task<void> tcp_read(io::tcp_socket* s, sentinel) {
  char b[8];
  (void)co_await s->read_some(io::make_buffer(b, 8));
}
io::task<void> tcp_accept(io::tcp_acceptor* a, sentinel) { (void)co_await a->accept(); }
io::task<void> tcp_connect(io::tcp_socket* s, io::endpoint ep, sentinel) { (void)co_await s->connect(ep); }
io::task<void> udp_receive(io::udp_socket* s, sentinel) {
  char b[8];
  io::endpoint from;
  (void)co_await s->receive_from(io::make_buffer(b, 8), from);
}
io::task<void> local_read(io::local_stream_socket* s, sentinel) {
  char b[8];
  (void)co_await s->read_some(io::make_buffer(b, 8));
}
// The socket is a local of the frame, the read is awaited in a child frame.
io::task<void> child_read(io::tcp_socket* s, sentinel) {
  char b[8];
  (void)co_await s->read_some(io::make_buffer(b, 8));
}
io::task<void> owner_of_socket(io::io_context& ctx, io::tcp_acceptor* acc, sentinel, sentinel) {
  io::tcp_socket c(ctx);
  auto [ec] = co_await c.connect(acc->local_endpoint());
  assert(!ec);
  char b[8];
  (void)co_await c.read_some(io::make_buffer(b, 8));
}

static void poll_some(io::io_context& ctx) {
  for (int i = 0; i != 20; ++i)
    ctx.poll();
}

static void test_tcp_read() {
  std::atomic<int> d{0};
  std::optional<io::io_context> ctx;
  ctx.emplace();
  io::tcp_acceptor acc(*ctx, loopback());
  io::tcp_socket client(*ctx), server(*ctx);
  run(*ctx, make_tcp_pair(acc, &client, &server));
  ctx->restart();
  io::run_async(ctx->get_executor())(tcp_read(&server, sentinel(&d)));
  poll_some(*ctx);
  assert(server.__has_waits());
  ctx.reset();
  assert(d == 1);
  assert(!server.__has_waits());
}

static void test_tcp_accept() {
  std::atomic<int> d{0};
  std::optional<io::io_context> ctx;
  ctx.emplace();
  io::tcp_acceptor acc(*ctx, loopback());
  io::run_async(ctx->get_executor())(tcp_accept(&acc, sentinel(&d)));
  poll_some(*ctx);
  assert(acc.__has_waits());
  ctx.reset();
  assert(d == 1);
  assert(!acc.__has_waits());
}

static void test_tcp_connect() {
#if defined(__linux__)
  // A listening socket whose accept queue is full drops the connection requests: the connects stay in progress.
  std::atomic<int> d{0};
  std::optional<io::io_context> ctx;
  ctx.emplace();
  io::tcp_acceptor acc(*ctx, loopback(), 1);
  std::vector<std::unique_ptr<io::tcp_socket>> socks;
  for (int i = 0; i != 8; ++i) {
    socks.push_back(std::make_unique<io::tcp_socket>(*ctx));
    io::run_async(ctx->get_executor())(tcp_connect(socks.back().get(), acc.local_endpoint(), sentinel(&d)));
  }
  poll_some(*ctx);
  int pending = 0;
  for (auto& s : socks)
    pending += s->__has_waits();
  assert(pending > 0);
  ctx.reset();
  assert(d == 8);
  for (auto& s : socks)
    assert(!s->__has_waits());
#endif
}

static void test_udp_receive() {
  std::atomic<int> d{0};
  std::optional<io::io_context> ctx;
  ctx.emplace();
  io::udp_socket s(*ctx);
  assert(!s.open() && !s.bind(loopback()));
  io::run_async(ctx->get_executor())(udp_receive(&s, sentinel(&d)));
  poll_some(*ctx);
  assert(s.__has_waits());
  ctx.reset();
  assert(d == 1);
  assert(!s.__has_waits());
}

static void test_local_read() {
  std::atomic<int> d{0};
  int fds[2];
  assert(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
  for (int fd : fds) {
    assert(::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK) == 0);
    assert(::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0);
  }
  {
    std::optional<io::io_context> ctx;
    ctx.emplace();
    io::local_stream_socket a(*ctx), b(*ctx);
    assert(!a.assign(fds[0]) && !b.assign(fds[1]));
    io::run_async(ctx->get_executor())(local_read(&a, sentinel(&d)));
    poll_some(*ctx);
    assert(a.__has_waits());
    ctx.reset();
    assert(d == 1);
    assert(!a.__has_waits());
  }
}

static void test_socket_owned_by_frame() {
  for (int round = 0; round != 20; ++round) {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      io::tcp_acceptor acc(ctx, loopback());
      io::run_async(ctx.get_executor())(owner_of_socket(ctx, &acc, sentinel(&d), sentinel(&d)));
      poll_some(ctx);
    }
    assert(d == 2);
  }
}

int main(int, char**) {
  test_tcp_read();
  test_tcp_accept();
  test_tcp_connect();
  test_udp_receive();
  test_local_read();
  test_socket_owned_by_frame();
  return 0;
}
