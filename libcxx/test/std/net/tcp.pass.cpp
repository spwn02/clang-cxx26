//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux || darwin || freebsd || windows
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <net>

// tcp_socket and tcp_acceptor over the loopback interface: echo, large transfers, scatter/gather, partial reads, end of
// stream, refused connections, options, move semantics, the Stream concept and any_stream.

#include <net>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

static_assert(io::Stream<io::tcp_socket>);
static_assert(io::ReadStream<io::tcp_socket> && io::WriteStream<io::tcp_socket>);
static_assert(!std::is_copy_constructible_v<io::tcp_socket>);
static_assert(std::is_nothrow_move_constructible_v<io::tcp_socket>);
static_assert(std::is_nothrow_move_assignable_v<io::tcp_socket>);
static_assert(!std::is_copy_constructible_v<io::tcp_acceptor>);
static_assert(std::is_nothrow_move_constructible_v<io::tcp_acceptor>);
static_assert(std::is_constructible_v<io::any_stream, io::tcp_socket*>);

// --- echo, through a function that only knows any_stream

io::task<std::string> round_trip(io::any_stream& s, std::string msg) {
  auto [wec, wn] = co_await io::write(s, cbuf(msg));
  assert(!wec && wn == msg.size());
  std::string back(msg.size(), '\0');
  auto [rec, rn] = co_await io::read(s, io::make_buffer(back.data(), back.size()));
  assert(!rec && rn == msg.size());
  co_return back;
}

io::task<void> echo_server(io::tcp_acceptor& acc, int* served) {
  auto [ec, sock] = co_await acc.accept();
  assert(!ec && sock.is_open());
  assert(sock.remote_endpoint().address().is_loopback());
  assert(sock.local_endpoint() == acc.local_endpoint());
  char buf[256];
  for (;;) {
    auto [rec, n] = co_await sock.read_some(io::make_buffer(buf, sizeof buf));
    if (rec) {
      assert(rec == io::error::eof && n == 0);
      break;
    }
    auto [wec, wn] = co_await io::write(sock, io::make_buffer(static_cast<const void*>(buf), n));
    assert(!wec && wn == n);
    ++*served;
  }
}

io::task<void> echo_client(io::io_context& ctx, io::endpoint server, std::string* got) {
  io::tcp_socket s(ctx);
  assert(!s.is_open() && s.native_handle() == -1);
  auto [cec] = co_await s.connect(server);
  assert(!cec && s.is_open());
  assert(s.remote_endpoint() == server);
  assert(s.local_endpoint().address().is_loopback() && s.local_endpoint().port() != 0);
  io::any_stream as(&s);
  *got = co_await round_trip(as, "hello, sockets");
  *got += co_await round_trip(as, " and again");
  assert(!s.shutdown(io::shutdown_send));
  char c;
  auto [ec, n] = co_await s.read_some(io::make_buffer(&c, 1)); // the server closes after the end of stream
  assert(ec == io::error::eof && n == 0);
}

static void test_echo() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  assert(acc.is_open() && acc.local_endpoint().port() != 0);
  assert(acc.local_endpoint().address().is_loopback());
  std::string got;
  int served = 0;
  run(ctx, [](io::io_context& c, io::tcp_acceptor& a, std::string* g, int* sv) -> io::task<void> {
    co_await io::when_all(echo_server(a, sv), echo_client(c, a.local_endpoint(), g));
  }(ctx, acc, &got, &served));
  assert(got == "hello, sockets and again");
  assert(served >= 2);
}

// --- 8 MiB each way, with the writer and the reader of the client running at the same time

constexpr std::size_t big = 8u << 20;

static unsigned char pattern(std::size_t i) { return static_cast<unsigned char>((i * 2654435761u) >> 13); }

io::task<void> big_server(io::tcp_acceptor& acc) {
  auto [ec, sock] = co_await acc.accept();
  assert(!ec);
  std::vector<char> buf(64 * 1024);
  std::size_t total = 0;
  for (;;) {
    auto [rec, n] = co_await sock.read_some(io::make_buffer(buf.data(), buf.size()));
    if (rec) {
      assert(rec == io::error::eof);
      break;
    }
    auto [wec, wn] = co_await io::write(sock, io::make_buffer(static_cast<const void*>(buf.data()), n));
    assert(!wec && wn == n);
    total += n;
  }
  assert(total == big);
}

io::task<void> big_writer(io::tcp_socket& s, const std::vector<unsigned char>& data) {
  auto [ec, n] = co_await io::write(s, io::make_buffer(static_cast<const void*>(data.data()), data.size()));
  assert(!ec && n == data.size());
  assert(!s.shutdown(io::shutdown_send));
}

