//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_LOCAL_STREAM_ACCEPTOR_H
#define _LIBCPP___NET_LOCAL_STREAM_ACCEPTOR_H

#include <__config>
#include <__coroutine/coroutine_traits.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__net/local_endpoint.h>
#include <__net/local_stream_socket.h>
#include <__net/socket_core.h>
#include <__net/support/posix_socket.h>
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

// Whether bind() removes an existing file system entry at the path first (a stale socket file of a previous run).
enum class bind_option { none, unlink_existing };

// P4100R1 [8.9]; provisional specification: Corosio's local_stream_acceptor. A listening Unix domain stream socket.
// The library never unlinks the socket path on close: that is the caller's job (or use bind_option::unlink_existing
// before the next bind).
class local_stream_acceptor : public __socket_core {
public:
  using endpoint_type = io::local_endpoint;

  _LIBCPP_HIDE_FROM_ABI explicit local_stream_acceptor(execution_context& __ctx) noexcept : __socket_core(__ctx) {}
  // Opens, binds and listens; throws system_error.
  _LIBCPP_HIDE_FROM_ABI local_stream_acceptor(
      execution_context& __ctx,
      const io::local_endpoint& __ep,
      int __backlog     = 128,
      bind_option __opt = bind_option::none)
      : __socket_core(__ctx) {
    error_code __ec = open();
    if (!__ec)
      __ec = bind(__ep, __opt);
    if (!__ec)
      __ec = listen(__backlog);
    if (__ec) {
      close();
      std::__throw_system_error(__ec.value(), "local_stream_acceptor");
    }
  }
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, local_stream_acceptor>)
  _LIBCPP_HIDE_FROM_ABI explicit local_stream_acceptor(const _Ex& __ex) noexcept
      : local_stream_acceptor(__ex.context()) {}

  _LIBCPP_HIDE_FROM_ABI local_stream_acceptor(local_stream_acceptor&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI local_stream_acceptor& operator=(local_stream_acceptor&&) noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open() noexcept { return __open(__sock_domain::__local); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code
  bind(const io::local_endpoint& __ep, bind_option __opt = bind_option::none) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    if (__opt == bind_option::unlink_existing)
      io::__sock_unlink(__ep);
    return io::__sock_bind(native_handle(), io::__to_sockaddr(__ep));
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code listen(int __backlog = 128) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_listen(native_handle(), __backlog);
  }

  _LIBCPP_HIDE_FROM_ABI void close() noexcept { __close(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<local_stream_socket>> accept() { return __accept_socket(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::local_endpoint local_endpoint() const noexcept {
    __sockaddr_buf __a;
    if (!is_open() || io::__sock_local_address(native_handle(), __a))
      return io::local_endpoint();
    return io::__from_sockaddr_local(__a);
  }

private:
  _LIBCPP_HIDE_FROM_ABI task<io_result<local_stream_socket>> __accept_socket() {
    auto [__ec, __fd] = co_await __accept_raw();
    if (__ec)
      co_return io_result<local_stream_socket>(__ec, local_stream_socket(__context()));
    co_return io_result<local_stream_socket>(error_code(), local_stream_socket(__context(), __fd));
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_LOCAL_STREAM_ACCEPTOR_H
