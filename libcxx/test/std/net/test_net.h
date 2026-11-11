//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_NET_TEST_NET_H
#define TEST_STD_NET_TEST_NET_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <io>
#include <io_buffer>
#include <io_context>
#include <io_stream>
#include <io_timer>
#include <net>
#include <string>
#include <system_error>
#include <utility>

#include "test_ctx.h"

namespace test_net {

namespace io = std::io;
using namespace test_ctx;

// Runs the task on the context (with a watchdog that stops a hung test) until the context has no more work.
inline void run(io::io_context& ctx, io::task<void> t, ms limit = ms(8000)) {
  bool done = false;
  io::run_async(
      ctx.get_executor(), [&] { done = true; }, [](std::exception_ptr) { assert(false && "task failed"); })(
      std::move(t));
  watchdog wd(ctx, limit);
  ctx.run();
  assert(done);
}

inline io::endpoint loopback(std::uint16_t port = 0) { return io::endpoint(io::ipv4_address::loopback(), port); }

// Waits for the duration on a timer of the context.
inline io::task<void> sleep_for(io::io_context& ctx, ms d) {
  io::timer t(ctx, d);
  (void)co_await t.wait();
}

inline io::const_buffer cbuf(const std::string& s) { return io::const_buffer(s.data(), s.size()); }

// A failed check of an I/O result prints what was checked, the error code (value, category, message) and the byte count
// before it aborts: a failure on a platform that cannot be debugged interactively is then self-explanatory.
inline void io_check_failed(const char* cond, std::error_code ec, long long n, const char* file, int line) {
  std::fprintf(stderr, "%s:%d: check failed: %s\n  error_code = %d (%s): %s\n  byte count = %lld\n", file, line, cond,
               ec.value(), ec.category().name(), ec.message().c_str(), n);
  std::fflush(stderr);
  std::abort();
}

} // namespace test_net

#define IO_CHECK(ec, n, cond) \
  ((cond) ? (void)0 : ::test_net::io_check_failed(#cond, (ec), static_cast<long long>(n), __FILE__, __LINE__))
#define IO_CHECK_EC(ec, cond) IO_CHECK(ec, -1, cond)

#endif // TEST_STD_NET_TEST_NET_H