io::task<void> big_reader(io::tcp_socket& s, std::vector<unsigned char>& out) {
  auto [ec, n] = co_await io::read(s, io::make_buffer(out.data(), out.size()));
  assert(!ec && n == out.size());
}

io::task<void> big_client(io::io_context& ctx, io::endpoint server) {
  io::tcp_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  assert(!cec);
  std::vector<unsigned char> data(big), back(big, 0);
  for (std::size_t i = 0; i != big; ++i)
    data[i] = pattern(i);
  co_await io::when_all(big_writer(s, data), big_reader(s, back));
  for (std::size_t i = 0; i != big; ++i)
    if (back[i] != data[i]) {
      assert(false && "corrupted echo");
      break;
    }
}

static void test_large_transfer() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(
      ctx,
      [](io::io_context& c, io::tcp_acceptor& a) -> io::task<void> {
        co_await io::when_all(big_server(a), big_client(c, a.local_endpoint()));
      }(ctx, acc),
      ms(20000));
}

// --- scatter/gather and partial reads

io::task<void> gather_server(io::tcp_acceptor& acc) {
  auto [ec, sock] = co_await acc.accept();
  assert(!ec);
  // A scatter read of 3 + 5 + 7 = 15 bytes into three buffers.
  char a[3], b[5], c[7];
  std::array<io::mutable_buffer, 3> bufs{io::make_buffer(a, 3), io::make_buffer(b, 5), io::make_buffer(c, 7)};
  auto [rec, rn] = co_await io::read(sock, bufs);
  assert(!rec && rn == 15);
  assert(std::string(a, 3) == "abc" && std::string(b, 5) == "defgh" && std::string(c, 7) == "ijklmno");
  // Four bytes at a time: every read_some returns at most the room of the buffer.
  std::string rest;
  for (;;) {
    char small[4];
    auto [sec, sn] = co_await sock.read_some(io::make_buffer(small, sizeof small));
    if (sec) {
      assert(sec == io::error::eof && sn == 0);
      break;
    }
    assert(sn >= 1 && sn <= 4);
    rest.append(small, sn);
  }
  assert(rest == "0123456789");
  // An empty buffer completes at once.
  auto [zec, zn] = co_await sock.read_some(io::mutable_buffer());
  assert(!zec && zn == 0);
}

io::task<void> gather_client(io::io_context& ctx, io::endpoint server) {
  io::tcp_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  assert(!cec);
  std::string p1 = "abc", p2 = "defgh", p3, p4 = "ijklmno";
  std::vector<io::const_buffer> bufs{cbuf(p1), cbuf(p2), cbuf(p3), cbuf(p4)}; // one is empty
  auto [wec, wn] = co_await io::write(s, bufs);
  assert(!wec && wn == 15);
  std::string ten = "0123456789";
  for (std::size_t i = 0; i < ten.size(); i += 3) {
    auto [ec, n] = co_await s.write_some(io::make_buffer(ten.data() + i, std::min<std::size_t>(3, ten.size() - i)));
    assert(!ec && n >= 1);
  }
  s.close();
  assert(!s.is_open());
}

static void test_gather_and_partial() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, [](io::io_context& c, io::tcp_acceptor& a) -> io::task<void> {
    co_await io::when_all(gather_server(a), gather_client(c, a.local_endpoint()));
  }(ctx, acc));
}

// --- end of stream when the peer closes, errors on a closed socket

io::task<void> eof_server(io::tcp_acceptor& acc) {
  auto [ec, sock] = co_await acc.accept();
  assert(!ec);
  auto [wec, wn] = co_await io::write(sock, cbuf("bye"));
  assert(!wec && wn == 3);
  sock.close();
}

io::task<void> eof_client(io::io_context& ctx, io::endpoint server) {
  io::tcp_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  assert(!cec);
  char buf[16];
  auto [ec1, n1] = co_await s.read_some(io::make_buffer(buf, sizeof buf));
  assert(!ec1 && n1 == 3 && std::string(buf, 3) == "bye");
  auto [ec2, n2] = co_await s.read_some(io::make_buffer(buf, sizeof buf));
  assert(ec2 == io::error::eof && ec2 == io::cond::eof && n2 == 0);
  auto [ec3, n3] = co_await s.read_some(io::make_buffer(buf, sizeof buf)); // and again
  assert(ec3 == io::cond::eof && n3 == 0);
  s.close();
  auto [ec4, n4] = co_await s.read_some(io::make_buffer(buf, sizeof buf));
  assert(ec4 == std::errc::bad_file_descriptor && n4 == 0);
  auto [ec5, n5] = co_await s.write_some(io::make_buffer(static_cast<const void*>(buf), 1));
  assert(ec5 == std::errc::bad_file_descriptor && n5 == 0);
  assert(s.shutdown(io::shutdown_both) == std::errc::bad_file_descriptor);
}

