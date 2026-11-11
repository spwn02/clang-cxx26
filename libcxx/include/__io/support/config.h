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
//   - _LIBCPP_HAS_IO_KQUEUE: the macOS and FreeBSD backend (kqueue, with EVFILT_USER for the wake-up).
//   - _LIBCPP_HAS_IO_REACTOR: one of the above. Code that does not depend on which one gates on this macro.
//
// The IOCP (Windows) backend will be added behind the same interface and define the same macros. On a platform without
// a backend, <io_context>, <io_timer> and <io_signal> are valid, empty headers.
#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS && defined(__linux__) && __has_include(<sys/epoll.h>) && \
    __has_include(<sys/eventfd.h>)
#  define _LIBCPP_HAS_IO_EPOLL 1
#else
#  define _LIBCPP_HAS_IO_EPOLL 0
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS && (defined(__APPLE__) || defined(__FreeBSD__)) &&      \
    __has_include(<sys/event.h>)
#  define _LIBCPP_HAS_IO_KQUEUE 1
#else
#  define _LIBCPP_HAS_IO_KQUEUE 0
#endif

#if _LIBCPP_HAS_IO_EPOLL || _LIBCPP_HAS_IO_KQUEUE
#  define _LIBCPP_HAS_IO_REACTOR 1
#else
#  define _LIBCPP_HAS_IO_REACTOR 0
#endif

// The BSD socket API (<sys/socket.h>, <netinet/in.h>, <sys/un.h>) on top of a backend: <net> (addresses, TCP and Unix
// domain stream sockets). The platform specifics are in libcxx/include/__net/support/.
#if _LIBCPP_HAS_IO_REACTOR && __has_include(<sys/socket.h>) && __has_include(<netinet/in.h>) && \
    __has_include(<netinet/tcp.h>) && __has_include(<sys/un.h>) && __has_include(<netdb.h>)
#  define _LIBCPP_HAS_IO_SOCKETS 1
#else
#  define _LIBCPP_HAS_IO_SOCKETS 0
#endif

// POSIX file I/O (<fcntl.h>, <sys/uio.h>, <sys/stat.h>) on top of a backend: <io_file>. Regular files cannot be waited
// for with epoll (kqueue could, but the behaviour differs between systems), so the operations run on helper threads.
// The platform specifics are in libcxx/include/__io_file/support/.
#if _LIBCPP_HAS_IO_REACTOR && __has_include(<fcntl.h>) && __has_include(<sys/uio.h>) && __has_include(<sys/stat.h>) && \
    __has_include(<unistd.h>)
#  define _LIBCPP_HAS_IO_FILES 1
#else
#  define _LIBCPP_HAS_IO_FILES 0
#endif

// TLS (<net_tls>): the engine is OpenSSL, loaded at run time with dlopen (no build or link dependency, no OpenSSL
// header is included). On top of the sockets only because the stream is useful with them; the stream itself works on
// any Stream. The loader is in libcxx/include/__net_tls/support/openssl_dl.h behind the internal __tls_engine
// interface.
#if _LIBCPP_HAS_IO_REACTOR && __has_include(<dlfcn.h>)
#  define _LIBCPP_HAS_IO_TLS 1
#else
#  define _LIBCPP_HAS_IO_TLS 0
#endif

// sigaction() plus a self-pipe. Defined together with a backend that is also available on a POSIX system.
#define _LIBCPP_HAS_IO_POSIX_SIGNALS _LIBCPP_HAS_IO_REACTOR

#endif // _LIBCPP___IO_SUPPORT_CONFIG_H
