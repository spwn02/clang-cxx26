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

// udp_socket over the loopback interface: echo with the sender endpoint, connected mode, empty and large datagrams,
// cancellation, the options (broadcast, multicast), message flags and move semantics.

#include <net>

#include <array>
#include <cstdio>
#include <thread>
#include <string>
#include <vector>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

namespace so = io::socket_option;

static_assert(std::is_nothrow_move_constructible_v<io::udp_socket>);
static_assert(!std::is_copy_constructible_v<io::udp_socket>);
static_assert(std::is_same_v<io::udp_socket::endpoint_type, io::endpoint>);

io::task<void> echo_server(io::udp_socket& s, int count, int* served) {
  for (int i = 0; i != count; ++i) {
    char buf[2048];
    io::endpoint from;
    auto [ec, n] = co_await s.receive_from(io::make_buffer(buf, sizeof buf), from);
    assert(!ec);
    assert(from.address().is_loopback() && from.port() != 0);
    auto [wec, wn] = co_await s.send_to(io::make_buffer(static_cast<const void*>(buf), n), from);
    assert(!wec && wn == n);
    ++*served;
  }
}

io::task<void> echo_client(io::io_context& ctx, io::endpoint server, std::string* got) {
  io::udp_socket c(ctx);
  assert(!c.open());
  assert(c.is_open());
  for (const char* m : {"first datagram", "second", "x"}) {
    std::string msg = m;
    auto [wec, wn]  = co_await c.send_to(cbuf(msg), server);
    assert(!wec && wn == msg.size());
    char buf[64];
    io::endpoint from;
    auto [rec, rn] = co_await c.receive_from(io::make_buffer(buf, sizeof buf), from);
    assert(!rec);
    assert(from == server); // the reply comes from the address the datagram was sent to
    *got += std::string(buf, rn);
  }
  // The send_to bound the socket to an ephemeral port.
  assert(c.local_endpoint().port() != 0);
}

void test_echo() {
  io::io_context ctx;
  io::udp_socket server(ctx);
  assert(!server.open());
  assert(!server.bind(loopback()));
  io::endpoint ep = server.local_endpoint();
  assert(ep.port() != 0 && ep.address().is_loopback());
  int served = 0;
  std::string got;
  io::run_async(ctx.get_executor())(echo_server(server, 3, &served));
  io::run_async(ctx.get_executor())(echo_client(ctx, ep, &got));
  watchdog wd(ctx);
  ctx.run();
  assert(served == 3);
  assert(got == "first datagramsecondx");
}

io::task<void> connected(io::io_context& ctx, io::udp_socket& a, io::endpoint b_ep, io::udp_socket& b, bool* done) {
  // a connects to b; b stays unconnected and replies with send_to to the sender it learned.
  auto [cec] = co_await a.connect(b_ep);
  assert(!cec);
  assert(a.remote_endpoint() == b_ep);
  std::string msg = "ping";
  auto [wec, wn]  = co_await a.send(cbuf(msg));
  assert(!wec && wn == 4);
  char buf[16];
  io::endpoint from;
  auto [rec, rn] = co_await b.receive_from(io::make_buffer(buf, sizeof buf), from);
  assert(!rec && rn == 4 && std::string(buf, rn) == "ping");
  assert(from == a.local_endpoint());
  std::string pong = "pong!";
  auto [sec, sn]   = co_await b.send_to(cbuf(pong), from);
  assert(!sec && sn == 5);
  auto [r2, n2] = co_await a.receive(io::make_buffer(buf, sizeof buf));
  assert(!r2 && std::string(buf, n2) == "pong!");

  // A connected socket receives only the datagrams of its peer: a third socket's datagram is dropped.
  io::udp_socket stranger(ctx);
  assert(!stranger.open());
  std::string lost = "intruder";
  auto [lec, ln]   = co_await stranger.send_to(cbuf(lost), a.local_endpoint());
  assert(!lec && ln == lost.size());
  std::string real = "real";
  auto [s2, sn2]   = co_await b.send_to(cbuf(real), a.local_endpoint());
  assert(!s2);
  auto [r3, n3] = co_await a.receive(io::make_buffer(buf, sizeof buf));
  assert(!r3 && std::string(buf, n3) == "real");
  *done = true;
}

