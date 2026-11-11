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

// resolver: forward and reverse resolution without any external network (numeric hosts, /etc/hosts), the error codes,
// cancellation, and concurrent lookups.

#include <net>

#include <atomic>
#include <cstdio>
#include <memory>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

static_assert(std::ranges::input_range<io::resolver_results>);
static_assert(std::is_convertible_v<io::resolver_entry, io::endpoint>);
static_assert(std::is_error_code_enum_v<io::resolver_error>);
static_assert(std::is_nothrow_move_constructible_v<io::resolver>);

void test_error_category() {
  std::error_code ec = io::resolver_error::host_not_found;
  assert(ec.category() == io::resolver_category());
  assert(ec == std::errc::no_such_device_or_address); // what Corosio reports
  assert(!ec.message().empty());
  assert(std::error_code(io::resolver_error::try_again) == std::errc::resource_unavailable_try_again);
  assert(std::error_code(io::resolver_error::bad_flags) == std::errc::invalid_argument);
  assert(std::error_code(io::resolver_error::host_not_found) != std::error_code(io::resolver_error::try_again));
  assert(std::string(io::resolver_category().name()) == "std::io resolver");
}

void test_flags() {
  io::resolve_flags f = io::resolve_flags::numeric_host | io::resolve_flags::numeric_service;
  assert((f & io::resolve_flags::numeric_host) == io::resolve_flags::numeric_host);
  assert((f & io::resolve_flags::passive) == io::resolve_flags::none);
  f &= io::resolve_flags::numeric_host;
  assert(f == io::resolve_flags::numeric_host);
  io::reverse_flags r = io::reverse_flags::none;
  r |= io::reverse_flags::numeric_host;
  assert(r == io::reverse_flags::numeric_host);
}

void test_host_name() {
  auto [ec, name] = io::host_name();
  assert(!ec && !name.empty());
}

io::task<void> numeric(io::io_context& ctx) {
  io::resolver r(ctx);
  {
    auto [ec, res] = co_await r.resolve("127.0.0.1", "8080", io::resolve_flags::numeric_host | io::resolve_flags::numeric_service);
    assert(!ec && res.size() == 1);
    assert(res[0].endpoint() == io::endpoint(io::ipv4_address::loopback(), 8080));
    assert(res[0].host_name() == "127.0.0.1" && res[0].service_name() == "8080");
    int n = 0;
    for (const io::resolver_entry& e : res) { // a range
      io::endpoint ep = e;
      assert(ep.port() == 8080);
      ++n;
    }
    assert(n == 1);
  }
  {
    auto [ec, res] = co_await r.resolve("::1", "443", io::resolve_flags::numeric_host);
    assert(!ec && res.size() == 1 && res[0].endpoint().is_v6());
    assert(res[0].endpoint().port() == 443);
    assert(res[0].endpoint().address() == io::ip_address(io::ipv6_address::loopback()));
  }
  {
    // A service only: the passive flag gives the wildcard address.
    auto [ec, res] = co_await r.resolve("", "9000", io::resolve_flags::passive | io::resolve_flags::numeric_service);
    assert(!ec && !res.empty());
    assert(res[0].endpoint().port() == 9000 && res[0].endpoint().address().is_unspecified());
  }
  {
    // Without a service the port is 0.
    auto [ec, res] = co_await r.resolve("10.1.2.3", "", io::resolve_flags::numeric_host);
    assert(!ec && res.size() == 1 && res[0].endpoint().port() == 0);
  }
}

