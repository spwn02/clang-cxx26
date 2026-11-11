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

// The kernel cancels the io_uring requests of a task when the task exits. A transfer that was submitted by a thread
// that runs the context for a moment and then ends must not fail with a cancellation: it continues (on the readiness
// path) and completes with the data, on whichever backend the context runs.

#include <io>
#include <io_buffer>
#include <io_context>
#include <io_stream>
#include <net>

#include <cassert>
#include <string>
#include <thread>

namespace io = std::io;

struct state {
  std::error_code ec = std::make_error_code(std::errc::invalid_argument);
  std::string got;
  bool done = false;
};

static io::task<void> reader(io::tcp_socket* s, state* st) {
  char b[16];
  auto [ec, n] = co_await s->read_some(io::make_buffer(b, sizeof b));
  st->ec       = ec;
  st->got.assign(b, n);
  st->done = true;
}

static io::task<void> accept_and_connect(io::io_context& ctx, io::tcp_acceptor& acc, io::tcp_socket* client,
                                         io::tcp_socket* server) {
  auto [r1, r2] = co_await io::when_all(
      [](io::tcp_acceptor& a) -> io::task<io::tcp_socket> {
        auto [ec, sock] = co_await a.accept();
        assert(!ec);
        co_return std::move(sock);
      }(acc),
      client->connect(acc.local_endpoint()));
  *server = std::move(r1);
  (void)ctx;
}

int main(int, char**) {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, io::endpoint(io::ipv4_address::loopback(), 0));
  io::tcp_socket client(ctx), server(ctx);
  io::run_async(ctx.get_executor())(accept_and_connect(ctx, acc, &client, &server));
  ctx.run();
  ctx.restart();
  assert(client.is_open() && server.is_open());

  state st;
  // The read is started (and, on the ring, submitted) by a thread that ends right after.
  std::thread t([&] {
    io::run_async(ctx.get_executor())(reader(&server, &st));
    ctx.poll();
  });
  t.join();
  assert(!st.done);

  std::string msg = "after the thread ended";
  io::run_async(ctx.get_executor())([](io::tcp_socket* c, std::string* m) -> io::task<void> {
    auto [ec, n] = co_await c->write_some(io::const_buffer(m->data(), 16));
    assert(!ec && n == 16);
  }(&client, &msg));
  ctx.run();
  assert(st.done && !st.ec && st.got == msg.substr(0, 16));
  return 0;
}
