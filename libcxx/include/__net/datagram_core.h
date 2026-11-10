//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_DATAGRAM_CORE_H
#define _LIBCPP___NET_DATAGRAM_CORE_H

#include <__config>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_stream/error.h>
#include <__net/endpoint.h>
#include <__net/local_endpoint.h>
#include <__net/message_flags.h>
#include <__net/socket_core.h>
#include <__net/support/posix_socket.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_same.h>
#include <__utility/move.h>
#include <cstddef>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// What the datagram sockets (UDP, Unix domain datagram) have in common, with _Ep the type of their endpoints
// (io::endpoint or io::local_endpoint). A datagram is a unit: a send transfers one datagram or fails, a receive returns
// one datagram (an empty datagram is a valid result of 0 bytes, not the end of a stream; a datagram that is larger than
// the buffers is truncated, as the system does it).
template <class _Ep>
class __datagram_socket_base : public __socket_core {
  _LIBCPP_HIDE_FROM_ABI static _Ep __to_endpoint(const __sockaddr_buf& __a) noexcept {
    if constexpr (is_same_v<_Ep, io::endpoint>)
      return io::__from_sockaddr(__a);
    else
      return io::__from_sockaddr_local(__a);
  }

  template <class _Seq>
  _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __send_to_op(_Seq __seq, _Ep __dest, message_flags __flags) {
    const_buffer __bufs[__sock_max_buffers];
    size_t __n            = io::__collect_buffers(__seq, __bufs);
    __sockaddr_buf __addr = io::__to_sockaddr(__dest);
    for (;;) {
      size_t __got    = 0;
      error_code __ec = io::__sock_send_to(native_handle(), __bufs, __n, static_cast<int>(__flags), &__addr, __got);
      if (!__ec)
        co_return io_result<size_t>(error_code(), __got);
      if (!io::__is_would_block(__ec))
        co_return io_result<size_t>(__ec, 0);
      auto [__wec, __ev] = co_await __wait_for(__poll_write);
      if (__wec)
        co_return io_result<size_t>(__wec, 0);
    }
  }

  template <class _Seq>
  _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __send_op(_Seq __seq, message_flags __flags) {
    const_buffer __bufs[__sock_max_buffers];
    size_t __n = io::__collect_buffers(__seq, __bufs);
    for (;;) {
      size_t __got    = 0;
      error_code __ec = io::__sock_send_to(native_handle(), __bufs, __n, static_cast<int>(__flags), nullptr, __got);
      if (!__ec)
        co_return io_result<size_t>(error_code(), __got);
      if (!io::__is_would_block(__ec))
        co_return io_result<size_t>(__ec, 0);
      auto [__wec, __ev] = co_await __wait_for(__poll_write);
      if (__wec)
        co_return io_result<size_t>(__wec, 0);
    }
  }

  template <class _Seq>
  _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __receive_from_op(_Seq __seq, _Ep& __source, message_flags __flags) {
    mutable_buffer __bufs[__sock_max_buffers];
    size_t __n = io::__collect_buffers(__seq, __bufs);
    for (;;) {
      size_t __got = 0;
      __sockaddr_buf __from;
      error_code __ec =
          io::__sock_receive_from(native_handle(), __bufs, __n, static_cast<int>(__flags), __from, __got);
      if (!__ec) {
        __source = __to_endpoint(__from);
        co_return io_result<size_t>(error_code(), __got);
      }
      if (!io::__is_would_block(__ec))
        co_return io_result<size_t>(__ec, 0);
      auto [__wec, __ev] = co_await __wait_for(__poll_read);
      if (__wec)
        co_return io_result<size_t>(__wec, 0);
    }
  }

