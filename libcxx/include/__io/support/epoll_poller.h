//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_EPOLL_POLLER_H
#define _LIBCPP___IO_SUPPORT_EPOLL_POLLER_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_EPOLL

#  include <__io/support/poller_types.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <__system_error/throw_system_error.h>
#  include <cerrno>
#  include <cstdint>
#  include <sys/epoll.h>
#  include <sys/eventfd.h>
#  include <unistd.h>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The Linux backend of the poller interface (poller.h): one epoll instance plus an eventfd for __wakeup(). The wake-up
// event is consumed inside __wait and never reported.
class __epoll_poller {
  static constexpr uint64_t __wake_tag_ = ~uint64_t(0);

  int __epfd_;
  int __wakefd_;

  _LIBCPP_HIDE_FROM_ABI static unsigned __to_epoll(unsigned __ev, bool __oneshot) noexcept {
    unsigned __r = 0;
    if (__ev & __poll_read)
      __r |= EPOLLIN;
    if (__ev & __poll_write)
      __r |= EPOLLOUT;
    if (__oneshot)
      __r |= EPOLLONESHOT;
    return __r;
  }

  _LIBCPP_HIDE_FROM_ABI static unsigned __from_epoll(unsigned __ev) noexcept {
    unsigned __r = 0;
    if (__ev & EPOLLIN)
      __r |= __poll_read;
    if (__ev & EPOLLOUT)
      __r |= __poll_write;
    if (__ev & (EPOLLERR | EPOLLHUP))
      __r |= __poll_error;
    return __r;
  }

  _LIBCPP_HIDE_FROM_ABI error_code __ctl(int __op, int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot) noexcept {
    epoll_event __e;
    __e.events   = __to_epoll(__ev, __oneshot);
    __e.data.u64 = __tag;
    if (::epoll_ctl(__epfd_, __op, __fd, &__e) != 0)
      return error_code(errno, generic_category());
    return error_code();
  }

public:
  _LIBCPP_HIDE_FROM_ABI __epoll_poller() {
    __epfd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (__epfd_ < 0)
      std::__throw_system_error(errno, "epoll_create1 failed");
    __wakefd_ = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (__wakefd_ < 0) {
      int __err = errno;
      ::close(__epfd_);
      std::__throw_system_error(__err, "eventfd failed");
    }
    epoll_event __e;
    __e.events   = EPOLLIN;
    __e.data.u64 = __wake_tag_;
    if (::epoll_ctl(__epfd_, EPOLL_CTL_ADD, __wakefd_, &__e) != 0) {
      int __err = errno;
      ::close(__wakefd_);
      ::close(__epfd_);
      std::__throw_system_error(__err, "epoll_ctl failed");
    }
  }

  __epoll_poller(const __epoll_poller&)            = delete;
  __epoll_poller& operator=(const __epoll_poller&) = delete;

  _LIBCPP_HIDE_FROM_ABI ~__epoll_poller() {
    ::close(__wakefd_);
    ::close(__epfd_);
  }

  _LIBCPP_HIDE_FROM_ABI error_code __register(int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot) noexcept {
    return __ctl(EPOLL_CTL_ADD, __fd, __ev, __tag, __oneshot);
  }
  _LIBCPP_HIDE_FROM_ABI error_code __modify(int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot) noexcept {
    return __ctl(EPOLL_CTL_MOD, __fd, __ev, __tag, __oneshot);
  }
  _LIBCPP_HIDE_FROM_ABI error_code __deregister(int __fd) noexcept {
    if (::epoll_ctl(__epfd_, EPOLL_CTL_DEL, __fd, nullptr) != 0)
      return error_code(errno, generic_category());
    return error_code();
  }

  // Returns the number of events stored in __dst (at most __max, which is at most 64); 0 on timeout, on a wake-up and
  // when interrupted by a signal.
  _LIBCPP_HIDE_FROM_ABI int __wait(__poller_event* __dst, int __max, int __timeout_ms) noexcept {
    epoll_event __evs[64];
    if (__max > 64)
      __max = 64;
    int __n = ::epoll_wait(__epfd_, __evs, __max, __timeout_ms);
    if (__n <= 0)
      return 0;
    int __count = 0;
    for (int __i = 0; __i != __n; ++__i) {
      if (__evs[__i].data.u64 == __wake_tag_) {
        uint64_t __drain;
        ssize_t __r = ::read(__wakefd_, &__drain, sizeof(__drain));
        (void)__r;
        continue;
      }
      __dst[__count].__tag_    = static_cast<uintptr_t>(__evs[__i].data.u64);
      __dst[__count].__events_ = __from_epoll(__evs[__i].events);
      ++__count;
    }
    return __count;
  }

  _LIBCPP_HIDE_FROM_ABI void __wakeup() noexcept {
    uint64_t __one = 1;
    ssize_t __r    = ::write(__wakefd_, &__one, sizeof(__one));
    (void)__r;
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_EPOLL

#endif // _LIBCPP___IO_SUPPORT_EPOLL_POLLER_H
