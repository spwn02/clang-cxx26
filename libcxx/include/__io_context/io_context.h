//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_CONTEXT_IO_CONTEXT_H
#define _LIBCPP___IO_CONTEXT_IO_CONTEXT_H

#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/frame_allocator.h>
#include <__io/support/config.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>

#if _LIBCPP_HAS_IO_EPOLL
#  include <__io_context/reactor.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_EPOLL

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

class io_context;

// The io_context whose run loop the current thread is inside, if any.
_LIBCPP_HIDE_FROM_ABI inline const io_context*& __current_io_context() noexcept {
  static thread_local const io_context* __p = nullptr;
  return __p;
}

// P4100R1 [8.8]; provisional specification: Corosio. An event-loop ExecutionContext: the threads that call run() resume
// the posted continuations and drive the platform I/O of the context (the reactor, a service of the context that owns
// the epoll instance, the timer queue and the signal pipe).
//
// run() returns when there is no work left: no queued continuation, no continuation being resumed, no pending operation
// (timer wait, descriptor wait, signal wait) and no outstanding on_work_started() of an executor. Then the context is
// stopped, as by stop(); restart() makes it runnable again. Several threads may call run() concurrently; one of them at
// a time waits for I/O events, the others wait for posted continuations. The continuations are resumed in the order
// they were posted, except that concurrent threads of course run them concurrently.
//
// Differences from the shape of Asio's io_context (see the deviation notes of the networking series): run_for and
// run_until are not provided, and a continuation that is posted counts as work until it has been resumed (a bare post
// is therefore enough to keep run() going).
class io_context : public execution_context {
public:
  class executor_type;

private:
  mutable mutex __m_;
  condition_variable __cv_;
  continuation* __head_ = nullptr;
  continuation* __tail_ = nullptr;
  size_t __work_        = 0; // queued + being resumed + counted by on_work_started
  size_t __cv_waiters_  = 0;
  bool __polling_       = false; // a thread is inside the reactor's poll
  bool __stopped_       = false;
  __reactor_service* __reactor_;

  _LIBCPP_HIDE_FROM_ABI void __enqueue(continuation& __c) {
    __c.next = nullptr;
    bool __wake_poller;
    bool __wake_waiter;
    {
      lock_guard<mutex> __lk(__m_);
      if (__tail_)
        __tail_->next = &__c;
      else
        __head_ = &__c;
      __tail_ = &__c;
      ++__work_;
      __wake_poller = __polling_;
      __wake_waiter = !__polling_ && __cv_waiters_ != 0;
    }
    if (__wake_poller)
      __reactor_->__wakeup();
    else if (__wake_waiter)
      __cv_.notify_one();
  }

  _LIBCPP_HIDE_FROM_ABI void __work_started() noexcept {
    lock_guard<mutex> __lk(__m_);
    ++__work_;
  }

  _LIBCPP_HIDE_FROM_ABI void __work_finished() noexcept {
    lock_guard<mutex> __lk(__m_);
    if (--__work_ == 0 && __reactor_->__pending() == 0)
      __stop_locked();
  }

  // Precondition: __m_ is locked.
  _LIBCPP_HIDE_FROM_ABI void __stop_locked() noexcept {
    __stopped_ = true;
    __cv_.notify_all();
    if (__polling_)
      __reactor_->__wakeup();
  }

  // Resumes one continuation, or waits for I/O (if __block) and then resumes one. Returns the number resumed (0 or 1).
  // Precondition: __lk owns __m_.
  _LIBCPP_HIDE_FROM_ABI size_t __do_one(unique_lock<mutex>& __lk, bool __block) {
    bool __polled_without_blocking = false;
    while (true) {
      if (__stopped_)
        return 0;
      if (__head_) {
        continuation* __c = __head_;
        __head_           = __c->next;
        if (!__head_)
          __tail_ = nullptr;
        if (__head_ && __cv_waiters_ != 0)
          __cv_.notify_one();
        coroutine_handle<> __h = __c->h;
        __lk.unlock();
        const io_context* __saved = __current_io_context();
        __current_io_context()    = this;
        io::safe_resume(__h);
        __current_io_context() = __saved;
        __lk.lock();
        if (--__work_ == 0 && __reactor_->__pending() == 0)
          __stop_locked();
        return 1;
      }
      if (__work_ == 0 && __reactor_->__pending() == 0) {
        __stop_locked();
        return 0;
      }
      if (!__polling_) {
        if (!__block && __polled_without_blocking)
          return 0;
        __polling_ = true;
        __lk.unlock();
        __reactor_->__poll(__block);
        __lk.lock();
        __polling_ = false;
        if (__cv_waiters_ != 0)
          __cv_.notify_one(); // somebody has to take over the polling
        __polled_without_blocking = !__block;
        continue;
      }
      if (!__block)
        return 0;
      ++__cv_waiters_;
      __cv_.wait(__lk);
      --__cv_waiters_;
    }
  }

public:
  _LIBCPP_HIDE_FROM_ABI io_context() : __reactor_(&use_service<__reactor_service>()) {}

