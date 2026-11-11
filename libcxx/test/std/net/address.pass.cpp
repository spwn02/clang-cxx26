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

// ipv4_address, ipv6_address, ip_address, endpoint and local_endpoint: construction, parsing (valid and invalid input),
// formatting, comparison and hashing.

#include <net>

#include <cassert>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <unordered_set>

#include "test_macros.h"

namespace io = std::io;

static std::string v6str(const char* s) {
  auto [ec, a] = io::make_ipv6_address(s);
  assert(!ec);
  return a.to_string();
}

static void test_v4() {
  constexpr io::ipv4_address c(0x7f000001);
  static_assert(c.is_loopback());
  static_assert(io::ipv4_address::loopback() == c);
  static_assert(io::ipv4_address::any().is_unspecified());
  static_assert(io::ipv4_address::broadcast().to_uint() == 0xffffffffu);
  static_assert(io::ipv4_address(io::ipv4_address::bytes_type{1, 2, 3, 4}).to_uint() == 0x01020304u);
  static_assert(io::ipv4_address(0x01020304).to_bytes()[3] == 4);

  auto [ec, a] = io::make_ipv4_address("192.168.1.20");
  assert(!ec);
  assert(a.to_uint() == 0xc0a80114u);
  assert(a.to_string() == "192.168.1.20");
  assert(!a.is_loopback() && !a.is_multicast() && !a.is_unspecified());
  assert(std::get<1>(io::make_ipv4_address("224.0.0.1")).is_multicast());
  assert(std::get<1>(io::make_ipv4_address("127.5.5.5")).is_loopback());
  assert(std::get<1>(io::make_ipv4_address("0.0.0.0")).is_unspecified());
  assert(std::get<1>(io::make_ipv4_address("255.255.255.255")) == io::ipv4_address::broadcast());

  const char* bad[] = {
      "",
      "1.2.3",
      "1.2.3.4.5",
      "256.1.1.1",
      "1.2.3.-1",
      "01.2.3.4",
      "1.2.3.4 ",
      " 1.2.3.4",
      "1..2.3",
      "a.b.c.d",
      "1.2.3.4.",
      ".1.2.3",
      "1.2.3.1000",
      "1,2,3,4",
      "::1"};
  for (const char* s : bad) {
    auto [e, v] = io::make_ipv4_address(s);
    assert(e == std::errc::invalid_argument);
    assert(v == io::ipv4_address());
  }

  assert(io::ipv4_address(1) < io::ipv4_address(2));
  assert(io::ipv4_address(2) > io::ipv4_address(1));
  assert(io::ipv4_address(2) <= io::ipv4_address(2));
  assert((io::ipv4_address(5) <=> io::ipv4_address(5)) == 0);
  std::set<io::ipv4_address> s{io::ipv4_address(3), io::ipv4_address(1), io::ipv4_address(2)};
  assert(s.begin()->to_uint() == 1);
  assert(std::hash<io::ipv4_address>()(a) == std::hash<io::ipv4_address>()(io::ipv4_address(0xc0a80114u)));
}

