//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_POLLER_H
#define _LIBCPP___IO_SUPPORT_POLLER_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_EPOLL

#  include <__io/support/epoll_poller.h>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The one internal interface between the reactor and an operating system backend (a backend provides a class with
// exactly these members; the reactor never includes a system header):
//
//   __poller() throws system_error;   ~__poller()
//   error_code __register(int fd, unsigned events, uintptr_t tag, bool oneshot) noexcept;   // start watching fd
//   error_code __modify(int fd, unsigned events, uintptr_t tag, bool oneshot) noexcept;     // change what is watched
//   error_code __deregister(int fd) noexcept;
//   int  __wait(__poller_event* out, int max, int timeout_ms) noexcept;  // -1 blocks; returns the number of events
//   void __wakeup() noexcept;  // makes a concurrent or the next __wait return; callable from any thread
//
// Events are the neutral bits __poll_read, __poll_write, __poll_error of poller_types.h. A oneshot registration is
// disarmed after it delivered an event until __modify arms it again. Conditions are level-triggered.
using __poller _LIBCPP_NODEBUG = __epoll_poller;

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_EPOLL

#endif // _LIBCPP___IO_SUPPORT_POLLER_H
