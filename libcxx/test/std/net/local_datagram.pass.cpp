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

// local_datagram_socket: echo between bound sockets in a temporary directory, the unnamed sender, connected mode,
// connect_pair for datagram and stream sockets, empty datagrams and cancellation.

#include <net>

#include <cstdlib>
#include <string>
#include <unistd.h>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

static_assert(std::is_nothrow_move_constructible_v<io::local_datagram_socket>);
static_assert(std::is_same_v<io::local_datagram_socket::endpoint_type, io::local_endpoint>);

struct temp_dir {
  std::string path;
  temp_dir() {
    const char* base = std::getenv("TMPDIR");
    std::string tmpl = std::string(base && *base && std::string(base).size() < 60 ? base : "/tmp") + "/dgXXXXXX";
    char* r          = ::mkdtemp(tmpl.data());
    assert(r);
    path = r;
  }
  std::string file(const char* n) const { return path + "/" + n; }
  ~temp_dir() { (void)::rmdir(path.c_str()); }
};

io::local_endpoint ep_of(const std::string& p) {
  auto [ec, e] = io::make_local_endpoint(p);
  assert(!ec);
  return e;
}

io::task<void> echo(io::local_datagram_socket& server, io::local_datagram_socket& client, io::local_endpoint server_ep,
                    io::local_endpoint client_ep, bool* done) {
  std::string msg = "hello over a datagram socket";
  auto [wec, wn]  = co_await client.send_to(cbuf(msg), server_ep);
  IO_CHECK(wec, wn, !wec && wn == msg.size());
  char buf[128];
  io::local_endpoint from;
  auto [rec, rn] = co_await server.receive_from(io::make_buffer(buf, sizeof buf), from);
  IO_CHECK(rec, rn, !rec && std::string(buf, rn) == msg);
  assert(from == client_ep);
  auto [sec, sn] = co_await server.send_to(io::make_buffer(static_cast<const void*>(buf), rn), from);
  IO_CHECK(sec, sn, !sec && sn == rn);
  io::local_endpoint from2;
  auto [r2, n2] = co_await client.receive_from(io::make_buffer(buf, sizeof buf), from2);
  IO_CHECK(r2, n2, !r2 && std::string(buf, n2) == msg && from2 == server_ep);
  // An empty datagram is a datagram.
  auto [e1, en] = co_await client.send_to(io::const_buffer(nullptr, 0), server_ep);
  IO_CHECK(e1, en, !e1 && en == 0);
  auto [e2, en2] = co_await server.receive_from(io::make_buffer(buf, sizeof buf), from);
  IO_CHECK(e2, en2, !e2 && en2 == 0 && from == client_ep);
  *done = true;
}

