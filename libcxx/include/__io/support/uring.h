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
#  include <poll.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <unistd.h>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The kernel interface (the UAPI structures of <linux/io_uring.h> and the few constants this backend uses) is declared
// here and not included: the header is absent or too old (before 5.6) on many build hosts, and the interface is a
// stable ABI of the kernel. The layouts are static_assert-ed against their documented sizes.
struct __uring_sqe {
  uint8_t __opcode_;
  uint8_t __flags_;
  uint16_t __ioprio_;
  int32_t __fd_;
  uint64_t __off_; // also addr2
  uint64_t __addr_;
  uint32_t __len_;
  uint32_t __op_flags_; // msg_flags, accept_flags, ...
  uint64_t __user_data_;
  uint16_t __buf_index_;
  uint16_t __personality_;
  int32_t __splice_fd_in_;
  uint64_t __addr3_;
  uint64_t __pad2_;
};
struct __uring_cqe {
  uint64_t __user_data_;
  int32_t __res_;
  uint32_t __flags_;
};
struct __uring_sq_offsets {
  uint32_t __head_, __tail_, __ring_mask_, __ring_entries_, __flags_, __dropped_, __array_, __resv1_;
  uint64_t __user_addr_;
};
struct __uring_cq_offsets {
  uint32_t __head_, __tail_, __ring_mask_, __ring_entries_, __overflow_, __cqes_, __flags_, __resv1_;
  uint64_t __user_addr_;
};
struct __uring_params {
  uint32_t __sq_entries_, __cq_entries_, __flags_, __sq_thread_cpu_, __sq_thread_idle_, __features_, __wq_fd_;
  uint32_t __resv_[3];
  __uring_sq_offsets __sq_off_;
  __uring_cq_offsets __cq_off_;
};
struct __uring_probe_op {
  uint8_t __op_;
  uint8_t __resv_;
  uint16_t __flags_;
  uint32_t __resv2_;
};
struct __uring_probe {
  uint8_t __last_op_;
  uint8_t __ops_len_;
  uint16_t __resv_;
  uint32_t __resv2_[3];
  __uring_probe_op __ops_[1]; // a flexible array in the kernel's declaration
};
static_assert(sizeof(__uring_sqe) == 64, "the kernel's io_uring_sqe is 64 bytes");
static_assert(sizeof(__uring_cqe) == 16, "the kernel's io_uring_cqe is 16 bytes");
static_assert(sizeof(__uring_params) == 120, "the kernel's io_uring_params is 120 bytes");
static_assert(sizeof(__uring_probe_op) == 8, "the kernel's io_uring_probe_op is 8 bytes");

inline constexpr uint32_t __uring_setup_clamp      = 1u << 4;
inline constexpr uint32_t __uring_feat_single_mmap = 1u << 0;
inline constexpr uint32_t __uring_feat_nodrop      = 1u << 1;
inline constexpr uint32_t __uring_sq_cq_overflow   = 1u << 1;
inline constexpr uint32_t __uring_enter_getevents  = 1u << 0;
inline constexpr uint32_t __uring_probe_supported  = 1u << 0;
inline constexpr unsigned __uring_register_probe   = 8;
inline constexpr uint64_t __uring_off_sq_ring      = 0;
inline constexpr uint64_t __uring_off_sqes         = 0x10000000ull;
inline constexpr uint8_t __uring_op_readv          = 1;
inline constexpr uint8_t __uring_op_writev         = 2;
inline constexpr uint8_t __uring_op_sendmsg        = 9;
inline constexpr uint8_t __uring_op_recvmsg        = 10;
inline constexpr uint8_t __uring_op_accept         = 13;
inline constexpr uint8_t __uring_op_async_cancel   = 14;

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
  uint64_t __addr_;   // msghdr*, iovec*, or the sockaddr* of an accept
  uint32_t __len_;    // number of iovecs of a vectored transfer
  uint32_t __flags_;  // msg flags, or accept flags
  uint64_t __offset_; // file offset of a vectored transfer (accept: the address of the socklen_t)
  uint64_t __user_data_;
};

struct __uring_completion {
  uint64_t __user_data_;
  int32_t __res_;
};

class __uring {
  int __fd_              = -1;
  unsigned __sq_entries_ = 0;
  unsigned __cq_entries_ = 0;
  void* __ring_ptr_      = nullptr; // SQ and CQ rings (one mapping with the single-mmap feature)
  size_t __ring_size_    = 0;
  void* __sqes_ptr_      = nullptr;
  size_t __sqes_size_    = 0;

  unsigned* __sq_head_    = nullptr;
  unsigned* __sq_tail_    = nullptr;
  unsigned __sq_mask_     = 0;
  unsigned* __sq_array_   = nullptr;
  unsigned* __sq_flags_   = nullptr;
  unsigned* __cq_head_    = nullptr;
  unsigned* __cq_tail_    = nullptr;
  unsigned __cq_mask_     = 0;
  __uring_cqe* __cqes_    = nullptr;
  __uring_sqe* __sqes_    = nullptr;
  unsigned __unsubmitted_ = 0;