io::task<void> errors(io::io_context& ctx) {
  io::resolver r(ctx);
  {
    // A name is not numeric: the lookup is refused without any query.
    auto [ec, res] = co_await r.resolve("not-numeric.invalid", "80", io::resolve_flags::numeric_host);
    assert(ec == io::resolver_error::host_not_found);
    assert(ec == std::errc::no_such_device_or_address);
    assert(res.empty());
  }
  {
    auto [ec, res] = co_await r.resolve("127.0.0.1", "http", io::resolve_flags::numeric_host | io::resolve_flags::numeric_service);
    assert(ec == io::resolver_error::host_not_found);
  }
  {
    auto [ec, res] = co_await r.resolve("", "");
    assert(ec);
    assert(res.empty());
  }
  {
    // The reserved top-level domain .invalid never resolves; a sandbox without DNS may report a temporary failure.
    auto [ec, res] = co_await r.resolve("no-such-host.invalid", "80");
    assert(ec == io::resolver_error::host_not_found || ec == io::resolver_error::try_again ||
           ec == io::resolver_error::no_recovery || ec == io::resolver_error::no_data);
    assert(res.empty());
  }
}

io::task<void> names(io::io_context& ctx, bool* ran) {
  io::resolver r(ctx);
  auto [ec, res] = co_await r.resolve("localhost", "8080", io::resolve_flags::numeric_service);
  if (ec) { // no /etc/hosts entry in this sandbox
    std::printf("skipped: localhost does not resolve (%s)\n", ec.message().c_str());
    co_return;
  }
  *ran = true;
  assert(!res.empty());
  bool loop = false;
  for (const io::resolver_entry& e : res) {
    assert(e.endpoint().port() == 8080);
    assert(e.endpoint().address().is_loopback());
    assert(e.host_name() == "localhost");
    loop = true;
  }
  assert(loop);
  // A service name from /etc/services, if the system has it.
  auto [sec, sres] = co_await r.resolve("127.0.0.1", "http", io::resolve_flags::numeric_host);
  if (!sec) {
    assert(sres[0].endpoint().port() == 80);
  } else {
    std::printf("skipped: the service http is not known (%s)\n", sec.message().c_str());
  }
}

io::task<void> reverse(io::io_context& ctx) {
  io::resolver r(ctx);
  io::endpoint ep(io::ipv4_address::loopback(), 8080);
  {
    auto [ec, n] = co_await r.resolve(ep, io::reverse_flags::numeric_host | io::reverse_flags::numeric_service);
    assert(!ec);
    assert(n.host_name() == "127.0.0.1");
    assert(n.service_name() == "8080");
  }
  {
    auto [ec, n] = co_await r.resolve(io::endpoint(io::ipv6_address::loopback(), 80),
                                      io::reverse_flags::numeric_host | io::reverse_flags::numeric_service);
    assert(!ec && n.host_name() == "::1" && n.service_name() == "80");
  }
  {
    // Whatever the system knows: either a name or (without name_required) the numeric address.
    auto [ec, n] = co_await r.resolve(ep, io::reverse_flags::numeric_service);
    assert(!ec && !n.host_name().empty());
  }
  {
    // A numeric address without a name, with name_required, fails with host_not_found.
    auto [ec, n] = co_await r.resolve(io::endpoint(io::ipv4_address(0xc0000201u), 80), // 192.0.2.1 (TEST-NET-1)
                                      io::reverse_flags::name_required | io::reverse_flags::numeric_service);
    assert(ec); // not found, or a temporary failure when the sandbox has no DNS
    assert(n.host_name().empty());
  }
}

// A lookup of a name is cancelled before it can finish: it completes with error::canceled at once.
io::task<void> cancelled_by_token(io::resolver& r, std::error_code* ec_out, bool* fin) {
  auto [ec, res] = co_await r.resolve("some-name-to-wait-for.invalid", "80");
  *ec_out        = ec;
  *fin           = true;
}

void test_cancel_token() {
  io::io_context ctx;
  io::resolver r(ctx);
  std::stop_source src;
  std::error_code ec;
  bool fin = false;
  src.request_stop(); // before the start: never queued
  io::run_async(ctx.get_executor(), src.get_token())(cancelled_by_token(r, &ec, &fin));
  watchdog wd(ctx);
  ctx.run();
  assert(fin && ec == io::cond::canceled);
}

io::task<void> stop_now(std::stop_source* src) {
  src->request_stop();
  co_return;
}

