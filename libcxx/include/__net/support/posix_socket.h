//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_SUPPORT_POSIX_SOCKET_H
#define _LIBCPP___NET_SUPPORT_POSIX_SOCKET_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_SOCKETS
#  include <__io_buffer/buffer.h>
#  include <__net/endpoint.h>
#  include <__net/family.h>
#  include <__net/local_endpoint.h>
#  include <__system_error/errc.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
// The system socket headers use names such as __in and __out for their parameters, which are macros of some platform
// SDKs (SAL annotations) and of the tests of <__config> hygiene; they are hidden while the headers are included.
#  pragma push_macro("__in")
#  undef __in
#  pragma push_macro("__out")
#  undef __out
#  pragma push_macro("__inout")
#  undef __inout
#  pragma push_macro("__pre")
#  undef __pre
#  pragma push_macro("__post")
#  undef __post
#  pragma push_macro("__deref")
#  undef __deref
#  pragma push_macro("__bound")
#  undef __bound
#  pragma push_macro("__nz")
#  undef __nz
#  pragma push_macro("__part")
#  undef __part
#  pragma push_macro("__full")
#  undef __full
#  pragma push_macro("__allocator")
#  undef __allocator
#  pragma push_macro("__deallocate")
#  undef __deallocate
#  pragma push_macro("__format_string")
#  undef __format_string
#  pragma push_macro("__input")
#  undef __input
#  pragma push_macro("__output")
#  undef __output
#  pragma push_macro("__acquire")
#  undef __acquire
#  pragma push_macro("__release")
#  undef __release
#  include <cerrno>
#  include <cstddef>
#  include <cstdint>
#  include <cstring>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <string_view>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <sys/un.h>
#  include <unistd.h>
#  pragma pop_macro("__release")
#  pragma pop_macro("__acquire")
#  pragma pop_macro("__output")
#  pragma pop_macro("__input")
#  pragma pop_macro("__format_string")
#  pragma pop_macro("__deallocate")
#  pragma pop_macro("__allocator")
#  pragma pop_macro("__full")
#  pragma pop_macro("__part")
#  pragma pop_macro("__nz")
#  pragma pop_macro("__bound")
#  pragma pop_macro("__deref")
#  pragma pop_macro("__post")
#  pragma pop_macro("__pre")
#  pragma pop_macro("__inout")
#  pragma pop_macro("__out")
#  pragma pop_macro("__in")
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The platform layer of <net> for the BSD socket API (Linux): the only place that includes the system socket headers.
// A Windows or kqueue-based backend provides the same functions (the same names, the same meaning) in a sibling file.
//
// Every function is non-blocking and noexcept and reports failure as an error_code in generic_category (the errno
// value). __is_would_block tells "try again when the descriptor is ready" from a real error.

using __native_socket _LIBCPP_NODEBUG             = int;
inline constexpr __native_socket __invalid_socket = -1;

enum class __sock_domain { __v4, __v6, __local };
enum class __sock_type { __stream, __datagram };