  // The system call numbers are the same on every architecture that has io_uring (the generic table), except on the
  // ones with an own numbering, where the call is reported as unavailable.
#  if defined(SYS_io_uring_setup) && defined(SYS_io_uring_enter) && defined(SYS_io_uring_register)
  static constexpr long __nr_setup_    = SYS_io_uring_setup;
  static constexpr long __nr_enter_    = SYS_io_uring_enter;
  static constexpr long __nr_register_ = SYS_io_uring_register;
#  elif defined(__x86_64__) || defined(__i386__) || defined(__aarch64__) || defined(__arm__) || defined(__riscv) ||    \
      defined(__s390x__) || defined(__powerpc64__) || defined(__loongarch__)
  static constexpr long __nr_setup_    = 425;
  static constexpr long __nr_enter_    = 426;
  static constexpr long __nr_register_ = 427;
#  else
  static constexpr long __nr_setup_    = -1;
  static constexpr long __nr_enter_    = -1;
  static constexpr long __nr_register_ = -1;
#  endif

  _LIBCPP_HIDE_FROM_ABI static long __sys_setup(unsigned __entries, __uring_params* __p) noexcept {
    if (__nr_setup_ < 0) {
      errno = ENOSYS;
      return -1;
    }
    return ::syscall(__nr_setup_, __entries, __p);
  }
  _LIBCPP_HIDE_FROM_ABI static long
  __sys_enter(int __fd, unsigned __to_submit, unsigned __min_complete, unsigned __flags) noexcept {
    if (__nr_enter_ < 0) {
      errno = ENOSYS;
      return -1;
    }
    return ::syscall(__nr_enter_, __fd, __to_submit, __min_complete, __flags, nullptr, 0);
  }
  _LIBCPP_HIDE_FROM_ABI static long __sys_register(int __fd, unsigned __op, void* __arg, unsigned __n) noexcept {
    if (__nr_register_ < 0) {
      errno = ENOSYS;
      return -1;
    }
    return ::syscall(__nr_register_, __fd, __op, __arg, __n);
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
    alignas(__uring_probe) unsigned char __buf[sizeof(__uring_probe) + __nops * sizeof(__uring_probe_op)];
    ::memset(__buf, 0, sizeof(__buf));
    if (__sys_register(__fd_, __uring_register_probe, __buf, __nops) < 0)
      return false;
    auto* __pr = reinterpret_cast<__uring_probe*>(__buf);
    auto __has = [&](unsigned __op) {
      return __op <= __pr->__last_op_ && __op < __nops && (__pr->__ops_[__op].__flags_ & __uring_probe_supported) != 0;
    };
    return __has(__uring_op_recvmsg) && __has(__uring_op_sendmsg) && __has(__uring_op_accept) &&
           __has(__uring_op_async_cancel) && __has(__uring_op_readv) && __has(__uring_op_writev);
  }

  _LIBCPP_HIDE_FROM_ABI bool __sq_full() const noexcept {
    return *__sq_tail_ - __load_acquire(__sq_head_) >= __sq_entries_;
  }

  _LIBCPP_HIDE_FROM_ABI __uring_sqe* __next_sqe() noexcept {
    unsigned __idx   = *__sq_tail_ & __sq_mask_;
    __uring_sqe* __s = &__sqes_[__idx];
    ::memset(__s, 0, sizeof(*__s));
    __sq_array_[__idx] = __idx;
    return __s;
  }
  _LIBCPP_HIDE_FROM_ABI void __publish() noexcept {
    __store_release(__sq_tail_, *__sq_tail_ + 1);
    ++__unsubmitted_;
  }

public:
  _LIBCPP_HIDE_FROM_ABI __uring() noexcept = default;
  __uring(const __uring&)                  = delete;
  __uring& operator=(const __uring&)       = delete;
  _LIBCPP_HIDE_FROM_ABI ~__uring() { __unmap(); }

  _LIBCPP_HIDE_FROM_ABI int __fd() const noexcept { return __fd_; }
  _LIBCPP_HIDE_FROM_ABI unsigned __completion_capacity() const noexcept { return __cq_entries_; }

