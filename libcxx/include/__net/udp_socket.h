//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_UDP_SOCKET_H
#define _LIBCPP___NET_UDP_SOCKET_H

#include <__config>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/support/config.h>
#include <__net/datagram_core.h>
#include <__net/endpoint.h>
#include <__net/family.h>
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

// P4100R1 [8.10]; provisional specification: Corosio's udp_socket. A UDP datagram socket in two modes: connectionless
// (send_to() names the destination of each datagram, receive_from() reports the sender) and connected (connect() sets the
// peer, send() and receive() need no endpoint, and the system discards the datagrams of other senders). The operations
// are IoAwaitables with the rules of tcp_socket (completion on the executor of the awaiting coroutine, cancellation
// through the stop token or cancel(), one operation of each direction at a time). Options: broadcast, multicast_loop,
// multicast_hops, join_group and leave_group in socket_option, besides those of the stream sockets.
//
// Deviations from Corosio: receive_from()/receive() are the names of P4100R1, recv_from()/recv() are kept as aliases;
// a datagram that is larger than the buffers is truncated without an indication (as in Corosio); there is no wait().
class udp_socket : public __datagram_socket_base<io::endpoint> {
public:
  _LIBCPP_HIDE_FROM_ABI explicit udp_socket(execution_context& __ctx) noexcept : __datagram_socket_base(__ctx) {}
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, udp_socket>)
  _LIBCPP_HIDE_FROM_ABI explicit udp_socket(const _Ex& __ex) noexcept : udp_socket(__ex.context()) {}

  _LIBCPP_HIDE_FROM_ABI udp_socket(udp_socket&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI udp_socket& operator=(udp_socket&&) noexcept = default;

  // Creates the socket. Opening an open socket succeeds without doing anything.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open(io::family __f = io::family::v4) noexcept {
    return __open(__f == io::family::v6 ? __sock_domain::__v6 : __sock_domain::__v4, __sock_type::__datagram);
  }

  // Associates the socket with a local address and port. receive_from() needs it (a socket that was not bound receives
  // nothing; a send_to() binds it to an ephemeral port).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code bind(const io::endpoint& __ep) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_bind(native_handle(), io::__to_sockaddr(__ep));
  }

  // Releases the descriptor. Pending operations complete with error::canceled.
  _LIBCPP_HIDE_FROM_ABI void close() noexcept { __close(); }

  // The address the socket is bound to; the default endpoint if the socket is not open or not bound.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::endpoint local_endpoint() const noexcept {
    __sockaddr_buf __a;
    if (!is_open() || io::__sock_local_address(native_handle(), __a))
      return io::endpoint();
    return io::__from_sockaddr(__a);
  }
  // The address of the peer of a connected socket; the default endpoint otherwise.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::endpoint remote_endpoint() const noexcept {
    __sockaddr_buf __a;
    if (!is_open() || io::__sock_remote_address(native_handle(), __a))
      return io::endpoint();
    return io::__from_sockaddr(__a);
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_UDP_SOCKET_H
