//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_KQUEUE_POLLER_H
#define _LIBCPP___IO_SUPPORT_KQUEUE_POLLER_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_KQUEUE

#  include <__io/support/poller_types.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <__system_error/throw_system_error.h>
// The system headers use names such as __in and __out for their parameters, which are macros of some platform SDKs (SAL
// annotations) and of the tests of <__config> hygiene; they are hidden while the headers are included.
#  pragma push_macro("__in")
#  undef __in
#  pragma push_macro("__out")
#  undef __out
#  pragma push_macro("__inout")
#  undef __inout
#  pragma push_macro("__pre")
#  undef __pre
#  pragma push_macro("__post")
#  undef __post
#  pragma push_macro("__deref")
#  undef __deref
#  pragma push_macro("__bound")
#  undef __bound
#  pragma push_macro("__nz")
#  undef __nz
#  pragma push_macro("__part")
#  undef __part
#  pragma push_macro("__full")
#  undef __full
#  pragma push_macro("__allocator")
#  undef __allocator
#  pragma push_macro("__deallocate")
#  undef __deallocate
#  pragma push_macro("__format_string")
#  undef __format_string
#  pragma push_macro("__input")
#  undef __input
#  pragma push_macro("__output")
#  undef __output
#  pragma push_macro("__acquire")
#  undef __acquire
#  pragma push_macro("__release")
#  undef __release
#  include <cerrno>
#  include <cstdint>
#  include <cstring>
#  include <fcntl.h>
#  include <sys/event.h>
#  include <sys/types.h>
#  include <time.h>
#  include <unistd.h>
#  pragma pop_macro("__release")
#  pragma pop_macro("__acquire")
#  pragma pop_macro("__output")
#  pragma pop_macro("__input")
#  pragma pop_macro("__format_string")
#  pragma pop_macro("__deallocate")
#  pragma pop_macro("__allocator")
#  pragma pop_macro("__full")
#  pragma pop_macro("__part")
#  pragma pop_macro("__nz")
#  pragma pop_macro("__bound")
#  pragma pop_macro("__deref")
#  pragma pop_macro("__post")
#  pragma pop_macro("__pre")
#  pragma pop_macro("__inout")
#  pragma pop_macro("__out")
#  pragma pop_macro("__in")

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The macOS and FreeBSD backend of the poller interface (poller.h): one kqueue. The wake-up is an EVFILT_USER event
// (NOTE_TRIGGER, EV_CLEAR), which is consumed inside __wait and never reported; no extra descriptor is needed.
//
// The reactor arms descriptor waits as one-shot registrations and re-arms them with __modify; a kqueue has one
// registration per (descriptor, filter), so a registration is up to two kevents (EVFILT_READ and EVFILT_WRITE).
//   - A one-shot registration uses EV_ADD | EV_ONESHOT: the kernel deletes the event after it was returned. This is the
//     closest match of EPOLLONESHOT, whose arming applies to the descriptor as a whole: the other filter of the same
//     descriptor stays armed on kqueue until __modify (which the reactor calls after every delivery that leaves waits
//     behind) or __deregister removes it. A late event of the filter that is no longer wanted is harmless: the reactor
//     finds no wait that wants it.
//     EV_DISPATCH (disable instead of delete, EV_ENABLE to re-arm) was not chosen: it keeps the registration, and with
//     it the knote of a descriptor the reactor has forgotten, alive until the descriptor is closed.
//   - A persistent source (oneshot == false) uses EV_ADD alone: level-triggered, delivered on every __wait while ready.
//   - Both are level-triggered (no EV_CLEAR), as the poller interface requires: a descriptor that is ready when the
//     registration is added is reported by the next __wait.
//   - EV_ADD of an existing registration modifies it, so __register and __modify do the same thing; __modify also
//     deletes the filter that is no longer wanted (ENOENT, the filter has fired as one-shot, is not an error).
// Closing a descriptor removes its registrations from the kqueue.
//
// The kqueue descriptor is close-on-exec (kqueue1 does not exist on macOS: fcntl). A kqueue is not inherited by the
// child of fork(): after fork the child must not use an io_context of its parent (the parent's poller keeps working).
class __kqueue_poller {
  static constexpr uintptr_t __wake_ident_ = 0;

  int __kq_;