void test_echo() {
  temp_dir d;
  io::io_context ctx;
  io::local_datagram_socket server(ctx), client(ctx);
  assert(!server.open() && !client.open());
  io::local_endpoint sep = ep_of(d.file("s")), cep = ep_of(d.file("c"));
  assert(!server.bind(sep) && !client.bind(cep));
  assert(server.local_endpoint() == sep);
  bool done = false;
  io::run_async(ctx.get_executor())(echo(server, client, sep, cep, &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
  server.close();
  client.close();
  (void)::unlink(d.file("s").c_str());
  (void)::unlink(d.file("c").c_str());
}

io::task<void> unnamed_and_connected(io::io_context& ctx, io::local_datagram_socket& server, io::local_endpoint sep,
                                     bool* done) {
  // A client that is not bound has no address: the sender is the empty endpoint.
  io::local_datagram_socket anon(ctx);
  auto [cec] = co_await anon.connect(sep);
  IO_CHECK_EC(cec, !cec && anon.remote_endpoint() == sep);
  std::string msg = "anon";
  auto [wec, wn]  = co_await anon.send(cbuf(msg));
  IO_CHECK(wec, wn, !wec && wn == 4);
  char buf[16];
  io::local_endpoint from = ep_of("sentinel");
  auto [rec, rn] = co_await server.receive_from(io::make_buffer(buf, sizeof buf), from);
  IO_CHECK(rec, rn, !rec && rn == 4 && from.empty());
  *done = true;
}

void test_unnamed() {
  temp_dir d;
  io::io_context ctx;
  io::local_datagram_socket server(ctx);
  assert(!server.open());
  io::local_endpoint sep = ep_of(d.file("s"));
  assert(!server.bind(sep));
  bool done = false;
  io::run_async(ctx.get_executor())(unnamed_and_connected(ctx, server, sep, &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
  server.close();
  (void)::unlink(d.file("s").c_str());
}

io::task<void> pair_ops(io::local_datagram_socket& a, io::local_datagram_socket& b, bool* done) {
  std::string msg = "through a pair";
  auto [wec, wn]  = co_await a.send(cbuf(msg));
  IO_CHECK(wec, wn, !wec && wn == msg.size());
  char buf[64];
  auto [rec, rn] = co_await b.receive(io::make_buffer(buf, sizeof buf));
  IO_CHECK(rec, rn, !rec && std::string(buf, rn) == msg);
  // Datagram boundaries are kept: two sends are two receives.
  std::string m1 = "one", m2 = "three";
  (void)co_await b.send(cbuf(m1));
  (void)co_await b.send(cbuf(m2));
  auto [r1, n1] = co_await a.receive(io::make_buffer(buf, sizeof buf));
  IO_CHECK(r1, n1, !r1 && n1 == 3);
  auto [r2, n2] = co_await a.receive(io::make_buffer(buf, sizeof buf));
  IO_CHECK(r2, n2, !r2 && n2 == 5);
  *done = true;
}

void test_pair() {
  io::io_context ctx;
  io::local_datagram_socket a(ctx), b(ctx);
  assert(!io::connect_pair(a, b));
  assert(a.is_open() && b.is_open());
  io::local_datagram_socket c(ctx);
  assert(!c.open());
  assert(io::connect_pair(a, c) == std::errc::already_connected); // open sockets are refused and left alone
  assert(a.is_open() && c.is_open());
  bool done = false;
  io::run_async(ctx.get_executor())(pair_ops(a, b, &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
}

io::task<void> stream_pair_ops(io::local_stream_socket& a, io::local_stream_socket& b, bool* done) {
  std::string msg = "stream pair";
  auto [wec, wn]  = co_await io::write(a, cbuf(msg));
  IO_CHECK(wec, wn, !wec && wn == msg.size());
  std::string back(msg.size(), '\0');
  auto [rec, rn] = co_await io::read(b, io::make_buffer(back.data(), back.size()));
  IO_CHECK_EC(rec, !rec && back == msg);
  *done = true;
}

void test_stream_pair() {
  io::io_context ctx;
  io::local_stream_socket a(ctx), b(ctx);
  assert(!io::connect_pair(a, b));
  bool done = false;
  io::run_async(ctx.get_executor())(stream_pair_ops(a, b, &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
}

io::task<void> waiter(io::local_datagram_socket& s, std::error_code* ec, bool* fin) {
  char buf[8];
  io::local_endpoint from;
  auto [e, n] = co_await s.receive_from(io::make_buffer(buf, sizeof buf), from);
  *ec         = e;
  *fin        = true;
}

io::task<void> cancel_after(io::io_context& ctx, io::local_datagram_socket* s) {
  co_await sleep_for(ctx, ms(20));
  s->cancel();
}

void test_cancel_and_errors() {
  temp_dir d;
  io::io_context ctx;
  io::local_datagram_socket s(ctx);
  assert(!s.open());
  assert(!s.bind(ep_of(d.file("w"))));
  std::error_code ec;
  bool fin = false;
  io::run_async(ctx.get_executor())(waiter(s, &ec, &fin));
  io::run_async(ctx.get_executor())(cancel_after(ctx, &s));
  watchdog wd(ctx);
  ctx.run();
  assert(fin && ec == io::cond::canceled);
  s.close();
  (void)::unlink(d.file("w").c_str());
}

io::task<void> no_peer(io::io_context& ctx, std::string missing, bool* done) {
  io::local_datagram_socket s(ctx);
  assert(!s.open());
  std::string m = "x";
  auto [e1, n1] = co_await s.send_to(cbuf(m), ep_of(missing));
  assert(e1 == std::errc::no_such_file_or_directory || e1 == std::errc::connection_refused);
  auto [e2] = co_await s.connect(ep_of(missing));
  assert(e2);
  io::local_datagram_socket closed(ctx);
  auto [e3, n3] = co_await closed.send_to(cbuf(m), ep_of(missing));
  assert(e3 == std::errc::bad_file_descriptor);
  *done = true;
}

void test_errors() {
  temp_dir d;
  io::io_context ctx;
  bool done = false;
  io::run_async(ctx.get_executor())(no_peer(ctx, d.file("missing"), &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
}

int main(int, char**) {
  test_echo();
  test_unnamed();
  test_pair();
  test_stream_pair();
  test_cancel_and_errors();
  test_errors();
  return 0;
}