io::task<void> cancel_now(io::resolver* r) {
  r->cancel();
  co_return;
}

void test_cancel_while_running() {
  // The blocking call cannot be interrupted. Many requests on at most four helper threads: when the stop request arrives
  // (right after they were started, before any completion could be processed) most are still queued or running, and it
  // completes them with canceled at once; the ones that had finished keep their result.
  io::io_context ctx;
  constexpr int n = 40;
  std::vector<io::resolver> rs;
  for (int i = 0; i != n; ++i)
    rs.emplace_back(ctx);
  std::vector<std::error_code> ecs(n);
  std::unique_ptr<bool[]> fins(new bool[n]());
  std::stop_source src;
  for (int i = 0; i != n; ++i)
    io::run_async(ctx.get_executor(), src.get_token())(cancelled_by_token(rs[i], &ecs[i], &fins[i]));
  io::run_async(ctx.get_executor())(stop_now(&src));
  watchdog wd(ctx);
  ctx.run();
  int canceled = 0;
  for (int i = 0; i != n; ++i) {
    assert(fins[i]);
    assert(ecs[i]); // a lookup of a .invalid name never succeeds
    if (ecs[i] == io::cond::canceled)
      ++canceled;
  }
  assert(canceled > 0);
}

void test_cancel_member() {
  io::io_context ctx;
  io::resolver r(ctx);
  constexpr int n = 12;
  std::error_code ecs[n];
  bool fins[n] = {};
  // One resolver with several operations: not allowed concurrently in the Corosio model, but cancel() reaches them all.
  for (int i = 0; i != n; ++i)
    io::run_async(ctx.get_executor())(cancelled_by_token(r, &ecs[i], &fins[i]));
  io::run_async(ctx.get_executor())(cancel_now(&r));
  watchdog wd(ctx);
  ctx.run();
  int canceled = 0;
  for (int i = 0; i != n; ++i) {
    assert(fins[i] && ecs[i]);
    canceled += ecs[i] == io::cond::canceled;
  }
  assert(canceled > 0);
}

// 50 lookups on 4 context threads.
std::atomic<int> concurrent_ok{0}, concurrent_bad{0};

io::task<void> one_lookup(io::io_context& ctx, int i) {
  io::resolver r(ctx);
  std::string host = "10.0." + std::to_string(i / 250) + "." + std::to_string(i % 250 + 1);
  auto [ec, res]   = co_await r.resolve(host, std::to_string(1000 + i), io::resolve_flags::numeric_host | io::resolve_flags::numeric_service);
  if (!ec && res.size() == 1 && res[0].endpoint().port() == 1000 + i && res[0].host_name() == host)
    ++concurrent_ok;
  else
    ++concurrent_bad;
  auto [rec, n] = co_await r.resolve(res[0].endpoint(), io::reverse_flags::numeric_host | io::reverse_flags::numeric_service);
  if (rec || n.host_name() != host)
    ++concurrent_bad;
}

void test_concurrent() {
  io::io_context ctx;
  for (int i = 0; i != 50; ++i)
    io::run_async(ctx.get_executor())(one_lookup(ctx, i));
  watchdog wd(ctx, ms(15000));
  std::vector<std::thread> threads;
  for (int i = 0; i != 4; ++i)
    threads.emplace_back([&] { ctx.run(); });
  for (auto& t : threads)
    t.join();
  assert(concurrent_ok.load() == 50);
  assert(concurrent_bad.load() == 0);
}

int main(int, char**) {
  test_error_category();
  test_flags();
  test_host_name();
  {
    io::io_context ctx;
    run(ctx, numeric(ctx));
  }
  {
    io::io_context ctx;
    run(ctx, errors(ctx));
  }
  {
    io::io_context ctx;
    bool ran = false;
    run(ctx, names(ctx, &ran));
  }
  {
    io::io_context ctx;
    run(ctx, reverse(ctx));
  }
  test_cancel_token();
  test_cancel_while_running();
  test_cancel_member();
  test_concurrent();
  return 0;
}