// A socket address in the representation of the platform.
struct __sockaddr_buf {
  sockaddr_storage __ss_;
  socklen_t __len_;
};

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __errno_code(int __e) noexcept {
  return error_code(__e, generic_category());
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __last_error() noexcept { return __errno_code(errno); }

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline bool __is_would_block(const error_code& __ec) noexcept {
  return __ec == errc::resource_unavailable_try_again || __ec == errc::operation_would_block;
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline bool __is_in_progress(const error_code& __ec) noexcept {
  return __ec == errc::operation_in_progress || __ec == errc::interrupted;
}

// --- addresses

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline __sockaddr_buf __to_sockaddr(const endpoint& __ep) noexcept {
  __sockaddr_buf __b;
  ::memset(&__b.__ss_, 0, sizeof(__b.__ss_));
  if (__ep.is_v4()) {
    sockaddr_in __a;
    ::memset(&__a, 0, sizeof(__a));
    __a.sin_family      = AF_INET;
    __a.sin_port        = htons(__ep.port());
    __a.sin_addr.s_addr = htonl(__ep.address().to_v4().to_uint());
    ::memcpy(&__b.__ss_, &__a, sizeof(__a));
    __b.__len_ = sizeof(__a);
  } else {
    sockaddr_in6 __a;
    ::memset(&__a, 0, sizeof(__a));
    ipv6_address __v6 = __ep.address().to_v6();
    __a.sin6_family   = AF_INET6;
    __a.sin6_port     = htons(__ep.port());
    auto __bytes      = __v6.to_bytes();
    ::memcpy(&__a.sin6_addr, __bytes.data(), 16);
    __a.sin6_scope_id = __v6.scope_id();
    ::memcpy(&__b.__ss_, &__a, sizeof(__a));
    __b.__len_ = sizeof(__a);
  }
  return __b;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline endpoint __from_sockaddr(const __sockaddr_buf& __b) noexcept {
  if (__b.__ss_.ss_family == AF_INET) {
    sockaddr_in __a;
    ::memcpy(&__a, &__b.__ss_, sizeof(__a));
    return endpoint(ip_address(ipv4_address(ntohl(__a.sin_addr.s_addr))), ntohs(__a.sin_port));
  }
  if (__b.__ss_.ss_family == AF_INET6) {
    sockaddr_in6 __a;
    ::memcpy(&__a, &__b.__ss_, sizeof(__a));
    ipv6_address::bytes_type __bytes;
    ::memcpy(__bytes.data(), &__a.sin6_addr, 16);
    return endpoint(ip_address(ipv6_address(__bytes, __a.sin6_scope_id)), ntohs(__a.sin6_port));
  }
  return endpoint();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline __sockaddr_buf __to_sockaddr(const local_endpoint& __ep) noexcept {
  __sockaddr_buf __b;
  ::memset(&__b.__ss_, 0, sizeof(__b.__ss_));
  sockaddr_un __a;
  ::memset(&__a, 0, sizeof(__a));
  __a.sun_family  = AF_UNIX;
  string_view __p = __ep.path();
  ::memcpy(__a.sun_path, __p.data(), __p.size());
  ::memcpy(&__b.__ss_, &__a, sizeof(__a));
  // A path name includes its terminating NUL in the length, an abstract name (leading NUL) is exactly its bytes.
  __b.__len_ = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + __p.size() + (__ep.is_abstract() ? 0 : 1));
  if (__p.empty())
    __b.__len_ = sizeof(sa_family_t);
  return __b;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline local_endpoint __from_sockaddr_local(const __sockaddr_buf& __b) noexcept {
  if (__b.__ss_.ss_family != AF_UNIX || __b.__len_ <= offsetof(sockaddr_un, sun_path))
    return local_endpoint();
  sockaddr_un __a;
  ::memcpy(&__a, &__b.__ss_, sizeof(__a));
  size_t __n = __b.__len_ - offsetof(sockaddr_un, sun_path);
  if (__n > sizeof(__a.sun_path))
    __n = sizeof(__a.sun_path);
  if (__a.sun_path[0] != '\0') { // a path name: up to the terminating NUL
    size_t __k = 0;
    while (__k != __n && __a.sun_path[__k] != '\0')
      ++__k;
    __n = __k;
  }
  auto [__ec, __e] = make_local_endpoint(string_view(__a.sun_path, __n));
  return __ec ? local_endpoint() : __e;
}

// --- sockets

// A non-blocking, close-on-exec stream or datagram socket.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_open(__sock_domain __d, __native_socket& __fd, __sock_type __t = __sock_type::__stream) noexcept {
  int __af = __d == __sock_domain::__v4 ? AF_INET : (__d == __sock_domain::__v6 ? AF_INET6 : AF_UNIX);
  int __ty = __t == __sock_type::__stream ? SOCK_STREAM : SOCK_DGRAM;
  int __s  = ::socket(__af, __ty | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (__s < 0)
    return __last_error();
  __fd = __s;
  return error_code();
}

_LIBCPP_HIDE_FROM_ABI inline void __sock_close(__native_socket __fd) noexcept { (void)::close(__fd); }

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_bind(__native_socket __fd, const __sockaddr_buf& __a) noexcept {
  return ::bind(__fd, reinterpret_cast<const sockaddr*>(&__a.__ss_), __a.__len_) == 0 ? error_code() : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_listen(__native_socket __fd, int __backlog) noexcept {
  return ::listen(__fd, __backlog) == 0 ? error_code() : __last_error();
}

// Starts a connection. A connection that cannot complete at once reports operation_in_progress (the caller waits for
// writability and asks __sock_pending_error).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_connect(__native_socket __fd, const __sockaddr_buf& __a) noexcept {
  return ::connect(__fd, reinterpret_cast<const sockaddr*>(&__a.__ss_), __a.__len_) == 0
           ? error_code()
           : __last_error();
}

// The result of a connection started by __sock_connect (SO_ERROR).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_pending_error(__native_socket __fd) noexcept {
  int __e         = 0;
  socklen_t __len = sizeof(__e);
  if (::getsockopt(__fd, SOL_SOCKET, SO_ERROR, &__e, &__len) != 0)
    return __last_error();
  return __errno_code(__e);
}

// Accepts one connection as a non-blocking, close-on-exec socket; the peer address goes to __peer.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_accept(__native_socket __fd, __native_socket& __res, __sockaddr_buf& __peer) noexcept {
  __peer.__len_ = sizeof(__peer.__ss_);
  int __s = ::accept4(__fd, reinterpret_cast<sockaddr*>(&__peer.__ss_), &__peer.__len_, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (__s < 0)
    return __last_error();
  __res = __s;
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_local_address(__native_socket __fd, __sockaddr_buf& __a) noexcept {
  __a.__len_ = sizeof(__a.__ss_);
  return ::getsockname(__fd, reinterpret_cast<sockaddr*>(&__a.__ss_), &__a.__len_) == 0 ? error_code() : __last_error();
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_remote_address(__native_socket __fd, __sockaddr_buf& __a) noexcept {
  __a.__len_ = sizeof(__a.__ss_);
  return ::getpeername(__fd, reinterpret_cast<sockaddr*>(&__a.__ss_), &__a.__len_) == 0 ? error_code() : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_shutdown(__native_socket __fd, shutdown_type __what) noexcept {
  int __how = __what == shutdown_receive ? SHUT_RD : (__what == shutdown_send ? SHUT_WR : SHUT_RDWR);
  return ::shutdown(__fd, __how) == 0 ? error_code() : __last_error();
}

// At most this many buffers go into one call (the rest of a gather list is left for the next call).
inline constexpr size_t __sock_max_buffers = 16;

// Receives into the first __n buffers (scatter). __got is the number of bytes; 0 with a non-empty buffer set means that
// the peer has shut down its sending side.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_receive(__native_socket __fd, const mutable_buffer* __bufs, size_t __n, size_t& __got) noexcept {
  iovec __iov[__sock_max_buffers];
  if (__n > __sock_max_buffers)
    __n = __sock_max_buffers;
  for (size_t __i = 0; __i != __n; ++__i) {
    __iov[__i].iov_base = __bufs[__i].data();
    __iov[__i].iov_len  = __bufs[__i].size();
  }
  msghdr __m;
  ::memset(&__m, 0, sizeof(__m));
  __m.msg_iov    = __iov;
  __m.msg_iovlen = __n;
  for (;;) {
    ssize_t __r = ::recvmsg(__fd, &__m, MSG_CMSG_CLOEXEC);
    if (__r >= 0) {
      __got = static_cast<size_t>(__r);
      return error_code();
    }
    if (errno != EINTR)
      return __last_error();
  }
}

// Sends the first __n buffers (gather). MSG_NOSIGNAL: a closed peer is an error code, not SIGPIPE.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_send(__native_socket __fd, const const_buffer* __bufs, size_t __n, size_t& __got) noexcept {
  iovec __iov[__sock_max_buffers];
  if (__n > __sock_max_buffers)
    __n = __sock_max_buffers;
  for (size_t __i = 0; __i != __n; ++__i) {
    __iov[__i].iov_base = const_cast<void*>(__bufs[__i].data());
    __iov[__i].iov_len  = __bufs[__i].size();
  }
  msghdr __m;
  ::memset(&__m, 0, sizeof(__m));
  __m.msg_iov    = __iov;
  __m.msg_iovlen = __n;
  for (;;) {
    ssize_t __r = ::sendmsg(__fd, &__m, MSG_NOSIGNAL);
    if (__r >= 0) {
      __got = static_cast<size_t>(__r);
      return error_code();
    }
    if (errno != EINTR)
      return __last_error();
  }
}

// A connected pair of Unix domain sockets (socketpair), non-blocking and close-on-exec.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_pair(__sock_type __t, __native_socket& __a, __native_socket& __b) noexcept {
  int __fds[2];
  int __ty = __t == __sock_type::__stream ? SOCK_STREAM : SOCK_DGRAM;
  if (::socketpair(AF_UNIX, __ty | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, __fds) != 0)
    return __last_error();
  __a = __fds[0];
  __b = __fds[1];
  return error_code();
}

// The portable message flags (message_flags) in the representation of the platform.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline int __to_native_msg_flags(int __f) noexcept {
  int __r = 0;
  if (__f & 1)
    __r |= MSG_PEEK;
  if (__f & 2)
    __r |= MSG_OOB;
  if (__f & 4)
    __r |= MSG_DONTROUTE;
  return __r;
}

// Receives one datagram into the first __n buffers (scatter); a datagram that is larger than the buffers is truncated.
// __got may be 0 (an empty datagram). The sender address goes to __from (its length is 0 for an unnamed sender).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_receive_from(
    __native_socket __fd,
    const mutable_buffer* __bufs,
    size_t __n,
    int __flags,
    __sockaddr_buf& __from,
    size_t& __got) noexcept {
  iovec __iov[__sock_max_buffers];
  if (__n > __sock_max_buffers)
    __n = __sock_max_buffers;
  for (size_t __i = 0; __i != __n; ++__i) {
    __iov[__i].iov_base = __bufs[__i].data();
    __iov[__i].iov_len  = __bufs[__i].size();
  }
  ::memset(&__from.__ss_, 0, sizeof(__from.__ss_));
  msghdr __m;
  ::memset(&__m, 0, sizeof(__m));
  __m.msg_name    = &__from.__ss_;
  __m.msg_namelen = sizeof(__from.__ss_);
  __m.msg_iov     = __iov;
  __m.msg_iovlen  = __n;
  for (;;) {
    ssize_t __r = ::recvmsg(__fd, &__m, MSG_CMSG_CLOEXEC | io::__to_native_msg_flags(__flags));
    if (__r >= 0) {
      __from.__len_ = __m.msg_namelen;
      __got         = static_cast<size_t>(__r);
      return error_code();
    }
    if (errno != EINTR)
      return __last_error();
  }
}

// Sends the first __n buffers as one datagram (gather) to __to, or to the connected peer if __to is null. Zero buffers
// send an empty datagram.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_send_to(
    __native_socket __fd,
    const const_buffer* __bufs,
    size_t __n,
    int __flags,
    const __sockaddr_buf* __to,
    size_t& __got) noexcept {
  iovec __iov[__sock_max_buffers];
  if (__n > __sock_max_buffers)
    __n = __sock_max_buffers;
  for (size_t __i = 0; __i != __n; ++__i) {
    __iov[__i].iov_base = const_cast<void*>(__bufs[__i].data());
    __iov[__i].iov_len  = __bufs[__i].size();
  }
  msghdr __m;
  ::memset(&__m, 0, sizeof(__m));
  if (__to) {
    __m.msg_name    = const_cast<sockaddr_storage*>(&__to->__ss_);
    __m.msg_namelen = __to->__len_;
  }
  __m.msg_iov    = __iov;
  __m.msg_iovlen = __n;
  for (;;) {
    ssize_t __r = ::sendmsg(__fd, &__m, MSG_NOSIGNAL | io::__to_native_msg_flags(__flags));
    if (__r >= 0) {
      __got = static_cast<size_t>(__r);
      return error_code();
    }
    if (errno != EINTR)
      return __last_error();
  }
}

