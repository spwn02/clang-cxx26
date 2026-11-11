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

// If libssl cannot be loaded, every TLS operation completes with tls_error::engine_unavailable and nothing crashes. The
// loader is forced to fail with its test hook (the private __ossl_forced_unavailable flag).

#include <net_tls>

#include <cassert>
#include <exception>
#include <io>
#include <io_buffer>
#include <io_context>
#include <net>
#include <string>
#include <system_error>
#include <utility>

#include "test_macros.h"

namespace io = std::io;

static io::task<void> body(io::io_context& ctx) {
  io::tcp_socket sock(ctx); // never connected: the engine fails before the transport is touched
  io::tls_context cc;
  assert(!cc.use_system_trust() && !cc.set_verify_mode(io::tls_verify_mode::peer)); // configuring still works
  io::tls_stream tls(&sock, cc);
  tls.set_hostname("localhost");
  auto [hec] = co_await tls.handshake(io::tls_role::client);
  assert(hec == io::tls_error::engine_unavailable);
  assert(hec.category() == io::tls_category());
  assert(!hec.message().empty());
  assert(!tls.is_established());
  char b[4];
  auto [rec, rn] = co_await tls.read_some(io::mutable_buffer(b, 4));
  assert(rec == io::tls_error::not_established && rn == 0);
  auto [wec, wn] = co_await tls.write_some(io::const_buffer(b, 4));
  assert(wec == io::tls_error::not_established && wn == 0);
  auto [sec] = co_await tls.shutdown();
  assert(sec == io::tls_error::not_established);
  assert(!tls.error_detail().empty());
  auto [h2] = co_await tls.handshake(io::tls_role::server); // and again, as a server
  assert(h2 == io::tls_error::engine_unavailable);
}

int main(int, char**) {
  bool before = io::tls_engine_available();
  io::__ossl_forced_unavailable().store(true);
  assert(!io::tls_engine_available());
  assert(io::__ossl_get() == nullptr);

  io::io_context ctx;
  bool done = false;
  io::run_async(
      ctx.get_executor(), [&] { done = true; }, [](std::exception_ptr) { assert(false); })(body(ctx));
  ctx.run();
  assert(done);

  io::__ossl_forced_unavailable().store(false);
  assert(io::tls_engine_available() == before); // the hook does not unload or reload anything
  return 0;
}
