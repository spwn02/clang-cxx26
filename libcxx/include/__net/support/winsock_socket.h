//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___NET_SUPPORT_WINSOCK_SOCKET_H
#define _LIBCPP___NET_SUPPORT_WINSOCK_SOCKET_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_SOCKETS && _LIBCPP_HAS_IO_WINSOCK
#  include <__io/support/winsock.h>
#  include <__io_buffer/buffer.h>
#  include <__net/endpoint.h>
#  include <__net/family.h>
#  include <__net/local_endpoint.h>
#  include <__system_error/errc.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <cstddef>
#  include <cstdint>
#  include <cstdio>
#  include <cstring>
#  include <string_view>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS && _LIBCPP_HAS_IO_WINSOCK

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The platform layer of <net> for Winsock 2: the same functions with the same meaning as posix_socket.h, which describes
// the contract. What differs on Windows:
//   - a SOCKET is a handle (a pointer-sized integer whose value is small in practice); the rest of the library keeps
//     descriptors in an int, so a socket is kept as the int that holds the same value (INVALID_SOCKET is -1 in both);
//   - the sockets are created overlapped and not inheritable, and made non-blocking with ioctlsocket (FIONBIO);
//   - errors are WSAE* numbers; they are translated to the POSIX values (see winsock.h);
//   - a non-blocking connect that is in progress reports WSAEWOULDBLOCK, which is reported here as EINPROGRESS;
//   - there is no SIGPIPE, no SO_REUSEPORT, no abstract Unix domain name, no Unix domain datagram socket and no
//     socketpair: a stream pair is made from a loopback connection, the rest report an error;
//   - a datagram that is larger than the buffers is reported as WSAEMSGSIZE after it has been truncated, which is the
//     truncation the other systems do silently.

using __native_socket _LIBCPP_NODEBUG             = int;
inline constexpr __native_socket __invalid_socket = -1;

inline constexpr bool __sock_has_atomic_flags = false;
inline constexpr int __sock_create_flags      = 0;
inline constexpr int __msg_nosignal           = 0;
inline constexpr int __msg_cmsg_cloexec       = 0;

enum class __sock_domain { __v4, __v6, __local };
enum class __sock_type { __stream, __datagram };

