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

// connect(socket, endpoints): the composed connect that tries each endpoint in order, over the results of a resolver, a
// vector of endpoints and an iterator range, for TCP and Unix domain stream sockets.

#include <net>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "test_macros.h"
#include "test_net.h"

using namespace test_net;

static_assert(std::is_same_v<io::tcp_socket::endpoint_type, io::endpoint>);

// A port nobody listens on: the port of an acceptor that was closed.
std::uint16_t closed_port(io::io_context& ctx) {
  io::tcp_acceptor a(ctx, loopback(), 1);
  std::uint16_t p = a.local_endpoint().port();
  a.close();
  return p;
}

io::task<void> tcp_cases(io::io_context& ctx, io::endpoint good, std::uint16_t dead, bool* done) {
  io::endpoint bad = loopback(dead);
  {
    // The first endpoint is refused, the second connects.
    io::tcp_socket s(ctx);
    std::vector<io::endpoint> eps{bad, good};
    auto [ec, ep] = co_await io::connect(s, eps);
    assert(!ec && ep == good);
    assert(s.is_open() && s.remote_endpoint() == good);
  }
  {
    // The iterator form completes with the iterator of the connected endpoint.
    io::tcp_socket s(ctx);
    std::vector<io::endpoint> eps{bad, bad, good, bad};
    auto [ec, it] = co_await io::connect(s, eps.begin(), eps.end());
    assert(!ec && it == eps.begin() + 2);
    io::tcp_socket s2(ctx);
    std::vector<io::endpoint> none{bad};
    auto [ec2, it2] = co_await io::connect(s2, none.begin(), none.end());
    assert(ec2 == std::errc::connection_refused && it2 == none.end());
  }
  {
    // A condition sees the error of the previous attempt and skips endpoints.
    io::tcp_socket s(ctx);
    std::vector<io::endpoint> eps{bad, good};
    int calls = 0;
    std::error_code seen;
    auto cond = [&](const std::error_code& last, const io::endpoint&) {
      ++calls;
      seen = last;
      return true;
    };
    auto [ec, ep] = co_await io::connect(s, eps, cond);
    assert(!ec && ep == good && calls == 2);
    assert(seen == std::errc::connection_refused);
    io::tcp_socket s2(ctx);
    auto [ec2, ep2] =
        co_await io::connect(s2, eps, [&](const std::error_code&, const io::endpoint& e) { return e != good; });
    assert(ec2 == std::errc::connection_refused && ep2 == io::endpoint());
    io::tcp_socket s3(ctx);
    auto [ec3, ep3] = co_await io::connect(s3, eps, [](const std::error_code&, const io::endpoint&) { return false; });
    assert(ec3 == std::errc::no_such_device_or_address);
  }
  {
    // Every endpoint fails: the error of the last attempt and the empty endpoint. An empty range is
    // no_such_device_or_address.
    io::tcp_socket s(ctx);
    auto [ec, ep] = co_await io::connect(s, std::vector<io::endpoint>{bad, bad});
    assert(ec == std::errc::connection_refused && ep == io::endpoint());
    auto [ec2, ep2] = co_await io::connect(s, std::vector<io::endpoint>{});
    assert(ec2 == std::errc::no_such_device_or_address && ep2 == io::endpoint());
  }
  {
    // An open socket is closed before the first attempt (so options set before are lost, as documented).
    io::tcp_socket s(ctx);
    assert(!s.open(io::family::v6));
    auto [ec, ep] = co_await io::connect(s, std::vector<io::endpoint>{good});
    assert(!ec && s.remote_endpoint() == good);
  }
  *done = true;
}

io::task<void> resolved(io::io_context& ctx, std::uint16_t port, std::uint16_t dead, bool* done) {
  io::resolver r(ctx);
  {
    // The numeric address as a resolver result: connect() takes the results as they are.
    auto [rec, res] = co_await r.resolve("127.0.0.1", std::to_string(port),
                                         io::resolve_flags::numeric_host | io::resolve_flags::numeric_service);
    assert(!rec && res.size() == 1);
    io::tcp_socket s(ctx);
    auto [ec, ep] = co_await io::connect(s, res);
    assert(!ec && ep == io::endpoint(io::ipv4_address::loopback(), port));
  }
  {
    // localhost has the IPv6 and the IPv4 loopback address on most systems; the acceptor listens on IPv4 only, so
    // an IPv6 entry in front of it is refused and the next one connects.
    auto [rec, res] = co_await r.resolve("localhost", std::to_string(port), io::resolve_flags::numeric_service);
    if (rec) {
      std::printf("skipped: localhost does not resolve (%s)\n", rec.message().c_str());
    } else {
      io::tcp_socket s(ctx);
      auto [ec, ep] = co_await io::connect(s, res);
      assert(!ec);
      assert(ep.is_v4() && ep.port() == port);
    }
  }
  {
    // Everything refused: the error of the last attempt.
    auto [rec, res] = co_await r.resolve("127.0.0.1", std::to_string(dead),
                                         io::resolve_flags::numeric_host | io::resolve_flags::numeric_service);
    assert(!rec);
    io::tcp_socket s(ctx);
    auto [ec, ep] = co_await io::connect(s, res);
    assert(ec == std::errc::connection_refused);
  }
  *done = true;
}

struct temp_dir {
  std::string path;
  temp_dir() {
    const char* base = std::getenv("TMPDIR");
    std::string tmpl = std::string(base && *base && std::string(base).size() < 60 ? base : "/tmp") + "/cnXXXXXX";
    char* r          = ::mkdtemp(tmpl.data());
    assert(r);
    path = r;
  }
  ~temp_dir() { (void)::rmdir(path.c_str()); }
};

io::task<void> local_cases(io::io_context& ctx, io::local_endpoint good, io::local_endpoint missing, bool* done) {
  io::local_stream_socket s(ctx);
  auto [ec, ep] = co_await io::connect(s, std::vector<io::local_endpoint>{missing, good});
  assert(!ec && ep == good);
  io::local_stream_socket s2(ctx);
  auto [ec2, ep2] = co_await io::connect(s2, std::vector<io::local_endpoint>{missing});
  assert(ec2 && ep2.empty());
  *done = true;
}

int main(int, char**) {
  {
    io::io_context ctx;
    io::tcp_acceptor acc(ctx, loopback(), 16);
    std::uint16_t dead = closed_port(ctx);
    bool done          = false;
    run(ctx, tcp_cases(ctx, acc.local_endpoint(), dead, &done));
    assert(done);
    done = false;
    ctx.restart();
    run(ctx, resolved(ctx, acc.local_endpoint().port(), dead, &done));
    assert(done);
  }
  {
    temp_dir d;
    io::io_context ctx;
    auto [e1, good]    = io::make_local_endpoint(d.path + "/sock");
    auto [e2, missing] = io::make_local_endpoint(d.path + "/missing");
    assert(!e1 && !e2);
    io::local_stream_acceptor acc(ctx, good, 4, io::bind_option::unlink_existing);
    bool done = false;
    run(ctx, local_cases(ctx, good, missing, &done));
    assert(done);
    acc.close();
    (void)::unlink((d.path + "/sock").c_str());
  }
  return 0;
}