static void test_v6() {
  assert(io::ipv6_address::any().is_unspecified());
  assert(io::ipv6_address::loopback().is_loopback());
  assert(io::ipv6_address::loopback().to_string() == "::1");
  assert(io::ipv6_address::any().to_string() == "::");

  assert(v6str("::1") == "::1");
  assert(v6str("::") == "::");
  assert(v6str("2001:db8::1") == "2001:db8::1");
  assert(v6str("2001:0db8:0000:0000:0000:0000:0000:0001") == "2001:db8::1");
  assert(v6str("2001:DB8::ABCD") == "2001:db8::abcd");
  assert(v6str("1:2:3:4:5:6:7:8") == "1:2:3:4:5:6:7:8");
  assert(v6str("1:0:0:2:0:0:0:3") == "1:0:0:2::3");   // the longest run is compressed
  assert(v6str("1:0:0:2:0:0:3:4") == "1::2:0:0:3:4"); // the first of equal runs
  assert(v6str("1::") == "1::");
  assert(v6str("1:2:3:4:5:6:7::") == "1:2:3:4:5:6:7:0" || v6str("1:2:3:4:5:6:7::") == "1:2:3:4:5:6:7::");
  assert(v6str("1:0:3:4:5:6:7:8") == "1:0:3:4:5:6:7:8"); // a single zero group is not compressed
  assert(v6str("fe80::1%3") == "fe80::1%3");
  assert(v6str("::ffff:1.2.3.4") == "::ffff:1.2.3.4");
  assert(v6str("::ffff:0102:0304") == "::ffff:1.2.3.4");
  assert(v6str("64:ff9b::192.0.2.33") == "64:ff9b::c000:221");
  assert(v6str("::1.2.3.4") == "::102:304");

  auto [ec, m] = io::make_ipv6_address("::ffff:10.0.0.1");
  assert(!ec && m.is_v4_mapped());
  assert(m.to_v4() == io::ipv4_address(0x0a000001));
  assert(io::ipv6_address(io::ipv4_address(0x0a000001)) == m);
  assert(!io::ipv6_address::loopback().is_v4_mapped());
  assert(std::get<1>(io::make_ipv6_address("ff02::1")).is_multicast());
  assert(std::get<1>(io::make_ipv6_address("fe80::1%7")).scope_id() == 7);
  assert(std::get<1>(io::make_ipv6_address("fe80::1")) != std::get<1>(io::make_ipv6_address("fe80::1%7")));
  assert(std::get<1>(io::make_ipv6_address("::1")).to_bytes()[15] == 1);

  const char* bad[] = {
      "",
      ":",
      ":::",
      "1:2:3:4:5:6:7",
      "1:2:3:4:5:6:7:8:9",
      "1::2::3",
      "12345::",
      "g::",
      "1:2:3:4:5:6:7:8::",
      ":1:2:3:4:5:6:7",
      "1:2:3:4:5:6:7:",
      "::1.2.3",
      "::1.2.3.4.5",
      "1.2.3.4",
      "::256.1.1.1",
      "fe80::1%",
      "fe80::1%x",
      "fe80::1%eth0",
      "fe80::1%99999999999",
      "[::1]",
      "::1 ",
      " ::1",
      "1:2:3:4:5:6:1.2.3.4:5"};
  for (const char* s : bad) {
    auto [e, v] = io::make_ipv6_address(s);
    assert(e == std::errc::invalid_argument);
    assert(v == io::ipv6_address());
  }

  assert(std::get<1>(io::make_ipv6_address("::1")) < std::get<1>(io::make_ipv6_address("::2")));
  assert(std::get<1>(io::make_ipv6_address("::1")) < std::get<1>(io::make_ipv6_address("::1%1")));
  std::set<io::ipv6_address> s{std::get<1>(io::make_ipv6_address("::3")), std::get<1>(io::make_ipv6_address("::1"))};
  assert(s.size() == 2 && *s.begin() == io::ipv6_address::loopback());
  assert(std::hash<io::ipv6_address>()(m) ==
         std::hash<io::ipv6_address>()(std::get<1>(io::make_ipv6_address("::ffff:10.0.0.1"))));
}

