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

// The end of a TLS stream: close_notify (read reports io::error::eof), a transport that ends without it
// (io::error::stream_truncated), and cancellation of a blocked handshake and read through the stop token.

#include <net_tls>

#include <stop_token>
#include <string>

#include "test_macros.h"
#include "test_tls.h"

using namespace test_tls;

struct session {
  tcp_pair p;
  io::tls_context sc = server_context();
  io::tls_context cc = client_context();
  io::tls_stream ts;
  io::tls_stream tc;
  explicit session(io::io_context& ctx) : p(ctx), ts(&p.server, sc), tc(&p.client, cc) { tc.set_hostname("localhost"); }
};

static io::task<void> close_notify_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  session s(ctx);
  co_await connect_pair(acc, &s.p);
  auto [cec, sec] = co_await handshake_both(s.tc, s.ts);
  assert(!cec && !sec);
  io::any_stream as(&s.ts), ac(&s.tc);
  assert(!(co_await write_all(ac, "hello", 5)));
  char buf[16];
  assert(!(co_await read_exact(as, buf, 5)));
  assert(std::string(buf, 5) == "hello");

  // The client sends close_notify; the server reads the end of the stream, answers, and the client's shutdown completes.
  auto [r1, r2] = co_await io::when_all(
      [&]() -> io::task<std::error_code> {
        auto [ec, n] = co_await as.read_some(io::mutable_buffer(buf, sizeof buf));
        assert(ec == io::error::eof && ec == io::cond::eof && n == 0);
        auto [again, n2] = co_await as.read_some(io::mutable_buffer(buf, sizeof buf)); // stays at the end
        assert(again == io::error::eof && n2 == 0);
        auto [sec2] = co_await s.ts.shutdown();
        co_return sec2;
      }(),
      s.tc.shutdown());
  assert(!r1 && !std::get<0>(r2));
  // Reading from the client after the exchange is the end of the stream as well.
  auto [ec3, n3] = co_await ac.read_some(io::mutable_buffer(buf, sizeof buf));
  assert(ec3 == io::error::eof && n3 == 0);
}

static io::task<void> truncated_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  session s(ctx);
  co_await connect_pair(acc, &s.p);
  auto [cec, sec] = co_await handshake_both(s.tc, s.ts);
  assert(!cec && !sec);
  io::any_stream as(&s.ts), ac(&s.tc);
  assert(!(co_await write_all(ac, "bye", 3)));
  // The transport ends without a close_notify (a FIN; closing the socket with unread data, the session tickets of the
  // server, would be a reset, which is passed through as it is).
  assert(!s.p.client.shutdown(io::shutdown_send));
  char buf[16];
  assert(!(co_await read_exact(as, buf, 3)));
  auto [ec, n] = co_await as.read_some(io::mutable_buffer(buf, sizeof buf));
  assert(ec == io::error::stream_truncated && ec == io::cond::stream_truncated && ec != io::error::eof && n == 0);
  assert(!ec.message().empty());
}

static io::task<void> truncated_shutdown_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  session s(ctx);
  co_await connect_pair(acc, &s.p);
  auto [cec, sec] = co_await handshake_both(s.tc, s.ts);
  assert(!cec && !sec);
  s.p.server.close(); // the peer disappears: our close_notify is never answered
  auto [ec] = co_await s.tc.shutdown();
  assert(ec == io::cond::stream_truncated || ec); // stream_truncated, or a reset of the connection
  assert(ec);
}

static io::task<void> handshake_with(io::tls_stream& tc, std::error_code* out, bool* finished) {
  auto [ec] = co_await tc.handshake(io::tls_role::client);
  *out      = ec;
  *finished = true;
}

static io::task<void> read_with(io::any_stream& s, std::error_code* out, bool* finished) {
  char b[8];
  auto [ec, n] = co_await s.read_some(io::mutable_buffer(b, sizeof b));
  *out         = ec;
  *finished    = true;
}

static io::task<void> cancel_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  // A handshake with a server that never answers is cancelled by its stop token.
  {
    session s(ctx);
    co_await connect_pair(acc, &s.p);
    std::stop_source src;
    std::error_code ec;
    bool finished = false;
    io::run_async(ctx.get_executor(), src.get_token())(handshake_with(s.tc, &ec, &finished));
    co_await sleep_for(ctx, ms(40));
    assert(!finished);
    src.request_stop();
    co_await sleep_for(ctx, ms(40));
    assert(finished && ec == io::cond::canceled && ec == io::error::canceled);
    assert(!s.tc.is_established());
  }
  // A pending read of an established stream is cancelled as well, and the stream is usable afterwards.
  {
    session s(ctx);
    co_await connect_pair(acc, &s.p);
    auto [cec, sec] = co_await handshake_both(s.tc, s.ts);
    assert(!cec && !sec);
    io::any_stream as(&s.ts), ac(&s.tc);
    std::stop_source src;
    std::error_code ec;
    bool finished = false;
    io::run_async(ctx.get_executor(), src.get_token())(read_with(as, &ec, &finished));
    co_await sleep_for(ctx, ms(40));
    assert(!finished);
    src.request_stop();
    co_await sleep_for(ctx, ms(40));
    assert(finished && ec == io::cond::canceled);
    // The cancelled read did not consume anything: data still arrives.
    assert(!(co_await write_all(ac, "again", 5)));
    char buf[8];
    assert(!(co_await read_exact(as, buf, 5)));
    assert(std::string(buf, 5) == "again");
  }
  // An already stopped token: the handshake completes at once with canceled.
  {
    session s(ctx);
    co_await connect_pair(acc, &s.p);
    std::stop_source src;
    src.request_stop();
    std::error_code ec;
    bool finished = false;
    io::run_async(ctx.get_executor(), src.get_token())(handshake_with(s.tc, &ec, &finished));
    co_await sleep_for(ctx, ms(40));
    assert(finished && ec == io::cond::canceled);
  }
}

static void test_close_notify() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, close_notify_body(ctx, acc));
}
static void test_truncated() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, truncated_body(ctx, acc));
}
static void test_truncated_shutdown() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, truncated_shutdown_body(ctx, acc));
}
static void test_cancel() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, cancel_body(ctx, acc));
}

int main(int, char**) {
  TLS_REQUIRE_ENGINE();
  test_close_notify();
  test_truncated();
  test_truncated_shutdown();
  test_cancel();
  return 0;
}
