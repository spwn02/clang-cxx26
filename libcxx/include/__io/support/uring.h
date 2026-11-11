//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_URING_H
#define _LIBCPP___IO_SUPPORT_URING_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_URING

#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <cerrno>
#  include <cstdint>
#  include <cstring>
#  include <linux/io_uring.h>
#  include <poll.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The completion backend of Linux (io_uring), called with raw system calls: there is no build or link dependency on
// liburing, the same way the TLS engine loads OpenSSL with dlopen. It is an addition to the epoll poller, not a
// replacement: the ring serves the operations that the kernel can run to completion (the stream and datagram transfers
// and the accept of the sockets), and the file descriptor waits, the timers and the signal pipe stay on epoll. The ring
// descriptor is itself pollable, so the reactor registers it with the poller and reaps the completions after the wait.
//
// Not thread-safe: the reactor serializes the submissions with its lock, and one thread at a time reaps.
//
// Opening fails with the error of the system when the kernel is too old (before 5.6), has io_uring disabled
// (kernel.io_uring_disabled), or a seccomp filter refuses the call (as in many container runtimes); the reactor then
// runs on epoll alone.

// The operations the reactor submits (a neutral enumeration; the kernel opcode is chosen in __prepare).
enum class __uring_opcode : unsigned char { __recvmsg, __sendmsg, __accept, __readv, __writev };

struct __uring_request {
  __uring_opcode __op_;
  int __fd_;
  uint64_t __addr_;     // msghdr*, iovec*, or the sockaddr* of an accept
  uint32_t __len_;      // number of iovecs of a vectored transfer
  uint32_t __flags_;    // msg flags, or accept flags
  uint64_t __offset_;   // file offset of a vectored transfer (accept: the address of the socklen_t)
  uint64_t __user_data_;
};

struct __uring_completion {
  uint64_t __user_data_;
  int32_t __res_;
};

class __uring {
  int __fd_ = -1;
  unsigned __sq_entries_ = 0;
  unsigned __cq_entries_ = 0;
  void* __ring_ptr_      = nullptr; // SQ and CQ rings (one mapping with IORING_FEAT_SINGLE_MMAP)
  size_t __ring_size_    = 0;
  void* __sqes_ptr_      = nullptr;
  size_t __sqes_size_    = 0;

  unsigned* __sq_head_   = nullptr;
  unsigned* __sq_tail_   = nullptr;
  unsigned __sq_mask_    = 0;
  unsigned* __sq_array_  = nullptr;
  unsigned* __sq_flags_  = nullptr;
  unsigned* __cq_head_   = nullptr;
  unsigned* __cq_tail_   = nullptr;
  unsigned __cq_mask_    = 0;
  io_uring_cqe* __cqes_  = nullptr;
  io_uring_sqe* __sqes_  = nullptr;
  unsigned __unsubmitted_ = 0;

  _LIBCPP_HIDE_FROM_ABI static long __sys_setup(unsigned __entries, io_uring_params* __p) noexcept {
#  ifdef SYS_io_uring_setup
    return ::syscall(SYS_io_uring_setup, __entries, __p);
#  else
    (void)__entries;
    (void)__p;
    errno = ENOSYS;
    return -1;
#  endif
  }
  _LIBCPP_HIDE_FROM_ABI static long
  __sys_enter(int __fd, unsigned __to_submit, unsigned __min_complete, unsigned __flags) noexcept {
#  ifdef SYS_io_uring_enter
    return ::syscall(SYS_io_uring_enter, __fd, __to_submit, __min_complete, __flags, nullptr, 0);
#  else
    (void)__fd;
    (void)__to_submit;
    (void)__min_complete;
    (void)__flags;
    errno = ENOSYS;
    return -1;
#  endif
  }
  _LIBCPP_HIDE_FROM_ABI static long __sys_register(int __fd, unsigned __op, void* __arg, unsigned __n) noexcept {
#  ifdef SYS_io_uring_register
    return ::syscall(SYS_io_uring_register, __fd, __op, __arg, __n);
#  else
    (void)__fd;
    (void)__op;
    (void)__arg;
    (void)__n;
    errno = ENOSYS;
    return -1;
#  endif
  }

  _LIBCPP_HIDE_FROM_ABI static unsigned __load_acquire(const unsigned* __p) noexcept {
    return __atomic_load_n(__p, __ATOMIC_ACQUIRE);
  }
  _LIBCPP_HIDE_FROM_ABI static void __store_release(unsigned* __p, unsigned __v) noexcept {
    __atomic_store_n(__p, __v, __ATOMIC_RELEASE);
  }

  _LIBCPP_HIDE_FROM_ABI void __unmap() noexcept {
    if (__sqes_ptr_)
      ::munmap(__sqes_ptr_, __sqes_size_);
    if (__ring_ptr_)
      ::munmap(__ring_ptr_, __ring_size_);
    if (__fd_ >= 0)
      ::close(__fd_);
    __sqes_ptr_ = __ring_ptr_ = nullptr;
    __fd_                     = -1;
  }

