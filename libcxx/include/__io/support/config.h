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
//   - _LIBCPP_HAS_IO_WINSOCK: the Windows backend (Winsock 2): WSAPoll over the registered sockets, with a loopback
//     datagram socket for the wake-up. Readiness is level-triggered like on the other systems, so the operations above
//     the poller are the same "try, wait for readiness, retry" loops.
//   - _LIBCPP_HAS_IO_IOCP: the Windows completion backend (an I/O completion port), an addition to the readiness
//     poller that is chosen at run time, the way _LIBCPP_HAS_IO_URING is on Linux.
//
// On a platform without a backend, <io_context>, <io_timer> and <io_signal> are valid, empty headers.
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

// _LIBCPP_HAS_IO_URING: the Linux completion backend (io_uring through raw system calls, no liburing). It is an
// addition to epoll, chosen at run time: the reactor opens a ring when the kernel allows it and runs on epoll alone
// otherwise (kernel before 5.6, kernel.io_uring_disabled, a seccomp filter, a container runtime). The environment
// variable _LIBCPP_IO_BACKEND selects "epoll", "io_uring" or "auto" (the default) for a test or a diagnosis.
#if _LIBCPP_HAS_IO_EPOLL && defined(__linux__) && __has_include(<sys/syscall.h>) && __has_include(<sys/mman.h>) &&   \
    __has_include(<poll.h>)
#  define _LIBCPP_HAS_IO_URING 1
#else
#  define _LIBCPP_HAS_IO_URING 0
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS && defined(_WIN32) && __has_include(<winsock2.h>) &&   \
    __has_include(<ws2tcpip.h>)
#  define _LIBCPP_HAS_IO_WINSOCK 1
#else
#  define _LIBCPP_HAS_IO_WINSOCK 0
#endif

// _LIBCPP_HAS_IO_IOCP: the Windows completion backend (an I/O completion port), an addition to the readiness poller. The
// reactor opens a port when the environment variable _LIBCPP_IO_BACKEND is "iocp" or "auto" (the default) and the open
// succeeds, and runs the transfers of the sockets on it; "winsock" selects the readiness poller alone.
#if _LIBCPP_HAS_IO_WINSOCK
#  define _LIBCPP_HAS_IO_IOCP 1
#else
#  define _LIBCPP_HAS_IO_IOCP 0
#endif

#if _LIBCPP_HAS_IO_EPOLL || _LIBCPP_HAS_IO_KQUEUE || _LIBCPP_HAS_IO_WINSOCK
#  define _LIBCPP_HAS_IO_REACTOR 1
#else
#  define _LIBCPP_HAS_IO_REACTOR 0
#endif

// The BSD socket API (<sys/socket.h>, <netinet/in.h>, <sys/un.h>) on top of a backend: <net> (addresses, TCP and Unix
// domain stream sockets). The platform specifics are in libcxx/include/__net/support/.
#if (_LIBCPP_HAS_IO_EPOLL || _LIBCPP_HAS_IO_KQUEUE) && __has_include(<sys/socket.h>) &&                                \
    __has_include(<netinet/in.h>) && __has_include(<netinet/tcp.h>) && __has_include(<sys/un.h>) &&                  \
    __has_include(<netdb.h>)
#  define _LIBCPP_HAS_IO_SOCKETS 1
#elif _LIBCPP_HAS_IO_WINSOCK && __has_include(<afunix.h>)
#  define _LIBCPP_HAS_IO_SOCKETS 1
#else
#  define _LIBCPP_HAS_IO_SOCKETS 0
#endif

// POSIX file I/O (<fcntl.h>, <sys/uio.h>, <sys/stat.h>) on top of a backend: <io_file>. Regular files cannot be waited
// for with epoll (kqueue could, but the behaviour differs between systems), so the operations run on helper threads.
// The platform specifics are in libcxx/include/__io_file/support/.
#if (_LIBCPP_HAS_IO_EPOLL || _LIBCPP_HAS_IO_KQUEUE) && __has_include(<fcntl.h>) && __has_include(<sys/uio.h>) && __has_include(<sys/stat.h>) && \
    __has_include(<unistd.h>)
#  define _LIBCPP_HAS_IO_FILES 1
#elif _LIBCPP_HAS_IO_WINSOCK
#  define _LIBCPP_HAS_IO_FILES 1 // Win32 handles (libcxx/include/__io_file/support/win32_file.h)
#else
#  define _LIBCPP_HAS_IO_FILES 0
#endif

// TLS (<net_tls>): the engine is OpenSSL, loaded at run time with dlopen (no build or link dependency, no OpenSSL
// header is included). On top of the sockets only because the stream is useful with them; the stream itself works on
// any Stream. The loader is in libcxx/include/__net_tls/support/openssl_dl.h behind the internal __tls_engine
// interface.
#if (_LIBCPP_HAS_IO_EPOLL || _LIBCPP_HAS_IO_KQUEUE) && __has_include(<dlfcn.h>)
#  define _LIBCPP_HAS_IO_TLS 1
#else
#  define _LIBCPP_HAS_IO_TLS 0
#endif

// signal_set: sigaction() plus a self-pipe on POSIX systems, the C library's signal() plus a self-pipe on Windows
// (libcxx/include/__io/support/win32_signal.h). Defined together with a backend.
#define _LIBCPP_HAS_IO_POSIX_SIGNALS _LIBCPP_HAS_IO_REACTOR

#endif // _LIBCPP___IO_SUPPORT_CONFIG_H
