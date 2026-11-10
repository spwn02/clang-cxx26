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

// Synchronous streams: a stream whose operations return immediately-ready awaitables satisfies ReadStream/WriteStream,
// never suspends the awaiting task, and works through the composed read/write and through the type-erasing wrappers.

#include <io_stream>

#include <array>
#include <cassert>
#include <string>
#include <vector>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;
using test_io::test_context;

// A task that counts how often it was resumed from the run loop: a task that never suspends runs to completion in the
// single resumption that starts it.
io::task<void> direct_read(memory_read_stream& s, std::string& out, std::error_code* ecs) {
  char buf[4];
  for (;;) {
    auto [ec, n] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
    out.append(buf, n);
    if (ec) {
      *ecs = ec;
      co_return;
    }
  }
}

io::task<void> via_any_read(io::any_read_stream& s, std::string& out, std::error_code* ecs) {
  char buf[5];
  for (;;) {
    auto [ec, n] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
    out.append(buf, n);
    if (ec) {
      *ecs = ec;
      co_return;
    }
  }
}

io::task<void> via_any_write(io::any_write_stream& s, std::string_view text, std::size_t* total) {
  auto [ec, n] = co_await s.write_some(io::const_buffer(text.data(), text.size()));
  assert(!ec);
  *total = n;
}

io::task<void> read_exact(memory_read_stream& s, std::string& out, std::error_code* ecp, std::size_t* np, std::size_t want) {
  out.resize(want);
  auto [ec, n] = co_await io::read(s, io::mutable_buffer(out.data(), want));
  *ecp         = ec;
  *np          = n;
}

io::task<void> write_all(memory_write_stream& s, std::string_view text, std::error_code* ecp, std::size_t* np) {
  auto [ec, n] = co_await io::write(s, io::const_buffer(text.data(), text.size()));
  *ecp         = ec;
  *np          = n;
}

