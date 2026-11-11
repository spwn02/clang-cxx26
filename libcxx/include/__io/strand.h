//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_STRAND_H
#define _LIBCPP___IO_STRAND_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/coroutine_traits.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__exception/terminate.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/frame_allocator.h>
#include <__memory/shared_ptr.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

template <class _Ex>
class strand;

// The state that all copies of a strand share: the queue of waiting continuations and the flag that says a drain
// coroutine is scheduled or running. Whoever finds the flag clear after enqueueing becomes responsible for scheduling
// the drain.
struct __strand_impl {
  mutex __mtx_;
  continuation* __head_ = nullptr;
  continuation* __tail_ = nullptr;
  bool __busy_          = false;
  continuation __drain_cont_;

  _LIBCPP_HIDE_FROM_ABI __strand_impl()          = default;
  __strand_impl(const __strand_impl&)            = delete;
  __strand_impl& operator=(const __strand_impl&) = delete;

  // Continuations still queued when the last owner goes away are abandoned, like the ones of a stopped thread_pool: a
  // raw one is destroyed, a chained one belongs to its chain (see continuation). The queue is walked through the
  // continuations, which live in frames; those must still be alive, which is why a drain that is destroyed without
  // having finished clears the queue first (__drain_guard).
  _LIBCPP_HIDE_FROM_ABI ~__strand_impl() { __abandon_queue(); }

  _LIBCPP_HIDE_FROM_ABI void __abandon_queue() noexcept {
    continuation* __c = __head_;
    __head_           = nullptr;
    __tail_           = nullptr;
    while (__c) {
      continuation* __next = __c->next;
      if (__c->h && !__c->__chained_)
        __c->h.destroy();
      __c = __next;
    }
  }

  // Takes the chained continuations out of the queue (their chains are destroyed by the context, which may free them);
  // the raw ones stay until the state is destroyed.
  _LIBCPP_HIDE_FROM_ABI void __unlink_chained() noexcept {
    continuation* __c = __head_;
    __head_           = nullptr;
    __tail_           = nullptr;
    while (__c) {
      continuation* __next = __c->next;
      if (!__c->__chained_) {
        __c->next = nullptr;
        if (__tail_)
          __tail_->next = __c;
        else
          __head_ = __c;
        __tail_ = __c;
      }
      __c = __next;
    }
  }

  // Precondition: __mtx_ is locked.
  _LIBCPP_HIDE_FROM_ABI void __enqueue_locked(continuation& __c) noexcept {
    __c.next = nullptr;
    if (__tail_)
      __tail_->next = &__c;
    else
      __head_ = &__c;
    __tail_ = &__c;
  }
};

// The strand whose continuation the current thread is resuming, if any.
_LIBCPP_HIDE_FROM_ABI inline const __strand_impl*& __current_strand() noexcept {
  static thread_local const __strand_impl* __p = nullptr;
  return __p;
}

// The coroutine that runs the continuations of a strand, one after the other, until the queue is empty. It owns a
// reference to the shared state, so the state outlives the drain. Its frame is destroyed when it finishes (or by the
// inner executor, if that abandons it).
struct __strand_drain {
  struct promise_type {
    execution_context::__frame_node __node_; // registered with the context of the inner executor, until it finishes
    __strand_impl* __impl_;
    bool __finished_ = false;

    _LIBCPP_HIDE_FROM_ABI explicit promise_type(const shared_ptr<__strand_impl>& __i) noexcept : __impl_(__i.get()) {}
    promise_type(const promise_type&)            = delete;
    promise_type& operator=(const promise_type&) = delete;
    // A drain that is destroyed before the queue ran empty (it never started, or its context was shut down) takes the
    // chained continuations out of the queue right away, while they are still alive: the strand state may outlive the
    // context (copies of the strand), and then its destructor must not walk through freed frames. The promise is
    // destroyed with the frame, also when the body never ran.
    _LIBCPP_HIDE_FROM_ABI ~promise_type() {
      if (!__finished_)
        __impl_->__unlink_chained();
    }
    _LIBCPP_HIDE_FROM_ABI __strand_drain get_return_object() noexcept {
      return __strand_drain{coroutine_handle<promise_type>::from_promise(*this)};
    }
    _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI suspend_never final_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI void return_void() noexcept {}
    _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept { std::terminate(); }
  };

  coroutine_handle<promise_type> __h_;
};

