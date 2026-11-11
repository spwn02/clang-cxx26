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

// <net_tls>

// 50 concurrent TLS sessions on 4 threads of one io_context, all created from the same two tls_contexts (the native
// context is built once, by whichever handshake comes first).

#include <net_tls>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "test_macros.h"
#include "test_tls.h"

using namespace test_tls;

constexpr int clients     = 50;
constexpr std::size_t blk = 60000;

std::atomic<int> served{0}, finished_clients{0}, failures{0};

io::task<void> handler(io::tcp_socket s, const io::tls_context* sc) {
  io::tls_stream tls(&s, *sc);
  auto [hec] = co_await tls.handshake(io::tls_role::server);
  if (hec) {
    ++failures;
    co_return;
  }
  io::any_stream as(&tls);
  std::string buf(blk, '\0');
  if (co_await read_exact(as, buf.data(), buf.size())) {
    ++failures;
    co_return;
  }
  if (co_await write_all(as, buf.data(), buf.size())) {
    ++failures;
    co_return;
  }
  char c;
  auto [rec, rn] = co_await as.read_some(io::mutable_buffer(&c, 1));
  if (rec != io::error::eof || rn != 0) {
    ++failures;
    co_return;
  }
  auto [sec] = co_await tls.shutdown();
  if (sec)
    ++failures;
  ++served;
}

io::task<void> acceptor_loop(io::io_context& ctx, io::tcp_acceptor& acc, const io::tls_context* sc) {
  for (;;) {
    auto [ec, s] = co_await acc.accept();
    if (ec) {
      if (ec != io::cond::canceled)
        ++failures;
      co_return;
    }
    io::run_async(ctx.get_executor())(handler(std::move(s), sc));
  }
}

io::task<void> client(io::io_context& ctx, io::endpoint server, int id, io::tcp_acceptor& acc, const io::tls_context* cc) {
  io::tcp_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  if (cec) {
    ++failures;
  } else {
    io::tls_stream tls(&s, *cc);
    tls.set_hostname("localhost");
    auto [hec] = co_await tls.handshake(io::tls_role::client);
    if (hec) {
      ++failures;
    } else {
      io::any_stream as(&tls);
      std::string data = pattern(blk, static_cast<unsigned>(id));
      std::string back(blk, '\0');
      auto [w, r] = co_await io::when_all(write_all(as, data.data(), data.size()), read_exact(as, back.data(), back.size()));
      auto [sec]  = co_await tls.shutdown();
      if (w || r || back != data || sec)
        ++failures;
    }
  }
  if (++finished_clients == clients)
    acc.cancel();
}

int main(int, char**) {
  TLS_REQUIRE_ENGINE();
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback(), 128);
  io::endpoint ep    = acc.local_endpoint();
  io::tls_context sc = server_context();
  io::tls_context cc = client_context();
  io::run_async(ctx.get_executor())(acceptor_loop(ctx, acc, &sc));
  for (int i = 0; i != clients; ++i)
    io::run_async(ctx.get_executor())(client(ctx, ep, i, acc, &cc));
  watchdog wd(ctx, ms(20000));
  std::vector<std::thread> threads;
  for (int i = 0; i != 4; ++i)
    threads.emplace_back([&] { ctx.run(); });
  for (auto& t : threads)
    t.join();
  assert(failures.load() == 0);
  assert(finished_clients.load() == clients);
  assert(served.load() == clients);
  return 0;
}