void test_connected() {
  io::io_context ctx;
  io::udp_socket a(ctx), b(ctx);
  assert(!a.open() && !b.open());
  assert(!a.bind(loopback()) && !b.bind(loopback()));
  bool done = false;
  io::run_async(ctx.get_executor())(connected(ctx, a, b.local_endpoint(), b, &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
}

io::task<void> empty_and_large(io::udp_socket& a, io::udp_socket& b, bool* done) {
  io::endpoint dest = b.local_endpoint();
  // An empty datagram is a datagram of 0 bytes, not the end of a stream.
  auto [wec, wn] = co_await a.send_to(io::const_buffer(nullptr, 0), dest);
  assert(!wec && wn == 0);
  char buf[16] = {1};
  io::endpoint from;
  auto [rec, rn] = co_await b.receive_from(io::make_buffer(buf, sizeof buf), from);
  assert(!rec && rn == 0);
  assert(from == a.local_endpoint());
  // A large datagram: scatter on both sides.
#if defined(__linux__)
  std::string big(60000, '\0');
#else
  std::string big(8000, '\0'); // macOS and FreeBSD: net.inet.udp.maxdgram is 9216
#endif
  for (std::size_t i = 0; i != big.size(); ++i)
    big[i] = static_cast<char>('a' + i % 23);
  auto [bec, bn] = co_await a.send_to(cbuf(big), dest);
  assert(!bec && bn == big.size());
  std::string back(big.size(), '\0');
  std::array<io::mutable_buffer, 2> parts = {io::mutable_buffer(back.data(), 1000), io::mutable_buffer(back.data() + 1000, back.size() - 1000)};
  auto [gec, gn] = co_await b.receive_from(parts, from);
  assert(!gec && gn == big.size() && back == big);
  // A datagram that is larger than the buffer is truncated, as the system does it.
  std::string tr(100, 'z');
  (void)co_await a.send_to(cbuf(tr), dest);
  char small[10];
  auto [tec, tn] = co_await b.receive_from(io::make_buffer(small, sizeof small), from);
  assert(!tec && tn == sizeof small);
  // Gather on the send side; peek leaves the datagram in the queue.
  std::string p1 = "gath", p2 = "ered";
  std::array<io::const_buffer, 2> gparts = {cbuf(p1), cbuf(p2)};
  (void)co_await a.send_to(gparts, dest);
  char pk[16], rd[16];
  auto [pec, pn] = co_await b.receive_from(io::make_buffer(pk, sizeof pk), from, io::message_flags::peek);
  assert(!pec && std::string(pk, pn) == "gathered");
  auto [dec, dn] = co_await b.receive_from(io::make_buffer(rd, sizeof rd), from);
  assert(!dec && std::string(rd, dn) == "gathered");
  *done = true;
}

void test_empty_and_large() {
  io::io_context ctx;
  io::udp_socket a(ctx), b(ctx);
  assert(!a.open() && !b.open());
  assert(!a.bind(loopback()) && !b.bind(loopback()));
  int sz = 1 << 20;
  assert(!b.set_option(so::receive_buffer_size(sz)));
  bool done = false;
  io::run_async(ctx.get_executor())(empty_and_large(a, b, &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
}

io::task<void> waiter(io::udp_socket& s, std::error_code* ec, bool* fin) {
  char buf[8];
  io::endpoint from;
  auto [e, n] = co_await s.receive_from(io::make_buffer(buf, sizeof buf), from);
  *ec         = e;
  *fin        = true;
}

io::task<void> close_later(io::io_context& ctx, io::udp_socket* s, bool cancel_only) {
  co_await sleep_for(ctx, ms(20));
  if (cancel_only)
    s->cancel();
  else
    s->close();
}

io::task<void> stop_after(io::io_context& ctx, std::stop_source* src) {
  co_await sleep_for(ctx, ms(20));
  src->request_stop();
}

void test_cancel() {
  for (int mode = 0; mode != 3; ++mode) {
    io::io_context ctx;
    io::udp_socket s(ctx);
    assert(!s.open() && !s.bind(loopback()));
    std::error_code ec;
    bool fin = false;
    std::stop_source src;
    io::run_async(ctx.get_executor(), src.get_token())(waiter(s, &ec, &fin));
    if (mode == 2)
      io::run_async(ctx.get_executor())(stop_after(ctx, &src));
    else
      io::run_async(ctx.get_executor())(close_later(ctx, &s, mode == 0));
    watchdog wd(ctx);
    ctx.run();
    assert(fin && ec == io::cond::canceled);
    if (mode == 0)
      assert(s.is_open());
    if (mode == 1)
      assert(!s.is_open());
  }
}

io::task<void> closed_socket(io::io_context& ctx, bool* done) {
  io::udp_socket s(ctx);
  std::string m = "x";
  auto [e1, n1] = co_await s.send_to(cbuf(m), loopback(9));
  assert(e1 == std::errc::bad_file_descriptor);
  char b[4];
  io::endpoint from;
  auto [e2, n2] = co_await s.receive_from(io::make_buffer(b, 4), from);
  assert(e2 == std::errc::bad_file_descriptor);
  assert(s.bind(loopback()) == std::errc::bad_file_descriptor);
  *done = true;
}

void test_closed() {
  io::io_context ctx;
  bool done = false;
  io::run_async(ctx.get_executor())(closed_socket(ctx, &done));
  watchdog wd(ctx);
  ctx.run();
  assert(done);
}

void test_options_and_move() {
  io::io_context ctx;
  io::udp_socket s(ctx);
  assert(s.set_option(so::broadcast(true)) == std::errc::bad_file_descriptor);
  assert(!s.open());
  assert(!s.set_option(so::broadcast(true)));
  auto [ec, bc] = s.get_option<so::broadcast>();
  assert(!ec && bc.value());
  assert(!s.set_option(so::broadcast(false)));
  assert(!std::get<1>(s.get_option<so::broadcast>()));
  assert(!s.set_option(so::reuse_address(true)));
  assert(!s.set_option(so::multicast_loop(false)));
  assert(!std::get<1>(s.get_option<so::multicast_loop>()).value());
  assert(!s.set_option(so::multicast_loop(true)));
  assert(std::get<1>(s.get_option<so::multicast_loop>()).value());
  assert(!s.set_option(so::multicast_hops(3)));
  assert(std::get<1>(s.get_option<so::multicast_hops>()).value() == 3);
  assert(!s.bind(loopback()));
  io::endpoint ep = s.local_endpoint();
  io::udp_socket m(std::move(s));
  assert(!s.is_open() && m.is_open() && m.local_endpoint() == ep);
  io::udp_socket n(ctx);
  n = std::move(m);
  assert(n.is_open() && n.local_endpoint() == ep);
  auto [rec, fd] = n.release();
  assert(!rec && fd >= 0 && !n.is_open());
  io::udp_socket adopted(ctx);
  assert(!adopted.assign(fd));
  assert(adopted.is_open() && adopted.local_endpoint() == ep);
  assert(adopted.assign(fd) == std::errc::already_connected);
  assert(std::get<0>(n.release()) == std::errc::bad_file_descriptor);
}

void test_ipv6() {
  io::io_context ctx;
  io::udp_socket s(ctx);
  if (s.open(io::family::v6)) {
    std::printf("skipped: no IPv6\n");
    return;
  }
  if (s.bind(io::endpoint(io::ipv6_address::loopback(), 0))) {
    std::printf("skipped: no IPv6 loopback\n");
    return;
  }
  assert(s.local_endpoint().is_v6() && s.local_endpoint().port() != 0);
  assert(!s.set_option(so::multicast_loop(true)));
  assert(!s.set_option(so::multicast_hops(2)));
  assert(std::get<1>(s.get_option<so::multicast_hops>()).value() == 2);
}

// Joins a group and receives what is sent to it; skipped when the sandbox has no multicast route.
io::task<void> multicast(io::io_context& ctx, bool* ran) {
  io::ipv4_address group(io::ipv4_address::bytes_type{239, 255, 77, 77});
  io::udp_socket rx(ctx), tx(ctx);
  assert(!rx.open() && !tx.open());
  assert(!rx.set_option(so::reuse_address(true)));
  assert(!rx.bind(io::endpoint(io::ipv4_address::any(), 0)));
  std::uint16_t port = rx.local_endpoint().port();
  if (auto jec = rx.set_option(so::join_group(group, io::ipv4_address::loopback()))) {
    std::printf("skipped: cannot join a multicast group (%s)\n", jec.message().c_str());
    co_return;
  }
  if (tx.set_option(so::multicast_loop(true)) || tx.set_option(so::multicast_hops(1))) {
    std::printf("skipped: multicast options\n");
    co_return;
  }
  // Send through the loopback interface.
  std::string msg = "to the group";
  auto [wec, wn]  = co_await tx.send_to(cbuf(msg), io::endpoint(io::ip_address(group), port));
  if (wec) {
    std::printf("skipped: no multicast route (%s)\n", wec.message().c_str());
    (void)rx.set_option(so::leave_group(group, io::ipv4_address::loopback()));
    co_return;
  }
  // The datagram may not arrive on a sandbox without the loopback membership: wait a short while.
  char buf[64];
  io::endpoint from;
  auto [rec, rn] = co_await rx.receive_from(io::make_buffer(buf, sizeof buf), from);
  if (rec == io::cond::canceled) {
    std::printf("skipped: the group does not deliver on this host (no multicast route through the loopback)\n");
    co_return;
  }
  assert(!rec && std::string(buf, rn) == msg);
  *ran = true;
  assert(!rx.set_option(so::leave_group(group, io::ipv4_address::loopback())));
  assert(rx.set_option(so::leave_group(group, io::ipv4_address::loopback()))); // not a member any more
}

void test_multicast_options_only() {
  io::io_context ctx;
  io::udp_socket s(ctx);
  assert(!s.open());
  io::ipv4_address group(io::ipv4_address::bytes_type{239, 1, 2, 3});
  // A group that is not a multicast address is refused by the system.
  assert(s.set_option(so::join_group(io::ipv4_address::loopback())));
  // Dropping a group that was not joined is refused.
  assert(s.set_option(so::leave_group(group)));
}

int main(int, char**) {
  test_echo();
  test_connected();
  test_empty_and_large();
  test_cancel();
  test_closed();
  test_options_and_move();
  test_ipv6();
  test_multicast_options_only();
  {
    io::io_context ctx;
    bool ran = false;
    std::stop_source src;
    // The reception is cancelled after 700 ms if the group does not deliver.
    io::run_async(ctx.get_executor(), src.get_token())(multicast(ctx, &ran));
    std::thread t([&] {
      std::this_thread::sleep_for(ms(700));
      src.request_stop();
    });
    ctx.run();
    t.join();
  }
  return 0;
}