  io_context(const io_context&)            = delete;
  io_context& operator=(const io_context&) = delete;

  // Shuts the services down, abandons the queued continuations (a raw one is destroyed, not resumed; a chained one is
  // dropped) and destroys the chains that are still suspended (see execution_context::shutdown). The timers, signal
  // sets and other objects that were made from the context must be destroyed before it. Preconditions: no thread is
  // inside run(); no other thread posts concurrently.
  _LIBCPP_HIDE_FROM_ABI ~io_context() {
    __shutdown_services(); // joins the helper threads that post completions
    continuation* __c;
    {
      lock_guard<mutex> __lk(__m_);
      __c     = __head_;
      __head_ = nullptr;
      __tail_ = nullptr;
    }
    while (__c) {
      continuation* __next = __c->next;
      if (__c->h && !__c->__chained_) // a chained one belongs to its chain, see continuation
        __c->h.destroy();
      __c = __next;
    }
    shutdown();
    destroy();
  }

  _LIBCPP_HIDE_FROM_ABI executor_type get_executor() const noexcept;

  // Runs the event loop until the context is stopped or runs out of work; returns the number of continuations resumed.
  _LIBCPP_HIDE_FROM_ABI size_t run() {
    unique_lock<mutex> __lk(__m_);
    size_t __n = 0;
    while (__do_one(__lk, true))
      ++__n;
    return __n;
  }

  // Waits until one continuation has been resumed (or the context is stopped or out of work); returns 1 or 0.
  _LIBCPP_HIDE_FROM_ABI size_t run_one() {
    unique_lock<mutex> __lk(__m_);
    return __do_one(__lk, true);
  }

  // Resumes what is ready without waiting; returns the number of continuations resumed.
  _LIBCPP_HIDE_FROM_ABI size_t poll() {
    unique_lock<mutex> __lk(__m_);
    size_t __n = 0;
    while (__do_one(__lk, false))
      ++__n;
    return __n;
  }

  _LIBCPP_HIDE_FROM_ABI size_t poll_one() {
    unique_lock<mutex> __lk(__m_);
    return __do_one(__lk, false);
  }

  // Makes every run()/run_one()/poll() return as soon as its current continuation has been resumed. May be called from
  // any thread, including from inside a resumed coroutine.
  _LIBCPP_HIDE_FROM_ABI void stop() noexcept {
    lock_guard<mutex> __lk(__m_);
    __stop_locked();
  }

  // Makes the context runnable again after it was stopped. Precondition: no thread is inside run().
  _LIBCPP_HIDE_FROM_ABI void restart() noexcept {
    lock_guard<mutex> __lk(__m_);
    __stopped_ = false;
  }

  _LIBCPP_HIDE_FROM_ABI bool stopped() const noexcept {
    lock_guard<mutex> __lk(__m_);
    return __stopped_;
  }
};

class io_context::executor_type {
  friend class io_context;
  io_context* __ctx_ = nullptr;

  _LIBCPP_HIDE_FROM_ABI explicit executor_type(io_context& __c) noexcept : __ctx_(&__c) {}

public:
  _LIBCPP_HIDE_FROM_ABI executor_type() = default;

  _LIBCPP_HIDE_FROM_ABI friend bool operator==(const executor_type&, const executor_type&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI io_context& context() const noexcept { return *__ctx_; }
  _LIBCPP_HIDE_FROM_ABI void on_work_started() const noexcept { __ctx_->__work_started(); }
  _LIBCPP_HIDE_FROM_ABI void on_work_finished() const noexcept { __ctx_->__work_finished(); }

  // True if the calling thread is inside run(), run_one() or poll() of the context.
  _LIBCPP_HIDE_FROM_ABI bool running_in_this_thread() const noexcept { return __current_io_context() == __ctx_; }

  // Runs inline (by returning the handle for symmetric transfer) in a thread that runs the context, otherwise posts.
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> dispatch(continuation& __c) const {
    if (__current_io_context() == __ctx_)
      return __c.h;
    __ctx_->__enqueue(__c);
    return noop_coroutine();
  }
  _LIBCPP_HIDE_FROM_ABI void post(continuation& __c) const { __ctx_->__enqueue(__c); }
};

_LIBCPP_HIDE_FROM_ABI inline io_context::executor_type io_context::get_executor() const noexcept {
  return executor_type(const_cast<io_context&>(*this));
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_EPOLL

#endif // _LIBCPP___IO_CONTEXT_IO_CONTEXT_H
