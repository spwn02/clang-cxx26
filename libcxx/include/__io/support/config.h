//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_CONFIG_H
#define _LIBCPP___IO_SUPPORT_CONFIG_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

// The platform layer of the networking series (Stage Two). Everything that is specific to an operating system lives in
// libcxx/include/__io/support/<backend>_*.h behind one small internal interface (see poller.h), the same way
// libcxx/include/__thread/support/ is split per platform:
//
//   - _LIBCPP_HAS_IO_EPOLL: the Linux backend (epoll + eventfd), which also provides the POSIX signal support.
//
// The kqueue (macOS, BSD) and IOCP (Windows) backends will be added behind the same interface and define the same
// macros. On a platform without a backend, <io_context>, <io_timer> and <io_signal> are valid, empty headers.
#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS && defined(__linux__) && __has_include(<sys/epoll.h>) && \
    __has_include(<sys/eventfd.h>)
#  define _LIBCPP_HAS_IO_EPOLL 1
#else
#  define _LIBCPP_HAS_IO_EPOLL 0
#endif

// The BSD socket API (<sys/socket.h>, <netinet/in.h>, <sys/un.h>) on top of a backend: <net> (addresses, TCP and Unix
// domain stream sockets). The platform specifics are in libcxx/include/__net/support/.
#if _LIBCPP_HAS_IO_EPOLL && __has_include(<sys/socket.h>) && __has_include(<netinet/in.h>) && \
    __has_include(<netinet/tcp.h>) && __has_include(<sys/un.h>) && __has_include(<netdb.h>)
#  define _LIBCPP_HAS_IO_SOCKETS 1
#else
#  define _LIBCPP_HAS_IO_SOCKETS 0
#endif

// sigaction() plus a self-pipe. Defined together with a backend that is also available on a POSIX system.
#define _LIBCPP_HAS_IO_POSIX_SIGNALS _LIBCPP_HAS_IO_EPOLL

#endif // _LIBCPP___IO_SUPPORT_CONFIG_H
