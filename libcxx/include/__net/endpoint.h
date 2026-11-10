//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_ENDPOINT_H
#define _LIBCPP___NET_ENDPOINT_H

#include <__config>
#include <__functional/hash.h>
#include <__io/support/config.h>
#include <__io_stream/error.h>
#include <__net/ip_address.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's endpoint. An IP address of either family and a port in host
// byte order.
class endpoint {
  ip_address __addr_;
  uint16_t __port_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr endpoint() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI constexpr endpoint(const ip_address& __a, uint16_t __p) noexcept : __addr_(__a), __port_(__p) {}
  // The IPv4 any address (0.0.0.0) and the port.
  _LIBCPP_HIDE_FROM_ABI constexpr explicit endpoint(uint16_t __p) noexcept : __port_(__p) {}
  // The address of __e with another port.
  _LIBCPP_HIDE_FROM_ABI constexpr endpoint(const endpoint& __e, uint16_t __p) noexcept
      : __addr_(__e.__addr_), __port_(__p) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_v4() const noexcept { return __addr_.is_v4(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_v6() const noexcept { return __addr_.is_v6(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr ip_address address() const noexcept { return __addr_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr uint16_t port() const noexcept { return __port_; }

  // "a.b.c.d:port" or "[v6]:port".
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string to_string() const {
    string __s = is_v4() ? __addr_.to_string() : "[" + __addr_.to_string() + "]";
    __s += ':';
    __s += std::to_string(__port_);
    return __s;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const endpoint& __a, const endpoint& __b) noexcept {
    return __a.__port_ == __b.__port_ && __a.__addr_ == __b.__addr_;
  }
  // By address (IPv4 before IPv6), then by port.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr strong_ordering
  operator<=>(const endpoint& __a, const endpoint& __b) noexcept {
    if (auto __c = __a.__addr_ <=> __b.__addr_; __c != 0)
      return __c;
    return __a.__port_ <=> __b.__port_;
  }
};

// Parses "1.2.3.4", "1.2.3.4:80", "::1", "2001:db8::1" and "[::1]", "[::1]:80" (a port after brackets; a bare IPv6
// address has no port). The port is a decimal number of 0..65535 and is 0 if absent.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline io_result<endpoint> make_endpoint(string_view __s) noexcept {
  auto __fail = [] { return io_result<endpoint>(make_error_code(errc::invalid_argument), endpoint()); };
  auto __port = [](string_view __p, uint16_t& __res) {
    if (__p.empty() || __p.size() > 5)
      return false;
    uint32_t __v = 0;
    for (char __c : __p) {
      if (__c < '0' || __c > '9')
        return false;
      __v = __v * 10 + static_cast<uint32_t>(__c - '0');
    }
    if (__v > 65535)
      return false;
    __res = static_cast<uint16_t>(__v);
    return true;
  };
  uint16_t __p    = 0;
  size_t __colons = 0;
  for (char __c : __s)
    __colons += (__c == ':');
  if (!__s.empty() && __s.front() == '[') {
    size_t __close = __s.find(']');
    if (__close == string_view::npos)
      return __fail();
    ipv6_address::bytes_type __b{};
    uint32_t __scope;
    string_view __host = __s.substr(1, __close - 1);
    if (!io::__parse_ipv6(__host.data(), __host.data() + __host.size(), __b, __scope))
      return __fail();
    string_view __rest = __s.substr(__close + 1);
    if (!__rest.empty()) {
      if (__rest.front() != ':' || !__port(__rest.substr(1), __p))
        return __fail();
    }
    return io_result<endpoint>(error_code(), endpoint(ip_address(ipv6_address(__b, __scope)), __p));
  }
  if (__colons >= 2) {
    auto [__ec, __a] = io::make_ipv6_address(__s);
    if (__ec)
      return __fail();
    return io_result<endpoint>(error_code(), endpoint(ip_address(__a), 0));
  }
  string_view __host = __s;
  if (__colons == 1) {
    size_t __c = __s.find(':');
    __host     = __s.substr(0, __c);
    if (!__port(__s.substr(__c + 1), __p))
      return __fail();
  }
  auto [__ec, __a] = io::make_ipv4_address(__host);
  if (__ec)
    return __fail();
  return io_result<endpoint>(error_code(), endpoint(ip_address(__a), __p));
}

} // namespace io

template <>
struct hash<io::endpoint> {
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const io::endpoint& __e) const noexcept {
    size_t __h1 = hash<io::ip_address>()(__e.address());
    size_t __h2 = hash<uint16_t>()(__e.port());
    return __h1 ^ (__h2 + 0x9e3779b9 + (__h1 << 6) + (__h1 >> 2));
  }
};

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_ENDPOINT_H