  template <class _Seq>
  _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __receive_op(_Seq __seq, message_flags __flags) {
    mutable_buffer __bufs[__sock_max_buffers];
    size_t __n = io::__collect_buffers(__seq, __bufs);
    for (;;) {
      size_t __got = 0;
      __sockaddr_buf __from;
      error_code __ec =
          io::__sock_receive_from(native_handle(), __bufs, __n, static_cast<int>(__flags), __from, __got);
      if (!__ec)
        co_return io_result<size_t>(error_code(), __got);
      if (!io::__is_would_block(__ec))
        co_return io_result<size_t>(__ec, 0);
      auto [__wec, __ev] = co_await __wait_for(__poll_read);
      if (__wec)
        co_return io_result<size_t>(__wec, 0);
    }
  }

protected:
  using __socket_core::__socket_core;

public:
  using endpoint_type = _Ep;

  // Sends the buffers as one datagram to the destination. A closed socket reports errc::bad_file_descriptor. The buffer
  // sequence is copied into the operation; the bytes it refers to must stay valid until it completes.
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>>
  send_to(_Seq __seq, _Ep __dest, message_flags __flags = message_flags::none) {
    return this->template __send_to_op<_Seq>(std::move(__seq), __dest, __flags);
  }
  // Receives one datagram into the buffers; the address of the sender goes to __source (which must stay valid until the
  // operation completes). Completes with the size of the datagram, which may be 0.
  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>>
  receive_from(_Seq __seq, _Ep& __source, message_flags __flags = message_flags::none) {
    return this->template __receive_from_op<_Seq>(std::move(__seq), __source, __flags);
  }
  // Corosio's name for receive_from.
  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>>
  recv_from(_Seq __seq, _Ep& __source, message_flags __flags = message_flags::none) {
    return this->template __receive_from_op<_Seq>(std::move(__seq), __source, __flags);
  }

  // The connected mode (after connect()): the datagrams go to the peer, and only the peer's datagrams are received.
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> send(_Seq __seq, message_flags __flags = message_flags::none) {
    return this->template __send_op<_Seq>(std::move(__seq), __flags);
  }
  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>>
  receive(_Seq __seq, message_flags __flags = message_flags::none) {
    return this->template __receive_op<_Seq>(std::move(__seq), __flags);
  }
  // Corosio's name for receive.
  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> recv(_Seq __seq, message_flags __flags = message_flags::none) {
    return this->template __receive_op<_Seq>(std::move(__seq), __flags);
  }

  // Sets the default peer (opening the socket with the family of the endpoint if necessary). Datagrams from other
  // senders are discarded by the system from then on. For UDP and Unix domain sockets this completes at once.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<>> connect(_Ep __ep) {
    return __connect_dgram(__ep);
  }

  // Adopts a datagram socket created elsewhere (inherited, received, socketpair). The descriptor must already be
  // non-blocking and close-on-exec; the object must be closed (errc::already_connected otherwise, and the descriptor
  // stays with the caller).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code assign(__native_socket __fd) noexcept {
    if (is_open())
      return make_error_code(errc::already_connected);
    __adopt(__fd, io::family::v4);
    return error_code();
  }

  // Releases the descriptor and returns it; pending operations complete with error::canceled.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io_result<__native_socket> release() noexcept {
    if (!is_open())
      return io_result<__native_socket>(make_error_code(errc::bad_file_descriptor), __invalid_socket);
    return io_result<__native_socket>(error_code(), __detach());
  }

  // Disables sending and/or receiving (the connected mode).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code shutdown(shutdown_type __what) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_shutdown(native_handle(), __what);
  }

private:
  _LIBCPP_HIDE_FROM_ABI task<io_result<>> __connect_dgram(_Ep __ep) {
    return __connect_raw(__domain_of(__ep), io::__to_sockaddr(__ep), __sock_type::__datagram);
  }
  _LIBCPP_HIDE_FROM_ABI static __sock_domain __domain_of(const _Ep& __ep) noexcept {
    if constexpr (is_same_v<_Ep, io::endpoint>)
      return __ep.is_v6() ? __sock_domain::__v6 : __sock_domain::__v4;
    else
      return __sock_domain::__local;
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_DATAGRAM_CORE_H