// A socket address in the representation of the platform.
struct __sockaddr_buf {
  sockaddr_storage __ss_;
  int __len_;
};

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __errno_code(int __e) noexcept {
  return error_code(__e, generic_category());
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __last_error() noexcept { return io::__last_wsa_error(); }

// A send to a connection that the system aborted (the peer is gone) reports a reset, as the POSIX systems do.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __send_error(int __e) noexcept {
  return io::__wsa_error_code(__e == WSAECONNABORTED ? WSAECONNRESET : __e);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline SOCKET __to_win_socket(__native_socket __s) noexcept {
  return static_cast<SOCKET>(static_cast<intptr_t>(__s));
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline __native_socket __from_win_socket(SOCKET __s) noexcept {
  return static_cast<__native_socket>(static_cast<intptr_t>(__s));
}

// Makes a new socket non-blocking and not inheritable by child processes.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_prepare(__native_socket __n) noexcept {
  SOCKET __s  = io::__to_win_socket(__n);
  u_long __nb = 1;
  if (::ioctlsocket(__s, FIONBIO, &__nb) != 0)
    return __last_error();
  (void)::SetHandleInformation(reinterpret_cast<HANDLE>(__s), HANDLE_FLAG_INHERIT, 0);
  return error_code();
}

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
  ::memcpy(__a.sun_path, __p.data(), __p.size()); // local_endpoint::max_path_length fits sun_path
  __b.__len_ = static_cast<int>(offsetof(sockaddr_un, sun_path) + __p.size() + (__ep.is_abstract() ? 0 : 1));
  if (__p.empty())
    __b.__len_ = static_cast<int>(offsetof(sockaddr_un, sun_path));
  ::memcpy(&__b.__ss_, &__a, sizeof(__a));
  return __b;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline local_endpoint __from_sockaddr_local(const __sockaddr_buf& __b) noexcept {
  if (__b.__ss_.ss_family != AF_UNIX || static_cast<size_t>(__b.__len_) <= offsetof(sockaddr_un, sun_path))
    return local_endpoint();
  sockaddr_un __a;
  ::memcpy(&__a, &__b.__ss_, sizeof(__a));
  size_t __n = static_cast<size_t>(__b.__len_) - offsetof(sockaddr_un, sun_path);
  if (__n > sizeof(__a.sun_path))
    __n = sizeof(__a.sun_path);
  // There is no abstract namespace: a leading NUL is the address of an unnamed socket.
  if (__n == 0 || __a.sun_path[0] == '\0')
    return local_endpoint();
  size_t __k = 0;
  while (__k != __n && __a.sun_path[__k] != '\0')
    ++__k;
  auto [__ec, __e] = make_local_endpoint(string_view(__a.sun_path, __k));
  return __ec ? local_endpoint() : __e;
}

// --- sockets

// A non-blocking, non-inheritable stream or datagram socket.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_open(__sock_domain __d, __native_socket& __fd, __sock_type __t = __sock_type::__stream) noexcept {
  if (error_code __ec = io::__winsock_init())
    return __ec;
  int __af = __d == __sock_domain::__v4 ? AF_INET : (__d == __sock_domain::__v6 ? AF_INET6 : AF_UNIX);
  int __ty = __t == __sock_type::__stream ? SOCK_STREAM : SOCK_DGRAM;
  SOCKET __s = ::WSASocketW(__af, __ty, 0, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
  if (__s == INVALID_SOCKET)
    return __last_error();
  __native_socket __n = io::__from_win_socket(__s);
  if (error_code __ec = io::__sock_prepare(__n)) {
    ::closesocket(__s);
    return __ec;
  }
  if (__t == __sock_type::__datagram && __d != __sock_domain::__local) {
    // An ICMP "port unreachable" must not turn into an error of the next receive (the other systems do not do that).
    DWORD __off = 0, __ret = 0;
    (void)::WSAIoctl(__s, _WSAIOW(IOC_VENDOR, 12), &__off, sizeof(__off), nullptr, 0, &__ret, nullptr, nullptr);
  }
  __fd = __n;
  return error_code();
}

_LIBCPP_HIDE_FROM_ABI inline void __sock_close(__native_socket __fd) noexcept {
  (void)::closesocket(io::__to_win_socket(__fd));
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline bool __is_unsupported_abstract(const __sockaddr_buf& __a) noexcept {
  if (__a.__ss_.ss_family != AF_UNIX || static_cast<size_t>(__a.__len_) <= offsetof(sockaddr_un, sun_path))
    return false;
  sockaddr_un __u;
  ::memcpy(&__u, &__a.__ss_, sizeof(__u));
  return __u.sun_path[0] == '\0';
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_bind(__native_socket __fd, const __sockaddr_buf& __a) noexcept {
  if (io::__is_unsupported_abstract(__a))
    return __errno_code(EAFNOSUPPORT);
  return ::bind(io::__to_win_socket(__fd), reinterpret_cast<const sockaddr*>(&__a.__ss_), __a.__len_) == 0
           ? error_code()
           : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_listen(__native_socket __fd, int __backlog) noexcept {
  return ::listen(io::__to_win_socket(__fd), __backlog) == 0 ? error_code() : __last_error();
}

// Starts a connection. A connection that cannot complete at once reports operation_in_progress (the caller waits for
// writability and asks __sock_pending_error).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_connect(__native_socket __fd, const __sockaddr_buf& __a) noexcept {
  if (io::__is_unsupported_abstract(__a))
    return __errno_code(EAFNOSUPPORT);
  if (::connect(io::__to_win_socket(__fd), reinterpret_cast<const sockaddr*>(&__a.__ss_), __a.__len_) == 0)
    return error_code();
  int __e = ::WSAGetLastError();
  if (__e == WSAEWOULDBLOCK) // a non-blocking connect that is under way
    return __errno_code(EINPROGRESS);
  return io::__wsa_error_code(__e);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_pending_error(__native_socket __fd) noexcept {
  int __e   = 0;
  int __len = sizeof(__e);
  if (::getsockopt(io::__to_win_socket(__fd), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&__e), &__len) != 0)
    return __last_error();
  return io::__wsa_error_code(__e);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_accept(__native_socket __fd, __native_socket& __res, __sockaddr_buf& __peer) noexcept {
  __peer.__len_ = sizeof(__peer.__ss_);
  SOCKET __s    = ::accept(io::__to_win_socket(__fd), reinterpret_cast<sockaddr*>(&__peer.__ss_), &__peer.__len_);
  if (__s == INVALID_SOCKET)
    return __last_error();
  __native_socket __n = io::__from_win_socket(__s);
  if (error_code __ec = io::__sock_prepare(__n)) {
    ::closesocket(__s);
    return __ec;
  }
  __res = __n;
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_local_address(__native_socket __fd, __sockaddr_buf& __a) noexcept {
  __a.__len_ = sizeof(__a.__ss_);
  return ::getsockname(io::__to_win_socket(__fd), reinterpret_cast<sockaddr*>(&__a.__ss_), &__a.__len_) == 0
           ? error_code()
           : __last_error();
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_remote_address(__native_socket __fd, __sockaddr_buf& __a) noexcept {
  __a.__len_ = sizeof(__a.__ss_);
  return ::getpeername(io::__to_win_socket(__fd), reinterpret_cast<sockaddr*>(&__a.__ss_), &__a.__len_) == 0
           ? error_code()
           : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_shutdown(__native_socket __fd, shutdown_type __what) noexcept {
  int __how = __what == shutdown_receive ? SD_RECEIVE : (__what == shutdown_send ? SD_SEND : SD_BOTH);
  return ::shutdown(io::__to_win_socket(__fd), __how) == 0 ? error_code() : __last_error();
}

// At most this many buffers go into one call (the rest of a gather list is left for the next call).
inline constexpr size_t __sock_max_buffers = 16;

// The buffers as WSABUF (a length is a ULONG, so a larger buffer is shortened: the caller sees a short transfer).
template <class _Buf, class _Ptr>
_LIBCPP_HIDE_FROM_ABI DWORD __to_wsabuf(const _Buf* __bufs, size_t __n, WSABUF* __dst, _Ptr) noexcept {
  if (__n > __sock_max_buffers)
    __n = __sock_max_buffers;
  for (size_t __i = 0; __i != __n; ++__i) {
    size_t __sz     = __bufs[__i].size();
    __dst[__i].len  = __sz > 0x7fffffffu ? 0x7fffffffu : static_cast<ULONG>(__sz);
    __dst[__i].buf  = reinterpret_cast<CHAR*>(const_cast<_Ptr*>(static_cast<const _Ptr*>(__bufs[__i].data())));
  }
  if (__n == 0) {
    __dst[0].len = 0;
    __dst[0].buf = nullptr;
    __n          = 1;
  }
  return static_cast<DWORD>(__n);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline size_t __wsabuf_total(const WSABUF* __b, DWORD __n) noexcept {
  size_t __t = 0;
  for (DWORD __i = 0; __i != __n; ++__i)
    __t += __b[__i].len;
  return __t;
}

// Receives into the first __n buffers (scatter). __got is the number of bytes; 0 with a non-empty buffer set means that
// the peer has shut down its sending side.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_receive(__native_socket __fd, const mutable_buffer* __bufs, size_t __n, size_t& __got) noexcept {
  WSABUF __wb[__sock_max_buffers];
  DWORD __cnt   = io::__to_wsabuf(__bufs, __n, __wb, char());
  DWORD __bytes = 0, __flags = 0;
  if (::WSARecv(io::__to_win_socket(__fd), __wb, __cnt, &__bytes, &__flags, nullptr, nullptr) != 0)
    return __last_error();
  __got = __bytes;
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_send(__native_socket __fd, const const_buffer* __bufs, size_t __n, size_t& __got) noexcept {
  WSABUF __wb[__sock_max_buffers];
  DWORD __cnt   = io::__to_wsabuf(__bufs, __n, __wb, char());
  DWORD __bytes = 0;
  if (::WSASend(io::__to_win_socket(__fd), __wb, __cnt, &__bytes, 0, nullptr, nullptr) != 0)
    return io::__send_error(::WSAGetLastError());
  __got = __bytes;
  return error_code();
}

// A connected pair of stream sockets from a loopback connection (Windows has no socketpair); a pair of datagram sockets
// does not exist for Unix domain sockets there.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_pair(__sock_type __t, __native_socket& __a, __native_socket& __b) noexcept {
  if (__t != __sock_type::__stream)
    return __errno_code(EAFNOSUPPORT);
  if (error_code __ec = io::__winsock_init())
    return __ec;
  SOCKET __l = ::WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
  if (__l == INVALID_SOCKET)
    return __last_error();
  SOCKET __c = INVALID_SOCKET, __s = INVALID_SOCKET;
  sockaddr_in __addr;
  ::memset(&__addr, 0, sizeof(__addr));
  __addr.sin_family      = AF_INET;
  __addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int __len              = sizeof(__addr);
  int __err              = 0;
  do {
    if (::bind(__l, reinterpret_cast<sockaddr*>(&__addr), sizeof(__addr)) != 0 || ::listen(__l, 1) != 0 ||
        ::getsockname(__l, reinterpret_cast<sockaddr*>(&__addr), &__len) != 0) {
      __err = ::WSAGetLastError();
      break;
    }
    __c = ::WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
    if (__c == INVALID_SOCKET || ::connect(__c, reinterpret_cast<sockaddr*>(&__addr), sizeof(__addr)) != 0) {
      __err = ::WSAGetLastError();
      break;
    }
    __s = ::accept(__l, nullptr, nullptr);
    if (__s == INVALID_SOCKET) {
      __err = ::WSAGetLastError();
      break;
    }
  } while (false);
  ::closesocket(__l);
  if (__err != 0 || io::__sock_prepare(io::__from_win_socket(__c)) || io::__sock_prepare(io::__from_win_socket(__s))) {
    if (__c != INVALID_SOCKET)
      ::closesocket(__c);
    if (__s != INVALID_SOCKET)
      ::closesocket(__s);
    return __err != 0 ? io::__wsa_error_code(__err) : __errno_code(EIO);
  }
  __a = io::__from_win_socket(__c);
  __b = io::__from_win_socket(__s);
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
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_receive_from(
    __native_socket __fd,
    const mutable_buffer* __bufs,
    size_t __n,
    int __flags,
    __sockaddr_buf& __from,
    size_t& __got) noexcept {
  WSABUF __wb[__sock_max_buffers];
  DWORD __cnt = io::__to_wsabuf(__bufs, __n, __wb, char());
  ::memset(&__from.__ss_, 0, sizeof(__from.__ss_));
  DWORD __bytes = 0, __fl = static_cast<DWORD>(io::__to_native_msg_flags(__flags));
  INT __flen = sizeof(__from.__ss_);
  if (::WSARecvFrom(io::__to_win_socket(__fd), __wb, __cnt, &__bytes, &__fl,
                    reinterpret_cast<sockaddr*>(&__from.__ss_), &__flen, nullptr, nullptr) != 0) {
    int __e = ::WSAGetLastError();
    if (__e != WSAEMSGSIZE)
      return io::__wsa_error_code(__e);
    // Truncated: the buffers are full and the rest of the datagram is gone, like MSG_TRUNC elsewhere.
    __bytes = static_cast<DWORD>(io::__wsabuf_total(__wb, __cnt));
  }
  __from.__len_ = __flen;
  __got         = __bytes;
  return error_code();
}

// Sends the first __n buffers as one datagram (gather) to __to, or to the connected peer if __to is null.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_send_to(
    __native_socket __fd,
    const const_buffer* __bufs,
    size_t __n,
    int __flags,
    const __sockaddr_buf* __to,
    size_t& __got) noexcept {
  WSABUF __wb[__sock_max_buffers];
  DWORD __cnt   = io::__to_wsabuf(__bufs, __n, __wb, char());
  DWORD __bytes = 0;
  int __rc;
  if (__to)
    __rc = ::WSASendTo(io::__to_win_socket(__fd), __wb, __cnt, &__bytes, static_cast<DWORD>(io::__to_native_msg_flags(__flags)),
                       reinterpret_cast<const sockaddr*>(&__to->__ss_), __to->__len_, nullptr, nullptr);
  else
    __rc = ::WSASend(io::__to_win_socket(__fd), __wb, __cnt, &__bytes, static_cast<DWORD>(io::__to_native_msg_flags(__flags)),
                     nullptr, nullptr);
  if (__rc != 0)
    return io::__send_error(::WSAGetLastError());
  __got = __bytes;
  return error_code();
}

// Joins or leaves a multicast group.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sock_membership(
    __native_socket __fd,
    bool __join,
    const ip_address& __group,
    const ipv4_address& __iface,
    unsigned __if_index) noexcept {
  SOCKET __s = io::__to_win_socket(__fd);
  if (__group.is_v4()) {
    ip_mreq __r;
    ::memset(&__r, 0, sizeof(__r));
    __r.imr_multiaddr.s_addr = htonl(__group.to_v4().to_uint());
    __r.imr_interface.s_addr = htonl(__iface.to_uint());
    return ::setsockopt(__s, IPPROTO_IP, __join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP,
                        reinterpret_cast<const char*>(&__r), sizeof(__r)) == 0
             ? error_code()
             : __last_error();
  }
  ipv6_address __g = __group.to_v6();
  ipv6_mreq __r;
  ::memset(&__r, 0, sizeof(__r));
  auto __bytes = __g.to_bytes();
  ::memcpy(&__r.ipv6mr_multiaddr, __bytes.data(), 16);
  __r.ipv6mr_interface = __if_index != 0 ? __if_index : __g.scope_id();
  return ::setsockopt(__s, IPPROTO_IPV6, __join ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP,
                      reinterpret_cast<const char*>(&__r), sizeof(__r)) == 0
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

// The family of the socket (the options of multicast have a different level and name for IPv4 and IPv6).
_LIBCPP_HIDE_FROM_ABI inline bool __sock_is_v6(__native_socket __fd) noexcept {
  WSAPROTOCOL_INFOW __pi;
  int __len = sizeof(__pi);
  return ::getsockopt(io::__to_win_socket(__fd), SOL_SOCKET, SO_PROTOCOL_INFOW, reinterpret_cast<char*>(&__pi), &__len) == 0 &&
         __pi.iAddressFamily == AF_INET6;
}

// False for an option that Windows does not have (SO_REUSEPORT).
_LIBCPP_HIDE_FROM_ABI inline bool
__sockopt_location(__native_socket __fd, __sockopt_id __id, int& __level, int& __name) noexcept {
  switch (__id) {
  case __sockopt_id::__broadcast:
    __level = SOL_SOCKET, __name = SO_BROADCAST;
    return true;
  case __sockopt_id::__multicast_loop:
    if (io::__sock_is_v6(__fd))
      __level = IPPROTO_IPV6, __name = IPV6_MULTICAST_LOOP;
    else
      __level = IPPROTO_IP, __name = IP_MULTICAST_LOOP;
    return true;
  case __sockopt_id::__multicast_hops:
    if (io::__sock_is_v6(__fd))
      __level = IPPROTO_IPV6, __name = IPV6_MULTICAST_HOPS;
    else
      __level = IPPROTO_IP, __name = IP_MULTICAST_TTL;
    return true;
  case __sockopt_id::__reuse_address:
    __level = SOL_SOCKET, __name = SO_REUSEADDR;
    return true;
  case __sockopt_id::__reuse_port:
    return false;
  case __sockopt_id::__no_delay:
    __level = IPPROTO_TCP, __name = TCP_NODELAY;
    return true;
  case __sockopt_id::__keep_alive:
    __level = SOL_SOCKET, __name = SO_KEEPALIVE;
    return true;
  case __sockopt_id::__v6_only:
    __level = IPPROTO_IPV6, __name = IPV6_V6ONLY;
    return true;
  case __sockopt_id::__receive_buffer_size:
    __level = SOL_SOCKET, __name = SO_RCVBUF;
    return true;
  case __sockopt_id::__send_buffer_size:
    __level = SOL_SOCKET, __name = SO_SNDBUF;
    return true;
  }
  return false;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_set_int(__native_socket __fd, __sockopt_id __id, int __v) noexcept {
  int __level, __name;
  if (!io::__sockopt_location(__fd, __id, __level, __name))
    return __errno_code(ENOPROTOOPT);
  return ::setsockopt(io::__to_win_socket(__fd), __level, __name, reinterpret_cast<const char*>(&__v), sizeof(__v)) == 0
           ? error_code()
           : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_get_int(__native_socket __fd, __sockopt_id __id, int& __v) noexcept {
  int __level, __name;
  if (!io::__sockopt_location(__fd, __id, __level, __name))
    return __errno_code(ENOPROTOOPT);
  int __r   = 0;
  int __len = sizeof(__r);
  if (::getsockopt(io::__to_win_socket(__fd), __level, __name, reinterpret_cast<char*>(&__r), &__len) != 0)
    return __last_error();
  __v = __r;
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_set_linger(__native_socket __fd, bool __on, int __seconds) noexcept {
  ::linger __l;
  __l.l_onoff  = __on ? 1 : 0;
  __l.l_linger = static_cast<u_short>(__seconds);
  return ::setsockopt(io::__to_win_socket(__fd), SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&__l), sizeof(__l)) == 0
           ? error_code()
           : __last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sock_get_linger(__native_socket __fd, bool& __on, int& __seconds) noexcept {
  ::linger __l;
  int __len = sizeof(__l);
  if (::getsockopt(io::__to_win_socket(__fd), SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&__l), &__len) != 0)
    return __last_error();
  __on      = __l.l_onoff != 0;
  __seconds = __l.l_linger;
  return error_code();
}

// Removes the file system entry of a path name socket (bind_option::unlink_existing).
_LIBCPP_HIDE_FROM_ABI inline void __sock_unlink(const local_endpoint& __ep) noexcept {
  if (__ep.empty() || __ep.is_abstract())
    return;
  char __path[local_endpoint::max_path_length + 1] = {};
  string_view __p                                  = __ep.path();
  ::memcpy(__path, __p.data(), __p.size());
  (void)::remove(__path);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS && _LIBCPP_HAS_IO_WINSOCK

#endif // _LIBCPP___NET_SUPPORT_WINSOCK_SOCKET_H