static void test_ip_address() {
  io::ip_address d;
  assert(d.is_v4() && !d.is_v6() && d.is_unspecified() && d.family() == io::family::v4);

  auto [e4, a4] = io::make_ip_address("10.1.2.3");
  assert(!e4 && a4.is_v4() && a4.to_string() == "10.1.2.3" && a4.to_v4() == io::ipv4_address(0x0a010203));
  auto [e6, a6] = io::make_ip_address("2001:db8::7");
  assert(!e6 && a6.is_v6() && a6.family() == io::family::v6 && a6.to_string() == "2001:db8::7");
  assert(a4 != a6 && a4 < a6 && a6 > a4); // IPv4 sorts before IPv6
  assert(a4 == io::ip_address(io::ipv4_address(0x0a010203)));
  assert(io::ip_address(io::ipv4_address::loopback()).is_loopback());
  assert(io::ip_address(io::ipv6_address::loopback()).is_loopback());
  assert(io::ip_address(io::ipv4_address(0xe0000001)).is_multicast());
  // A v4-mapped address is an IPv6 value that does not equal the IPv4 address it maps.
  io::ip_address mapped = std::get<1>(io::make_ip_address("::ffff:10.1.2.3"));
  assert(mapped.is_v6() && mapped.is_v4_mapped() && mapped != a4 && mapped.to_v4() == a4.to_v4());
  assert(!a4.is_v4_mapped());
  assert(std::hash<io::ip_address>()(a4) == std::hash<io::ip_address>()(io::ip_address(io::ipv4_address(0x0a010203))));

  for (const char* bad : {"localhost", "", "1.2.3", "[::1]", "1.2.3.4:80", "::g"}) {
    auto [e, v] = io::make_ip_address(bad);
    assert(e == std::errc::invalid_argument);
    assert(v == io::ip_address());
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  try {
    (void)a4.to_v6();
    assert(false);
  } catch (const std::system_error& e) {
    assert(e.code() == std::errc::address_family_not_supported);
  }
  try {
    (void)a6.to_v4();
    assert(false);
  } catch (const std::system_error& e) {
    assert(e.code() == std::errc::address_family_not_supported);
  }
#endif
  std::unordered_set<io::ip_address> set{a4, a6, mapped};
  assert(set.size() == 3);
}

static void test_endpoint() {
  io::endpoint d;
  assert(d.is_v4() && d.port() == 0 && d.address().is_unspecified());
  io::endpoint e(io::ipv4_address::loopback(), 8080);
  assert(e.is_v4() && e.port() == 8080 && e.address().is_loopback());
  assert(e.to_string() == "127.0.0.1:8080");
  io::endpoint any(443);
  assert(any.address().is_unspecified() && any.port() == 443);
  io::endpoint e2(e, 9090);
  assert(e2.address() == e.address() && e2.port() == 9090 && e2 != e);
  io::endpoint e6(io::ipv6_address::loopback(), 80);
  assert(e6.is_v6() && !e6.is_v4() && e6.to_string() == "[::1]:80");
  assert(e < e6 && e < e2 && !(e2 < e));

  struct {
    const char* text;
    const char* addr;
    int port;
  } good[] = {
      {"1.2.3.4", "1.2.3.4", 0},
      {"1.2.3.4:80", "1.2.3.4", 80},
      {"1.2.3.4:65535", "1.2.3.4", 65535},
      {"::1", "::1", 0},
      {"2001:db8::1", "2001:db8::1", 0},
      {"[::1]", "::1", 0},
      {"[::1]:8080", "::1", 8080},
      {"[2001:db8::1]:1", "2001:db8::1", 1},
      {"1.2.3.4:0", "1.2.3.4", 0}};
  for (auto& g : good) {
    auto [ec, ep] = io::make_endpoint(g.text);
    assert(!ec);
    assert(ep.address().to_string() == g.addr);
    assert(ep.port() == g.port);
    // formatting and parsing round trip
    auto [ec2, back] = io::make_endpoint(ep.to_string());
    assert(!ec2 && back == ep);
  }
  for (const char* bad :
       {"",
        "1.2.3.4:",
        "1.2.3.4:65536",
        "1.2.3.4:x",
        "1.2.3.4:80:80x",
        "[::1",
        "[::1]:",
        "[::1]x",
        "[1.2.3.4]:80",
        "[::1]:99999",
        "localhost:80",
        ":80"}) {
    auto [ec, ep] = io::make_endpoint(bad);
    assert(ec == std::errc::invalid_argument);
    assert(ep == io::endpoint());
  }
  std::unordered_set<io::endpoint> set{e, e2, e6, e};
  assert(set.size() == 3);
  std::map<io::endpoint, int> m{{e6, 1}, {e, 2}};
  assert(m.begin()->first == e);
}

static void test_local_endpoint() {
  io::local_endpoint d;
  assert(d.empty() && !d.is_abstract() && d.path().empty());
  auto [ec, p] = io::make_local_endpoint("/tmp/some.sock");
  assert(!ec && !p.empty() && !p.is_abstract() && p.path() == "/tmp/some.sock" && p.to_string() == "/tmp/some.sock");
  using namespace std::string_literals;
  auto [ec2, ab] = io::make_local_endpoint("\0name"s);
  assert(!ec2 && ab.is_abstract() && ab.path().size() == 5 && ab.to_string() == "[abstract:name]");
  auto [ec3, empty] = io::make_local_endpoint("");
  assert(!ec3 && empty.empty());
  std::string longest(io::local_endpoint::max_path_length, 'x');
  assert(!std::get<0>(io::make_local_endpoint(longest)));
  auto [ec4, too_long] = io::make_local_endpoint(longest + "x");
  assert(ec4 == std::errc::filename_too_long && too_long.empty());
  assert(p == std::get<1>(io::make_local_endpoint("/tmp/some.sock")));
  assert(p != ab && (p <=> ab) != 0);
  static_assert(std::is_trivially_copyable_v<io::local_endpoint>);
}

int main(int, char**) {
  test_v4();
  test_v6();
  test_ip_address();
  test_endpoint();
  test_local_endpoint();
  return 0;
}