int main(int, char**) {
  using namespace std::string_view_literals;

  // A task awaiting a synchronous stream is never suspended: ctx.posted counts only the start.
  {
    test_context ctx;
    memory_read_stream s{.data = "hello, synchronous world"sv, .chunk = 7};
    std::string out;
    std::error_code ec;
    auto t = direct_read(s, out, &ec);
    test_io::run_root(ctx, t);
    assert(out == "hello, synchronous world");
    assert(ec == io::cond::eof);
    assert(ec == io::error::eof);
    assert(ctx.posted == 1);
  }

  // Through any_read_stream: the ready awaitable still never suspends the task.
  {
    test_context ctx;
    memory_read_stream s{.data = "wrapped but still synchronous"sv, .chunk = 6};
    io::any_read_stream w(&s);
    assert(w.has_value());
    std::string out;
    std::error_code ec;
    auto t = via_any_read(w, out, &ec);
    test_io::run_root(ctx, t);
    assert(out == "wrapped but still synchronous");
    assert(ec == io::cond::eof);
    assert(ctx.posted == 1);
    assert(s.calls == 7); // 6 data reads (29 bytes, 5 at a time) + 1 eof read
  }

  // The write direction.
  {
    test_context ctx;
    memory_write_stream s;
    io::any_write_stream w(&s);
    std::size_t total = 0;
    auto t            = via_any_write(w, "abcdef"sv, &total);
    test_io::run_root(ctx, t);
    assert(total == 6 && s.out == "abcdef");
    assert(ctx.posted == 1);
  }

  // read(): loops over partial reads until the buffer is full.
  {
    test_context ctx;
    memory_read_stream s{.data = "0123456789abcdef"sv, .chunk = 3};
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = read_exact(s, out, &ec, &n, 10);
    test_io::run_root(ctx, t);
    assert(!ec && n == 10 && out == "0123456789");
    assert(s.calls == 4); // 3 + 3 + 3 + 1
    assert(ctx.posted == 1);
  }

  // read(): end of stream before the buffer is full reports eof with the bytes transferred.
  {
    test_context ctx;
    memory_read_stream s{.data = "short"sv, .chunk = 2};
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = read_exact(s, out, &ec, &n, 12);
    test_io::run_root(ctx, t);
    assert(ec == io::cond::eof && n == 5);
    assert(out.substr(0, 5) == "short");
  }

  // read(): a different error is propagated with the partial count; a full buffer is a success even if the stream
  // would report an error on its next call.
  {
    test_context ctx;
    memory_read_stream s{.data = "0123456789"sv, .chunk = 4, .fail_at = 6, .fail_ec = std::make_error_code(std::errc::connection_reset)};
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = read_exact(s, out, &ec, &n, 9);
    test_io::run_root(ctx, t);
    assert(ec == std::errc::connection_reset && n == 6);
  }
  {
    test_context ctx;
    memory_read_stream s{.data = "0123456789"sv, .chunk = 4, .fail_at = 6, .fail_ec = std::make_error_code(std::errc::connection_reset)};
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = read_exact(s, out, &ec, &n, 6);
    test_io::run_root(ctx, t);
    assert(!ec && n == 6 && out == "012345");
  }

  // read(): an empty buffer completes without calling the stream.
  {
    test_context ctx;
    memory_read_stream s{.data = "x"sv};
    std::string out;
    std::error_code ec;
    std::size_t n = 99;
    auto t        = read_exact(s, out, &ec, &n, 0);
    test_io::run_root(ctx, t);
    assert(!ec && n == 0 && s.calls == 0);
  }

  // write(): loops over partial writes.
  {
    test_context ctx;
    memory_write_stream s;
    s.chunk = 4;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = write_all(s, "the quick brown fox"sv, &ec, &n);
    test_io::run_root(ctx, t);
    assert(!ec && n == 19 && s.out == "the quick brown fox");
    assert(s.calls == 5);
  }

  // write(): an error stops the loop and reports the bytes written so far.
  {
    test_context ctx;
    memory_write_stream s;
    s.chunk   = 4;
    s.fail_at = 10;
    s.fail_ec = std::make_error_code(std::errc::broken_pipe);
    std::error_code ec;
    std::size_t n = 0;
    auto t        = write_all(s, "the quick brown fox"sv, &ec, &n);
    test_io::run_root(ctx, t);
    assert(ec == std::errc::broken_pipe && n == 10 && s.out == "the quick ");
  }

  // Multi-buffer sequences with a partial stream: read() and write() resume in the middle of a buffer.
  {
    test_context ctx;
    memory_read_stream s{.data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv, .chunk = 5};
    char a[3] = {}, b[8] = {}, c[6] = {};
    std::array<io::mutable_buffer, 3> bufs{io::mutable_buffer(a, 3), io::mutable_buffer(b, 8), io::mutable_buffer(c, 6)};
    std::error_code ec;
    std::size_t n = 0;
    auto task     = [](memory_read_stream& rs, const std::array<io::mutable_buffer, 3>& bf, std::error_code* ecp,
                   std::size_t* np) -> io::task<void> {
      auto [rec, rn] = co_await io::read(rs, bf);
      *ecp           = rec;
      *np            = rn;
    }(s, bufs, &ec, &n);
    test_io::run_root(ctx, task);
    assert(!ec && n == 17);
    assert(std::string(a, 3) == "ABC" && std::string(b, 8) == "DEFGHIJK" && std::string(c, 6) == "LMNOPQ");
  }
  {
    test_context ctx;
    memory_write_stream s;
    s.chunk = 4;
    std::array<io::const_buffer, 3> bufs{io::const_buffer("aa", 2), io::const_buffer("bbbbb", 5), io::const_buffer("ccc", 3)};
    std::error_code ec;
    std::size_t n = 0;
    auto task     = [](memory_write_stream& ws, const std::array<io::const_buffer, 3>& bf, std::error_code* ecp,
                   std::size_t* np) -> io::task<void> {
      auto [wec, wn] = co_await io::write(ws, bf);
      *ecp           = wec;
      *np            = wn;
    }(s, bufs, &ec, &n);
    test_io::run_root(ctx, task);
    assert(!ec && n == 10 && s.out == "aabbbbbccc");
  }

  // read() into a DynamicBuffer reads until end of stream, which is a success.
  {
    test_context ctx;
    std::string payload(5000, 'x');
    for (std::size_t i = 0; i != payload.size(); ++i)
      payload[i] = char('a' + i % 26);
    memory_read_stream s{.data = payload, .chunk = 777};
    std::string sink;
    std::error_code ec;
    std::size_t n = 0;
    auto task     = [](memory_read_stream& rs, std::string& str, std::error_code* ecp, std::size_t* np) -> io::task<void> {
      io::string_dynamic_buffer db(str);
      auto [rec, rn] = co_await io::read(rs, db);
      *ecp           = rec;
      *np            = rn;
    }(s, sink, &ec, &n);
    test_io::run_root(ctx, task);
    assert(!ec && n == 5000 && sink == payload);
  }
  // ... and a full dynamic buffer is reported as errc::no_buffer_space.
  {
    test_context ctx;
    std::string payload(100, 'z');
    memory_read_stream s{.data = payload};
    std::string sink;
    std::error_code ec;
    std::size_t n = 0;
    auto task     = [](memory_read_stream& rs, std::string& str, std::error_code* ecp, std::size_t* np) -> io::task<void> {
      io::string_dynamic_buffer db(str, 40);
      auto [rec, rn] = co_await io::read(rs, db, 16);
      *ecp           = rec;
      *np            = rn;
    }(s, sink, &ec, &n);
    test_io::run_root(ctx, task);
    assert(ec == std::errc::no_buffer_space && n == 40 && sink == std::string(40, 'z'));
  }

  return 0;
}
