//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++29
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_stream>

// A Stream pipeline: a tcp-like mock stream (reads alternate between completing inline and suspending, writes always
// suspend) is wrapped in an any_stream; a non-template function that takes any_stream& echoes what it reads, using
// the composed read/write algorithms on the wrapper.

#include <io_stream>

#include <atomic>
#include <cassert>
#include <string>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;
using test_io::test_context;

// Compiled against the erased type only: this is the ABI firewall of the design.
io::io_task<std::size_t> echo_upper(io::any_stream& s) {
  std::size_t total = 0;
  char buf[10];
  for (;;) {
    auto [ec, n] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
    for (std::size_t i = 0; i != n; ++i)
      if (buf[i] >= 'a' && buf[i] <= 'z')
        buf[i] = char(buf[i] - 'a' + 'A');
    if (n != 0) {
      auto [wec, wn] = co_await io::write(s, io::const_buffer(buf, n)); // composed write over the wrapper
      total += wn;
      if (wec)
        co_return io::io_result<std::size_t>{wec, total};
    }
    if (ec == io::cond::eof)
      co_return io::io_result<std::size_t>{std::error_code{}, total};
    if (ec)
      co_return io::io_result<std::size_t>{ec, total};
  }
}

io::io_task<std::size_t> slurp(io::any_stream& s, std::string& out) {
  io::string_dynamic_buffer db(out);
  co_return co_await io::read(s, db, 8);
}

io::task<void> serve(mock_tcp& tcp, std::error_code* ec, std::size_t* n) {
  io::any_stream s(&tcp);
  auto [e, c] = co_await echo_upper(s);
  *ec         = e;
  *n          = c;
}

io::task<void> serve_slurp(mock_tcp& tcp, std::error_code* ec, std::size_t* n, std::string* out) {
  io::any_stream s(&tcp);
  auto [e, c] = co_await slurp(s, *out);
  *ec         = e;
  *n          = c;
}

int main(int, char**) {
  using namespace std::string_view_literals;

  {
    test_context ctx;
    mock_tcp tcp;
    tcp.rd.data  = "hello from the client, in lower case"sv;
    tcp.rd.chunk = 7;
    std::error_code ec = std::make_error_code(std::errc::io_error);
    std::size_t n      = 0;
    auto t             = serve(tcp, &ec, &n);
    test_io::run_root(ctx, t);
    assert(!ec);
    assert(n == tcp.rd.data.size());
    assert(tcp.wr.out == "HELLO FROM THE CLIENT, IN LOWER CASE");
    assert(ctx.posted > 6);
  }

  // Errors on the underlying stream propagate through the wrapper and the algorithms.
  {
    test_context ctx;
    mock_tcp tcp;
    tcp.rd.data    = "0123456789"sv;
    tcp.rd.fail_at = 4;
    tcp.rd.fail_ec = std::make_error_code(std::errc::connection_reset);
    std::error_code ec;
    std::size_t n = 0;
    auto t        = serve(tcp, &ec, &n);
    test_io::run_root(ctx, t);
    assert(ec == std::errc::connection_reset && n == 4 && tcp.wr.out == "0123");
  }
  {
    test_context ctx;
    mock_tcp tcp;
    tcp.rd.data   = "0123456789"sv;
    tcp.wr.fail_at = 3;
    tcp.wr.fail_ec = std::make_error_code(std::errc::broken_pipe);
    std::error_code ec;
    std::size_t n = 0;
    auto t        = serve(tcp, &ec, &n);
    test_io::run_root(ctx, t);
    assert(ec == std::errc::broken_pipe && tcp.wr.out == "012");
  }

  // read() into a dynamic buffer through the wrapper.
  {
    test_context ctx;
    mock_tcp tcp;
    tcp.rd.data  = "a request of unknown length that is read until the peer closes"sv;
    tcp.rd.chunk = 9;
    std::string out;
    std::error_code ec = std::make_error_code(std::errc::io_error);
    std::size_t n      = 0;
    auto t             = serve_slurp(tcp, &ec, &n, &out);
    test_io::run_root(ctx, t);
    assert(!ec && out == tcp.rd.data && n == out.size());
  }

  // The same pipeline on a thread_pool, many connections at once.
  {
    io::thread_pool pool(4);
    constexpr int N = 32;
    mock_tcp tcps[N];
    std::error_code ecs[N];
    std::size_t ns[N] = {};
    std::atomic<int> done{0};
    for (int i = 0; i != N; ++i) {
      tcps[i].rd.data  = "concurrent connection payload";
      tcps[i].rd.chunk = 1 + i % 5;
      io::run_async(pool.get_executor(), [&] { ++done; }, [](std::exception_ptr) { assert(false); })(
          serve(tcps[i], &ecs[i], &ns[i]));
    }
    pool.join();
    assert(done == N);
    for (int i = 0; i != N; ++i)
      assert(!ecs[i] && ns[i] == 29 && tcps[i].wr.out == "CONCURRENT CONNECTION PAYLOAD");
  }

  return 0;
}