  // The operations this backend needs must be supported by the kernel; the probe exists since 5.6.
  _LIBCPP_HIDE_FROM_ABI bool __probe() noexcept {
    constexpr unsigned __nops = 64;
    alignas(io_uring_probe) unsigned char __buf[sizeof(io_uring_probe) + __nops * sizeof(io_uring_probe_op)];
    ::memset(__buf, 0, sizeof(__buf));
    // IORING_REGISTER_PROBE is 8
    if (__sys_register(__fd_, 8, __buf, __nops) < 0)
      return false;
    auto* __pr = reinterpret_cast<io_uring_probe*>(__buf);
    auto __has = [&](unsigned __op) {
      return __op <= __pr->last_op && __op < __nops && (__pr->ops[__op].flags & IO_URING_OP_SUPPORTED) != 0;
    };
    return __has(IORING_OP_RECVMSG) && __has(IORING_OP_SENDMSG) && __has(IORING_OP_ACCEPT) &&
           __has(IORING_OP_ASYNC_CANCEL) && __has(IORING_OP_READV) && __has(IORING_OP_WRITEV);
  }

public:
  _LIBCPP_HIDE_FROM_ABI __uring() noexcept = default;
  __uring(const __uring&)                  = delete;
  __uring& operator=(const __uring&)       = delete;
  _LIBCPP_HIDE_FROM_ABI ~__uring() { __unmap(); }

  _LIBCPP_HIDE_FROM_ABI int __fd() const noexcept { return __fd_; }
  _LIBCPP_HIDE_FROM_ABI unsigned __completion_capacity() const noexcept { return __cq_entries_; }

  _LIBCPP_HIDE_FROM_ABI error_code __open(unsigned __entries) noexcept {
    io_uring_params __p;
    ::memset(&__p, 0, sizeof(__p));
    __p.flags = IORING_SETUP_CLAMP;
    long __fd = __sys_setup(__entries, &__p);
    if (__fd < 0)
      return error_code(errno, generic_category());
    __fd_ = static_cast<int>(__fd);
    if (!(__p.features & IORING_FEAT_SINGLE_MMAP) || !(__p.features & IORING_FEAT_NODROP) || !__probe()) {
      __unmap();
      return error_code(ENOSYS, generic_category());
    }
    __sq_entries_ = __p.sq_entries;
    __cq_entries_ = __p.cq_entries;
    size_t __sq_sz = __p.sq_off.array + __p.sq_entries * sizeof(unsigned);
    size_t __cq_sz = __p.cq_off.cqes + __p.cq_entries * sizeof(io_uring_cqe);
    __ring_size_   = __sq_sz > __cq_sz ? __sq_sz : __cq_sz;
    __ring_ptr_    = ::mmap(nullptr, __ring_size_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, __fd_,
                            IORING_OFF_SQ_RING);
    if (__ring_ptr_ == MAP_FAILED) {
      __ring_ptr_ = nullptr;
      int __e     = errno;
      __unmap();
      return error_code(__e, generic_category());
    }
    __sqes_size_ = __p.sq_entries * sizeof(io_uring_sqe);
    __sqes_ptr_  = ::mmap(nullptr, __sqes_size_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, __fd_,
                          IORING_OFF_SQES);
    if (__sqes_ptr_ == MAP_FAILED) {
      __sqes_ptr_ = nullptr;
      int __e     = errno;
      __unmap();
      return error_code(__e, generic_category());
    }
    auto* __r   = static_cast<unsigned char*>(__ring_ptr_);
    __sq_head_  = reinterpret_cast<unsigned*>(__r + __p.sq_off.head);
    __sq_tail_  = reinterpret_cast<unsigned*>(__r + __p.sq_off.tail);
    __sq_mask_  = *reinterpret_cast<unsigned*>(__r + __p.sq_off.ring_mask);
    __sq_array_ = reinterpret_cast<unsigned*>(__r + __p.sq_off.array);
    __sq_flags_ = reinterpret_cast<unsigned*>(__r + __p.sq_off.flags);
    __cq_head_  = reinterpret_cast<unsigned*>(__r + __p.cq_off.head);
    __cq_tail_  = reinterpret_cast<unsigned*>(__r + __p.cq_off.tail);
    __cq_mask_  = *reinterpret_cast<unsigned*>(__r + __p.cq_off.ring_mask);
    __cqes_     = reinterpret_cast<io_uring_cqe*>(__r + __p.cq_off.cqes);
    __sqes_     = static_cast<io_uring_sqe*>(__sqes_ptr_);
    return error_code();
  }

