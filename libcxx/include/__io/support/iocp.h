//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_IOCP_H
#define _LIBCPP___IO_SUPPORT_IOCP_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_IOCP
#  include <__io/support/winsock.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_IOCP

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The Windows completion backend: an I/O completion port, used the way the ring of uring.h is on Linux. The sockets of
// the context are associated with the port when their first transfer is submitted; a transfer is one overlapped call
// (WSARecv, WSARecvFrom, WSASend, WSASendTo), and its completion packet is dequeued by a thread of the reactor, which
// completes the operation. Waits for readiness, timers, accept and connect stay on the readiness poller.
//
// The OVERLAPPED of a request is the first member of __iocp_ov, which carries the operation it belongs to.
struct __iocp_ov {
  OVERLAPPED __ov_;
  void* __op_;
};

struct __iocp_packet {
  __iocp_ov* __ov_ = nullptr; // null for the packet that stops the thread
  DWORD __bytes_   = 0;
  bool __ok_       = false;
};

class __iocp {
  HANDLE __port_ = nullptr;

public:
  __iocp() noexcept                = default;
  __iocp(const __iocp&)            = delete;
  __iocp& operator=(const __iocp&) = delete;
  _LIBCPP_HIDE_FROM_ABI ~__iocp() { __close(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code __open() noexcept {
    __port_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
    if (!__port_)
      return error_code(static_cast<int>(::GetLastError()), system_category());
    return error_code();
  }
  _LIBCPP_HIDE_FROM_ABI void __close() noexcept {
    if (__port_) {
      ::CloseHandle(__port_);
      __port_ = nullptr;
    }
  }

  // Associates the socket with the port; a socket that is associated already is not an error.
  _LIBCPP_HIDE_FROM_ABI void __associate(intptr_t __socket) noexcept {
    (void)::CreateIoCompletionPort(reinterpret_cast<HANDLE>(static_cast<SOCKET>(__socket)), __port_, 0, 0);
  }

  // Blocks for the next packet. False: the port is closed or broken.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool __dequeue(__iocp_packet& __p) noexcept {
    DWORD __bytes    = 0;
    ULONG_PTR __key  = 0;
    OVERLAPPED* __ov = nullptr;
    BOOL __ok        = ::GetQueuedCompletionStatus(__port_, &__bytes, &__key, &__ov, INFINITE);
    if (!__ov) {
      if (!__ok)
        return false;
      __p.__ov_ = nullptr; // the stop packet
      return true;
    }
    __p.__ov_    = reinterpret_cast<__iocp_ov*>(__ov);
    __p.__bytes_ = __bytes;
    __p.__ok_    = __ok != FALSE;
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI void __post_stop() noexcept { (void)::PostQueuedCompletionStatus(__port_, 0, 0, nullptr); }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_IOCP

#endif // _LIBCPP___IO_SUPPORT_IOCP_H
