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

// Suspending streams: operations that complete later through the executor, directly, through the composed read/write
// and through the type-erasing wrappers; cancellation through the stop token; the same on a thread_pool.

#include <io_stream>

#include <atomic>
#include <cassert>
#include <stop_token>
#include <string>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;
using test_io::test_context;

io::task<void> read_exact(io::any_read_stream& s, std::string& out, std::error_code* ecp, std::size_t* np, std::size_t want) {
  out.resize(want);
  auto [ec, n] = co_await io::read(s, io::mutable_buffer(out.data(), want));
  *ecp         = ec;
  *np          = n;
}

template <class S>
io::task<void> read_template(S& s, std::string& out, std::error_code* ecp, std::size_t* np, std::size_t want) {
  out.resize(want);
  auto [ec, n] = co_await io::read(s, io::mutable_buffer(out.data(), want));
  *ecp         = ec;
  *np          = n;
}

template <class S>
io::task<void> write_template(S& s, std::string_view text, std::error_code* ecp, std::size_t* np) {
  auto [ec, n] = co_await io::write(s, io::const_buffer(text.data(), text.size()));
  *ecp         = ec;
  *np          = n;
}

// First read_some succeeds, then the task requests stop; the next operation observes it and reports canceled.
io::task<void> cancel_midway(io::any_read_stream& s, std::stop_source& src, std::error_code* first, std::error_code* second,
                             std::size_t* n2) {
  char buf[4];
  auto [ec1, n1] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
  *first         = ec1;
  src.request_stop();
  auto [ec2, nn] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
  *second        = ec2;
  *n2            = nn;
}

int main(int, char**) {
  using namespace std::string_view_literals;

  // Every read_some suspends once through the executor: posted == start + one per operation.
  {
    test_context ctx;
    deferred_read_stream s;
    s.inner = memory_read_stream{.data = "0123456789"sv, .chunk = 4};
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = read_template(s, out, &ec, &n, 10);
    test_io::run_root(ctx, t);
    assert(!ec && n == 10 && out == "0123456789");
    assert(s.suspensions == 3 && ctx.posted == 1 + 3);
  }

  // The same through any_read_stream (no ready shortcut, the awaitable's await_suspend runs).
  {
    test_context ctx;
    deferred_read_stream s;
    s.inner = memory_read_stream{.data = "0123456789"sv, .chunk = 4};
    io::any_read_stream w(&s);
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = read_exact(w, out, &ec, &n, 10);
    test_io::run_root(ctx, t);
    assert(!ec && n == 10 && out == "0123456789");
    assert(s.suspensions == 3 && ctx.posted == 1 + 3);
  }

  // eof propagates through the wrapper as the portable condition.
  {
    test_context ctx;
    deferred_read_stream s;
    s.inner = memory_read_stream{.data = "abc"sv};
    io::any_read_stream w(&s);
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = read_exact(w, out, &ec, &n, 8);
    test_io::run_root(ctx, t);
    assert(ec == io::cond::eof && ec == io::error::eof && ec.category() == io::io_category() && n == 3);
  }

  // Writes.
  {
    test_context ctx;
    deferred_write_stream s;
    s.inner.chunk = 5;
    std::error_code ec;
    std::size_t n = 0;
    auto t        = write_template(s, "0123456789ABCDEF"sv, &ec, &n);
    test_io::run_root(ctx, t);
    assert(!ec && n == 16 && s.inner.out == "0123456789ABCDEF");
    assert(s.suspensions == 4);
  }

  // Cancellation: the stream sees the stop token of the environment and reports canceled.
  {
    test_context ctx;
    deferred_read_stream s;
    s.inner = memory_read_stream{.data = "0123456789"sv, .chunk = 4};
    std::stop_source src;
    src.request_stop();
    std::string out;
    std::error_code ec;
    std::size_t n = 99;
    auto t        = read_template(s, out, &ec, &n, 10);
    test_io::run_root(ctx, t, src.get_token());
    assert(ec == io::cond::canceled && ec == io::error::canceled && n == 0);
  }
  {
    test_context ctx;
    deferred_read_stream s;
    s.inner = memory_read_stream{.data = "0123456789"sv, .chunk = 4};
    io::any_read_stream w(&s);
    std::stop_source src;
    std::error_code e1, e2;
    std::size_t n2 = 99;
    auto t         = cancel_midway(w, src, &e1, &e2, &n2);
    test_io::run_root(ctx, t, src.get_token());
    assert(!e1);
    assert(e2 == io::cond::canceled && n2 == 0);
    assert(e2 != io::cond::eof);
  }
  // Cancellation of a composed write reports the bytes written before it.
  {
    test_context ctx;
    deferred_write_stream s;
    s.inner.chunk = 3;
    std::stop_source src;
    src.request_stop();
    std::error_code ec;
    std::size_t n = 99;
    auto t        = write_template(s, "abcdef"sv, &ec, &n);
    test_io::run_root(ctx, t, src.get_token());
    assert(ec == io::cond::canceled && n == 0 && s.inner.out.empty());
  }

  // On a thread_pool: the continuation is posted to a worker and resumed there.
  {
    io::thread_pool pool(3);
    deferred_read_stream s;
    std::string payload(100000, 'p');
    s.inner = memory_read_stream{.data = payload, .chunk = 1000};
    io::any_read_stream w(&s);
    std::string out;
    std::error_code ec;
    std::size_t n = 0;
    std::atomic<int> done{0};
    io::run_async(pool.get_executor(), [&] { ++done; }, [](std::exception_ptr) { assert(false); })(
        read_exact(w, out, &ec, &n, 100000));
    pool.join();
    assert(done == 1);
    assert(!ec && n == 100000 && out == payload);
    assert(s.suspensions == 100);
  }

  return 0;
}