// Joins or leaves a multicast group. An IPv4 group takes the address of the interface (any if zero), an IPv6 group the
// index of the interface (0: the scope of the group, then the choice of the system).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_membership(
    __native_socket __fd, bool __join, const ip_address& __group, const ipv4_address& __iface, unsigned __if_index) noexcept {
  if (__group.is_v4()) {
    ip_mreq __r;
    ::memset(&__r, 0, sizeof(__r));
    __r.imr_multiaddr.s_addr = htonl(__group.to_v4().to_uint());
    __r.imr_interface.s_addr = htonl(__iface.to_uint());
    return ::setsockopt(__fd, IPPROTO_IP, __join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, &__r, sizeof(__r)) == 0
             ? error_code()
             : __last_error();
  }
  ipv6_address __g = __group.to_v6();
  ipv6_mreq __r;
  ::memset(&__r, 0, sizeof(__r));
  auto __bytes = __g.to_bytes();
  ::memcpy(&__r.ipv6mr_multiaddr, __bytes.data(), 16);
  __r.ipv6mr_interface = __if_index != 0 ? __if_index : __g.scope_id();
  return ::setsockopt(__fd, IPPROTO_IPV6, __join ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP, &__r, sizeof(__r)) == 0
           ? error_code()
           : __last_error();
}