  _LIBCPP_HIDE_FROM_ABI static struct kevent
  __make(uintptr_t __ident, int __filter, unsigned __flags, unsigned __fflags, uintptr_t __tag) noexcept {
    struct kevent __e;
    ::memset(&__e, 0, sizeof(__e));
    __e.ident  = __ident;
    __e.filter = static_cast<decltype(__e.filter)>(__filter);
    __e.flags  = static_cast<decltype(__e.flags)>(__flags);
    __e.fflags = static_cast<decltype(__e.fflags)>(__fflags);
    __e.udata  = reinterpret_cast<void*>(__tag);
    return __e;
  }

  // Applies the changes and returns the first real failure. EV_RECEIPT makes kevent report the outcome of every change
  // as an EV_ERROR entry of the event list (data == 0 on success) instead of failing the whole call.
  _LIBCPP_HIDE_FROM_ABI error_code __apply(struct kevent* __changes, int __n, bool __keep_enoent_for_delete) noexcept {
    struct kevent __res[4];
    timespec __zero = {0, 0};
    int __r;
    do {
      __r = ::kevent(__kq_, __changes, __n, __res, __n, &__zero);
    } while (__r < 0 && errno == EINTR);
    if (__r < 0)
      return error_code(errno, generic_category());
    for (int __i = 0; __i != __r; ++__i) {
      if ((__res[__i].flags & EV_ERROR) == 0 || __res[__i].data == 0)
        continue;
      int __err = static_cast<int>(__res[__i].data);
      // Which change is this? The receipt carries the ident and the filter; a delete of a filter that is not armed is
      // not an error (unless the caller deregisters and nothing was armed at all, see __deregister).
      bool __was_delete = false;
      for (int __j = 0; __j != __n; ++__j)
        if (__changes[__j].ident == __res[__i].ident && __changes[__j].filter == __res[__i].filter &&
            (__changes[__j].flags & EV_DELETE))
          __was_delete = true;
      if (__was_delete && __err == ENOENT && !__keep_enoent_for_delete)
        continue;
      return error_code(__err, generic_category());
    }
    return error_code();
  }

  _LIBCPP_HIDE_FROM_ABI error_code
  __set(int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot, bool __prune) noexcept {
    struct kevent __c[2];
    int __n           = 0;
    unsigned __add    = EV_ADD | EV_RECEIPT | (__oneshot ? EV_ONESHOT : 0u);
    unsigned __del    = EV_DELETE | EV_RECEIPT;
    uintptr_t __ident = static_cast<uintptr_t>(__fd);
    if (__ev & __poll_read)
      __c[__n++] = __make(__ident, EVFILT_READ, __add, 0, __tag);
    else if (__prune)
      __c[__n++] = __make(__ident, EVFILT_READ, __del, 0, 0);
    if (__ev & __poll_write)
      __c[__n++] = __make(__ident, EVFILT_WRITE, __add, 0, __tag);
    else if (__prune)
      __c[__n++] = __make(__ident, EVFILT_WRITE, __del, 0, 0);
    if (__n == 0)
      return error_code();
    return __apply(__c, __n, false);
  }

  _LIBCPP_HIDE_FROM_ABI static unsigned __from_event(const struct kevent& __e) noexcept {
    unsigned __r = 0;
    if (__e.flags & EV_ERROR)
      return __poll_error;
    if (__e.filter == EVFILT_READ) {
      __r |= __poll_read;
      // EV_EOF on a read filter: the peer closed its side (a pipe writer, a socket FIN) or a socket error is pending
      // (in fflags). epoll reports the hang-up of a pipe as EPOLLHUP; the same condition is reported as __poll_error
      // here. Every consumer retries the system call after a wake-up, so the extra bit on a half-closed socket (epoll
      // reports only EPOLLIN for a FIN) only makes a concurrent write wait retry.
      if (__e.flags & EV_EOF)
        __r |= __poll_error;
    } else if (__e.filter == EVFILT_WRITE) {
      __r |= __poll_write;
      // EV_EOF on a write filter: the reading side is gone (a refused connection, a broken pipe); fflags has the error.
      if (__e.flags & EV_EOF)
        __r |= __poll_error;
    }
    return __r;
  }

public:
  _LIBCPP_HIDE_FROM_ABI __kqueue_poller() {
    __kq_ = ::kqueue();
    if (__kq_ < 0)
      std::__throw_system_error(errno, "kqueue failed");
    if (::fcntl(__kq_, F_SETFD, FD_CLOEXEC) != 0) {
      int __err = errno;
      ::close(__kq_);
      std::__throw_system_error(__err, "fcntl(FD_CLOEXEC) failed");
    }
    struct kevent __c = __make(__wake_ident_, EVFILT_USER, EV_ADD | EV_CLEAR | EV_RECEIPT, 0, 0);
    if (error_code __ec = __apply(&__c, 1, true)) {
      ::close(__kq_);
      std::__throw_system_error(__ec.value(), "kevent(EVFILT_USER) failed");
    }
  }