struct __strand_drain_finished {
  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
  _LIBCPP_HIDE_FROM_ABI bool await_suspend(coroutine_handle<__strand_drain::promise_type> __h) const noexcept {
    __h.promise().__finished_ = true;
    return false;
  }
  _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
};

_LIBCPP_HIDE_FROM_ABI inline __strand_drain __strand_run_queue(shared_ptr<__strand_impl> __impl) {
  while (true) {
    coroutine_handle<> __h;
    {
      unique_lock<mutex> __lk(__impl->__mtx_);
      continuation* __c = __impl->__head_;
      if (!__c) {
        __impl->__busy_ = false;
        break;
      }
      __impl->__head_ = __c->next;
      if (!__impl->__head_)
        __impl->__tail_ = nullptr;
      __h = __c->h;
    }
    const __strand_impl*& __cur = io::__current_strand();
    const __strand_impl* __prev = __cur;
    __cur                       = __impl.get();
    io::safe_resume(__h);
    io::__current_strand() = __prev;
  }
  co_await __strand_drain_finished{};
}

// A strand wraps an inner Executor and serializes the resumption of the continuations that go through it: the
// continuations of one strand (and of every copy of it) never run concurrently, and they run in the order they were
// queued. The provisional specification is Capy's strand (no WG21 paper exists yet).
template <class _Ex>
class strand {
  static_assert(Executor<_Ex>, "io::strand<Ex> requires Ex to be an Executor");

  shared_ptr<__strand_impl> __impl_;
  _Ex __ex_;

  template <class>
  friend class strand;

public:
  using inner_executor_type = _Ex;

  // A fresh strand: it serializes with its own copies only.
  _LIBCPP_HIDE_FROM_ABI explicit strand(_Ex __ex)
      : __impl_(std::make_shared<__strand_impl>()), __ex_(std::move(__ex)) {}

  _LIBCPP_HIDE_FROM_ABI strand(const strand&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI strand(strand&&) noexcept                 = default;
  _LIBCPP_HIDE_FROM_ABI strand& operator=(const strand&) noexcept = default;
  _LIBCPP_HIDE_FROM_ABI strand& operator=(strand&&) noexcept      = default;

  _LIBCPP_HIDE_FROM_ABI const _Ex& get_inner_executor() const noexcept { return __ex_; }

  _LIBCPP_HIDE_FROM_ABI decltype(auto) context() const noexcept { return __ex_.context(); }
  _LIBCPP_HIDE_FROM_ABI void on_work_started() const noexcept { __ex_.on_work_started(); }
  _LIBCPP_HIDE_FROM_ABI void on_work_finished() const noexcept { __ex_.on_work_finished(); }

  // True when the calling thread is resuming a continuation of this strand.
  _LIBCPP_HIDE_FROM_ABI bool running_in_this_thread() const noexcept { return io::__current_strand() == __impl_.get(); }

  // Two strands are equal when they share their serialization state.
  _LIBCPP_HIDE_FROM_ABI friend bool operator==(const strand& __a, const strand& __b) noexcept {
    return __a.__impl_ == __b.__impl_;
  }

  // Always queues; the continuations of the strand run in FIFO order on the inner executor.
  _LIBCPP_HIDE_FROM_ABI void post(continuation& __c) const {
    bool __schedule;
    {
      unique_lock<mutex> __lk(__impl_->__mtx_);
      __impl_->__enqueue_locked(__c);
      __schedule       = !__impl_->__busy_;
      __impl_->__busy_ = true;
    }
    if (__schedule) {
      __strand_drain __drain            = io::__strand_run_queue(__impl_);
      __impl_->__drain_cont_.h          = __drain.__h_;
      __impl_->__drain_cont_.__chained_ = true; // the drain is destroyed by its context, or ends itself
      __drain.__h_.promise().__node_.__register(__ex_.context(), __drain.__h_, /*__first=*/true);
      __ex_.post(__impl_->__drain_cont_);
    }
  }

  // Runs inline (by returning the handle for symmetric transfer) when the calling thread is already resuming a
  // continuation of this strand, otherwise queues the continuation and returns noop_coroutine().
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> dispatch(continuation& __c) const {
    if (running_in_this_thread())
      return __c.h;
    post(__c);
    return noop_coroutine();
  }
};

template <class _Ex>
strand(_Ex) -> strand<_Ex>;

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STRAND_H
