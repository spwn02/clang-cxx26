//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_LOCAL_DATAGRAM_SOCKET_H
#define _LIBCPP___NET_LOCAL_DATAGRAM_SOCKET_H

#include <__config>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/support/config.h>
#include <__net/datagram_core.h>
#include <__net/local_endpoint.h>
#include <__net/local_stream_socket.h>
#include <__net/support/posix_socket.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's local_datagram_socket. A Unix domain datagram socket (AF_UNIX,
// SOCK_DGRAM) with the interface and the rules of udp_socket, addressed by local_endpoint. A socket that sends without
// being bound has no address: the sender of such a datagram is reported as the empty local_endpoint. The path of a bound
// socket is not removed by the library.
class local_datagram_socket : public __datagram_socket_base<io::local_endpoint> {
public:
  _LIBCPP_HIDE_FROM_ABI explicit local_datagram_socket(execution_context& __ctx) noexcept
      : __datagram_socket_base(__ctx) {}
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, local_datagram_socket>)
  _LIBCPP_HIDE_FROM_ABI explicit local_datagram_socket(const _Ex& __ex) noexcept
      : local_datagram_socket(__ex.context()) {}

  _LIBCPP_HIDE_FROM_ABI local_datagram_socket(local_datagram_socket&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI local_datagram_socket& operator=(local_datagram_socket&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open() noexcept {
    return __open(__sock_domain::__local, __sock_type::__datagram);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code bind(const io::local_endpoint& __ep) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_bind(native_handle(), io::__to_sockaddr(__ep));
  }

  _LIBCPP_HIDE_FROM_ABI void close() noexcept { __close(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::local_endpoint local_endpoint() const noexcept {
    __sockaddr_buf __a;
    if (!is_open() || io::__sock_local_address(native_handle(), __a))
      return io::local_endpoint();
    return io::__from_sockaddr_local(__a);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::local_endpoint remote_endpoint() const noexcept {
    __sockaddr_buf __a;
    if (!is_open() || io::__sock_remote_address(native_handle(), __a))
      return io::local_endpoint();
    return io::__from_sockaddr_local(__a);
  }
};

// P4100R1 [8.9]; provisional specification: Corosio's connect_pair. Synchronously connects two closed sockets of a kind
// to each other (socketpair); the sockets may belong to any context. Fails with errc::already_connected if either is
// open (Corosio: error::already_open; both stay untouched), otherwise with the error of the system.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code connect_pair(local_datagram_socket& __a, local_datagram_socket& __b) noexcept {
  if (__a.is_open() || __b.is_open())
    return make_error_code(errc::already_connected);
  __native_socket __x, __y;
  if (error_code __ec = io::__sock_pair(__sock_type::__datagram, __x, __y))
    return __ec;
  (void)__a.assign(__x);
  (void)__b.assign(__y);
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code connect_pair(local_stream_socket& __a, local_stream_socket& __b) noexcept {
  if (__a.is_open() || __b.is_open())
    return make_error_code(errc::already_connected);
  __native_socket __x, __y;
  if (error_code __ec = io::__sock_pair(__sock_type::__stream, __x, __y))
    return __ec;
  (void)__a.assign(__x);
  (void)__b.assign(__y);
  return error_code();
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_LOCAL_DATAGRAM_SOCKET_H
