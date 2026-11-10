//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_IPV6_ADDRESS_H
#define _LIBCPP___NET_IPV6_ADDRESS_H

#include <__config>
#include <__functional/hash.h>
#include <__io/support/config.h>
#include <__io_stream/error.h>
#include <__net/ipv4_address.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__system_error/throw_system_error.h>
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

// P4100R1 [8.9]; provisional specification: Corosio's ipv6_address. Sixteen bytes in network order plus the zone (an
// interface index, 0 when the address is not scoped). Text form: RFC 4291 / RFC 5952 (lower case, the longest run of
// zero groups of length two or more compressed), the IPv4-mapped form printed with a dotted tail, a numeric zone
// suffix "%N".
class ipv6_address {
public:
  static constexpr size_t max_str_len = 60;
  using bytes_type                    = array<unsigned char, 16>;

private:
  bytes_type __b_{};
  uint32_t __scope_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr ipv6_address() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI constexpr explicit ipv6_address(const bytes_type& __b, uint32_t __scope = 0) noexcept
      : __b_(__b), __scope_(__scope) {}
  // The IPv4-mapped address ::ffff:a.b.c.d
  _LIBCPP_HIDE_FROM_ABI constexpr explicit ipv6_address(const ipv4_address& __a) noexcept {
    __b_[10] = 0xff;
    __b_[11] = 0xff;
    auto __v = __a.to_bytes();
    for (int __i = 0; __i != 4; ++__i)
      __b_[12 + __i] = __v[__i];
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr uint32_t scope_id() const noexcept { return __scope_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bytes_type to_bytes() const noexcept { return __b_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_unspecified() const noexcept {
    for (unsigned char __c : __b_)
      if (__c != 0)
        return false;
    return true;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_loopback() const noexcept {
    for (int __i = 0; __i != 15; ++__i)
      if (__b_[__i] != 0)
        return false;
    return __b_[15] == 1;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_v4_mapped() const noexcept {
    for (int __i = 0; __i != 10; ++__i)
      if (__b_[__i] != 0)
        return false;
    return __b_[10] == 0xff && __b_[11] == 0xff;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_multicast() const noexcept { return __b_[0] == 0xff; }

  // The inverse of the mapping constructor; throws system_error(errc::address_family_not_supported) if the address
  // is not v4-mapped.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI ipv4_address to_v4() const {
    if (!is_v4_mapped())
      std::__throw_system_error(static_cast<int>(errc::address_family_not_supported), "address is not IPv4-mapped");
    return ipv4_address(ipv4_address::bytes_type{__b_[12], __b_[13], __b_[14], __b_[15]});
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string to_string() const {
    static constexpr char __hex[] = "0123456789abcdef";
    uint16_t __w[8];
    for (int __i = 0; __i != 8; ++__i)
      __w[__i] = static_cast<uint16_t>((__b_[2 * __i] << 8) | __b_[2 * __i + 1]);
    string __s;
    if (is_v4_mapped()) {
      __s = "::ffff:";
      __s += to_v4().to_string();
    } else {
      // The longest run of zero groups, if it has at least two.
      int __best = -1, __best_len = 0;
      for (int __i = 0; __i < 8;) {
        if (__w[__i] != 0) {
          ++__i;
          continue;
        }
        int __j = __i;
        while (__j < 8 && __w[__j] == 0)
          ++__j;
        if (__j - __i > __best_len) {
          __best     = __i;
          __best_len = __j - __i;
        }
        __i = __j;
      }
      if (__best_len < 2)
        __best = -1;
      for (int __i = 0; __i < 8;) {
        if (__i == __best) {
          __s += "::";
          __i += __best_len;
          continue;
        }
        if (!__s.empty() && __s.back() != ':')
          __s += ':';
        bool __lead = false;
        for (int __sh = 12; __sh >= 0; __sh -= 4) {
          unsigned __d = (__w[__i] >> __sh) & 0xf;
          if (__d != 0 || __lead || __sh == 0) {
            __s += __hex[__d];
            __lead = true;
          }
        }
        ++__i;
      }
    }
    if (__scope_ != 0) {
      __s += '%';
      __s += std::to_string(__scope_);
    }
    return __s;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr ipv6_address any() noexcept { return ipv6_address(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr ipv6_address loopback() noexcept {
    bytes_type __b{};
    __b[15] = 1;
    return ipv6_address(__b);
  }

  // Equal if the bytes and the zone are equal.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const ipv6_address& __a, const ipv6_address& __b) noexcept {
    return __a.__b_ == __b.__b_ && __a.__scope_ == __b.__scope_;
  }
  // Lexicographic by the bytes in network order, then by the zone.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr strong_ordering
  operator<=>(const ipv6_address& __a, const ipv6_address& __b) noexcept {
    for (int __i = 0; __i != 16; ++__i)
      if (__a.__b_[__i] != __b.__b_[__i])
        return __a.__b_[__i] <=> __b.__b_[__i];
    return __a.__scope_ <=> __b.__scope_;
  }
};

// Parses [__first, __last) completely: IPv6address of RFC 4291 / RFC 3986 with an optional "%zone" where the zone is a
// decimal interface index.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool
__parse_ipv6(const char* __first, const char* __last, ipv6_address::bytes_type& __res, uint32_t& __scope) noexcept {
  __scope           = 0;
  const char* __pct = __first;
  while (__pct != __last && *__pct != '%')
    ++__pct;
  if (__pct != __last) {
    const char* __z = __pct + 1;
    if (__z == __last || __last - __z > 10)
      return false;
    uint64_t __v = 0;
    for (; __z != __last; ++__z) {
      if (*__z < '0' || *__z > '9')
        return false;
      __v = __v * 10 + static_cast<uint64_t>(*__z - '0');
    }
    if (__v > 0xffffffffu)
      return false;
    __scope = static_cast<uint32_t>(__v);
    __last  = __pct;
  }

  uint16_t __w[8] = {};
  int __n         = 0;
  int __gap       = -1;
  const char* __i = __first;
  if (__i == __last)
    return false;
  if (*__i == ':') {
    if (__last - __i < 2 || __i[1] != ':')
      return false;
    __gap = 0;
    __i += 2;
  }
  while (__i != __last) {
    const char* __start = __i;
    uint32_t __val      = 0;
    while (__i != __last && __i - __start < 4) {
      char __c = *__i;
      uint32_t __d;
      if (__c >= '0' && __c <= '9')
        __d = static_cast<uint32_t>(__c - '0');
      else if (__c >= 'a' && __c <= 'f')
        __d = static_cast<uint32_t>(__c - 'a' + 10);
      else if (__c >= 'A' && __c <= 'F')
        __d = static_cast<uint32_t>(__c - 'A' + 10);
      else
        break;
      __val = __val * 16 + __d;
      ++__i;
    }
    if (__i == __start)
      return false;
    if (__i != __last && *__i == '.') { // an embedded IPv4 address ends the address
      uint32_t __v4;
      if (__n > 6 || !io::__parse_ipv4(__start, __last, __v4))
        return false;
      __w[__n++] = static_cast<uint16_t>(__v4 >> 16);
      __w[__n++] = static_cast<uint16_t>(__v4 & 0xffff);
      __i        = __last;
      break;
    }
    if (__n == 8)
      return false;
    __w[__n++] = static_cast<uint16_t>(__val);
    if (__i == __last)
      break;
    if (*__i != ':')
      return false;
    ++__i;
    if (__i != __last && *__i == ':') {
      if (__gap >= 0)
        return false;
      __gap = __n;
      ++__i;
    } else if (__i == __last) {
      return false; // a single trailing colon
    }
  }
  if (__gap < 0) {
    if (__n != 8)
      return false;
  } else if (__n > 7) {
    return false;
  }
  uint16_t __all[8] = {};
  if (__gap < 0) {
    for (int __k = 0; __k != 8; ++__k)
      __all[__k] = __w[__k];
  } else {
    for (int __k = 0; __k != __gap; ++__k)
      __all[__k] = __w[__k];
    for (int __k = __gap; __k != __n; ++__k)
      __all[8 - (__n - __k)] = __w[__k];
  }
  for (int __k = 0; __k != 8; ++__k) {
    __res[2 * __k]     = static_cast<unsigned char>(__all[__k] >> 8);
    __res[2 * __k + 1] = static_cast<unsigned char>(__all[__k] & 0xff);
  }
  return true;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline io_result<ipv6_address> make_ipv6_address(string_view __s) noexcept {
  ipv6_address::bytes_type __b{};
  uint32_t __scope;
  if (!io::__parse_ipv6(__s.data(), __s.data() + __s.size(), __b, __scope))
    return io_result<ipv6_address>(make_error_code(errc::invalid_argument), ipv6_address());
  return io_result<ipv6_address>(error_code(), ipv6_address(__b, __scope));
}

} // namespace io

template <>
struct hash<io::ipv6_address> {
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const io::ipv6_address& __a) const noexcept {
    size_t __h = hash<uint32_t>()(__a.scope_id());
    for (unsigned char __c : __a.to_bytes())
      __h = __h * 1099511628211ull + __c;
    return __h;
  }
};

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_IPV6_ADDRESS_H
