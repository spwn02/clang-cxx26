//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_WINSOCK_H
#define _LIBCPP___IO_SUPPORT_WINSOCK_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_WINSOCK

#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <__system_error/throw_system_error.h>
#  include <cerrno>

// The only place that includes the Windows socket headers. The SAL annotation names are macros of the Windows SDK and
// identifiers in this library, and winsock2.h has to come before windows.h; it is included with WIN32_LEAN_AND_MEAN so
// that it does not drag in winsock.h.
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
#  pragma push_macro("__valid")
#  undef __valid
#  pragma push_macro("__nothrow")
#  undef __nothrow
#  pragma push_macro("__callback")
#  undef __callback
#  pragma push_macro("min")
#  undef min
#  pragma push_macro("max")
#  undef max
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    define _LIBCPP_IO_DEFINED_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#    define _LIBCPP_IO_DEFINED_NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  if __has_include(<afunix.h>)
#    include <afunix.h>
#  endif
#  include <mswsock.h>
#  include <windows.h>
#  ifdef _LIBCPP_IO_DEFINED_LEAN_AND_MEAN
#    undef WIN32_LEAN_AND_MEAN
#    undef _LIBCPP_IO_DEFINED_LEAN_AND_MEAN
#  endif
#  ifdef _LIBCPP_IO_DEFINED_NOMINMAX
#    undef NOMINMAX
#    undef _LIBCPP_IO_DEFINED_NOMINMAX
#  endif
#  pragma pop_macro("max")
#  pragma pop_macro("min")
#  pragma pop_macro("__callback")
#  pragma pop_macro("__nothrow")
#  pragma pop_macro("__valid")
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

#  pragma comment(lib, "ws2_32.lib")

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// Winsock reports its errors in its own number space (WSAE*). They are translated to the POSIX values of <cerrno> (which
// the Microsoft C library defines), so that every layer above compares the same errc values on every system.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __wsa_error_code(int __e) noexcept {
  switch (__e) {
  case 0:
    return error_code();
  case WSAEINTR:
    return error_code(EINTR, generic_category());
  case WSAEBADF:
  case WSAENOTSOCK: // a closed or never opened socket: the descriptor is bad, like on the POSIX systems
    return error_code(EBADF, generic_category());
  case WSAEACCES:
    return error_code(EACCES, generic_category());
  case WSAEFAULT:
    return error_code(EFAULT, generic_category());
  case WSAEINVAL:
    return error_code(EINVAL, generic_category());
  case WSAEMFILE:
    return error_code(EMFILE, generic_category());
  case WSAEWOULDBLOCK:
    return error_code(EWOULDBLOCK, generic_category());
  case WSAEINPROGRESS:
    return error_code(EINPROGRESS, generic_category());
  case WSAEALREADY:
    return error_code(EALREADY, generic_category());
  case WSAEDESTADDRREQ:
    return error_code(EDESTADDRREQ, generic_category());
  case WSAEMSGSIZE:
    return error_code(EMSGSIZE, generic_category());
  case WSAEPROTOTYPE:
    return error_code(EPROTOTYPE, generic_category());
  case WSAENOPROTOOPT:
    return error_code(ENOPROTOOPT, generic_category());
  case WSAEPROTONOSUPPORT:
    return error_code(EPROTONOSUPPORT, generic_category());
  case WSAESOCKTNOSUPPORT:
  case WSAEOPNOTSUPP:
  case WSAEPFNOSUPPORT:
    return error_code(EOPNOTSUPP, generic_category());
  case WSAEAFNOSUPPORT:
    return error_code(EAFNOSUPPORT, generic_category());
  case WSAEADDRINUSE:
    return error_code(EADDRINUSE, generic_category());
  case WSAEADDRNOTAVAIL:
    return error_code(EADDRNOTAVAIL, generic_category());
  case WSAENETDOWN:
    return error_code(ENETDOWN, generic_category());
  case WSAENETUNREACH:
    return error_code(ENETUNREACH, generic_category());
  case WSAENETRESET:
    return error_code(ENETRESET, generic_category());
  case WSAECONNABORTED:
    return error_code(ECONNABORTED, generic_category());
  case WSAECONNRESET:
    return error_code(ECONNRESET, generic_category());
  case WSAENOBUFS:
    return error_code(ENOBUFS, generic_category());
  case WSAEISCONN:
    return error_code(EISCONN, generic_category());
  case WSAENOTCONN:
    return error_code(ENOTCONN, generic_category());
  case WSAESHUTDOWN:
    return error_code(EPIPE, generic_category());
  case WSAETIMEDOUT:
    return error_code(ETIMEDOUT, generic_category());
  case WSAECONNREFUSED:
    return error_code(ECONNREFUSED, generic_category());
  case WSAELOOP:
    return error_code(ELOOP, generic_category());
  case WSAENAMETOOLONG:
    return error_code(ENAMETOOLONG, generic_category());
  case WSAEHOSTUNREACH:
    return error_code(EHOSTUNREACH, generic_category());
  case WSAEHOSTDOWN:
    return error_code(EHOSTUNREACH, generic_category());
  case WSA_OPERATION_ABORTED:
    return error_code(ECANCELED, generic_category());
  default:
    return error_code(__e, system_category());
  }
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __last_wsa_error() noexcept {
  return io::__wsa_error_code(::WSAGetLastError());
}

// WSAStartup, once per process (a process that uses a socket has loaded the library for good: there is no matching
// WSACleanup, a later socket of a static object could not be closed after it). Every function that is the first to touch
// the Winsock API calls this.
_LIBCPP_HIDE_FROM_ABI inline error_code __winsock_init() noexcept {
  static const int __r = [] {
    WSADATA __d;
    return ::WSAStartup(MAKEWORD(2, 2), &__d);
  }();
  return __r == 0 ? error_code() : io::__wsa_error_code(__r);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_WINSOCK

#endif // _LIBCPP___IO_SUPPORT_WINSOCK_H
