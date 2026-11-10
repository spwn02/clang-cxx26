//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_CONTEXT_REACTOR_H
#define _LIBCPP___IO_CONTEXT_REACTOR_H

#include <__atomic/atomic.h>
#include <__atomic/memory_order.h>
#include <__chrono/duration.h>
#include <__chrono/steady_clock.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/execution_context.h>
#include <__io/support/config.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__utility/move.h>
#include <__utility/swap.h>
#include <__vector/vector.h>
#include <unordered_map>

#if _LIBCPP_HAS_IO_EPOLL
#  include <__io/support/poller.h>
#  include <__io_context/op.h>
#  include <__io_stream/error.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_EPOLL

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

class __reactor_service;

// A wait for a file descriptor to become ready (see __reactor_wait in io_context).
struct __fd_op : __op_base {
  __reactor_service* __svc_;
  int __fd_;
  unsigned __want_; // __poll_read and/or __poll_write
  unsigned __got_ = 0;

  _LIBCPP_HIDE_FROM_ABI __fd_op(__reactor_service* __s, int __fd, unsigned __want) noexcept
      : __svc_(__s), __fd_(__fd), __want_(__want) {
    __cancel_ = &__cancel_cb;
  }
  // NOLINTBEGIN(bugprone-use-after-move): only the __op_base subobject is moved from
  _LIBCPP_HIDE_FROM_ABI __fd_op(__fd_op&& __o) noexcept
      : __op_base(std::move(__o)), __svc_(__o.__svc_), __fd_(__o.__fd_), __want_(__o.__want_) {
    __cancel_ = &__cancel_cb;
  }
  // NOLINTEND(bugprone-use-after-move)
  _LIBCPP_HIDE_FROM_ABI static void __cancel_cb(__op_base*) noexcept;
};

// A wait for a point in time. The deadline is a copy of the expiry of the timer at the time of the wait.
struct __timer_op : __op_base {
  __reactor_service* __svc_;
  chrono::steady_clock::time_point __deadline_{};
  size_t __seq_         = 0;
  size_t __idx_         = 0;
  __op_list* __waiters_ = nullptr; // the waiters of the timer object, in order of arrival

  _LIBCPP_HIDE_FROM_ABI explicit __timer_op(__reactor_service* __s) noexcept : __svc_(__s) { __cancel_ = &__cancel_cb; }
  _LIBCPP_HIDE_FROM_ABI __timer_op(__timer_op&& __o) noexcept
      : __op_base(std::move(__o)), __svc_(__o.__svc_) { // NOLINT(bugprone-use-after-move)
    __cancel_ = &__cancel_cb;
  }
  _LIBCPP_HIDE_FROM_ABI static void __cancel_cb(__op_base*) noexcept;
};