// --- options

enum class __sockopt_id {
  __reuse_address,
  __reuse_port,
  __no_delay,
  __keep_alive,
  __v6_only,
  __receive_buffer_size,
  __send_buffer_size,
  __broadcast,
  __multicast_loop,
  __multicast_hops
};

// The options of multicast have a different level and name for IPv4 and IPv6 sockets: the family is asked of the socket.
_LIBCPP_HIDE_FROM_ABI inline bool __sock_is_v6(__native_socket __fd) noexcept {
  int __d         = 0;
  socklen_t __len = sizeof(__d);
  return ::getsockopt(__fd, SOL_SOCKET, SO_DOMAIN, &__d, &__len) == 0 && __d == AF_INET6;
}

_LIBCPP_HIDE_FROM_ABI inline void
__sockopt_location(__native_socket __fd, __sockopt_id __id, int& __level, int& __name) noexcept {
  switch (__id) {
  case __sockopt_id::__broadcast:
    __level = SOL_SOCKET, __name = SO_BROADCAST;
    break;
  case __sockopt_id::__multicast_loop:
    if (io::__sock_is_v6(__fd))
      __level = IPPROTO_IPV6, __name = IPV6_MULTICAST_LOOP;
    else
      __level = IPPROTO_IP, __name = IP_MULTICAST_LOOP;
    break;
  case __sockopt_id::__multicast_hops:
    if (io::__sock_is_v6(__fd))
      __level = IPPROTO_IPV6, __name = IPV6_MULTICAST_HOPS;
    else
      __level = IPPROTO_IP, __name = IP_MULTICAST_TTL;
    break;
  case __sockopt_id::__reuse_address:
    __level = SOL_SOCKET, __name = SO_REUSEADDR;
    break;
  case __sockopt_id::__reuse_port:
    __level = SOL_SOCKET, __name = SO_REUSEPORT;
    break;
  case __sockopt_id::__no_delay:
    __level = IPPROTO_TCP, __name = TCP_NODELAY;
    break;
  case __sockopt_id::__keep_alive:
    __level = SOL_SOCKET, __name = SO_KEEPALIVE;
    break;
  case __sockopt_id::__v6_only:
    __level = IPPROTO_IPV6, __name = IPV6_V6ONLY;
    break;
  case __sockopt_id::__receive_buffer_size:
    __level = SOL_SOCKET, __name = SO_RCVBUF;
    break;
  case __sockopt_id::__send_buffer_size:
    __level = SOL_SOCKET, __name = SO_SNDBUF;
    break;
  }
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_set_int(__native_socket __fd, __sockopt_id __id, int __v) noexcept {
  int __level, __name;
  io::__sockopt_location(__fd, __id, __level, __name);
  return ::setsockopt(__fd, __level, __name, &__v, sizeof(__v)) == 0 ? error_code() : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_get_int(__native_socket __fd, __sockopt_id __id, int& __v) noexcept {
  int __level, __name;
  io::__sockopt_location(__fd, __id, __level, __name);
  int __r         = 0;
  socklen_t __len = sizeof(__r);
  if (::getsockopt(__fd, __level, __name, &__r, &__len) != 0)
    return __last_error();
  __v = __r;
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_set_linger(__native_socket __fd, bool __on, int __seconds) noexcept {
  ::linger __l;
  __l.l_onoff  = __on ? 1 : 0;
  __l.l_linger = __seconds;
  return ::setsockopt(__fd, SOL_SOCKET, SO_LINGER, &__l, sizeof(__l)) == 0 ? error_code() : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_get_linger(__native_socket __fd, bool& __on, int& __seconds) noexcept {
  ::linger __l;
  socklen_t __len = sizeof(__l);
  if (::getsockopt(__fd, SOL_SOCKET, SO_LINGER, &__l, &__len) != 0)
    return __last_error();
  __on      = __l.l_onoff != 0;
  __seconds = __l.l_linger;
  return error_code();
}

// Removes the file system entry of a path name socket (bind_option::unlink_existing).
_LIBCPP_HIDE_FROM_ABI inline void __sock_unlink(const local_endpoint& __ep) noexcept {
  if (__ep.empty() || __ep.is_abstract())
    return;
  sockaddr_un __a;
  ::memset(&__a, 0, sizeof(__a));
  string_view __p = __ep.path();
  ::memcpy(__a.sun_path, __p.data(), __p.size());
  (void)::unlink(__a.sun_path);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_SUPPORT_POSIX_SOCKET_H
