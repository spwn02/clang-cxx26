//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library
// REQUIRES: linux || darwin || freebsd || windows

// The documented usage pattern of the library (P4100R1 section 13): coroutines taking any_stream&, an echo server and
// clients on one io_context. Also checks that io::task<> means task<void>.

#include <io>
#include <io_context>
#include <io_stream>
#include <io_timer>
#include <net>
#include <cassert>
#include <string>

using namespace std::io;

task<> handle(any_stream& s) {
  char buf[256];
  for (;;) {
    auto [ec, n] = co_await s.read_some(mutable_buffer(buf, sizeof buf));
    if (ec) co_return;
    auto [wec, wn] = co_await write(s, const_buffer(buf, n));
    if (wec) co_return;
  }
}

task<> serve(tcp_acceptor& acc, int clients) {
  for (int i = 0; i < clients; ++i) {
    auto [ec, sock] = co_await acc.accept();
    if (ec) co_return;
    any_stream s(&sock);
    co_await handle(s);
  }
}

task<> client(io_context& ctx, endpoint ep, std::string msg, int& ok) {
  tcp_socket sock(ctx);
  auto [cec] = co_await sock.connect(ep);
  if (cec) co_return;
  any_stream s(&sock);
  auto [wec, wn] = co_await write(s, const_buffer(msg.data(), msg.size()));
  std::string got(msg.size(), '\0');
  auto [rec, rn] = co_await read(s, mutable_buffer(got.data(), got.size()));
  ok += (!wec && !rec && got == msg);
  sock.close();
}

int main() {
  io_context ctx;
  tcp_acceptor acc(ctx, endpoint(ipv4_address::loopback(), 0));
  endpoint ep = acc.local_endpoint();
  int ok = 0;
  auto ex = ctx.get_executor();
  run_async(ex)(serve(acc, 3));
  for (int i = 0; i < 3; ++i)
    run_async(ex)(client(ctx, ep, "hello " + std::to_string(i), ok));
  ctx.run();
  assert(ok == 3);
  return 0;
}