static void test_eof() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, [](io::io_context& c, io::tcp_acceptor& a) -> io::task<void> {
    co_await io::when_all(eof_server(a), eof_client(c, a.local_endpoint()));
  }(ctx, acc));
}

// The peer resets the connection while the other side writes: an error code, no SIGPIPE.
io::task<void> reset_pair(io::io_context& ctx, io::tcp_acceptor& acc) {
  io::tcp_socket c(ctx);
  auto [r1, r2] = co_await io::when_all(
      [](io::tcp_acceptor& a) -> io::task<io::tcp_socket> {
        auto [ec, s] = co_await a.accept();
        assert(!ec);
        co_return std::move(s);
      }(acc),
      c.connect(acc.local_endpoint()));
  io::tcp_socket server = std::move(r1);
  assert(!std::get<0>(r2));
  server.close(); // the client's next writes fail
  std::string chunk(64 * 1024, 'x');
  std::error_code last;
  for (int i = 0; i != 200 && !last; ++i) {
    auto [ec, n] = co_await c.write_some(cbuf(chunk));
    last         = ec;
    if (i > 2)
      co_await sleep_for(ctx, ms(1));
  }
  assert(last == std::errc::broken_pipe || last == std::errc::connection_reset);
}

static void test_peer_closed_write() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, reset_pair(ctx, acc));
}

// --- connect to a port nobody listens on

io::task<void> refused(io::io_context& ctx, io::endpoint dead) {
  io::tcp_socket s(ctx);
  auto [ec] = co_await s.connect(dead);
  assert(ec == std::errc::connection_refused);
  assert(s.remote_endpoint() == io::endpoint());
}

static void test_connect_refused() {
  io::io_context ctx;
  io::endpoint dead;
  {
    io::tcp_acceptor acc(ctx, loopback());
    dead = acc.local_endpoint();
  } // closed: nobody listens on the port any more
  run(ctx, refused(ctx, dead));
}

// --- options, state and move semantics

io::task<void> options_body(io::io_context& ctx) {
  namespace so = io::socket_option;
  io::tcp_socket s(ctx);
  // A closed socket
  assert(s.set_option(so::no_delay(true)) == std::errc::bad_file_descriptor);
  assert(std::get<0>(s.get_option<so::no_delay>()) == std::errc::bad_file_descriptor);
  assert(s.bind(loopback()) == std::errc::bad_file_descriptor);
  assert(s.local_endpoint() == io::endpoint() && s.remote_endpoint() == io::endpoint());

  assert(!s.open() && s.is_open());
  assert(!s.open()); // opening an open socket succeeds
  assert(!s.set_option(so::no_delay(true)));
  assert(std::get<1>(s.get_option<so::no_delay>()).value());
  assert(!s.set_option(so::no_delay(false)));
  assert(!std::get<1>(s.get_option<so::no_delay>()));
  assert(!s.set_option(so::keep_alive(true)));
  assert(static_cast<bool>(std::get<1>(s.get_option<so::keep_alive>())));
  assert(!s.set_option(so::reuse_address(true)));
  assert(std::get<1>(s.get_option<so::reuse_address>()).value());
  assert(!s.set_option(so::reuse_port(true)));
  assert(std::get<1>(s.get_option<so::reuse_port>()).value());
  assert(!s.set_option(so::receive_buffer_size(65536)));
  assert(std::get<1>(s.get_option<so::receive_buffer_size>()).value() >= 65536); // the kernel doubles it
  assert(!s.set_option(so::send_buffer_size(65536)));
  assert(std::get<1>(s.get_option<so::send_buffer_size>()).value() >= 65536);
  assert(!s.set_option(so::linger(true, 3)));
  auto [lec, lg] = s.get_option<so::linger>();
  assert(!lec && lg.enabled() && lg.timeout() == 3);
  assert(!s.set_option(so::linger(false, 0)));
  assert(!std::get<1>(s.get_option<so::linger>()).enabled());

  // bind to an ephemeral port
  assert(!s.bind(loopback()));
  assert(s.local_endpoint().port() != 0 && s.local_endpoint().address().is_loopback());
  // v6_only does not apply to a v4 socket
  assert(s.set_option(so::v6_only(true)));

  // Moves
  int fd = s.native_handle();
  io::tcp_socket t(std::move(s));
  assert(t.is_open() && t.native_handle() == fd);
  assert(!s.is_open() && s.native_handle() == -1); // NOLINT(bugprone-use-after-move)
  io::tcp_socket u(ctx);
  assert(!u.open());
  int ufd = u.native_handle();
  assert(ufd != fd);
  u = std::move(t);                                // closes the descriptor u had
  assert(u.native_handle() == fd && !t.is_open()); // NOLINT(bugprone-use-after-move)
  u.close();
  assert(!u.is_open() && u.native_handle() == -1);
  u.close(); // closing twice is fine

  // Acceptor state and moves
  io::tcp_acceptor a(ctx);
  assert(!a.is_open() && a.listen() == std::errc::bad_file_descriptor);
  assert(!a.open() && a.is_open());
  assert(!a.set_option(so::reuse_address(true)));
  assert(!a.bind(loopback()));
  assert(!a.listen(16));
  io::endpoint ep = a.local_endpoint();
  assert(ep.port() != 0);
  io::tcp_acceptor b(std::move(a));
  assert(b.is_open() && !a.is_open() && b.local_endpoint() == ep); // NOLINT(bugprone-use-after-move)
  // A second acceptor on the same endpoint fails (the convenience constructor throws; the pieces return the error)
  io::tcp_acceptor c(ctx);
  assert(!c.open());
  assert(c.bind(ep) == std::errc::address_in_use);
  // bind to an address that is not local
  io::tcp_acceptor d(ctx);
  assert(!d.open());
  assert(d.bind(io::endpoint(io::ipv4_address(0x08080808), 1)) == std::errc::address_not_available);
#ifndef TEST_HAS_NO_EXCEPTIONS
  try {
    io::tcp_acceptor e(ctx, ep);
    assert(false);
  } catch (const std::system_error& e) {
    assert(e.code() == std::errc::address_in_use);
  }
#endif
  co_return;
}

