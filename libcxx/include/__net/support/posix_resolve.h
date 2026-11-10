//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_SUPPORT_POSIX_RESOLVE_H
#define _LIBCPP___NET_SUPPORT_POSIX_RESOLVE_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_SOCKETS
#  include <__net/endpoint.h>
#  include <__net/resolver_error.h>
#  include <__net/support/posix_socket.h> // includes <netdb.h> and <unistd.h>
#  include <__system_error/errc.h>
#  include <__system_error/error_code.h>
#  include <string>
#  include <vector>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The platform layer of the resolver for the BSD socket API: getaddrinfo, getnameinfo and gethostname. These calls
// block; the resolver service calls them from its helper threads. A Windows backend provides the same functions.
// Failures are reported as an error_code: resolver_error for the failures of the name service, the errno value for
// EAI_SYSTEM.

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __gai_error(int __e) noexcept {
  switch (__e) {
  case 0:
    return error_code();
  case EAI_NONAME:
    return make_error_code(resolver_error::host_not_found);
#  ifdef EAI_NODATA
  case EAI_NODATA:
    return make_error_code(resolver_error::no_data);
#  endif
#  ifdef EAI_ADDRFAMILY
  case EAI_ADDRFAMILY:
    return make_error_code(resolver_error::no_data);
#  endif
  case EAI_AGAIN:
    return make_error_code(resolver_error::try_again);
  case EAI_FAIL:
    return make_error_code(resolver_error::no_recovery);
  case EAI_SERVICE:
    return make_error_code(resolver_error::service_not_found);
  case EAI_FAMILY:
    return make_error_code(resolver_error::family_not_supported);
  case EAI_SOCKTYPE:
    return make_error_code(resolver_error::socktype_not_supported);
  case EAI_BADFLAGS:
    return make_error_code(resolver_error::bad_flags);
  case EAI_MEMORY:
    return make_error_code(resolver_error::out_of_memory);
  case EAI_OVERFLOW:
    return make_error_code(resolver_error::overflow);
  case EAI_SYSTEM:
    return io::__last_error();
  default:
    return make_error_code(resolver_error::unknown);
  }
}

// The portable flags (resolve_flags: passive 1, numeric_host 4, numeric_service 8, address_configured 0x20, v4_mapped
// 0x800, all_matching 0x100) in the AI_* values of the platform.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline int __to_native_ai_flags(unsigned __f) noexcept {
  int __r = 0;
  if (__f & 0x01)
    __r |= AI_PASSIVE;
  if (__f & 0x04)
    __r |= AI_NUMERICHOST;
  if (__f & 0x08)
    __r |= AI_NUMERICSERV;
  if (__f & 0x20)
    __r |= AI_ADDRCONFIG;
  if (__f & 0x800)
    __r |= AI_V4MAPPED;
  if (__f & 0x100)
    __r |= AI_ALL;
  return __r;
}

// The portable flags (reverse_flags: numeric_host 1, numeric_service 2, name_required 4, datagram_service 8) in the NI_*
// values of the platform.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline int __to_native_ni_flags(unsigned __f) noexcept {
  int __r = 0;
  if (__f & 0x01)
    __r |= NI_NUMERICHOST;
  if (__f & 0x02)
    __r |= NI_NUMERICSERV;
  if (__f & 0x04)
    __r |= NI_NAMEREQD;
  if (__f & 0x08)
    __r |= NI_DGRAM;
  return __r;
}

// Forward resolution of a stream service: the endpoints of the host and the service, in the order of the system. An
// empty host or service is "not given" (a null pointer).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sys_resolve(const string& __host, const string& __service, unsigned __flags, vector<endpoint>& __results) {
  addrinfo __hints;
  ::memset(&__hints, 0, sizeof(__hints));
  __hints.ai_family   = AF_UNSPEC;
  __hints.ai_socktype = SOCK_STREAM;
  __hints.ai_flags    = io::__to_native_ai_flags(__flags);
  addrinfo* __res     = nullptr;
  int __r = ::getaddrinfo(__host.empty() ? nullptr : __host.c_str(),
                          __service.empty() ? nullptr : __service.c_str(),
                          &__hints,
                          &__res);
  if (__r != 0)
    return io::__gai_error(__r);
  for (addrinfo* __p = __res; __p; __p = __p->ai_next) {
    if (__p->ai_family != AF_INET && __p->ai_family != AF_INET6)
      continue;
    __sockaddr_buf __b;
    ::memset(&__b.__ss_, 0, sizeof(__b.__ss_));
    if (__p->ai_addrlen > sizeof(__b.__ss_))
      continue;
    ::memcpy(&__b.__ss_, __p->ai_addr, __p->ai_addrlen);
    __b.__len_ = __p->ai_addrlen;
    __results.push_back(io::__from_sockaddr(__b));
  }
  ::freeaddrinfo(__res);
  return error_code();
}

// Reverse resolution of an endpoint.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__sys_reverse_resolve(const endpoint& __ep, unsigned __flags, string& __host, string& __service) {
  __sockaddr_buf __b = io::__to_sockaddr(__ep);
  char __h[NI_MAXHOST];
  char __s[NI_MAXSERV];
  int __r = ::getnameinfo(
      reinterpret_cast<const sockaddr*>(&__b.__ss_),
      __b.__len_,
      __h,
      sizeof(__h),
      __s,
      sizeof(__s),
      io::__to_native_ni_flags(__flags));
  if (__r != 0)
    return io::__gai_error(__r);
  __host.assign(__h);
  __service.assign(__s);
  return error_code();
}

// The name of the local machine.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __sys_host_name(string& __name) {
  char __buf[256];
  if (::gethostname(__buf, sizeof(__buf)) != 0)
    return io::__last_error();
  __buf[sizeof(__buf) - 1] = '\0';
  __name.assign(__buf);
  return error_code();
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_SUPPORT_POSIX_RESOLVE_H
