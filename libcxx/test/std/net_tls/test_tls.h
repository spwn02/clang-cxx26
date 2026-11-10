//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_NET_TLS_TEST_TLS_H
#define TEST_STD_NET_TLS_TEST_TLS_H

#include <net_tls>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

#include "../net/test_net.h"
#include "tls_fixtures.h"

namespace test_tls {

namespace io = std::io;
using namespace test_net;

// The TLS tests run only where the engine can be loaded.
#define TLS_REQUIRE_ENGINE()                                                                                          \
  do {                                                                                                                \
    if (!io::tls_engine_available())                                                                                  \
      return 0;                                                                                                       \
  } while (0)

// A connected pair of TCP sockets on the loopback.
struct tcp_pair {
  io::tcp_socket client;
  io::tcp_socket server;
  explicit tcp_pair(io::io_context& ctx) : client(ctx), server(ctx) {}
};

inline io::task<void> connect_pair(io::tcp_acceptor& acc, tcp_pair* p) {
  auto [r1, r2] = co_await io::when_all(
      [](io::tcp_acceptor& a) -> io::task<io::tcp_socket> {
        auto [ec, s] = co_await a.accept();
        assert(!ec);
        co_return std::move(s);
      }(acc),
      p->client.connect(acc.local_endpoint()));
  assert(!std::get<0>(r2));
  p->server = std::move(r1);
}

inline io::tls_context server_context() {
  io::tls_context c;
  assert(!c.use_certificate_chain(tls_fixtures::localhost_cert));
  assert(!c.use_private_key(tls_fixtures::localhost_key));
  return c;
}

// A client that trusts the test certificate and verifies the peer.
inline io::tls_context client_context(bool trust = true) {
  io::tls_context c;
  if (trust)
    assert(!c.add_certificate_authority(tls_fixtures::localhost_cert));
  assert(!c.set_verify_mode(io::tls_verify_mode::peer));
  return c;
}

// Both sides of a handshake at once; the result codes of (client, server). A side that fails closes its socket (as an
// application would), so that the other side does not wait for it.
inline io::task<std::pair<std::error_code, std::error_code>>
handshake_both(io::tls_stream& client, io::tls_stream& server, tcp_pair* p = nullptr) {
  auto [rc, rs] = co_await io::when_all(
      [&]() -> io::task<io::io_result<>> {
        auto r = co_await client.handshake(io::tls_role::client);
        if (std::get<0>(r) && p)
          p->client.close();
        co_return r;
      }(),
      [&]() -> io::task<io::io_result<>> {
        auto r = co_await server.handshake(io::tls_role::server);
        if (std::get<0>(r) && p)
          p->server.close();
        co_return r;
      }());
  co_return std::pair<std::error_code, std::error_code>(std::get<0>(rc), std::get<0>(rs));
}

inline io::task<std::error_code> write_all(io::any_stream& s, const char* p, std::size_t n) {
  while (n) {
    auto [ec, w] = co_await s.write_some(io::const_buffer(p, n));
    if (ec)
      co_return ec;
    p += w;
    n -= w;
  }
  co_return std::error_code();
}

// Reads exactly n bytes (or until an error).
inline io::task<std::error_code> read_exact(io::any_stream& s, char* p, std::size_t n) {
  while (n) {
    auto [ec, r] = co_await s.read_some(io::mutable_buffer(p, n));
    if (ec)
      co_return ec;
    p += r;
    n -= r;
  }
  co_return std::error_code();
}

inline std::string pattern(std::size_t n, unsigned seed = 7) {
  std::string s(n, '\0');
  unsigned x = seed;
  for (auto& c : s) {
    x = x * 1664525u + 1013904223u;
    c = static_cast<char>(x >> 24);
  }
  return s;
}

} // namespace test_tls

#endif // TEST_STD_NET_TLS_TEST_TLS_H
