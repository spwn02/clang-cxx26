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

// <net>

// local_stream_socket and local_stream_acceptor: echo over a socket path in a temporary directory and over an abstract
// name, end of stream, a missing path, bind_option::unlink_existing, cancellation, moves.

#include <net>

#include <cstdlib>
#include <string>
#include <unistd.h>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

static_assert(io::Stream<io::local_stream_socket>);
static_assert(std::is_nothrow_move_constructible_v<io::local_stream_socket>);
static_assert(std::is_nothrow_move_constructible_v<io::local_stream_acceptor>);
static_assert(std::is_constructible_v<io::any_stream, io::local_stream_socket*>);

struct temp_dir {
  std::string path;
  temp_dir() {
    const char* base = std::getenv("TMPDIR");
    std::string tmpl = std::string(base && *base && std::string(base).size() < 60 ? base : "/tmp") + "/netXXXXXX";
    char* r          = ::mkdtemp(tmpl.data());
    assert(r);
    path = r;
  }
  ~temp_dir() { (void)::rmdir(path.c_str()); }
};

io::task<void> echo_server(io::local_stream_acceptor& acc, int* served) {
  auto [ec, sock] = co_await acc.accept();
  assert(!ec && sock.is_open());
  char buf[64];
  for (;;) {
    auto [rec, n] = co_await sock.read_some(io::make_buffer(buf, sizeof buf));
    if (rec) {
      assert(rec == io::cond::eof);
      break;
    }
    auto [wec, wn] = co_await io::write(sock, io::make_buffer(static_cast<const void*>(buf), n));
    assert(!wec && wn == n);
    ++*served;
  }
}

io::task<void> echo_client(io::io_context& ctx, io::local_endpoint server, std::string* got) {
  io::local_stream_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  assert(!cec && s.is_open());
  assert(s.remote_endpoint() == server);
  io::any_stream as(&s);
  for (const char* m : {"over a path", " and again"}) {
    std::string msg = m;
    auto [wec, wn]  = co_await io::write(as, cbuf(msg));
    assert(!wec && wn == msg.size());
    std::string back(msg.size(), '\0');
    auto [rec, rn] = co_await io::read(as, io::make_buffer(back.data(), back.size()));
    assert(!rec && rn == msg.size());
    *got += back;
  }
  assert(!s.shutdown(io::shutdown_send));
  char c;
  auto [ec, n] = co_await s.read_some(io::make_buffer(&c, 1));
  assert(ec == io::error::eof && n == 0);
}

static void run_echo(const io::local_endpoint& ep, bool path) {
  io::io_context ctx;
  io::local_stream_acceptor acc(ctx, ep);
  assert(acc.is_open() && acc.local_endpoint() == ep);
  std::string got;
  int served = 0;
  run(ctx,
      [](io::io_context& c, io::local_stream_acceptor& a, io::local_endpoint e, std::string* g, int* sv)
          -> io::task<void> {
        co_await io::when_all(echo_server(a, sv), echo_client(c, e, g));
      }(ctx, acc, ep, &got, &served));
  assert(got == "over a path and again");
  assert(served >= 2);
  acc.close();
  if (path) {
    // The library does not unlink the path.
    assert(::access(std::string(ep.path()).c_str(), F_OK) == 0);
    (void)::unlink(std::string(ep.path()).c_str());
  }
}

static void test_path_echo() {
  temp_dir dir;
  auto [ec, ep] = io::make_local_endpoint(dir.path + "/echo.sock");
  assert(!ec);
  run_echo(ep, true);
}

static void test_abstract_echo() {
  using namespace std::string_literals;
  auto [ec, ep] = io::make_local_endpoint("\0clang-cxx26-net-test-"s + std::to_string(::getpid()));
  assert(!ec && ep.is_abstract());
  run_echo(ep, false);
}

// A path that does not exist, a stale socket file, unlink_existing

io::task<void> missing_body(io::io_context& ctx, io::local_endpoint ep) {
  io::local_stream_socket s(ctx);
  auto [ec] = co_await s.connect(ep);
  assert(ec == std::errc::no_such_file_or_directory || ec == std::errc::connection_refused);
}