// The reactor: the part of an io_context that talks to the operating system. It is an execution_context service,
// created on first use (io_context creates it eagerly), and it owns
//   - the poller (epoll on Linux), with the wake-up mechanism,
//   - the file descriptor waits (one slot per descriptor with the list of its pending waits),
//   - the timer queue (a binary heap of the pending timer waits),
//   - the persistent event sources of other services (the signal pipe).
//
// Timers are driven by the timeout argument of the poller's wait (epoll_wait here) instead of a timerfd: it is one
// system call per loop iteration either way, it needs no file descriptor per context and no arming system call when the
// earliest deadline changes, and every platform backend has such a timeout (kevent, GetQueuedCompletionStatus). The
// cost is the resolution of that timeout, one millisecond on Linux: a deadline is rounded up to the next millisecond,
// so a timer never fires early and fires up to a millisecond late.
//
// File descriptor waits are level-triggered and one-shot: a wait completes once when the descriptor is ready, so there
// is no readiness to remember between the event and the wait that consumes it (an edge-triggered registration would
// lose an edge that happened before the wait was started). A wait on a descriptor that was closed while waiting never
// completes (the kernel forgets the registration); cancel it.
//
// Locking: one mutex protects every queue and slot. The mutex is never held while an operation is completed (completion
// posts the continuation to the executor of the awaiting coroutine) or a source callback runs.
//
// Work counting: the number of pending operations is counted here, and io_context::run() does not return while it is
// not zero (like on_work_started/on_work_finished of Asio and Corosio do for an operation in flight).
_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __reactor_service final : public execution_context::service {
  struct __fd_slot {
    __op_list __ops_;
    void (*__source_)() noexcept = nullptr; // a persistent source (the ops list is unused)
    bool __registered_           = false;
  };

  mutex __m_;
  __poller __poller_;
  unordered_map<int, __fd_slot> __slots_;
  vector<__timer_op*> __heap_;
  size_t __seq_ = 0;
  atomic<size_t> __pending_{0};
  atomic<bool> __in_wait_{false};
  bool __shut_down_ = false;

  // --- timer heap (the caller holds __m_)
  _LIBCPP_HIDE_FROM_ABI static bool __before(const __timer_op* __a, const __timer_op* __b) noexcept {
    return __a->__deadline_ < __b->__deadline_ || (__a->__deadline_ == __b->__deadline_ && __a->__seq_ < __b->__seq_);
  }
  _LIBCPP_HIDE_FROM_ABI void __heap_set(size_t __i, __timer_op* __op) noexcept {
    __heap_[__i] = __op;
    __op->__idx_ = __i;
  }
  _LIBCPP_HIDE_FROM_ABI void __sift_up(size_t __i) noexcept {
    __timer_op* __op = __heap_[__i];
    while (__i != 0) {
      size_t __parent = (__i - 1) / 2;
      if (!__before(__op, __heap_[__parent]))
        break;
      __heap_set(__i, __heap_[__parent]);
      __i = __parent;
    }
    __heap_set(__i, __op);
  }
  _LIBCPP_HIDE_FROM_ABI void __sift_down(size_t __i) noexcept {
    __timer_op* __op = __heap_[__i];
    size_t __n       = __heap_.size();
    while (true) {
      size_t __child = 2 * __i + 1;
      if (__child >= __n)
        break;
      if (__child + 1 < __n && __before(__heap_[__child + 1], __heap_[__child]))
        ++__child;
      if (!__before(__heap_[__child], __op))
        break;
      __heap_set(__i, __heap_[__child]);
      __i = __child;
    }
    __heap_set(__i, __op);
  }
  _LIBCPP_HIDE_FROM_ABI void __heap_push(__timer_op* __op) {
    __heap_.push_back(__op);
    __sift_up(__heap_.size() - 1);
  }
  _LIBCPP_HIDE_FROM_ABI void __heap_erase(__timer_op* __op) noexcept {
    size_t __i         = __op->__idx_;
    __timer_op* __last = __heap_.back();
    __heap_.pop_back();
    if (__last != __op) {
      __heap_set(__i, __last);
      __sift_up(__i);
      __sift_down(__last->__idx_);
    }
  }

  // --- descriptor slots (the caller holds __m_)
  _LIBCPP_HIDE_FROM_ABI static unsigned __interest(const __fd_slot& __s) noexcept {
    unsigned __r = 0;
    for (const __op_base* __b = __s.__ops_.__front(); __b; __b = __b->__next_)
      __r |= static_cast<const __fd_op*>(__b)->__want_;
    return __r;
  }

  // Arms the one-shot registration of the descriptor with what the pending waits want.
  _LIBCPP_HIDE_FROM_ABI error_code __arm(int __fd, __fd_slot& __s) noexcept {
    unsigned __want = __interest(__s);
    error_code __ec = __s.__registered_ ? __poller_.__modify(__fd, __want, static_cast<uintptr_t>(__fd), true)
                                        : __poller_.__register(__fd, __want, static_cast<uintptr_t>(__fd), true);
    if (__ec == errc::no_such_file_or_directory && __s.__registered_) // closed behind our back
      __ec = __poller_.__register(__fd, __want, static_cast<uintptr_t>(__fd), true);
    else if (__ec == errc::file_exists && !__s.__registered_)
      __ec = __poller_.__modify(__fd, __want, static_cast<uintptr_t>(__fd), true);
    if (!__ec)
      __s.__registered_ = true;
    return __ec;
  }

  _LIBCPP_HIDE_FROM_ABI void __drop_slot_if_empty(unordered_map<int, __fd_slot>::iterator __it) noexcept {
    if (__it->second.__ops_.__empty() && !__it->second.__source_) {
      if (__it->second.__registered_)
        (void)__poller_.__deregister(__it->first);
      __slots_.erase(__it);
    }
  }

  _LIBCPP_HIDE_FROM_ABI void __kick() noexcept {
    if (__in_wait_.load(memory_order_acquire))
      __poller_.__wakeup();
  }

public:
  _LIBCPP_HIDE_FROM_ABI explicit __reactor_service(execution_context&) {}

  // Abandons everything that is still pending: the coroutines that wait are destroyed, not resumed (like the queued
  // work of an io_context). Operations started afterwards complete with an error.
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL void shutdown() override {
    vector<coroutine_handle<>> __handles;
    {
      lock_guard<mutex> __lk(__m_);
      __shut_down_ = true;
      for (auto& __kv : __slots_) {
        while (__op_base* __op = __kv.second.__ops_.__front()) {
          __kv.second.__ops_.__remove(__op);
          __op->__state_ = __op_base::__done;
          __handles.push_back(__op->__cont_.h);
        }
      }
      for (__timer_op* __op : __heap_) {
        if (__op->__waiters_)
          __op->__waiters_->__remove(__op);
        __op->__state_ = __op_base::__done;
        __handles.push_back(__op->__cont_.h);
      }
      __heap_.clear();
      __pending_.store(0, memory_order_release);
    }
    for (coroutine_handle<> __h : __handles)
      if (__h)
        __h.destroy();
  }

  // ---------------------------------------------------------------------------------------------------------------
  // Interface for the services and awaitables built on the reactor. Each start function returns true if the operation
  // is queued (the caller must then not touch it any more and suspend), and false if it completed immediately with
  // __ec_ set (the caller does not suspend).

  _LIBCPP_HIDE_FROM_ABI size_t __pending() const noexcept { return __pending_.load(memory_order_acquire); }
  _LIBCPP_HIDE_FROM_ABI void __wakeup() noexcept { __poller_.__wakeup(); }

  // Counts an operation of another service (a signal wait) as pending work.
  _LIBCPP_HIDE_FROM_ABI void __op_started() noexcept { __pending_.fetch_add(1, memory_order_acq_rel); }

  // Completes a queued operation that its owner has already removed from its queue and moved to __done: posts the
  // continuation to the executor of the awaiting coroutine, then uncounts it. May be called from any thread, with no
  // lock held. The operation must not be touched afterwards.
  _LIBCPP_HIDE_FROM_ABI void __complete(__op_base* __op) noexcept {
    executor_ref __ex = __op->__ex_;
    continuation* __c = &__op->__cont_;
    __ex.post(*__c);
    if (__pending_.fetch_sub(1, memory_order_acq_rel) == 1)
      __poller_.__wakeup(); // run() may have to return
  }

  _LIBCPP_HIDE_FROM_ABI bool __start_fd_wait(__fd_op& __op) {
    lock_guard<mutex> __lk(__m_);
    if (__shut_down_ || __op.__cancel_requested_) {
      __op.__ec_    = make_error_code(error::canceled);
      __op.__state_ = __op_base::__done;
      return false;
    }
    auto __it = __slots_.try_emplace(__op.__fd_).first;
    if (__it->second.__source_) {
      __op.__ec_    = make_error_code(errc::device_or_resource_busy);
      __op.__state_ = __op_base::__done;
      return false;
    }
    __it->second.__ops_.__push_back(&__op);
    if (error_code __ec = __arm(__op.__fd_, __it->second)) {
      __it->second.__ops_.__remove(&__op);
      __drop_slot_if_empty(__it);
      __op.__ec_    = __ec;
      __op.__state_ = __op_base::__done;
      return false;
    }
    __op.__state_ = __op_base::__pending;
    __pending_.fetch_add(1, memory_order_acq_rel);
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI void __cancel_fd(__fd_op* __op) noexcept {
    {
      lock_guard<mutex> __lk(__m_);
      if (__op->__state_ == __op_base::__idle) {
        __op->__cancel_requested_ = true;
        return;
      }
      if (__op->__state_ == __op_base::__done)
        return;
      auto __it = __slots_.find(__op->__fd_);
      __it->second.__ops_.__remove(__op);
      __op->__state_ = __op_base::__done;
      __op->__ec_    = make_error_code(error::canceled);
      if (__it->second.__ops_.__empty())
        __drop_slot_if_empty(__it);
      else
        (void)__arm(__op->__fd_, __it->second);
    }
    __complete(__op);
  }

  _LIBCPP_HIDE_FROM_ABI bool __start_timer(__timer_op& __op, __op_list& __waiters) {
    lock_guard<mutex> __lk(__m_);
    if (__shut_down_ || __op.__cancel_requested_) {
      __op.__ec_    = make_error_code(error::canceled);
      __op.__state_ = __op_base::__done;
      return false;
    }
    __op.__seq_     = __seq_++;
    __op.__waiters_ = &__waiters;
    __heap_push(&__op);
    __waiters.__push_back(&__op);
    __op.__state_ = __op_base::__pending;
    __pending_.fetch_add(1, memory_order_acq_rel);
    if (__heap_.front() == &__op) // a new earliest deadline: a thread blocked in the poller must recompute its timeout
      __kick();
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI void __cancel_timer_op(__timer_op* __op) noexcept {
    {
      lock_guard<mutex> __lk(__m_);
      if (__op->__state_ == __op_base::__idle) {
        __op->__cancel_requested_ = true;
        return;
      }
      if (__op->__state_ == __op_base::__done)
        return;
      __heap_erase(__op);
      __op->__waiters_->__remove(__op);
      __op->__state_ = __op_base::__done;
      __op->__ec_    = make_error_code(error::canceled);
    }
    __complete(__op);
  }

  // Cancels all (or only the oldest) waits of one timer; returns how many were cancelled.
  _LIBCPP_HIDE_FROM_ABI size_t __cancel_timers(__op_list& __waiters, bool __one) noexcept {
    vector<__op_base*> __ops;
    {
      lock_guard<mutex> __lk(__m_);
      while (__op_base* __b = __waiters.__front()) {
        __timer_op* __op = static_cast<__timer_op*>(__b);
        __heap_erase(__op);
        __waiters.__remove(__op);
        __op->__state_ = __op_base::__done;
        __op->__ec_    = make_error_code(error::canceled);
        __ops.push_back(__op);
        if (__one)
          break;
      }
    }
    for (__op_base* __op : __ops)
      __complete(__op);
    return __ops.size();
  }

  // A persistent, level-triggered event source: __fn is called (from whichever thread polls, with no lock held) when
  // the descriptor is readable. It does not keep run() alive.
  _LIBCPP_HIDE_FROM_ABI error_code __add_source(int __fd, void (*__fn)() noexcept) {
    lock_guard<mutex> __lk(__m_);
    auto __it = __slots_.try_emplace(__fd).first;
    if (__it->second.__source_ == __fn)
      return error_code();
    if (__it->second.__source_ || !__it->second.__ops_.__empty())
      return make_error_code(errc::device_or_resource_busy);
    error_code __ec = __poller_.__register(__fd, __poll_read, static_cast<uintptr_t>(__fd), false);
    if (__ec) {
      __slots_.erase(__it);
      return __ec;
    }
    __it->second.__source_     = __fn;
    __it->second.__registered_ = true;
    return error_code();
  }

  _LIBCPP_HIDE_FROM_ABI void __remove_source(int __fd) noexcept {
    lock_guard<mutex> __lk(__m_);
    auto __it = __slots_.find(__fd);
    if (__it == __slots_.end() || !__it->second.__source_)
      return;
    (void)__poller_.__deregister(__fd);
    __slots_.erase(__it);
  }

  // One round of the event loop: waits for events (not at all if !__block; otherwise until the earliest timer expires,
  // an event arrives or __wakeup() is called), then completes the operations whose events arrived and whose timers
  // expired. Called by one thread at a time (io_context guarantees that).
  _LIBCPP_HIDE_FROM_ABI void __poll(bool __block) {
    __poller_event __evs[64];
    int __timeout = 0;
    if (__block) {
      lock_guard<mutex> __lk(__m_);
      if (__heap_.empty()) {
        __timeout = -1;
      } else {
        auto __ns =
            chrono::duration_cast<chrono::nanoseconds>(__heap_.front()->__deadline_ - chrono::steady_clock::now());
        if (__ns.count() <= 0)
          __timeout = 0;
        else {
          long long __ms = (__ns.count() + 999999) / 1000000;
          __timeout      = __ms > 3600000 ? 3600000 : static_cast<int>(__ms);
        }
      }
      if (__timeout != 0)
        __in_wait_.store(true, memory_order_release);
    }
    int __n = __poller_.__wait(__evs, 64, __timeout);
    if (__timeout != 0)
      __in_wait_.store(false, memory_order_release);

    vector<__op_base*> __done;
    vector<void (*)() noexcept> __sources;
    {
      lock_guard<mutex> __lk(__m_);
      for (int __i = 0; __i != __n; ++__i) {
        int __fd  = static_cast<int>(__evs[__i].__tag_);
        auto __it = __slots_.find(__fd);
        if (__it == __slots_.end())
          continue;
        __fd_slot& __s = __it->second;
        if (__s.__source_) {
          __sources.push_back(__s.__source_);
          continue;
        }
        unsigned __ready = __evs[__i].__events_;
        if (__ready & __poll_error)
          __ready |= __poll_read | __poll_write;
        for (__op_base* __b = __s.__ops_.__front(); __b;) {
          __op_base* __next = __b->__next_;
          __fd_op* __op     = static_cast<__fd_op*>(__b);
          if (__op->__want_ & __ready) {
            __s.__ops_.__remove(__op);
            __op->__got_   = __ready & (__op->__want_ | __poll_error);
            __op->__state_ = __op_base::__done;
            __done.push_back(__op);
          }
          __b = __next;
        }
        if (__s.__ops_.__empty())
          __drop_slot_if_empty(__it);
        else
          (void)__arm(__fd, __s);
      }
      auto __now = chrono::steady_clock::now();
      while (!__heap_.empty() && __heap_.front()->__deadline_ <= __now) {
        __timer_op* __op = __heap_.front();
        __heap_erase(__op);
        __op->__waiters_->__remove(__op);
        __op->__state_ = __op_base::__done;
        __op->__ec_    = error_code();
        __done.push_back(__op);
      }
    }
    for (void (*__fn)() noexcept : __sources)
      __fn();
    for (__op_base* __op : __done)
      __complete(__op);
  }
};
_LIBCPP_DIAGNOSTIC_POP

inline void __fd_op::__cancel_cb(__op_base* __b) noexcept {
  __fd_op* __op = static_cast<__fd_op*>(__b);
  __op->__svc_->__cancel_fd(__op);
}
inline void __timer_op::__cancel_cb(__op_base* __b) noexcept {
  __timer_op* __op = static_cast<__timer_op*>(__b);
  __op->__svc_->__cancel_timer_op(__op);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_EPOLL

#endif // _LIBCPP___IO_CONTEXT_REACTOR_H