  __kqueue_poller(const __kqueue_poller&)            = delete;
  __kqueue_poller& operator=(const __kqueue_poller&) = delete;

  _LIBCPP_HIDE_FROM_ABI ~__kqueue_poller() { ::close(__kq_); }

  _LIBCPP_HIDE_FROM_ABI error_code __register(int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot) noexcept {
    return __set(__fd, __ev, __tag, __oneshot, false);
  }
  _LIBCPP_HIDE_FROM_ABI error_code __modify(int __fd, unsigned __ev, uintptr_t __tag, bool __oneshot) noexcept {
    return __set(__fd, __ev, __tag, __oneshot, true);
  }
  // Removes both filters. ENOENT only if neither was armed (like EPOLL_CTL_DEL of an unknown descriptor); a one-shot
  // filter that has fired is already gone, so one of the two being absent is normal.
  _LIBCPP_HIDE_FROM_ABI error_code __deregister(int __fd) noexcept {
    uintptr_t __ident = static_cast<uintptr_t>(__fd);
    struct kevent __c[2];
    __c[0] = __make(__ident, EVFILT_READ, EV_DELETE | EV_RECEIPT, 0, 0);
    __c[1] = __make(__ident, EVFILT_WRITE, EV_DELETE | EV_RECEIPT, 0, 0);
    struct kevent __res[2];
    timespec __zero = {0, 0};
    int __r;
    do {
      __r = ::kevent(__kq_, __c, 2, __res, 2, &__zero);
    } while (__r < 0 && errno == EINTR);
    if (__r < 0)
      return error_code(errno, generic_category());
    int __failed = 0, __first_err = 0;
    for (int __i = 0; __i != __r; ++__i) {
      if ((__res[__i].flags & EV_ERROR) && __res[__i].data != 0) {
        ++__failed;
        if (__res[__i].data != ENOENT || __first_err == 0)
          __first_err = static_cast<int>(__res[__i].data);
      }
    }
    if (__failed == 2 || (__failed != 0 && __first_err != ENOENT))
      return error_code(__first_err, generic_category());
    return error_code();
  }

  // Returns the number of events stored in __dst (at most __max, which is at most 64); 0 on timeout, on a wake-up and
  // when interrupted by a signal. The timeout has the resolution of the kevent timespec (nanoseconds); the reactor
  // passes milliseconds.
  _LIBCPP_HIDE_FROM_ABI int __wait(__poller_event* __dst, int __max, int __timeout_ms) noexcept {
    struct kevent __evs[64];
    if (__max > 64)
      __max = 64;
    timespec __ts;
    timespec* __tp = nullptr;
    if (__timeout_ms >= 0) {
      __ts.tv_sec  = __timeout_ms / 1000;
      __ts.tv_nsec = static_cast<long>(__timeout_ms % 1000) * 1000000L;
      __tp         = &__ts;
    }
    int __n = ::kevent(__kq_, nullptr, 0, __evs, __max, __tp);
    if (__n <= 0)
      return 0;
    int __count = 0;
    for (int __i = 0; __i != __n; ++__i) {
      if (__evs[__i].filter == EVFILT_USER) // the wake-up; EV_CLEAR has already reset it
        continue;
      unsigned __ev = __from_event(__evs[__i]);
      if (__ev == 0)
        continue;
      __dst[__count].__tag_    = reinterpret_cast<uintptr_t>(__evs[__i].udata);
      __dst[__count].__events_ = __ev;
      ++__count;
    }
    return __count;
  }

  _LIBCPP_HIDE_FROM_ABI void __wakeup() noexcept {
    struct kevent __c = __make(__wake_ident_, EVFILT_USER, 0, NOTE_TRIGGER, 0);
    (void)::kevent(__kq_, &__c, 1, nullptr, 0, nullptr);
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_KQUEUE

#endif // _LIBCPP___IO_SUPPORT_KQUEUE_POLLER_H