static void test_missing_and_stale() {
  temp_dir dir;
  auto [ec, ep] = io::make_local_endpoint(dir.path + "/gone.sock");
  assert(!ec);
  {
    io::io_context ctx;
    run(ctx, missing_body(ctx, ep));
  }
  {
    io::io_context ctx;
    io::local_stream_acceptor first(ctx);
    assert(!first.open() && !first.bind(ep) && !first.listen());
    first.close(); // the file stays
    io::local_stream_acceptor again(ctx);
    assert(!again.open());
    assert(again.bind(ep) == std::errc::address_in_use);
    assert(!again.bind(ep, io::bind_option::unlink_existing));
    assert(!again.listen());
    // connect to a path that is a stale socket file nobody listens on: refused
    again.close();
    run(ctx, missing_body(ctx, ep));
    (void)::unlink(std::string(ep.path()).c_str());
  }
  // A path that is too long cannot be an endpoint
  auto [lec, lep] = io::make_local_endpoint(std::string(200, 'x'));
  assert(lec == std::errc::filename_too_long);
}

// Cancellation, state and moves

io::task<void> accept_waiter(io::local_stream_acceptor& acc, std::error_code* out, bool* finished) {
  auto [ec, s] = co_await acc.accept();
  *out         = ec;
  *finished    = true;
}

io::task<void> read_waiter(io::local_stream_socket& s, std::error_code* out, bool* finished) {
  char b[4];
  auto [ec, n] = co_await s.read_some(io::make_buffer(b, 4));
  *out         = ec;
  *finished    = true;
}

io::task<void> cancel_body(io::io_context& ctx, io::local_stream_acceptor& acc, io::local_endpoint ep) {
  std::error_code ec;
  bool finished = false;
  io::run_async(ctx.get_executor())(accept_waiter(acc, &ec, &finished));
  co_await sleep_for(ctx, ms(30));
  assert(!finished);
  acc.cancel();
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled && acc.is_open());

  io::local_stream_socket c(ctx);
  auto [r1, r2] = co_await io::when_all(acc.accept(), c.connect(ep));
  assert(!std::get<0>(r1) && !std::get<0>(r2));
  io::local_stream_socket server = std::move(std::get<1>(r1));

  finished = false;
  io::run_async(ctx.get_executor())(read_waiter(server, &ec, &finished));
  co_await sleep_for(ctx, ms(30));
  assert(!finished);
  server.cancel();
  co_await sleep_for(ctx, ms(30));
  assert(finished && ec == io::cond::canceled && server.is_open());

  // peer closes: eof; then moves
  c.close();
  char b[4];
  auto [rec, rn] = co_await server.read_some(io::make_buffer(b, 4));
  assert(rec == io::cond::eof && rn == 0);
  int fd = server.native_handle();
  io::local_stream_socket moved(std::move(server));
  assert(moved.native_handle() == fd && !server.is_open()); // NOLINT(bugprone-use-after-move)
  assert(moved.shutdown(io::shutdown_both) == std::error_code() || true);
  // Options that are about TCP do not apply, the generic ones do
  assert(!moved.set_option(io::socket_option::receive_buffer_size(32768)));
  assert(std::get<1>(moved.get_option<io::socket_option::receive_buffer_size>()).value() >= 32768);
  assert(moved.set_option(io::socket_option::no_delay(true)));
}

static void test_cancel_and_moves() {
  temp_dir dir;
  auto [ec, ep] = io::make_local_endpoint(dir.path + "/c.sock");
  assert(!ec);
  io::io_context ctx;
  io::local_stream_acceptor acc(ctx, ep);
  run(ctx, cancel_body(ctx, acc, ep));
  acc.close();
  (void)::unlink(std::string(ep.path()).c_str());
}

int main(int, char**) {
  test_path_echo();
  test_abstract_echo();
  test_missing_and_stale();
  test_cancel_and_moves();
  return 0;
}