static void test_options_and_moves() {
  io::io_context ctx;
  run(ctx, options_body(ctx));
}

// --- IPv6 (skipped at run time when the loopback interface has no ::1)

io::task<void> v6_body(io::io_context& ctx, bool* ran) {
  io::tcp_acceptor acc(ctx);
  if (acc.open(io::family::v6) || acc.bind(io::endpoint(io::ipv6_address::loopback(), 0)) || acc.listen())
    co_return; // no IPv6 here
  *ran = true;
  assert(acc.local_endpoint().is_v6() && acc.local_endpoint().address().is_loopback());
  io::tcp_socket c(ctx);
  auto [r1, r2] = co_await io::when_all(
      [](io::tcp_acceptor& a) -> io::task<io::tcp_socket> {
        auto [ec, s] = co_await a.accept();
        assert(!ec);
        co_return std::move(s);
      }(acc),
      c.connect(acc.local_endpoint()));
  assert(!std::get<0>(r2));
  io::tcp_socket s = std::move(r1);
  assert(s.remote_endpoint().is_v6() && c.remote_endpoint() == acc.local_endpoint());
  auto [wec, wn] = co_await io::write(c, cbuf("v6"));
  assert(!wec && wn == 2);
  char buf[2];
  auto [rec, rn] = co_await io::read(s, io::make_buffer(buf, 2));
  assert(!rec && rn == 2 && buf[0] == 'v' && buf[1] == '6');
  // The IPv6 acceptor is dual-stack: it accepts a v4-mapped client when the system allows it. Not required to work.
}

static void test_ipv6() {
  io::io_context ctx;
  bool ran = false;
  run(ctx, v6_body(ctx, &ran));
  (void)ran; // skipped when ::1 is not usable
}

// --- 100 sequential connections to one acceptor

io::task<void> many_server(io::tcp_acceptor& acc, int n) {
  for (int i = 0; i != n; ++i) {
    auto [ec, s] = co_await acc.accept();
    assert(!ec);
    char buf[4];
    auto [rec, rn] = co_await io::read(s, io::make_buffer(buf, 4));
    assert(!rec && rn == 4);
    auto [wec, wn] = co_await io::write(s, io::make_buffer(static_cast<const void*>(buf), 4));
    assert(!wec && wn == 4);
  }
}

io::task<void> many_client(io::io_context& ctx, io::endpoint server, int n) {
  for (int i = 0; i != n; ++i) {
    io::tcp_socket s(ctx);
    auto [cec] = co_await s.connect(server);
    assert(!cec);
    std::string msg = std::to_string(1000 + i);
    auto [wec, wn]  = co_await io::write(s, cbuf(msg));
    assert(!wec && wn == 4);
    char buf[4];
    auto [rec, rn] = co_await io::read(s, io::make_buffer(buf, 4));
    assert(!rec && rn == 4 && std::string(buf, 4) == msg);
  }
}

static void test_sequential() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(
      ctx,
      [](io::io_context& c, io::tcp_acceptor& a) -> io::task<void> {
        co_await io::when_all(many_server(a, 100), many_client(c, a.local_endpoint(), 100));
      }(ctx, acc),
      ms(15000));
}

int main(int, char**) {
  test_echo();
  test_large_transfer();
  test_gather_and_partial();
  test_eof();
  test_peer_closed_write();
  test_connect_refused();
  test_options_and_moves();
  test_ipv6();
  test_sequential();
  return 0;
}
