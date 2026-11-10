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

// <net_tls>

// io::tls_stream over tcp_sockets on the loopback: the handshake, echo of a small and a 4 MiB payload through an
// any_stream, ALPN, protocol windows, the Stream concepts. The tests pass trivially if libssl cannot be loaded.

#include <net_tls>

#include <cstdio>
#include <string>
#include <type_traits>

#include "test_macros.h"
#include "test_tls.h"

using namespace test_tls;

static_assert(io::ReadStream<io::tls_stream>);
static_assert(io::WriteStream<io::tls_stream>);
static_assert(io::Stream<io::tls_stream>);
static_assert(std::is_constructible_v<io::any_stream, io::tls_stream*>);
static_assert(std::is_constructible_v<io::tls_stream, io::tcp_socket*, const io::tls_context&>);
static_assert(std::is_constructible_v<io::tls_stream, io::any_stream&, const io::tls_context&>);
static_assert(std::is_nothrow_move_constructible_v<io::tls_stream>);
static_assert(!std::is_copy_constructible_v<io::tls_stream>);
static_assert(std::is_error_code_enum_v<io::tls_error>);

// Echoes everything until the close_notify of the peer (end of stream), then answers it with the own shutdown().
static io::task<std::error_code> echo(io::tls_stream& tls) {
  io::any_stream s(&tls);
  char buf[4096];
  for (;;) {
    auto [ec, n] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
    if (ec == io::error::eof)
      break;
    if (ec)
      co_return ec;
    auto wec = co_await write_all(s, buf, n);
    if (wec)
      co_return wec;
  }
  auto [sec] = co_await tls.shutdown();
  co_return sec;
}

static io::task<void> echo_body(io::io_context& ctx, io::tcp_acceptor& acc, std::size_t payload) {
  tcp_pair p(ctx);
  co_await connect_pair(acc, &p);
  io::tls_context sc = server_context();
  io::tls_context cc = client_context();
  io::tls_stream ts(&p.server, sc), tc(&p.client, cc);
  tc.set_hostname("localhost");
  auto [cec, sec] = co_await handshake_both(tc, ts, &p);
  assert(!cec && !sec);
  assert(tc.is_established() && ts.is_established());
  assert(tc.name() == "openssl");

  io::any_stream ac(&tc); // the TLS stream is a Stream
  std::string msg = pattern(payload);
  std::string back(payload, '\0');
  // The client writes and reads at the same time; then it shuts down while the echo server waits for the close_notify.
  auto [r_echo, r_client] = co_await io::when_all(echo(ts), [&]() -> io::task<std::error_code> {
    auto [w, r] = co_await io::when_all(write_all(ac, msg.data(), msg.size()), read_exact(ac, back.data(), back.size()));
    if (w || r)
      co_return w ? w : r;
    auto [shut] = co_await tc.shutdown();
    co_return shut;
  }());
  if (r_echo || r_client)
    std::fprintf(stderr, "echo=%s client=%s\n", r_echo.message().c_str(), r_client.message().c_str());
  assert(!r_echo && !r_client);
  assert(back == msg);
}

static void test_echo(std::size_t payload) {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, echo_body(ctx, acc, payload), ms(15000));
}

static io::task<void> alpn_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  tcp_pair p(ctx);
  co_await connect_pair(acc, &p);
  io::tls_context sc = server_context();
  assert(!sc.set_alpn({"h2", "http/1.1"}));
  io::tls_context cc = client_context();
  assert(!cc.set_alpn({"http/1.1", "spdy/3"}));
  io::tls_stream ts(&p.server, sc), tc(&p.client, cc);
  tc.set_hostname("localhost");
  assert(tc.alpn_protocol().empty()); // before the handshake
  auto [cec, sec] = co_await handshake_both(tc, ts, &p);
  assert(!cec && !sec);
  assert(tc.alpn_protocol() == "http/1.1" && ts.alpn_protocol() == "http/1.1");
}

