//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___NET_IPV4_ADDRESS_H
#define _LIBCPP___NET_IPV4_ADDRESS_H

#include <__config>
#include <__functional/hash.h>
#include <__io/support/config.h>
#include <__io_stream/error.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__utility/move.h>
#include <array>
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

// P4100R1 [8.9]; provisional specification: Corosio's ipv4_address. A value type for an IPv4 address, stored as the
// host-order integer whose most significant byte is the first octet.
class ipv4_address {
public:
  static constexpr size_t max_str_len = 15; // "255.255.255.255"
  using uint_type                     = uint32_t;
  using bytes_type                    = array<unsigned char, 4>;

private:
  uint_type __addr_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr ipv4_address() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI constexpr explicit ipv4_address(uint_type __u) noexcept : __addr_(__u) {}
  _LIBCPP_HIDE_FROM_ABI constexpr explicit ipv4_address(const bytes_type& __b) noexcept
      : __addr_((uint_type(__b[0]) << 24) | (uint_type(__b[1]) << 16) | (uint_type(__b[2]) << 8) | uint_type(__b[3])) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bytes_type to_bytes() const noexcept {
    return bytes_type{
        static_cast<unsigned char>(__addr_ >> 24),
        static_cast<unsigned char>(__addr_ >> 16),
        static_cast<unsigned char>(__addr_ >> 8),
        static_cast<unsigned char>(__addr_)};
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr uint_type to_uint() const noexcept { return __addr_; }

  // Dotted decimal.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string to_string() const {
    string __s;
    for (int __i = 0; __i != 4; ++__i) {
      if (__i != 0)
        __s += '.';
      __s += std::to_string((__addr_ >> (24 - 8 * __i)) & 0xff);
    }
    return __s;
  }

  // The loopback network 127.0.0.0/8.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_loopback() const noexcept { return (__addr_ >> 24) == 127; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_unspecified() const noexcept { return __addr_ == 0; }
  // 224.0.0.0/4
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_multicast() const noexcept { return (__addr_ >> 28) == 0xe; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr ipv4_address any() noexcept { return ipv4_address(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr ipv4_address loopback() noexcept {
    return ipv4_address(0x7f000001);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr ipv4_address broadcast() noexcept {
    return ipv4_address(0xffffffff);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(ipv4_address __a, ipv4_address __b) noexcept {
    return __a.__addr_ == __b.__addr_;
  }
  // Ordered by the integer value, most significant octet first.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr strong_ordering
  operator<=>(ipv4_address __a, ipv4_address __b) noexcept {
    return __a.__addr_ <=> __b.__addr_;
  }
};

// The strict dotted-decimal form of RFC 3986 dec-octet: four decimal octets of 0..255 without leading zeros. Parses
// [__first, __last) completely.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool
__parse_ipv4(const char* __first, const char* __last, uint32_t& __res) noexcept {
  uint32_t __v = 0;
  for (int __idx = 0; __idx != 4; ++__idx) {
    if (__idx != 0) {
      if (__first == __last || *__first != '.')
        return false;
      ++__first;
    }
    const char* __start = __first;
    uint32_t __o        = 0;
    while (__first != __last && *__first >= '0' && *__first <= '9' && __first - __start < 3) {
      __o = __o * 10 + static_cast<uint32_t>(*__first - '0');
      ++__first;
    }
    if (__first == __start || __o > 255 || (__first - __start > 1 && *__start == '0'))
      return false;
    __v = (__v << 8) | __o;
  }
  if (__first != __last)
    return false;
  __res = __v;
  return true;
}

// Returns the error code (errc::invalid_argument when s is not an address) and the address (default-constructed on
// failure).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline io_result<ipv4_address> make_ipv4_address(string_view __s) noexcept {
  uint32_t __v;
  if (!io::__parse_ipv4(__s.data(), __s.data() + __s.size(), __v))
    return io_result<ipv4_address>(make_error_code(errc::invalid_argument), ipv4_address());
  return io_result<ipv4_address>(error_code(), ipv4_address(__v));
}

} // namespace io

template <>
struct hash<io::ipv4_address> {
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const io::ipv4_address& __a) const noexcept {
    return hash<uint32_t>()(__a.to_uint());
  }
};

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_IPV4_ADDRESS_H
