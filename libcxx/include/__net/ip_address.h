//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_IP_ADDRESS_H
#define _LIBCPP___NET_IP_ADDRESS_H

#include <__config>
#include <__functional/hash.h>
#include <__io/support/config.h>
#include <__io_stream/error.h>
#include <__net/family.h>
#include <__net/ipv4_address.h>
#include <__net/ipv6_address.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__system_error/throw_system_error.h>
#include <compare>
#include <cstddef>
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

// P4100R1 [8.9]; provisional specification: Corosio's ip_address. Holds either an IPv4 or an IPv6 address. A v4-mapped
// IPv6 address is an IPv6 value: it does not compare equal to the IPv4 address it maps (normalize with to_v4()).
class ip_address {
  ipv4_address __v4_;
  ipv6_address __v6_;
  io::family __family_ = io::family::v4;

public:
  static constexpr size_t max_str_len = ipv6_address::max_str_len;

  _LIBCPP_HIDE_FROM_ABI constexpr ip_address() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI constexpr ip_address(const ipv4_address& __a) noexcept : __v4_(__a) {}
  _LIBCPP_HIDE_FROM_ABI constexpr ip_address(const ipv6_address& __a) noexcept
      : __v6_(__a), __family_(io::family::v6) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr io::family family() const noexcept { return __family_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_v4() const noexcept { return __family_ == io::family::v4; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_v6() const noexcept { return __family_ == io::family::v6; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_loopback() const noexcept {
    return is_v4() ? __v4_.is_loopback() : __v6_.is_loopback();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_unspecified() const noexcept {
    return is_v4() ? __v4_.is_unspecified() : __v6_.is_unspecified();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_multicast() const noexcept {
    return is_v4() ? __v4_.is_multicast() : __v6_.is_multicast();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_v4_mapped() const noexcept {
    return is_v6() && __v6_.is_v4_mapped();
  }

  // The held IPv4 address, or the one a v4-mapped IPv6 address maps; throws system_error
  // (errc::address_family_not_supported) otherwise.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI ipv4_address to_v4() const { return is_v4() ? __v4_ : __v6_.to_v4(); }
  // The held IPv6 address; throws system_error (errc::address_family_not_supported) if the address is IPv4.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI ipv6_address to_v6() const {
    if (is_v4())
      std::__throw_system_error(static_cast<int>(errc::address_family_not_supported), "address is not IPv6");
    return __v6_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string to_string() const {
    return is_v4() ? __v4_.to_string() : __v6_.to_string();
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const ip_address& __a, const ip_address& __b) noexcept {
    if (__a.__family_ != __b.__family_)
      return false;
    return __a.is_v4() ? __a.__v4_ == __b.__v4_ : __a.__v6_ == __b.__v6_;
  }
  // IPv4 before IPv6, then by value.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr strong_ordering
  operator<=>(const ip_address& __a, const ip_address& __b) noexcept {
    if (__a.__family_ != __b.__family_)
      return __a.is_v4() ? strong_ordering::less : strong_ordering::greater;
    return __a.is_v4() ? __a.__v4_ <=> __b.__v4_ : __a.__v6_ <=> __b.__v6_;
  }
};

// An IPv4 or an IPv6 address; the string must contain the address alone (no port, no brackets, no host name).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline io_result<ip_address> make_ip_address(string_view __s) noexcept {
  uint32_t __v4;
  if (io::__parse_ipv4(__s.data(), __s.data() + __s.size(), __v4))
    return io_result<ip_address>(error_code(), ip_address(ipv4_address(__v4)));
  ipv6_address::bytes_type __b{};
  uint32_t __scope;
  if (io::__parse_ipv6(__s.data(), __s.data() + __s.size(), __b, __scope))
    return io_result<ip_address>(error_code(), ip_address(ipv6_address(__b, __scope)));
  return io_result<ip_address>(make_error_code(errc::invalid_argument), ip_address());
}

} // namespace io

template <>
struct hash<io::ip_address> {
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const io::ip_address& __a) const noexcept {
    return __a.is_v4() ? hash<io::ipv4_address>()(__a.to_v4()) : hash<io::ipv6_address>()(__a.to_v6());
  }
};

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_IP_ADDRESS_H