  _LIBCPP_HIDE_FROM_ABI error_code __open(unsigned __entries) noexcept {
    __uring_params __p;
    ::memset(&__p, 0, sizeof(__p));
    __p.__flags_ = __uring_setup_clamp;
    long __fd    = __sys_setup(__entries, &__p);
    if (__fd < 0)
      return error_code(errno, generic_category());
    __fd_ = static_cast<int>(__fd);
    if (!(__p.__features_ & __uring_feat_single_mmap) || !(__p.__features_ & __uring_feat_nodrop) || !__probe()) {
      __unmap();
      return error_code(ENOSYS, generic_category());
    }
    __sq_entries_  = __p.__sq_entries_;
    __cq_entries_  = __p.__cq_entries_;
    size_t __sq_sz = __p.__sq_off_.__array_ + __p.__sq_entries_ * sizeof(unsigned);
    size_t __cq_sz = __p.__cq_off_.__cqes_ + __p.__cq_entries_ * sizeof(__uring_cqe);
    __ring_size_   = __sq_sz > __cq_sz ? __sq_sz : __cq_sz;
    __ring_ptr_    = ::mmap(
        nullptr,
        __ring_size_,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE,
        __fd_,
        static_cast<off_t>(__uring_off_sq_ring));
    if (__ring_ptr_ == MAP_FAILED) {
      __ring_ptr_ = nullptr;
      int __e     = errno;
      __unmap();
      return error_code(__e, generic_category());
    }
    __sqes_size_ = __p.__sq_entries_ * sizeof(__uring_sqe);
    __sqes_ptr_  = ::mmap(
        nullptr,
        __sqes_size_,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_POPULATE,
        __fd_,
        static_cast<off_t>(__uring_off_sqes));
    if (__sqes_ptr_ == MAP_FAILED) {
      __sqes_ptr_ = nullptr;
      int __e     = errno;
      __unmap();
      return error_code(__e, generic_category());
    }
    auto* __r   = static_cast<unsigned char*>(__ring_ptr_);
    __sq_head_  = reinterpret_cast<unsigned*>(__r + __p.__sq_off_.__head_);
    __sq_tail_  = reinterpret_cast<unsigned*>(__r + __p.__sq_off_.__tail_);
    __sq_mask_  = *reinterpret_cast<unsigned*>(__r + __p.__sq_off_.__ring_mask_);
    __sq_array_ = reinterpret_cast<unsigned*>(__r + __p.__sq_off_.__array_);
    __sq_flags_ = reinterpret_cast<unsigned*>(__r + __p.__sq_off_.__flags_);
    __cq_head_  = reinterpret_cast<unsigned*>(__r + __p.__cq_off_.__head_);
    __cq_tail_  = reinterpret_cast<unsigned*>(__r + __p.__cq_off_.__tail_);
    __cq_mask_  = *reinterpret_cast<unsigned*>(__r + __p.__cq_off_.__ring_mask_);
    __cqes_     = reinterpret_cast<__uring_cqe*>(__r + __p.__cq_off_.__cqes_);
    __sqes_     = static_cast<__uring_sqe*>(__sqes_ptr_);
    return error_code();
  }

  // Queues one request. Returns false if the submission queue is full (the caller flushes with __submit and retries or
  // refuses). The request is not visible to the kernel before __submit.
  _LIBCPP_HIDE_FROM_ABI bool __prepare(const __uring_request& __rq) noexcept {
    if (__sq_full())
      return false;
    __uring_sqe* __s  = __next_sqe();
    __s->__fd_        = __rq.__fd_;
    __s->__addr_      = __rq.__addr_;
    __s->__len_       = __rq.__len_;
    __s->__off_       = __rq.__offset_;
    __s->__op_flags_  = __rq.__flags_;
    __s->__user_data_ = __rq.__user_data_;
    switch (__rq.__op_) {
    case __uring_opcode::__recvmsg:
      __s->__opcode_ = __uring_op_recvmsg;
      break;
    case __uring_opcode::__sendmsg:
      __s->__opcode_ = __uring_op_sendmsg;
      break;
    case __uring_opcode::__accept:
      __s->__opcode_ = __uring_op_accept;
      break;
    case __uring_opcode::__readv:
      __s->__opcode_ = __uring_op_readv;
      break;
    case __uring_opcode::__writev:
      __s->__opcode_ = __uring_op_writev;
      break;
    }
    __publish();
    return true;
  }

  // Queues the cancellation of the request with the user data __target (its own completion has user data 0).
  _LIBCPP_HIDE_FROM_ABI bool __prepare_cancel(uint64_t __target) noexcept {
    if (__sq_full())
      return false;
    __uring_sqe* __s  = __next_sqe();
    __s->__opcode_    = __uring_op_async_cancel;
    __s->__fd_        = -1;
    __s->__addr_      = __target;
    __s->__user_data_ = 0;
    __publish();
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
  _LIBCPP_HIDE_FROM_ABI void __flush_events() noexcept { (void)__sys_enter(__fd_, 0, 0, __uring_enter_getevents); }

  // Copies up to __limit completions into __dst and releases them; returns the number. Also asks the kernel to flush
  // its overflow list when the completion queue overflowed.
  _LIBCPP_HIDE_FROM_ABI int __reap(__uring_completion* __dst, int __limit) noexcept {
    if (__load_acquire(__sq_flags_) & __uring_sq_cq_overflow)
      (void)__sys_enter(__fd_, 0, 0, __uring_enter_getevents);
    unsigned __head = *__cq_head_; // only this side writes the head
    unsigned __tail = __load_acquire(__cq_tail_);
    int __n         = 0;
    while (__head != __tail && __n != __limit) {
      const __uring_cqe& __c  = __cqes_[__head & __cq_mask_];
      __dst[__n].__user_data_ = __c.__user_data_;
      __dst[__n].__res_       = __c.__res_;
      ++__n;
      ++__head;
    }
    if (__n != 0)
      __store_release(__cq_head_, __head);
    return __n;
  }

  // Waits (at most __ms) for the ring to have a completion.
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
