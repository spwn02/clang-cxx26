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

// <net_tls>

// Certificate verification: an untrusted certificate, a trust anchor, host name and address matching, verification
// disabled, client certificates, files, and unusable configurations.

#include <net_tls>

#include <cstdio>
#include <string>

#include "test_macros.h"
#include "test_tls.h"

using namespace test_tls;

// Runs one handshake over a fresh connection; returns (client, server).
static io::task<std::pair<std::error_code, std::error_code>> one(
    io::io_context& ctx, io::tcp_acceptor& acc, const io::tls_context& cc, const io::tls_context& sc, const char* host) {
  tcp_pair p(ctx);
  co_await connect_pair(acc, &p);
  io::tls_stream ts(&p.server, sc), tc(&p.client, cc);
  if (host)
    tc.set_hostname(host);
  auto r = co_await handshake_both(tc, ts, &p);
  co_return r;
}

static bool is_ok(std::pair<std::error_code, std::error_code> r) { return !r.first && !r.second; }

static io::task<void> body(io::io_context& ctx, io::tcp_acceptor& acc) {
  io::tls_context sc = server_context();

  // untrusted certificate: peer verification without trust anchors
  {
    auto r = co_await one(ctx, acc, client_context(false), sc, "localhost");
    assert(r.first == io::tls_error::certificate_verify_failed);
    assert(r.second); // the alert of the client
  }
  // trusted
  assert(is_ok(co_await one(ctx, acc, client_context(), sc, "localhost")));
  // no host name: the certificate chain is verified, no name is checked
  assert(is_ok(co_await one(ctx, acc, client_context(), sc, nullptr)));
  // host name mismatch
  {
    auto r = co_await one(ctx, acc, client_context(), sc, "example.com");
    assert(r.first == io::tls_error::certificate_verify_failed);
  }
  // an address: matched against the iPAddress entries
  assert(is_ok(co_await one(ctx, acc, client_context(), sc, "127.0.0.1")));
  {
    auto r = co_await one(ctx, acc, client_context(), sc, "127.0.0.2");
    assert(r.first == io::tls_error::certificate_verify_failed);
  }
  // verification is on by default: no anchors means the self-signed server certificate is rejected
  {
    io::tls_context cc;
    auto r = co_await one(ctx, acc, cc, sc, "localhost");
    assert(r.first == io::tls_error::certificate_verify_failed);
  }
  // verification disabled explicitly: anything is accepted, even a wrong name and no anchors
  {
    io::tls_context cc;
    assert(!cc.set_verify_mode(io::tls_verify_mode::none));
    assert(is_ok(co_await one(ctx, acc, cc, sc, "example.com")));
  }
  // a certificate for another name that is trusted but does not match
  {
    io::tls_context other;
    assert(!other.use_certificate_chain(tls_fixtures::other_cert) && !other.use_private_key(tls_fixtures::other_key));
    auto r = co_await one(ctx, acc, client_context(), other, "localhost"); // CN=other is not trusted
    assert(r.first == io::tls_error::certificate_verify_failed);
  }
  // the system store does not contain the self-signed certificate
  {
    io::tls_context cc;
    assert(!cc.use_system_trust() && !cc.set_verify_mode(io::tls_verify_mode::peer));
    auto r = co_await one(ctx, acc, cc, sc, "localhost");
    assert(r.first == io::tls_error::certificate_verify_failed);
  }
  // verification depth is accepted
  {
    io::tls_context cc = client_context();
    assert(!cc.set_verify_depth(4) && cc.set_verify_depth(-1));
    assert(is_ok(co_await one(ctx, acc, cc, sc, "localhost")));
  }

  // client certificates: a server that requires one
  {
    io::tls_context srv = server_context();
    assert(!srv.add_certificate_authority(tls_fixtures::localhost_cert) &&
           !srv.set_verify_mode(io::tls_verify_mode::require_peer));
    // the client has none
    auto r = co_await one(ctx, acc, client_context(), srv, "localhost");
    assert(r.second || r.first); // at least the server refuses
    assert(r.second);
    // the client has a trusted one
    io::tls_context cc = client_context();
    assert(!cc.use_certificate_chain(tls_fixtures::localhost_cert) && !cc.use_private_key(tls_fixtures::localhost_key));
    assert(is_ok(co_await one(ctx, acc, cc, srv, "localhost")));
    // the client has an untrusted one
    io::tls_context bad = client_context();
    assert(!bad.use_certificate_chain(tls_fixtures::other_cert) && !bad.use_private_key(tls_fixtures::other_key));
    r = co_await one(ctx, acc, bad, srv, "localhost");
    assert(r.second);
    // optional (peer): a client without a certificate is accepted
    io::tls_context opt = server_context();
    assert(!opt.add_certificate_authority(tls_fixtures::localhost_cert) && !opt.set_verify_mode(io::tls_verify_mode::peer));
    assert(is_ok(co_await one(ctx, acc, client_context(), opt, "localhost")));
  }

  // unusable configurations
  {
    io::tls_context garbage;
    assert(!garbage.use_certificate_chain("not a certificate") && !garbage.use_private_key(tls_fixtures::localhost_key));
    auto r = co_await one(ctx, acc, client_context(), garbage, "localhost");
    assert(r.second == io::tls_error::bad_configuration);

    io::tls_context mismatch;
    assert(!mismatch.use_certificate_chain(tls_fixtures::localhost_cert) && !mismatch.use_private_key(tls_fixtures::other_key));
    r = co_await one(ctx, acc, client_context(), mismatch, "localhost");
    assert(r.second == io::tls_error::bad_configuration);

    io::tls_context badca;
    assert(!badca.add_certificate_authority("junk") && !badca.set_verify_mode(io::tls_verify_mode::peer));
    r = co_await one(ctx, acc, badca, sc, "localhost");
    assert(r.first == io::tls_error::bad_configuration);
  }
}

