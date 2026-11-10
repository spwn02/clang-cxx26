//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_LOCAL_STREAM_SOCKET_H
#define _LIBCPP___NET_LOCAL_STREAM_SOCKET_H

#include <__config>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__net/local_endpoint.h>
#include <__net/socket_core.h>
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

class local_stream_acceptor;

// P4100R1 [8.9]; provisional specification: Corosio's local_stream_socket. A Unix domain stream socket (AF_UNIX,
// SOCK_STREAM) with the interface and the rules of tcp_socket; it satisfies Stream. The endpoint is a path or, on
// Linux, an abstract name.
class local_stream_socket : public __stream_socket_base {
  friend class local_stream_acceptor;

  _LIBCPP_HIDE_FROM_ABI local_stream_socket(execution_context& __ctx, __native_socket __fd) noexcept
      : __stream_socket_base(__ctx) {
    __adopt(__fd, io::family::v4);
  }

public:
  using endpoint_type = io::local_endpoint;

  _LIBCPP_HIDE_FROM_ABI explicit local_stream_socket(execution_context& __ctx) noexcept : __stream_socket_base(__ctx) {}
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, local_stream_socket>)
  _LIBCPP_HIDE_FROM_ABI explicit local_stream_socket(const _Ex& __ex) noexcept : local_stream_socket(__ex.context()) {}

  _LIBCPP_HIDE_FROM_ABI local_stream_socket(local_stream_socket&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI local_stream_socket& operator=(local_stream_socket&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open() noexcept { return __open(__sock_domain::__local); }

  _LIBCPP_HIDE_FROM_ABI void close() noexcept { __close(); }

  // Connects to the path (opening the socket if necessary). Connecting to a path nobody listens on fails with
  // errc::connection_refused or errc::no_such_file_or_directory at once.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<>> connect(io::local_endpoint __ep) {
    return __connect_raw(__sock_domain::__local, io::__to_sockaddr(__ep));
  }

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

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_LOCAL_STREAM_SOCKET_H
