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

// One io_context run by four threads: an acceptor loop that starts a task per connection and fifty simultaneous clients
// that each send a block, read it back and compare. The acceptor is cancelled from outside when the clients are done.

#include <net>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

constexpr int clients     = 50;
constexpr std::size_t blk = 100000;

std::atomic<int> served{0}, finished_clients{0}, failures{0};

io::task<void> handler(io::tcp_socket s) {
  std::vector<char> buf(blk);
  auto [rec, rn] = co_await io::read(s, io::make_buffer(buf.data(), buf.size()));
  if (rec || rn != blk) {
    ++failures;
    co_return;
  }
  auto [wec, wn] = co_await io::write(s, io::make_buffer(static_cast<const void*>(buf.data()), buf.size()));
  if (wec || wn != blk)
    ++failures;
  ++served;
}

io::task<void> acceptor_loop(io::io_context& ctx, io::tcp_acceptor& acc) {
  for (;;) {
    auto [ec, s] = co_await acc.accept();
    if (ec) {
      if (ec != io::cond::canceled)
        ++failures;
      co_return;
    }
    io::run_async(ctx.get_executor())(handler(std::move(s)));
  }
}

io::task<void> client(io::io_context& ctx, io::endpoint server, int id, io::tcp_acceptor& acc) {
  io::tcp_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  if (cec) {
    ++failures;
  } else {
    std::string data(blk, static_cast<char>('a' + id % 26));
    data[0] = static_cast<char>(id);
    std::string back(blk, '\0');
    auto [wec, wn] = co_await io::write(s, cbuf(data));
    auto [rec, rn] = co_await io::read(s, io::make_buffer(back.data(), back.size()));
    if (wec || rec || wn != blk || rn != blk || back != data)
      ++failures;
  }
  if (++finished_clients == clients)
    acc.cancel();
}

int main(int, char**) {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback(), 128);
  io::endpoint ep = acc.local_endpoint();
  io::run_async(ctx.get_executor())(acceptor_loop(ctx, acc));
  for (int i = 0; i != clients; ++i)
    io::run_async(ctx.get_executor())(client(ctx, ep, i, acc));
  watchdog wd(ctx, ms(15000));
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