static io::task<void> alpn_server_preference_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  tcp_pair p(ctx);
  co_await connect_pair(acc, &p);
  io::tls_context sc = server_context();
  assert(!sc.set_alpn({"h2", "http/1.1"}));
  io::tls_context cc = client_context();
  assert(!cc.set_alpn({"http/1.1", "h2"})); // the server's order wins
  io::tls_stream ts(&p.server, sc), tc(&p.client, cc);
  tc.set_hostname("localhost");
  auto [cec, sec] = co_await handshake_both(tc, ts, &p);
  assert(!cec && !sec);
  assert(tc.alpn_protocol() == "h2");
  // no ALPN on the client: nothing is negotiated
  tcp_pair p2(ctx);
  co_await connect_pair(acc, &p2);
  io::tls_context cc2 = client_context();
  io::tls_stream ts2(&p2.server, sc), tc2(&p2.client, cc2);
  tc2.set_hostname("localhost");
  auto [cec2, sec2] = co_await handshake_both(tc2, ts2, &p2);
  assert(!cec2 && !sec2);
  assert(tc2.alpn_protocol().empty());
  // no protocol in common: the handshake fails
  tcp_pair p3(ctx);
  co_await connect_pair(acc, &p3);
  io::tls_context cc3 = client_context();
  assert(!cc3.set_alpn({"spdy/3"}));
  io::tls_stream ts3(&p3.server, sc), tc3(&p3.client, cc3);
  tc3.set_hostname("localhost");
  auto [cec3, sec3] = co_await handshake_both(tc3, ts3, &p3);
  assert(cec3 && sec3);
  assert(cec3.category() == io::tls_category());
  assert(!tc3.is_established());
}

static void test_alpn() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, alpn_body(ctx, acc));
}

static void test_alpn_preference() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, alpn_server_preference_body(ctx, acc));
}

static io::task<void> window_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  // TLS 1.3 only on both sides, and TLS 1.2 only on both sides, and a mismatch.
  struct cfg {
    io::tls_version smin, smax, cmin, cmax;
    bool ok;
  };
  const cfg cfgs[] = {
      {io::tls_version::tls_1_3, io::tls_version::tls_1_3, io::tls_version::tls_1_2, io::tls_version::tls_1_3, true},
      {io::tls_version::tls_1_2, io::tls_version::tls_1_2, io::tls_version::tls_1_2, io::tls_version::tls_1_3, true},
      {io::tls_version::tls_1_2, io::tls_version::tls_1_2, io::tls_version::tls_1_3, io::tls_version::tls_1_3, false},
      {io::tls_version::tls_1_2, io::tls_version::tls_1_3, io::tls_version::tls_1_2, io::tls_version::tls_1_3, true},
  };
  for (const cfg& c : cfgs) {
    tcp_pair p(ctx);
    co_await connect_pair(acc, &p);
    io::tls_context sc = server_context();
    assert(!sc.set_min_protocol_version(c.smin) && !sc.set_max_protocol_version(c.smax));
    io::tls_context cc = client_context();
    assert(!cc.set_min_protocol_version(c.cmin) && !cc.set_max_protocol_version(c.cmax));
    io::tls_stream ts(&p.server, sc), tc(&p.client, cc);
    tc.set_hostname("localhost");
    auto [cec, sec] = co_await handshake_both(tc, ts, &p);
    assert(bool(!cec) == c.ok && bool(!sec) == c.ok);
  }
  // A cipher list that cannot be used, and an inverted window, are configuration errors.
  {
    tcp_pair p(ctx);
    co_await connect_pair(acc, &p);
    io::tls_context sc = server_context();
    assert(!sc.set_ciphersuites("NOT-A-CIPHER"));
    io::tls_context cc = client_context();
    io::tls_stream ts(&p.server, sc), tc(&p.client, cc);
    auto [cec, sec] = co_await handshake_both(tc, ts, &p);
    assert(sec == io::tls_error::bad_configuration);
    assert(cec);
    assert(!ts.error_detail().empty());
  }
  {
    tcp_pair p(ctx);
    co_await connect_pair(acc, &p);
    io::tls_context sc = server_context();
    assert(!sc.set_min_protocol_version(io::tls_version::tls_1_3) && !sc.set_max_protocol_version(io::tls_version::tls_1_2));
    io::tls_stream ts(&p.server, sc);
    auto [ec] = co_await ts.handshake(io::tls_role::server);
    assert(ec == io::tls_error::bad_configuration);
  }
}

static void test_windows() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, window_body(ctx, acc));
}

// Operations before the handshake.
static io::task<void> state_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  tcp_pair p(ctx);
  co_await connect_pair(acc, &p);
  io::tls_context sc = server_context();
  io::tls_stream ts(&p.server, sc);
  char b[4];
  auto [rec, rn] = co_await ts.read_some(io::mutable_buffer(b, 4));
  assert(rec == io::tls_error::not_established && rn == 0);
  auto [wec, wn] = co_await ts.write_some(io::const_buffer(b, 4));
  assert(wec == io::tls_error::not_established);
  auto [sec] = co_await ts.shutdown();
  assert(sec == io::tls_error::not_established);
  assert(!rec.message().empty() && rec.category().name() == std::string("std::io::tls"));
  io::tls_context empty;
  (void)empty;
}

static void test_state() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, state_body(ctx, acc));
}

int main(int, char**) {
  TLS_REQUIRE_ENGINE();
  test_echo(100);
  test_echo(4u << 20);
  test_alpn();
  test_alpn_preference();
  test_windows();
  test_state();
  return 0;
}