static void test_verify() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, body(ctx, acc), ms(20000));
}

// The files of the API and the error returns of the setters.
static io::task<void> files_body(io::io_context& ctx, io::tcp_acceptor& acc) {
  auto write = [](const char* path, const char* text) {
    std::FILE* f = std::fopen(path, "wb");
    assert(f);
    std::fputs(text, f);
    std::fclose(f);
  };
  write("tls_test_cert.pem", tls_fixtures::localhost_cert);
  write("tls_test_key.pem", tls_fixtures::localhost_key);

  io::tls_context sc;
  assert(!sc.use_certificate_chain_file("tls_test_cert.pem") && !sc.use_private_key_file("tls_test_key.pem"));
  io::tls_context cc;
  assert(!cc.load_verify_file("tls_test_cert.pem") && !cc.set_verify_mode(io::tls_verify_mode::peer));
  auto r = co_await one(ctx, acc, cc, sc, "localhost");
  assert(is_ok(r));

  io::tls_context bad;
  assert(bad.use_certificate_file("/nonexistent/cert.pem") == std::errc::no_such_file_or_directory);
  assert(bad.use_private_key_file("/nonexistent/key.pem") == std::errc::no_such_file_or_directory);
  assert(bad.load_verify_file("/nonexistent/ca.pem") == std::errc::no_such_file_or_directory);
  assert(bad.set_alpn({""}) == std::errc::invalid_argument);
  assert(bad.set_alpn({std::string(256, 'x')}) == std::errc::invalid_argument);
  std::remove("tls_test_cert.pem");
  std::remove("tls_test_key.pem");
}

static void test_files() {
  io::io_context ctx;
  io::tcp_acceptor acc(ctx, loopback());
  run(ctx, files_body(ctx, acc));
}

int main(int, char**) {
  TLS_REQUIRE_ENGINE();
  test_verify();
  test_files();
  return 0;
}
