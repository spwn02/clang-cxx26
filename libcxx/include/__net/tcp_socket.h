//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TCP_SOCKET_H
#define _LIBCPP___NET_TCP_SOCKET_H

#include <__config>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__net/endpoint.h>
#include <__net/family.h>
#include <__net/socket_core.h>
#include <__net/socket_option.h>
#include <__net/support/posix_socket.h>
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

class tcp_acceptor;

// P4100R1 [8.9]; provisional specification: Corosio's tcp_socket. A TCP stream socket that satisfies Stream (and
// therefore converts to any_stream). Its operations are IoAwaitables: they complete on the executor of the awaiting
// coroutine, are cancelled through that coroutine's stop token or through cancel(), and report failure as an
// error_code (the errno value in generic_category; error::canceled and error::eof from the io category).
//
// The socket belongs to an execution_context that must be an io_context. One read and one write may be in flight at the
// same time; two operations of the same kind may not. The socket must outlive its operations, and a socket must not be
// moved or destroyed while an operation is pending except through close() or cancel() (the destructor closes).
class tcp_socket : public __stream_socket_base {
  friend class tcp_acceptor;

  // An accepted connection.
  _LIBCPP_HIDE_FROM_ABI tcp_socket(execution_context& __ctx, __native_socket __fd, io::family __f) noexcept
      : __stream_socket_base(__ctx) {
    __adopt(__fd, __f);
  }

public:
  using endpoint_type = io::endpoint;

  _LIBCPP_HIDE_FROM_ABI explicit tcp_socket(execution_context& __ctx) noexcept : __stream_socket_base(__ctx) {}
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, tcp_socket>)
  _LIBCPP_HIDE_FROM_ABI explicit tcp_socket(const _Ex& __ex) noexcept : tcp_socket(__ex.context()) {}

  _LIBCPP_HIDE_FROM_ABI tcp_socket(tcp_socket&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI tcp_socket& operator=(tcp_socket&&) noexcept = default;

  // Creates the socket. Opening an open socket succeeds without doing anything. connect() opens the socket by itself
  // with the family of the endpoint, so open() is needed only to set options before connecting.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open(io::family __f = io::family::v4) noexcept {
    return __open(__f == io::family::v6 ? __sock_domain::__v6 : __sock_domain::__v4);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code bind(const io::endpoint& __ep) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_bind(native_handle(), io::__to_sockaddr(__ep));
  }

  // Releases the descriptor. Pending operations complete with error::canceled.
  _LIBCPP_HIDE_FROM_ABI void close() noexcept { __close(); }

  // Connects to the endpoint (non-blocking connect, then a wait for writability and SO_ERROR). The socket is opened
  // with the family of the endpoint if it is not open. A connection that is cancelled leaves the socket in an
  // unspecified state: close it.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<>> connect(io::endpoint __ep) {
    return __connect_raw(__ep.is_v6() ? __sock_domain::__v6 : __sock_domain::__v4, io::__to_sockaddr(__ep));
  }

  // The address the socket is bound to; the default endpoint if the socket is not open or not bound.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::endpoint local_endpoint() const noexcept {
    __sockaddr_buf __a;
    if (!is_open() || io::__sock_local_address(native_handle(), __a))
      return io::endpoint();
    return io::__from_sockaddr(__a);
  }
  // The address of the peer; the default endpoint if the socket is not connected.
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

#endif // _LIBCPP___NET_TCP_SOCKET_H
