//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_POSIX_SIGNAL_H
#define _LIBCPP___IO_SUPPORT_POSIX_SIGNAL_H

#include <__atomic/atomic.h>
#include <__atomic/memory_order.h>
#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_POSIX_SIGNALS

#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <cerrno>
#  include <csignal>
#  include <fcntl.h>
#  include <signal.h>
#  include <unistd.h>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The POSIX half of signal_set: sigaction() and a self-pipe. Process signals are process-global, so everything in here
// is too. The functions are not thread-safe; the signal registry of signal_service.h serializes the callers.
//
// The signal handler does the only thing that is async-signal-safe and useful: it writes the signal number to the pipe
// (one byte). An event loop that watches the read end (__posix_signal_pipe_read_fd()) drains it in normal context
// and delivers the signals to the signal sets.

inline constexpr int __posix_max_signal = 65; // valid signal numbers are 1 .. __posix_max_signal - 1

// The abstract flag bits of signal_set::flags_t and their sigaction() counterparts.
inline constexpr unsigned __sig_flag_restart       = 1u << 0;
inline constexpr unsigned __sig_flag_no_child_stop = 1u << 1;
inline constexpr unsigned __sig_flag_no_child_wait = 1u << 2;
inline constexpr unsigned __sig_flag_no_defer      = 1u << 3;
inline constexpr unsigned __sig_flag_reset_handler = 1u << 4;
inline constexpr unsigned __sig_flag_all =
    __sig_flag_restart | __sig_flag_no_child_stop | __sig_flag_no_child_wait | __sig_flag_no_defer |
    __sig_flag_reset_handler;

_LIBCPP_HIDE_FROM_ABI inline int __posix_sa_flags(unsigned __f) noexcept {
  int __r = 0;
  if (__f & __sig_flag_restart)
    __r |= SA_RESTART;
  if (__f & __sig_flag_no_child_stop)
    __r |= SA_NOCLDSTOP;
#  ifdef SA_NOCLDWAIT
  if (__f & __sig_flag_no_child_wait)
    __r |= SA_NOCLDWAIT;
#  endif
  if (__f & __sig_flag_no_defer)
    __r |= SA_NODEFER;
  if (__f & __sig_flag_reset_handler)
    __r |= SA_RESETHAND;
  return __r;
}

_LIBCPP_HIDE_FROM_ABI inline bool __posix_flags_supported(unsigned __f) noexcept {
#  ifndef SA_NOCLDWAIT
  if (__f & __sig_flag_no_child_wait)
    return false;
#  endif
  return (__f & ~__sig_flag_all) == 0;
}

// Read only by the signal handler.
inline atomic<int> __posix_signal_write_fd{-1};

struct __posix_signal_state {
  int __read_fd_ = -1;
  struct sigaction __old_[__posix_max_signal];
  bool __installed_[__posix_max_signal] = {};
};

// Never destroyed, so that a late signal or the destruction of a static object cannot meet a dead object.
_LIBCPP_HIDE_FROM_ABI inline __posix_signal_state& __posix_state() noexcept {
  static __posix_signal_state* __s = new __posix_signal_state;
  return *__s;
}

// C language linkage: the type of sigaction()'s handler member is a C function type.
extern "C" inline void __libcpp_io_posix_signal_handler(int __sig) noexcept {
  int __saved = errno;
  int __fd    = __posix_signal_write_fd.load(memory_order_relaxed);
  if (__fd >= 0) {
    unsigned char __b = static_cast<unsigned char>(__sig);
    ssize_t __r       = ::write(__fd, &__b, 1);
    (void)__r;
  }
  errno = __saved;
}

// Creates the self-pipe on first use. Returns the read end in __fd.
_LIBCPP_HIDE_FROM_ABI inline error_code __posix_signal_ensure_pipe(int& __fd) noexcept {
  __posix_signal_state& __s = __posix_state();
  if (__s.__read_fd_ < 0) {
    int __p[2];
    if (::pipe(__p) != 0)
      return error_code(errno, generic_category());
    for (int __i = 0; __i != 2; ++__i) {
      ::fcntl(__p[__i], F_SETFD, FD_CLOEXEC);
      ::fcntl(__p[__i], F_SETFL, ::fcntl(__p[__i], F_GETFL) | O_NONBLOCK);
    }
    __s.__read_fd_ = __p[0];
    __posix_signal_write_fd.store(__p[1], memory_order_relaxed);
  }
  __fd = __s.__read_fd_;
  return error_code();
}

// Installs (or, when already installed by us, updates) the handler of __sig with the given abstract flags. The
// disposition that was in effect before the first install is remembered for __posix_signal_restore.
_LIBCPP_HIDE_FROM_ABI inline error_code __posix_signal_install(int __sig, unsigned __flags) noexcept {
  __posix_signal_state& __s = __posix_state();
  int __fd;
  if (error_code __ec = __posix_signal_ensure_pipe(__fd))
    return __ec;
  struct sigaction __sa;
  __sa.sa_handler = &__libcpp_io_posix_signal_handler;
  ::sigfillset(&__sa.sa_mask);
  __sa.sa_flags = __posix_sa_flags(__flags);
  if (::sigaction(__sig, &__sa, __s.__installed_[__sig] ? nullptr : &__s.__old_[__sig]) != 0)
    return error_code(errno, generic_category());
  __s.__installed_[__sig] = true;
  return error_code();
}

_LIBCPP_HIDE_FROM_ABI inline error_code __posix_signal_restore(int __sig) noexcept {
  __posix_signal_state& __s = __posix_state();
  if (!__s.__installed_[__sig])
    return error_code();
  __s.__installed_[__sig] = false;
  if (::sigaction(__sig, &__s.__old_[__sig], nullptr) != 0)
    return error_code(errno, generic_category());
  return error_code();
}

// Reads the signal numbers that arrived since the last call into __dst (at most __max); returns their number.
_LIBCPP_HIDE_FROM_ABI inline int __posix_signal_drain(int* __dst, int __max) noexcept {
  int __fd = __posix_state().__read_fd_;
  if (__fd < 0)
    return 0;
  unsigned char __buf[64];
  if (__max > 64)
    __max = 64;
  ssize_t __n = ::read(__fd, __buf, static_cast<size_t>(__max));
  if (__n <= 0)
    return 0;
  for (ssize_t __i = 0; __i != __n; ++__i)
    __dst[__i] = __buf[__i];
  return static_cast<int>(__n);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_POSIX_SIGNALS

#endif // _LIBCPP___IO_SUPPORT_POSIX_SIGNAL_H
