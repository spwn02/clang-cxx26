//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_CONNECT_H
#define _LIBCPP___NET_CONNECT_H

#include <__config>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__io_stream/error.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__net/endpoint.h>
#include <__net/local_endpoint.h>
#include <__ranges/concepts.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_convertible.h>
#include <__utility/move.h>
#include <concepts>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

struct __always_connect_condition {
  template <class _Ep>
  _LIBCPP_HIDE_FROM_ABI bool operator()(const error_code&, const _Ep&) const noexcept {
    return true;
  }
};

// P4100R1 [8.9]; provisional specification: Corosio's connect (range-based composed connect, after Asio). Tries the
// endpoints in order and stops at the first connection. Between two attempts the socket is closed so that the next
// attempt opens it with the family of its endpoint (so options set before connect() are lost: set them afterwards).
// The socket must have connect(endpoint) (an IoAwaitable of io_result<>), is_open() and close() and a nested
// endpoint_type: tcp_socket and local_stream_socket (and the datagram sockets, to set their default peer).
//
// Completes with the connected endpoint; if every attempt failed, with the error of the last one and the empty
// endpoint; if the range is empty (or the condition rejected every endpoint), with errc::no_such_device_or_address. An
// attempt that is cancelled ends the operation at once with its error. The condition is called as
// cond(error_code of the previous attempt, endpoint) and skips the endpoint if it returns false.
//
// The range is taken by value (pass an rvalue or use the iterator overloads to avoid the copy); the socket must outlive
// the operation.
template <class _Socket>
concept __connectable_socket = requires(_Socket& __s, typename _Socket::endpoint_type __ep) {
  { __s.is_open() } -> convertible_to<bool>;
  __s.close();
  __s.connect(__ep);
};

template <class _Socket, class _Range, class _Condition>
  requires __connectable_socket<_Socket> && ranges::input_range<_Range> &&
           convertible_to<ranges::range_reference_t<_Range>, typename _Socket::endpoint_type> &&
           predicate<_Condition&, const error_code&, const typename _Socket::endpoint_type&>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<typename _Socket::endpoint_type>>
connect(_Socket& __s, _Range __endpoints, _Condition __cond) {
  using _Ep = typename _Socket::endpoint_type;
  error_code __last;
  for (auto&& __e : __endpoints) {
    _Ep __ep = __e;
    if (!__cond(static_cast<const error_code&>(__last), static_cast<const _Ep&>(__ep)))
      continue;
    if (__s.is_open())
      __s.close();
    auto [__ec] = co_await __s.connect(__ep);
    if (!__ec)
      co_return io_result<_Ep>(error_code(), __ep);
    if (__ec == make_error_condition(cond::canceled))
      co_return io_result<_Ep>(__ec, _Ep());
    __last = __ec;
  }
  if (!__last)
    __last = make_error_code(errc::no_such_device_or_address);
  co_return io_result<_Ep>(__last, _Ep());
}

template <class _Socket, class _Range>
  requires __connectable_socket<_Socket> && ranges::input_range<_Range> &&
           convertible_to<ranges::range_reference_t<_Range>, typename _Socket::endpoint_type>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<typename _Socket::endpoint_type>>
connect(_Socket& __s, _Range __endpoints) {
  return io::connect(__s, std::move(__endpoints), __always_connect_condition());
}

// The iterator form completes with the iterator of the connected endpoint, or with __last on failure.
template <class _Socket, class _Iter, class _Condition>
  requires __connectable_socket<_Socket> && input_iterator<_Iter> &&
           convertible_to<iter_reference_t<_Iter>, typename _Socket::endpoint_type> &&
           predicate<_Condition&, const error_code&, const typename _Socket::endpoint_type&>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<_Iter>> connect(_Socket& __s, _Iter __first, _Iter __last, _Condition __cond) {
  using _Ep = typename _Socket::endpoint_type;
  error_code __lasterr;
  for (_Iter __it = __first; __it != __last; ++__it) {
    _Ep __ep = *__it;
    if (!__cond(static_cast<const error_code&>(__lasterr), static_cast<const _Ep&>(__ep)))
      continue;
    if (__s.is_open())
      __s.close();
    auto [__ec] = co_await __s.connect(__ep);
    if (!__ec)
      co_return io_result<_Iter>(error_code(), __it);
    if (__ec == make_error_condition(cond::canceled))
      co_return io_result<_Iter>(__ec, __last);
    __lasterr = __ec;
  }
  if (!__lasterr)
    __lasterr = make_error_code(errc::no_such_device_or_address);
  co_return io_result<_Iter>(__lasterr, __last);
}

template <class _Socket, class _Iter>
  requires __connectable_socket<_Socket> && input_iterator<_Iter> &&
           convertible_to<iter_reference_t<_Iter>, typename _Socket::endpoint_type>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<_Iter>> connect(_Socket& __s, _Iter __first, _Iter __last) {
  return io::connect(__s, std::move(__first), std::move(__last), __always_connect_condition());
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_CONNECT_H
