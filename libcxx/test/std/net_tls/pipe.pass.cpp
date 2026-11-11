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

// io::tls_stream over a stream that is not a socket: an in-memory pipe whose reads deliver at most a few bytes (so
// that TLS records arrive in pieces) and whose writes are accepted in small parts. Also TLS over TLS, and a stream
// that fails.

#include <net_tls>

#include <deque>
#include <string>

#include "test_macros.h"
#include "test_tls.h"

using namespace test_tls;

struct pipe_buf {
  std::string data;
  bool closed = false;
};

// One end of an in-memory duplex pipe; the reads poll the buffer on a timer of the context.
struct pipe_end {
  io::io_context* ctx;
  pipe_buf* in;
  pipe_buf* out;
  std::size_t chunk;
  bool fail_writes = false;

  template <io::MutableBufferSequence B>
  io::task<io::io_result<std::size_t>> read_some(B b) {
    for (;;) {
      if (!in->data.empty()) {
        std::size_t lim = in->data.size() < chunk ? in->data.size() : chunk;
        std::size_t n   = io::buffer_copy(b, io::const_buffer(in->data.data(), lim));
        in->data.erase(0, n);
        co_return io::io_result<std::size_t>(std::error_code(), n);
      }
      if (in->closed)
        co_return io::io_result<std::size_t>(io::make_error_code(io::error::eof), 0);
      co_await sleep_for(*ctx, ms(1));
    }
  }
  template <io::ConstBufferSequence B>
  io::task<io::io_result<std::size_t>> write_some(B b) {
    if (fail_writes)
      co_return io::io_result<std::size_t>(std::make_error_code(std::errc::broken_pipe), 0);
    io::const_buffer first;
    for (auto it = io::buffer_sequence_begin(b); it != io::buffer_sequence_end(b); ++it) {
      first = static_cast<io::const_buffer>(*it);
      if (first.size())
        break;
    }
    std::size_t n = first.size() < chunk * 3 ? first.size() : chunk * 3;
    out->data.append(static_cast<const char*>(first.data()), n);
    co_return io::io_result<std::size_t>(std::error_code(), n);
  }
};

static_assert(io::Stream<pipe_end>);

struct pipe_pair {
  pipe_buf a_to_b, b_to_a;
  pipe_end a, b;
  pipe_pair(io::io_context& ctx, std::size_t chunk)
      : a{&ctx, &b_to_a, &a_to_b, chunk}, b{&ctx, &a_to_b, &b_to_a, chunk} {}
};

static io::task<void> body(io::io_context& ctx) {
  pipe_pair pp(ctx, 7);
  io::tls_context sc = server_context();
  io::tls_context cc = client_context();
  io::tls_stream ts(&pp.b, sc), tc(&pp.a, cc);
  tc.set_hostname("localhost");
  auto [cec, sec] = co_await handshake_both(tc, ts);
  assert(!cec && !sec);
  io::any_stream as(&ts), ac(&tc);

  std::string msg = pattern(20000);
  std::string got(msg.size(), '\0');
  auto [w, r] = co_await io::when_all(write_all(ac, msg.data(), msg.size()), read_exact(as, got.data(), got.size()));
  assert(!w && !r);
  assert(got == msg);

  // close_notify through the pipe
  auto [e1, e2] = co_await io::when_all(
      [&]() -> io::task<std::error_code> {
        char c;
        auto [ec, n] = co_await as.read_some(io::mutable_buffer(&c, 1));
        assert(ec == io::error::eof);
        auto [x] = co_await ts.shutdown();
        co_return x;
      }(),
      tc.shutdown());
  assert(!e1 && !std::get<0>(e2));
}

// A stream whose writes fail: the handshake reports the error of the wrapped stream unchanged.
static io::task<void> failing_body(io::io_context& ctx) {
  pipe_pair pp(ctx, 64);
  pp.a.fail_writes = true;
  io::tls_context cc = client_context();
  io::tls_stream tc(&pp.a, cc);
  auto [ec] = co_await tc.handshake(io::tls_role::client);
  assert(ec == std::errc::broken_pipe);
  // and a read after the failure
  auto [rec, n] = co_await tc.read_some(io::mutable_buffer(nullptr, 0));
  (void)rec;
  (void)n;
}

// The end of the wrapped stream during the handshake is a truncation.
static io::task<void> eof_body(io::io_context& ctx) {
  pipe_pair pp(ctx, 64);
  pp.b_to_a.closed = true;
  io::tls_context cc = client_context();
  io::tls_stream tc(&pp.a, cc);
  auto [ec] = co_await tc.handshake(io::tls_role::client);
  assert(ec == io::cond::stream_truncated);
}

// TLS over TLS: the inner stream wraps the outer one (as any_stream&).
static io::task<void> nested_body(io::io_context& ctx) {
  pipe_pair pp(ctx, 4096);
  io::tls_context sc = server_context();
  io::tls_context cc = client_context();
  io::tls_stream ts(&pp.b, sc), tc(&pp.a, cc);
  tc.set_hostname("localhost");
  auto [cec, sec] = co_await handshake_both(tc, ts);
  assert(!cec && !sec);
  io::any_stream as(&ts), ac(&tc);
  io::tls_stream is(as, sc), ic(ac, cc);
  ic.set_hostname("localhost");
  auto [cec2, sec2] = co_await handshake_both(ic, is);
  assert(!cec2 && !sec2);
  io::any_stream ias(&is), iac(&ic);
  std::string msg = pattern(5000);
  std::string got(msg.size(), '\0');
  auto [w, r] = co_await io::when_all(write_all(iac, msg.data(), msg.size()), read_exact(ias, got.data(), got.size()));
  assert(!w && !r && got == msg);
}

static void run_one(io::task<void> (*f)(io::io_context&)) {
  io::io_context ctx;
  run(ctx, f(ctx), ms(15000));
}

int main(int, char**) {
  TLS_REQUIRE_ENGINE();
  run_one(body);
  run_one(failing_body);
  run_one(eof_body);
  run_one(nested_body);
  return 0;
}
