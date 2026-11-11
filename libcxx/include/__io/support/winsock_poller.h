//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_WINSOCK_POLLER_H
#define _LIBCPP___IO_SUPPORT_WINSOCK_POLLER_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_WINSOCK

#  include <__atomic/atomic.h>
#  include <__atomic/memory_order.h>
#  include <__io/support/poller_types.h>
#  include <__io/support/winsock.h>
#  include <__mutex/lock_guard.h>
#  include <__mutex/mutex.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <__system_error/throw_system_error.h>
#  include <__vector/vector.h>
#  include <cstdint>
#  include <cstring>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The Windows backend of the poller interface (poller.h): WSAPoll over the registered sockets. Winsock has no object
// that other threads can change while a thread waits on it (epoll, kqueue), so the registrations are a table under a
// lock, and the waiting thread polls a copy of its armed entries. A change to the table (register, modify, deregister)
// wakes a thread that is waiting so that it polls the new set. The wake-up is a datagram socket that is bound to the
// loopback address and connected to itself: a byte sent to it makes it readable. It is the first entry of every poll
// and its event is consumed inside __wait.
class __winsock_poller {
  struct __entry {
    SOCKET __s_;
    unsigned __events_;
    uintptr_t __tag_;
    bool __oneshot_;
    bool __armed_; // false after a oneshot registration delivered an event, until __modify
  };

  mutex __m_;
  vector<__entry> __entries_;
  SOCKET __wake_ = INVALID_SOCKET;
  atomic<bool> __waiting_{false};

  _LIBCPP_HIDE_FROM_ABI static SOCKET __to_socket(int __fd) noexcept {
    return static_cast<SOCKET>(static_cast<intptr_t>(__fd));
  }

  // The caller holds __m_.
  _LIBCPP_HIDE_FROM_ABI __entry* __find(SOCKET __s) noexcept {
    for (__entry& __e : __entries_)
      if (__e.__s_ == __s)
        return &__e;
    return nullptr;
  }

  _LIBCPP_HIDE_FROM_ABI void __poke() noexcept {
    if (__waiting_.load(memory_order_acquire))
      __wakeup();
  }

public:
  _LIBCPP_HIDE_FROM_ABI __winsock_poller() {
    if (error_code __ec = io::__winsock_init())
      std::__throw_system_error(__ec.value(), "WSAStartup failed");
    __wake_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (__wake_ == INVALID_SOCKET)
      std::__throw_system_error(::WSAGetLastError(), "wake-up socket failed");
    sockaddr_in __a;
    ::memset(&__a, 0, sizeof(__a));
    __a.sin_family      = AF_INET;
    __a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int __len           = sizeof(__a);
    u_long __nb         = 1;
    if (::bind(__wake_, reinterpret_cast<sockaddr*>(&__a), sizeof(__a)) != 0 ||
        ::getsockname(__wake_, reinterpret_cast<sockaddr*>(&__a), &__len) != 0 ||
        ::connect(__wake_, reinterpret_cast<sockaddr*>(&__a), sizeof(__a)) != 0 ||
        ::ioctlsocket(__wake_, FIONBIO, &__nb) != 0) {
      int __err = ::WSAGetLastError();
      ::closesocket(__wake_);
      std::__throw_system_error(__err, "wake-up socket setup failed");
    }
  }

  __winsock_poller(const __winsock_poller&)            = delete;
  __winsock_poller& operator=(const __winsock_poller&) = delete;

  _LIBCPP_HIDE_FROM_ABI ~__winsock_poller() { ::closesocket(__wake_); }

  _LIBCPP_HIDE_FROM_ABI error_code __register(int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot) noexcept {
    {
      lock_guard<mutex> __lk(__m_);
      SOCKET __s = __to_socket(__fd);
      if (__find(__s))
        return error_code(EEXIST, generic_category());
      __entries_.push_back(__entry{__s, __ev, __tag, __oneshot, true});
    }
    __poke();
    return error_code();
  }
  _LIBCPP_HIDE_FROM_ABI error_code __modify(int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot) noexcept {
    {
      lock_guard<mutex> __lk(__m_);
      __entry* __e = __find(__to_socket(__fd));
      if (!__e)
        return error_code(ENOENT, generic_category());
      __e->__events_  = __ev;
      __e->__tag_     = __tag;
      __e->__oneshot_ = __oneshot;
      __e->__armed_   = true;
    }
    __poke();
    return error_code();
  }
  _LIBCPP_HIDE_FROM_ABI error_code __deregister(int __fd) noexcept {
    {
      lock_guard<mutex> __lk(__m_);
      SOCKET __s = __to_socket(__fd);
      for (size_t __i = 0; __i != __entries_.size(); ++__i) {
        if (__entries_[__i].__s_ == __s) {
          __entries_[__i] = __entries_.back();
          __entries_.pop_back();
          __poke();
          return error_code();
        }
      }
    }
    return error_code(ENOENT, generic_category());
  }

  // Returns the number of events stored in __dst (at most __max, which is at most 64); 0 on timeout, on a wake-up and
  // on an error of the poll.
  _LIBCPP_HIDE_FROM_ABI int __wait(__poller_event* __dst, int __max, int __timeout_ms) noexcept {
    if (__max > 64)
      __max = 64;
    vector<WSAPOLLFD> __fds;
    {
      lock_guard<mutex> __lk(__m_);
      __fds.reserve(__entries_.size() + 1);
      WSAPOLLFD __w;
      __w.fd      = __wake_;
      __w.events  = POLLRDNORM;
      __w.revents = 0;
      __fds.push_back(__w);
      for (const __entry& __e : __entries_) {
        if (!__e.__armed_)
          continue;
        WSAPOLLFD __p;
        __p.fd     = __e.__s_;
        __p.events = static_cast<SHORT>(
            ((__e.__events_ & __poll_read) ? POLLRDNORM : 0) | ((__e.__events_ & __poll_write) ? POLLWRNORM : 0));
        __p.revents = 0;
        __fds.push_back(__p);
      }
      __waiting_.store(true, memory_order_release);
    }
    int __n = ::WSAPoll(__fds.data(), static_cast<ULONG>(__fds.size()), __timeout_ms);
    __waiting_.store(false, memory_order_release);
    if (__n <= 0)
      return 0;
    if (__fds[0].revents != 0) { // drain the wake-up
      char __buf[64];
      while (::recv(__wake_, __buf, sizeof(__buf), 0) > 0) {
      }
    }
    int __count = 0;
    lock_guard<mutex> __lk(__m_);
    for (size_t __i = 1; __i != __fds.size() && __count != __max; ++__i) {
      if (__fds[__i].revents == 0)
        continue;
      __entry* __e = __find(__fds[__i].fd);
      if (!__e || !__e->__armed_) // deregistered or changed while the poll ran
        continue;
      unsigned __r = 0;
      if (__fds[__i].revents & POLLRDNORM)
        __r |= __poll_read;
      if (__fds[__i].revents & POLLWRNORM)
        __r |= __poll_write;
      if (__fds[__i].revents & (POLLERR | POLLHUP | POLLNVAL))
        __r |= __poll_error;
      __dst[__count].__tag_    = __e->__tag_;
      __dst[__count].__events_ = __r;
      ++__count;
      if (__e->__oneshot_)
        __e->__armed_ = false;
    }
    return __count;
  }

  _LIBCPP_HIDE_FROM_ABI void __wakeup() noexcept {
    char __one = 1;
    (void)::send(__wake_, &__one, 1, 0);
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_WINSOCK

#endif // _LIBCPP___IO_SUPPORT_WINSOCK_POLLER_H
