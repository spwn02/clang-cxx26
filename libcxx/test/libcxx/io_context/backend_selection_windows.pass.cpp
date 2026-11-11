//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: windows
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_context>

// The run-time choice of the backend of an io_context on Windows (issue #527): _LIBCPP_IO_BACKEND selects "winsock" (the
// readiness poller alone), "iocp" or "auto" (the completion port when it can be opened). In every case the sockets work:
// an echo over TCP and a datagram exchange run on the context that was made, and with the port the transfers run on it.

#include <io>
#include <io_buffer>
#include <io_context>
#include <io_stream>
#include <net>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdlib.h>

#include "test_macros.h"

namespace io = std::io;

static io::task<void> echo_server(io::tcp_acceptor& acc, int* served) {
  auto [ec, sock] = co_await acc.accept();
  assert(!ec && sock.is_open());
  char buf[64];
  for (;;) {
    auto [rec, n] = co_await sock.read_some(io::make_buffer(buf, sizeof buf));
    if (rec) {
      assert(rec == io::error::eof);
      break;
    }
    auto [wec, wn] = co_await io::write(sock, io::make_buffer(static_cast<const void*>(buf), n));
    assert(!wec && wn == n);
    ++*served;
  }
}

static io::task<void> echo_client(io::io_context& ctx, io::endpoint server, std::string* got) {
  io::tcp_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  assert(!cec);
  for (const char* m : {"hello", " backend"}) {
    std::string msg = m;
    auto [wec, wn]  = co_await io::write(s, io::const_buffer(msg.data(), msg.size()));
    assert(!wec && wn == msg.size());
    std::string back(msg.size(), '\0');
    auto [rec, rn] = co_await io::read(s, io::make_buffer(back.data(), back.size()));
    assert(!rec && rn == msg.size());
    *got += back;
  }
  assert(!s.shutdown(io::shutdown_send));
}

static io::task<void> datagram(io::io_context& ctx, bool* ok) {
  io::udp_socket a(ctx), b(ctx);
  assert(!a.open(io::family::v4) && !b.open(io::family::v4));
  assert(!a.bind(io::endpoint(io::ipv4_address::loopback(), 0)));
  assert(!b.bind(io::endpoint(io::ipv4_address::loopback(), 0)));
  const char msg[] = "datagram";
  auto [sec, sn]   = co_await b.send_to(io::const_buffer(msg, sizeof msg), a.local_endpoint());
  assert(!sec && sn == sizeof msg);
  char in[32];
  io::endpoint from;
  auto [rec, rn] = co_await a.receive_from(io::make_buffer(in, sizeof in), from);
  assert(!rec && rn == sizeof msg && std::memcmp(in, msg, sizeof msg) == 0);
  assert(from == b.local_endpoint());
  *ok = true;
}

static void use_sockets(io::io_context& ctx) {
  io::tcp_acceptor acc(ctx, io::endpoint(io::ipv4_address::loopback(), 0));
  std::string got;
  int served = 0;
  bool dgram = false;
  bool done  = false;
  io::run_async(ctx.get_executor(), [&] { done = true; }, [](std::exception_ptr) { assert(false); })(
      [](io::io_context& c, io::tcp_acceptor& a, std::string* g, int* sv, bool* d) -> io::task<void> {
        co_await io::when_all(echo_server(a, sv), echo_client(c, a.local_endpoint(), g), datagram(c, d));
      }(ctx, acc, &got, &served, &dgram));
  ctx.run();
  assert(done && got == "hello backend" && served >= 2 && dgram);
}

struct backend_info {
  std::string name;
  std::error_code refusal;
  bool ring;
  std::size_t transfers;
};

// Makes a context with the given environment, uses its sockets, and reports the backend it ran on.
static backend_info try_backend(const char* backend, const char* refuse) {
  (void)refuse;
  ::_putenv_s("_LIBCPP_IO_BACKEND", backend ? backend : "");
  io::io_context ctx;
  auto& r = ctx.use_service<io::__reactor_service>();
  backend_info info{r.__backend(), r.__ring_refusal(), r.__has_ring(), 0};
  use_sockets(ctx);
  info.transfers = r.__ring_transfers();
  return info;
}

int main(int, char**) {
  // The readiness poller is always available, and asking for it does not even try the port.
  {
    auto i = try_backend("winsock", nullptr);
    assert(i.name == "winsock" && !i.refusal && !i.ring && i.transfers == 0);
  }
  // The port is the choice of "iocp" and "auto" (an empty variable is "auto" too), and the transfers of the sockets ran
  // on it.
  for (const char* mode : {"", "auto", "iocp"}) {
    auto i = try_backend(mode, nullptr);
    assert(i.ring && i.name == "iocp" && !i.refusal && i.transfers > 0);
  }
  return 0;
}
