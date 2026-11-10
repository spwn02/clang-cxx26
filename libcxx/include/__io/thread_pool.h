//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_THREAD_POOL_H
#define _LIBCPP___IO_THREAD_POOL_H

#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/frame_allocator.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>
#include <__thread/thread.h>
#include <__utility/move.h>
#include <__vector/vector.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

class thread_pool;

// The pool that the current thread is a worker of, if any.
_LIBCPP_HIDE_FROM_ABI inline const thread_pool*& __current_thread_pool() noexcept {
  static thread_local const thread_pool* __p = nullptr;
  return __p;
}

// P4100R1 [8.2]: a multi-threaded ExecutionContext. Work items share one queue. Worker threads start lazily, on the
// first post. join() waits for the outstanding work counted by the executors' on_work_started/on_work_finished (a bare
// post is not counted), then joins the workers. stop() makes the workers exit after their current item; whatever is
// still queued is destroyed (not resumed) when the pool is destroyed.
class thread_pool : public execution_context {
public:
  class executor_type;

private:
  mutex __mtx_;
  condition_variable __work_cv_;
  condition_variable __join_cv_;
  continuation* __head_ = nullptr;
  continuation* __tail_ = nullptr;
  size_t __outstanding_ = 0;
  size_t __num_threads_;
  bool __started_ = false;
  bool __stop_    = false;
  bool __joined_  = false;
  vector<thread> __threads_;

  _LIBCPP_HIDE_FROM_ABI void __worker() {
    __current_thread_pool() = this;
    unique_lock<mutex> __lk(__mtx_);
    while (true) {
      __work_cv_.wait(__lk, [this] { return __stop_ || __head_ != nullptr; });
      if (__stop_)
        break;
      continuation* __c = __head_;
      __head_           = __c->next;
      if (!__head_)
        __tail_ = nullptr;
      coroutine_handle<> __h = __c->h;
      __lk.unlock();
      io::safe_resume(__h);
      __lk.lock();
    }
    __current_thread_pool() = nullptr;
  }

  // Precondition: __mtx_ is locked.
  _LIBCPP_HIDE_FROM_ABI void __start_locked() {
    if (__started_ || __stop_)
      return;
    __started_ = true;
    __threads_.reserve(__num_threads_);
    for (size_t __i = 0; __i != __num_threads_; ++__i)
      __threads_.emplace_back([this] { __worker(); });
  }

  _LIBCPP_HIDE_FROM_ABI void __post(continuation& __c) {
    __c.next = nullptr;
    {
      unique_lock<mutex> __lk(__mtx_);
      if (__tail_)
        __tail_->next = &__c;
      else
        __head_ = &__c;
      __tail_ = &__c;
      __start_locked();
    }
    __work_cv_.notify_one();
  }

  _LIBCPP_HIDE_FROM_ABI void __work_started() noexcept {
    unique_lock<mutex> __lk(__mtx_);
    ++__outstanding_;
  }

  _LIBCPP_HIDE_FROM_ABI void __work_finished() noexcept {
    bool __zero;
    {
      unique_lock<mutex> __lk(__mtx_);
      __zero = --__outstanding_ == 0;
    }
    if (__zero)
      __join_cv_.notify_all();
  }

  _LIBCPP_HIDE_FROM_ABI void __join_threads() noexcept {
    vector<thread> __ts;
    {
      unique_lock<mutex> __lk(__mtx_);
      __ts = std::move(__threads_);
    }
    for (thread& __t : __ts)
      if (__t.joinable())
        __t.join();
  }

public:
  // Zero selects the hardware concurrency, or one when that is unknown.
  _LIBCPP_HIDE_FROM_ABI explicit thread_pool(size_t __num_threads = 0)
      : __num_threads_(__num_threads != 0 ? __num_threads
                                          : (thread::hardware_concurrency() != 0 ? thread::hardware_concurrency() : 1)) {}

  thread_pool(const thread_pool&)            = delete;
  thread_pool& operator=(const thread_pool&) = delete;

  // Stops the pool, joins the workers (without waiting for outstanding work), abandons the queued work and shuts the
  // services down. Preconditions: not called from a worker thread; no other thread posts concurrently.
  _LIBCPP_HIDE_FROM_ABI ~thread_pool() {
    stop();
    __join_threads();
    continuation* __c = __head_;
    __head_           = nullptr;
    __tail_           = nullptr;
    while (__c) {
      continuation* __next = __c->next;
      if (__c->h)
        __c->h.destroy();
      __c = __next;
    }
    shutdown();
    destroy();
  }

  // Blocks until all counted outstanding work is finished (or stop() is called), then joins the workers. Only the first
  // call joins. Precondition: not called from a worker thread of this pool.
  _LIBCPP_HIDE_FROM_ABI void join() noexcept {
    {
      unique_lock<mutex> __lk(__mtx_);
      if (__joined_)
        return;
      __joined_ = true;
      __join_cv_.wait(__lk, [this] { return __outstanding_ == 0 || __stop_; });
      __stop_ = true;
    }
    __work_cv_.notify_all();
    __join_threads();
  }

  // Makes the workers exit after their current item. May be called from any thread, including a worker.
  _LIBCPP_HIDE_FROM_ABI void stop() noexcept {
    {
      unique_lock<mutex> __lk(__mtx_);
      __stop_ = true;
    }
    __work_cv_.notify_all();
    __join_cv_.notify_all();
  }

  _LIBCPP_HIDE_FROM_ABI executor_type get_executor() const noexcept;
};

class thread_pool::executor_type {
  friend class thread_pool;
  thread_pool* __pool_ = nullptr;

  _LIBCPP_HIDE_FROM_ABI explicit executor_type(thread_pool& __p) noexcept : __pool_(&__p) {}

public:
  _LIBCPP_HIDE_FROM_ABI executor_type() = default;

  _LIBCPP_HIDE_FROM_ABI friend bool operator==(const executor_type&, const executor_type&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI thread_pool& context() const noexcept { return *__pool_; }
  _LIBCPP_HIDE_FROM_ABI void on_work_started() const noexcept { __pool_->__work_started(); }
  _LIBCPP_HIDE_FROM_ABI void on_work_finished() const noexcept { __pool_->__work_finished(); }

  // Runs inline (by returning the handle for symmetric transfer) on a worker of this pool, otherwise posts.
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> dispatch(continuation& __c) const {
    if (__current_thread_pool() == __pool_)
      return __c.h;
    __pool_->__post(__c);
    return noop_coroutine();
  }
  _LIBCPP_HIDE_FROM_ABI void post(continuation& __c) const { __pool_->__post(__c); }
};

_LIBCPP_HIDE_FROM_ABI inline thread_pool::executor_type thread_pool::get_executor() const noexcept {
  return executor_type(const_cast<thread_pool&>(*this));
}

// The process-wide pool, created on first use and destroyed at program exit. The function is header-only, so each
// program image that includes this header has its own (see the deviation note in the issue tracker).
_LIBCPP_HIDE_FROM_ABI inline thread_pool& system_context() {
  static thread_pool __pool;
  return __pool;
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_THREAD_POOL_H