  // Queues one request. Returns false if the submission queue is full (the caller flushes with __submit and retries or
  // refuses). The request is not visible to the kernel before __submit.
  _LIBCPP_HIDE_FROM_ABI bool __prepare(const __uring_request& __rq) noexcept {
    unsigned __tail = *__sq_tail_; // only this side writes the tail
    unsigned __head = __load_acquire(__sq_head_);
    if (__tail - __head >= __sq_entries_)
      return false;
    unsigned __idx    = __tail & __sq_mask_;
    io_uring_sqe* __s = &__sqes_[__idx];
    ::memset(__s, 0, sizeof(*__s));
    __s->fd        = __rq.__fd_;
    __s->addr      = __rq.__addr_;
    __s->len       = __rq.__len_;
    __s->off       = __rq.__offset_;
    __s->user_data = __rq.__user_data_;
    switch (__rq.__op_) {
    case __uring_opcode::__recvmsg:
      __s->opcode    = IORING_OP_RECVMSG;
      __s->msg_flags = __rq.__flags_;
      break;
    case __uring_opcode::__sendmsg:
      __s->opcode    = IORING_OP_SENDMSG;
      __s->msg_flags = __rq.__flags_;
      break;
    case __uring_opcode::__accept:
      __s->opcode       = IORING_OP_ACCEPT;
      __s->accept_flags = __rq.__flags_;
      break;
    case __uring_opcode::__readv:
      __s->opcode = IORING_OP_READV;
      break;
    case __uring_opcode::__writev:
      __s->opcode = IORING_OP_WRITEV;
      break;
    }
    __sq_array_[__idx] = __idx;
    __store_release(__sq_tail_, __tail + 1);
    ++__unsubmitted_;
    return true;
  }

  // Queues the cancellation of the request with the user data __target (its own completion has user data 0).
  _LIBCPP_HIDE_FROM_ABI bool __prepare_cancel(uint64_t __target) noexcept {
    unsigned __tail = *__sq_tail_;
    unsigned __head = __load_acquire(__sq_head_);
    if (__tail - __head >= __sq_entries_)
      return false;
    unsigned __idx    = __tail & __sq_mask_;
    io_uring_sqe* __s = &__sqes_[__idx];
    ::memset(__s, 0, sizeof(*__s));
    __s->opcode        = IORING_OP_ASYNC_CANCEL;
    __s->fd            = -1;
    __s->addr          = __target;
    __s->user_data     = 0;
    __sq_array_[__idx] = __idx;
    __store_release(__sq_tail_, __tail + 1);
    ++__unsubmitted_;
    return true;
  }

  // Takes back the requests that __submit failed to hand over (the kernel consumed none of them).
  _LIBCPP_HIDE_FROM_ABI void __discard_unsubmitted() noexcept {
    if (__unsubmitted_ != 0) {
      __store_release(__sq_tail_, *__sq_tail_ - __unsubmitted_);
      __unsubmitted_ = 0;
    }
  }

  // Hands the queued requests to the kernel. Returns 0 or the errno.
  _LIBCPP_HIDE_FROM_ABI int __submit() noexcept {
    while (__unsubmitted_ != 0) {
      long __r = __sys_enter(__fd_, __unsubmitted_, 0, 0);
      if (__r < 0) {
        if (errno == EINTR)
          continue;
        return errno;
      }
      if (static_cast<unsigned>(__r) > __unsubmitted_)
        __r = __unsubmitted_;
      __unsubmitted_ -= static_cast<unsigned>(__r);
      if (__r == 0)
        return EAGAIN;
    }
    return 0;
  }

  // Runs the work the kernel has deferred to this thread (the completions of the requests that were just canceled), so
  // that they are in the completion queue when it returns.
  _LIBCPP_HIDE_FROM_ABI void __flush_events() noexcept { (void)__sys_enter(__fd_, 0, 0, IORING_ENTER_GETEVENTS); }

  // Copies up to __max completions into __out and releases them; returns the number. Also asks the kernel to flush its
  // overflow list when the completion queue overflowed.
  _LIBCPP_HIDE_FROM_ABI int __reap(__uring_completion* __out, int __max) noexcept {
    if (__load_acquire(__sq_flags_) & IORING_SQ_CQ_OVERFLOW)
      (void)__sys_enter(__fd_, 0, 0, IORING_ENTER_GETEVENTS);
    unsigned __head = *__cq_head_; // only this side writes the head
    unsigned __tail = __load_acquire(__cq_tail_);
    int __n         = 0;
    while (__head != __tail && __n != __max) {
      const io_uring_cqe& __c = __cqes_[__head & __cq_mask_];
      __out[__n].__user_data_ = __c.user_data;
      __out[__n].__res_       = __c.res;
      ++__n;
      ++__head;
    }
    if (__n != 0)
      __store_release(__cq_head_, __head);
    return __n;
  }

  // Waits (at most __ms) for the ring to have a completion. Used by the shutdown to drain the requests in flight.
  _LIBCPP_HIDE_FROM_ABI void __wait_readable(int __ms) noexcept {
    pollfd __pf;
    __pf.fd      = __fd_;
    __pf.events  = POLLIN;
    __pf.revents = 0;
    (void)::poll(&__pf, 1, __ms);
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_URING

#endif // _LIBCPP___IO_SUPPORT_URING_H
