//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_WIN32_SIGNAL_H
#define _LIBCPP___IO_SUPPORT_WIN32_SIGNAL_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_POSIX_SIGNALS && _LIBCPP_HAS_IO_WINSOCK
#  include <__atomic/atomic.h>
#  include <__atomic/memory_order.h>
#  include <__io/support/winsock.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <csignal>
#  include <cstdint>
#  include <cstring>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_POSIX_SIGNALS && _LIBCPP_HAS_IO_WINSOCK

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The Windows half of signal_set, with the names and the meaning of posix_signal.h (which describes the contract): the
// C library's signal() and a self-pipe, which is a datagram socket that is bound to the loopback address and connected to
// itself (the reactor polls sockets only). The C library has SIGINT, SIGBREAK, SIGTERM, SIGABRT, SIGFPE, SIGILL and
// SIGSEGV; the others fail with invalid_argument. The handler is reset to the default after every delivery by the
// library, so the handler arms itself again. None of the sigaction flags exists.

inline constexpr int __posix_max_signal = 23; // valid signal numbers are 1 .. __posix_max_signal - 1

inline constexpr unsigned __sig_flag_restart       = 1u << 0;
inline constexpr unsigned __sig_flag_no_child_stop = 1u << 1;
inline constexpr unsigned __sig_flag_no_child_wait = 1u << 2;
inline constexpr unsigned __sig_flag_no_defer      = 1u << 3;
inline constexpr unsigned __sig_flag_reset_handler = 1u << 4;
inline constexpr unsigned __sig_flag_all =
    __sig_flag_restart | __sig_flag_no_child_stop | __sig_flag_no_child_wait | __sig_flag_no_defer |
    __sig_flag_reset_handler;

_LIBCPP_HIDE_FROM_ABI inline bool __posix_flags_supported(unsigned __f) noexcept { return __f == 0; }

using __win_sig_handler _LIBCPP_NODEBUG = decltype(::signal(0, nullptr));

// Read by the signal handler (-1: none).
inline atomic<intptr_t> __win_signal_socket{-1};

struct __posix_signal_state {
  SOCKET __socket_ = INVALID_SOCKET;
  __win_sig_handler __old_[__posix_max_signal];
  bool __installed_[__posix_max_signal] = {};
};

// Never destroyed, so that a late signal or the destruction of a static object cannot meet a dead object.
_LIBCPP_HIDE_FROM_ABI inline __posix_signal_state& __posix_state() noexcept {
  static __posix_signal_state* __s = new __posix_signal_state;
  return *__s;
}

extern "C" _LIBCPP_HIDE_FROM_ABI inline void __libcpp_io_win32_signal_handler(int __sig) noexcept {
  ::signal(__sig, reinterpret_cast<__win_sig_handler>(&__libcpp_io_win32_signal_handler)); // arm again
  intptr_t __s = __win_signal_socket.load(memory_order_relaxed);
  if (__s != -1) {
    char __b = static_cast<char>(__sig);
    (void)::send(static_cast<SOCKET>(__s), &__b, 1, 0);
  }
}

// Creates the self-pipe on first use. Returns the read end in __fd.
_LIBCPP_HIDE_FROM_ABI inline error_code __posix_signal_ensure_pipe(int& __fd) noexcept {
  __posix_signal_state& __st = __posix_state();
  if (__st.__socket_ == INVALID_SOCKET) {
    if (error_code __ec = io::__winsock_init())
      return __ec;
    SOCKET __s = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
    if (__s == INVALID_SOCKET)
      return io::__last_wsa_error();
    sockaddr_in __a;
    ::memset(&__a, 0, sizeof(__a));
    __a.sin_family      = AF_INET;
    __a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int __len           = sizeof(__a);
    u_long __nb         = 1;
    if (::bind(__s, reinterpret_cast<sockaddr*>(&__a), sizeof(__a)) != 0 ||
        ::getsockname(__s, reinterpret_cast<sockaddr*>(&__a), &__len) != 0 ||
        ::connect(__s, reinterpret_cast<sockaddr*>(&__a), sizeof(__a)) != 0 || ::ioctlsocket(__s, FIONBIO, &__nb) != 0) {
      error_code __ec = io::__last_wsa_error();
      ::closesocket(__s);
      return __ec;
    }
    __st.__socket_ = __s;
    __win_signal_socket.store(static_cast<intptr_t>(__s), memory_order_relaxed);
  }
  __fd = static_cast<int>(static_cast<intptr_t>(__st.__socket_));
  return error_code();
}

_LIBCPP_HIDE_FROM_ABI inline error_code __posix_signal_install(int __sig, unsigned __flags) noexcept {
  (void)__flags;
  __posix_signal_state& __st = __posix_state();
  int __fd;
  if (error_code __ec = __posix_signal_ensure_pipe(__fd))
    return __ec;
  __win_sig_handler __old = ::signal(__sig, reinterpret_cast<__win_sig_handler>(&__libcpp_io_win32_signal_handler));
  if (__old == SIG_ERR)
    return error_code(EINVAL, generic_category());
  if (!__st.__installed_[__sig])
    __st.__old_[__sig] = __old;
  __st.__installed_[__sig] = true;
  return error_code();
}

_LIBCPP_HIDE_FROM_ABI inline error_code __posix_signal_restore(int __sig) noexcept {
  __posix_signal_state& __st = __posix_state();
  if (!__st.__installed_[__sig])
    return error_code();
  __st.__installed_[__sig] = false;
  if (::signal(__sig, __st.__old_[__sig]) == SIG_ERR)
    return error_code(EINVAL, generic_category());
  return error_code();
}

// Reads the signal numbers that arrived since the last call into __dst (at most __max); returns their number.
_LIBCPP_HIDE_FROM_ABI inline int __posix_signal_drain(int* __dst, int __max) noexcept {
  SOCKET __s = __posix_state().__socket_;
  if (__s == INVALID_SOCKET)
    return 0;
  char __buf[64];
  if (__max > 64)
    __max = 64;
  int __n = ::recv(__s, __buf, __max, 0);
  if (__n <= 0)
    return 0;
  for (int __i = 0; __i != __n; ++__i)
    __dst[__i] = static_cast<unsigned char>(__buf[__i]);
  return __n;
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_POSIX_SIGNALS && _LIBCPP_HAS_IO_WINSOCK

#endif // _LIBCPP___IO_SUPPORT_WIN32_SIGNAL_H
