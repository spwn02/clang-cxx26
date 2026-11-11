//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_SOCKET_OPTION_H
#define _LIBCPP___NET_SOCKET_OPTION_H

#include <__config>
#include <__io/support/config.h>
#include <__net/ip_address.h>
#include <__net/ipv4_address.h>
#include <__net/ipv6_address.h>
#include <__net/support/posix_socket.h>
#include <__system_error/error_code.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's socket_option. A socket option is a small value type with
// set()/get() access through tcp_socket::set_option(opt) and tcp_socket::get_option<Opt>(). Deviation: Corosio's
// options expose level()/name()/data()/size() for the raw system call; here the system call is the platform layer's
// business, so an option has only the members below (the implementation hooks are private to the library).
namespace socket_option {

// A boolean option stored as an int.
template <__sockopt_id _Id>
class __boolean_option {
  int __v_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI __boolean_option() = default;
  _LIBCPP_HIDE_FROM_ABI explicit __boolean_option(bool __b) noexcept : __v_(__b ? 1 : 0) {}
  _LIBCPP_HIDE_FROM_ABI __boolean_option& operator=(bool __b) noexcept {
    __v_ = __b ? 1 : 0;
    return *this;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool value() const noexcept { return __v_ != 0; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return __v_ != 0; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool operator!() const noexcept { return __v_ == 0; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __set(__native_socket __fd) const noexcept {
    return io::__sock_set_int(__fd, _Id, __v_);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __get(__native_socket __fd) noexcept {
    int __r;
    if (error_code __ec = io::__sock_get_int(__fd, _Id, __r))
      return __ec;
    __v_ = __r != 0 ? 1 : 0;
    return error_code();
  }
};

// An integer option.
template <__sockopt_id _Id>
class __integer_option {
  int __v_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI __integer_option() = default;
  _LIBCPP_HIDE_FROM_ABI explicit __integer_option(int __i) noexcept : __v_(__i) {}
  _LIBCPP_HIDE_FROM_ABI __integer_option& operator=(int __i) noexcept {
    __v_ = __i;
    return *this;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI int value() const noexcept { return __v_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __set(__native_socket __fd) const noexcept {
    return io::__sock_set_int(__fd, _Id, __v_);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __get(__native_socket __fd) noexcept {
    return io::__sock_get_int(__fd, _Id, __v_);
  }
};

using reuse_address _LIBCPP_NODEBUG       = __boolean_option<__sockopt_id::__reuse_address>;       // SO_REUSEADDR
using reuse_port _LIBCPP_NODEBUG          = __boolean_option<__sockopt_id::__reuse_port>;          // SO_REUSEPORT
using no_delay _LIBCPP_NODEBUG            = __boolean_option<__sockopt_id::__no_delay>;            // TCP_NODELAY
using keep_alive _LIBCPP_NODEBUG          = __boolean_option<__sockopt_id::__keep_alive>;          // SO_KEEPALIVE
using v6_only _LIBCPP_NODEBUG             = __boolean_option<__sockopt_id::__v6_only>;             // IPV6_V6ONLY
using receive_buffer_size _LIBCPP_NODEBUG = __integer_option<__sockopt_id::__receive_buffer_size>; // SO_RCVBUF
using send_buffer_size _LIBCPP_NODEBUG    = __integer_option<__sockopt_id::__send_buffer_size>;    // SO_SNDBUF

using broadcast _LIBCPP_NODEBUG = __boolean_option<__sockopt_id::__broadcast>; // SO_BROADCAST
using multicast_loop _LIBCPP_NODEBUG =
    __boolean_option<__sockopt_id::__multicast_loop>; // IP_MULTICAST_LOOP, IPV6_MULTICAST_LOOP
using multicast_hops _LIBCPP_NODEBUG =
    __integer_option<__sockopt_id::__multicast_hops>; // IP_MULTICAST_TTL, IPV6_MULTICAST_HOPS

// SO_LINGER: whether close() blocks for the unsent data and for how many seconds.
class linger {
  bool __on_ = false;
  int __sec_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI linger() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI linger(bool __enabled, int __timeout) noexcept : __on_(__enabled), __sec_(__timeout) {}
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool enabled() const noexcept { return __on_; }
  _LIBCPP_HIDE_FROM_ABI void enabled(bool __v) noexcept { __on_ = __v; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI int timeout() const noexcept { return __sec_; }
  _LIBCPP_HIDE_FROM_ABI void timeout(int __v) noexcept { __sec_ = __v; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __set(__native_socket __fd) const noexcept {
    return io::__sock_set_linger(__fd, __on_, __sec_);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __get(__native_socket __fd) noexcept {
    return io::__sock_get_linger(__fd, __on_, __sec_);
  }
};

// Membership of a multicast group (IP_ADD_MEMBERSHIP / IP_DROP_MEMBERSHIP, IPV6_JOIN_GROUP / IPV6_LEAVE_GROUP): set
// with set_option() only (there is no value to read back). The family of the group, not that of the socket, selects the
// system call. Deviation: Corosio's options render themselves for the family of the socket (level(), name(), data());
// the options here look at the socket in the platform layer, and the multicast options of the dual-stack case are the
// system's.
template <bool _Join>
class __group_membership {
  ip_address __group_;
  ipv4_address __iface_;
  unsigned __if_index_ = 0;

public:
  _LIBCPP_HIDE_FROM_ABI __group_membership() noexcept = default;
  // The group; the interface is chosen by the system (for an IPv6 group by its zone).
  _LIBCPP_HIDE_FROM_ABI explicit __group_membership(const ip_address& __g) noexcept : __group_(__g) {}
  // An IPv4 group and the address of the local interface (any by default).
  _LIBCPP_HIDE_FROM_ABI
  __group_membership(const ipv4_address& __g, const ipv4_address& __iface = ipv4_address()) noexcept
      : __group_(__g), __iface_(__iface) {}
  // An IPv6 group and the index of the interface (0: the zone of the group, then the choice of the system).
  _LIBCPP_HIDE_FROM_ABI __group_membership(const ipv6_address& __g, unsigned __if_index = 0) noexcept
      : __group_(__g), __if_index_(__if_index) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI ip_address group() const noexcept { return __group_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __set(__native_socket __fd) const noexcept {
    return io::__sock_membership(__fd, _Join, __group_, __iface_, __if_index_);
  }
};

using join_group _LIBCPP_NODEBUG  = __group_membership<true>;
using leave_group _LIBCPP_NODEBUG = __group_membership<false>;

} // namespace socket_option

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_SOCKET_OPTION_H
