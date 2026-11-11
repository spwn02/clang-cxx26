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

// A context that is destroyed while a TLS handshake (and later a TLS read) waits for the peer destroys the chain of the
// coroutine, and the wait registered at the transport socket is gone with it (see the same test for <net>). The tests
// pass trivially if libssl cannot be loaded.

#include <net_tls>

#include <atomic>
#include <cassert>
#include <optional>
#include <utility>

#include "test_macros.h"
#include "../../std/net_tls/test_tls.h"

using namespace test_tls;

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};

io::task<void> client_handshake(io::tls_stream* s, sentinel) { (void)co_await s->handshake(io::tls_role::client); }

io::task<void> handshake_then_read(io::tls_stream* c, io::tls_stream* s, tcp_pair* p, sentinel) {
  auto [cec, sec] = co_await handshake_both(*c, *s, p);
  assert(!cec && !sec);
  char b[8];
  (void)co_await c->read_some(io::make_buffer(b, 8)); // the server sends nothing
}

static void poll_some(io::io_context& ctx) {
  for (int i = 0; i != 40; ++i)
    ctx.poll();
}

int main(int, char**) {
  TLS_REQUIRE_ENGINE();
  // The handshake waits for the server hello.
  {
    std::atomic<int> d{0};
    std::optional<io::io_context> ctx;
    ctx.emplace();
    io::tcp_acceptor acc(*ctx, loopback());
    tcp_pair p(*ctx);
    run(*ctx, connect_pair(acc, &p));
    ctx->restart();
    io::tls_context cc = client_context();
    io::tls_stream tc(&p.client, cc);
    tc.set_hostname("localhost");
    io::run_async(ctx->get_executor())(client_handshake(&tc, sentinel(&d)));
    poll_some(*ctx);
    assert(p.client.__has_waits());
    ctx->stop();
    ctx.reset();
    assert(d == 1);
    assert(!p.client.__has_waits());
  }
  // The handshake completes, the read waits for data.
  {
    std::atomic<int> d{0};
    std::optional<io::io_context> ctx;
    ctx.emplace();
    io::tcp_acceptor acc(*ctx, loopback());
    tcp_pair p(*ctx);
    run(*ctx, connect_pair(acc, &p));
    ctx->restart();
    io::tls_context sc = server_context();
    io::tls_context cc = client_context();
    io::tls_stream ts(&p.server, sc), tc(&p.client, cc);
    tc.set_hostname("localhost");
    io::run_async(ctx->get_executor())(handshake_then_read(&tc, &ts, &p, sentinel(&d)));
    poll_some(*ctx);
    assert(p.client.__has_waits());
    ctx.reset();
    assert(d == 1);
    assert(!p.client.__has_waits());
  }
  return 0;
}
