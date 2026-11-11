//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TCP_ACCEPTOR_H
#define _LIBCPP___NET_TCP_ACCEPTOR_H

#include <__config>
#include <__coroutine/coroutine_traits.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__net/endpoint.h>
#include <__net/family.h>
#include <__net/socket_core.h>
#include <__net/socket_option.h>
#include <__net/support/posix_socket.h>
#include <__net/tcp_socket.h>
#include <__system_error/error_code.h>
#include <__system_error/throw_system_error.h>
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

// P4100R1 [8.9]; provisional specification: Corosio's tcp_acceptor. A listening TCP socket. The convenience
// constructor opens, sets reuse_address (so that a server can bind its port again right after a restart), binds and
// listens, and throws system_error on failure; the piecewise open(), set_option(), bind() and listen() report errors
// as error codes and do not set reuse_address. An IPv6 acceptor is dual-stack (v6_only is cleared by open()).
// Same threading rules as tcp_socket; the destructor closes.
class tcp_acceptor : public __socket_core {
public:
  using endpoint_type = io::endpoint;

  _LIBCPP_HIDE_FROM_ABI explicit tcp_acceptor(execution_context& __ctx) noexcept : __socket_core(__ctx) {}
  // Opens (with the family of __ep), sets reuse_address (not on Windows, where the option means something else), binds
  // and listens; throws system_error.
  _LIBCPP_HIDE_FROM_ABI tcp_acceptor(execution_context& __ctx, const io::endpoint& __ep, int __backlog = 128)
      : __socket_core(__ctx) {
    error_code __ec = open(__ep.is_v6() ? io::family::v6 : io::family::v4);
#  if !_LIBCPP_HAS_IO_WINSOCK // on Windows SO_REUSEADDR lets another socket bind the port that is in use; it is not set
    if (!__ec)
      __ec = set_option(socket_option::reuse_address(true));
#  endif
    if (!__ec)
      __ec = bind(__ep);
    if (!__ec)
      __ec = listen(__backlog);
    if (__ec) {
      close();
      std::__throw_system_error(__ec.value(), "tcp_acceptor");
    }
  }
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, tcp_acceptor>)
  _LIBCPP_HIDE_FROM_ABI explicit tcp_acceptor(const _Ex& __ex) noexcept : tcp_acceptor(__ex.context()) {}
  template <Executor _Ex>
  _LIBCPP_HIDE_FROM_ABI tcp_acceptor(const _Ex& __ex, const io::endpoint& __ep, int __backlog = 128)
      : tcp_acceptor(__ex.context(), __ep, __backlog) {}

  _LIBCPP_HIDE_FROM_ABI tcp_acceptor(tcp_acceptor&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI tcp_acceptor& operator=(tcp_acceptor&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open(io::family __f = io::family::v4) noexcept {
    if (is_open())
      return error_code();
    if (error_code __ec = __open(__f == io::family::v6 ? __sock_domain::__v6 : __sock_domain::__v4))
      return __ec;
    if (__f == io::family::v6)
      (void)set_option(socket_option::v6_only(false));
    return error_code();
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code bind(const io::endpoint& __ep) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_bind(native_handle(), io::__to_sockaddr(__ep));
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code listen(int __backlog = 128) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_listen(native_handle(), __backlog);
  }

  // Releases the descriptor. Pending accepts complete with error::canceled.
  _LIBCPP_HIDE_FROM_ABI void close() noexcept { __close(); }

  // Accepts one connection; the socket is on the context of the acceptor. On failure the socket is closed.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<tcp_socket>> accept() { return __accept_socket(); }

  // The address the acceptor is bound to (useful after binding port 0); the default endpoint if it is not open.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::endpoint local_endpoint() const noexcept {
    __sockaddr_buf __a;
    if (!is_open() || io::__sock_local_address(native_handle(), __a))
      return io::endpoint();
    return io::__from_sockaddr(__a);
  }

private:
  _LIBCPP_HIDE_FROM_ABI task<io_result<tcp_socket>> __accept_socket() {
    io::family __f    = __family();
    auto [__ec, __fd] = co_await __accept_raw();
    if (__ec)
      co_return io_result<tcp_socket>(__ec, tcp_socket(__context()));
    co_return io_result<tcp_socket>(error_code(), tcp_socket(__context(), __fd, __f));
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_TCP_ACCEPTOR_H
