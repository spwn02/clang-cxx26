//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_CONTEXT_BLOCKING_SERVICE_H
#define _LIBCPP___IO_CONTEXT_BLOCKING_SERVICE_H

#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/execution_context.h>
#include <__io/io_env.h>
#include <__io/support/config.h>
#include <__io_context/op.h>
#include <__io_context/reactor.h>
#include <__io_stream/error.h>
#include <__memory/addressof.h>
#include <__memory/shared_ptr.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__thread/thread.h>
#include <__utility/move.h>
#include <__vector/vector.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_REACTOR

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

class __blocking_service;

// An operation that is a call which blocks (a file operation): the base of the awaiters of such operations. It lives in
// the frame of the awaiting coroutine. __work_ is called on a helper thread of the __blocking_service and stores its
// result in the operation (the base class has __ec_; a derived class adds its own members). The operation counts as
// work of the io_context until it completes.
//
// Cancellation: an operation that waits for a thread is cancelled at once (it completes with error::canceled and
// __work_ is never called). An operation whose call is in progress is not cancelled: the call cannot be interrupted,
// and the buffers it uses belong to the awaiting coroutine, which is not resumed before the call has returned. It
// completes with the result of the call.
//
// A detachable operation (the lookups of the resolver) is the exception: it does not use the memory of the awaiting
// coroutine. Instead of __work_ it has __detached_work_, which is called with __data_ (a shared state that the
// service keeps alive until the call has returned) and never touches the operation. Such an operation is also
// cancelled while its call is in progress: it completes at once with error::canceled, and the helper thread finishes
// the call and discards the result.
struct __blocking_op : __op_base {
  void (*__work_)(__blocking_op*) noexcept = nullptr;
  void (*__detached_work_)(void*) noexcept = nullptr;
  shared_ptr<void> __data_;
  __blocking_service* __svc_;
  const void* __owner_ = nullptr; // the object whose cancel() reaches this operation
  bool __running_      = false;

  _LIBCPP_HIDE_FROM_ABI static void __cancel_cb(__op_base* __b) noexcept;
  _LIBCPP_HIDE_FROM_ABI static void __abandon_cb(__op_base* __b) noexcept;

  _LIBCPP_HIDE_FROM_ABI __blocking_op(__blocking_service* __s, const void* __owner) noexcept
      : __svc_(__s), __owner_(__owner) {
    __cancel_  = &__cancel_cb;
    __abandon_ = &__abandon_cb;
  }
  // An awaiter is moved only before it is started.
  // NOLINTBEGIN(bugprone-use-after-move): only the __op_base subobject is moved from
  _LIBCPP_HIDE_FROM_ABI __blocking_op(__blocking_op&& __o) noexcept
      : __op_base(std::move(__o)),
        __work_(__o.__work_),
        __detached_work_(__o.__detached_work_),
        __data_(std::move(__o.__data_)),
        __svc_(__o.__svc_),
        __owner_(__o.__owner_) {
    __cancel_  = &__cancel_cb;
    __abandon_ = &__abandon_cb;
  }
  // NOLINTEND(bugprone-use-after-move)

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI inline bool await_suspend(coroutine_handle<> __h, const io_env* __env);
};

// The service of an execution_context that runs blocking calls on a small pool of helper threads: the mechanism of the
// file operations (see <io_file>) and of the resolver (see <net>), the operations that cannot be waited for with the
// reactor. The two share the threads of the context. The threads are started lazily
// (one more per request, up to __max_threads, when no thread is idle) and joined by shutdown(). Completions are posted
// to the executor of the awaiting coroutine through the reactor of the context, which counts the request as work.
// The two users do not share threads, though: the file operations (which can block for as long as a FIFO or a device
// has nothing to deliver) have a pool of their own, the lookups of the resolver another, so that blocked file reads
// cannot starve the lookups.
// Shutting down the context abandons the requests that wait for a thread and those that have completed after the
// shutdown (their coroutines are dropped, not resumed; the context destroys their chains, see
// execution_context::shutdown), and waits for the calls in progress.
_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __blocking_service final : public execution_context::service {
  static constexpr size_t __max_threads = 4; // per lane
  static constexpr size_t __lanes       = 2; // 0: the file operations, 1: the detachable operations (the resolver)

  static _LIBCPP_HIDE_FROM_ABI size_t __lane_of(const __blocking_op& __op) noexcept;

  __reactor_service* __reactor_;
  mutex __m_;
  condition_variable __cv_[__lanes];
  condition_variable __abandon_cv_; // an abandoning thread waits for a call in progress
  __op_list __queue_[__lanes];      // the requests that wait for a thread
  size_t __queued_[__lanes] = {};   // their number
  vector<thread> __threads_[__lanes];
  __blocking_op* __running_[__lanes][__max_threads] = {}; // the detachable operation that each thread runs, if any
  size_t __idle_[__lanes]                           = {};
  bool __shut_down_                                 = false;

  _LIBCPP_HIDE_FROM_ABI void __worker(size_t __lane, size_t __index) {
    unique_lock<mutex> __lk(__m_);
    for (;;) {
      ++__idle_[__lane];
      __cv_[__lane].wait(__lk, [this, __lane] { return __shut_down_ || __queued_[__lane] != 0; });
      --__idle_[__lane];
      if (__shut_down_)
        return;
      auto* __op = static_cast<__blocking_op*>(__queue_[__lane].__front());
      __queue_[__lane].__remove(__op);
      --__queued_[__lane];
      __op->__running_ = true;
      if (__op->__detached_work_) {
        // The operation may be cancelled (and its coroutine resumed, even destroyed) while the call is in progress,
        // so the call only uses the shared data, and the operation is not touched until the call has returned and
        // __running_ still names it (a cancellation clears the slot).
        void (*__fn)(void*) noexcept = __op->__detached_work_;
        shared_ptr<void> __data      = __op->__data_;
        __running_[__lane][__index]  = __op;
        __lk.unlock();
        __fn(__data.get());
        __data.reset();
        __lk.lock();
        if (__running_[__lane][__index] != __op) // detached: it has been completed, the result is dropped
          continue;
        __running_[__lane][__index] = nullptr;
      } else {
        __lk.unlock();
        __op->__work_(__op);
        __lk.lock();
      }
      const bool __orphan = __op->__orphaned_; // its awaiter is being destroyed and waits for the call to end
      __op->__state_      = __op_base::__done;
      if (__orphan) {
        __lk.unlock();
        __reactor_->__op_abandoned(); // the result is dropped; the operation is not touched again
        __abandon_cv_.notify_all();
        __lk.lock();
        continue;
      }
      if (!__shut_down_) { // after the shutdown the result is dropped, like the other pending operations
        __lk.unlock();
        __reactor_->__complete(__op);
        __lk.lock();
      }
    }
  }

public:
  _LIBCPP_HIDE_FROM_ABI explicit __blocking_service(execution_context& __ctx)
      : __reactor_(&__ctx.use_service<__reactor_service>()) {}

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL void shutdown() override {
    vector<thread> __threads;
    {
      lock_guard<mutex> __lk(__m_);
      __shut_down_ = true;
      for (size_t __l = 0; __l != __lanes; ++__l) {
        while (__op_base* __op = __queue_[__l].__front()) {
          __queue_[__l].__remove(__op);
          __op->__state_ = __op_base::__done;
        }
        __queued_[__l] = 0;
        for (thread& __t : __threads_[__l])
          __threads.push_back(std::move(__t));
        __threads_[__l].clear();
      }
    }
    for (condition_variable& __cv : __cv_)
      __cv.notify_all();
    for (thread& __t : __threads)
      if (__t.joinable())
        __t.join();
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL ~__blocking_service() override { shutdown(); }

  // Queues the operation. Returns false if it completed at once (the context is shut down, a thread could not be
  // started, or the operation was cancelled before it was queued): the operation has its error then.
  _LIBCPP_HIDE_FROM_ABI bool __start(__blocking_op& __op) {
    {
      lock_guard<mutex> __lk(__m_);
      if (__shut_down_ || __op.__cancel_requested_) {
        __op.__ec_    = make_error_code(error::canceled);
        __op.__state_ = __op_base::__done;
        return false;
      }
      const size_t __lane = __lane_of(__op);
      // Another thread when every thread of the lane has work and the pool is not full.
      if (__idle_[__lane] < __queued_[__lane] + 1 && __threads_[__lane].size() < __max_threads) {
#  if _LIBCPP_HAS_EXCEPTIONS
        try {
          __threads_[__lane].emplace_back([this, __lane, __i = __threads_[__lane].size()] { __worker(__lane, __i); });
        } catch (...) {
          if (__threads_[__lane].empty()) {
            __op.__ec_    = make_error_code(errc::resource_unavailable_try_again);
            __op.__state_ = __op_base::__done;
            return false;
          }
        }
#  else
        __threads_[__lane].emplace_back([this, __lane, __i = __threads_[__lane].size()] { __worker(__lane, __i); });
#  endif
      }
      __queue_[__lane].__push_back(std::addressof(__op));
      ++__queued_[__lane];
      __op.__state_ = __op_base::__pending;
      __reactor_->__op_started();
    }
    __cv_[__lane_of(__op)].notify_one();
    return true;
  }

  // Cancels an operation that waits for a thread (the stop callback).
  _LIBCPP_HIDE_FROM_ABI void __cancel_op(__blocking_op* __op) noexcept {
    {
      lock_guard<mutex> __lk(__m_);
      if (__op->__state_ == __op_base::__idle) {
        __op->__cancel_requested_ = true;
        return;
      }
      if (__op->__state_ == __op_base::__done)
        return;
      if (__op->__running_) {
        if (!__detach_locked(__op))
          return;
      } else {
        __finish_canceled_locked(__op);
      }
    }
    __reactor_->__complete(__op);
  }

  // The awaiter of a pending operation is destroyed (see __op_base::__abandon_). An operation that waits for a thread,
  // or a detachable one, is dropped at once; one whose call is in progress and uses the memory of the awaiter is waited
  // for (the helper thread then drops the result).
  _LIBCPP_HIDE_FROM_ABI void __abandon_op(__blocking_op* __op) noexcept {
    unique_lock<mutex> __lk(__m_);
    if (__op->__state_ != __op_base::__pending)
      return;
    if (!__op->__running_) {
      __finish_canceled_locked(__op);
    } else if (!__detach_locked(__op)) {
      __op->__orphaned_ = true;
      __abandon_cv_.wait(__lk, [__op] { return __op->__state_ != __op_base::__pending; });
      return; // the helper thread has uncounted it
    }
    __lk.unlock();
    __reactor_->__op_abandoned();
  }

  // Cancels every operation that waits for a thread and was started through __owner (cancel() and close()).
  _LIBCPP_HIDE_FROM_ABI void __cancel_owner(const void* __owner) noexcept {
    vector<__blocking_op*> __ops;
    {
      lock_guard<mutex> __lk(__m_);
      for (size_t __l = 0; __l != __lanes; ++__l) {
        for (__op_base* __b = __queue_[__l].__front(); __b;) {
          auto* __op = static_cast<__blocking_op*>(__b);
          __b        = __b->__next_;
          if (__op->__owner_ == __owner) {
            __finish_canceled_locked(__op);
            __ops.push_back(__op);
          }
        }
        for (__blocking_op*& __slot : __running_[__l]) {
          if (__slot && __slot->__owner_ == __owner) {
            __blocking_op* __op = __slot;
            if (__detach_locked(__op))
              __ops.push_back(__op);
          }
        }
      }
    }
    for (__blocking_op* __op : __ops)
      __reactor_->__complete(__op);
  }

private:
  // Completes an operation whose call is in progress, if it is detachable: the thread that runs it sees that its slot
  // is cleared when the call returns and drops the result. Returns false if the operation cannot be detached.
  _LIBCPP_HIDE_FROM_ABI bool __detach_locked(__blocking_op* __op) noexcept {
    if (!__op->__detached_work_)
      return false;
    for (__blocking_op*& __slot : __running_[__lane_of(*__op)]) {
      if (__slot == __op) {
        __slot = nullptr;
        break;
      }
    }
    __op->__state_ = __op_base::__done;
    __op->__ec_    = make_error_code(error::canceled);
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI void __finish_canceled_locked(__blocking_op* __op) noexcept {
    const size_t __lane = __lane_of(*__op);
    __queue_[__lane].__remove(__op);
    --__queued_[__lane];
    __op->__state_ = __op_base::__done;
    __op->__ec_    = make_error_code(error::canceled);
  }
};
_LIBCPP_DIAGNOSTIC_POP

_LIBCPP_HIDE_FROM_ABI inline size_t __blocking_service::__lane_of(const __blocking_op& __op) noexcept {
  return __op.__detached_work_ ? 1 : 0;
}

_LIBCPP_HIDE_FROM_ABI inline void __blocking_op::__abandon_cb(__op_base* __b) noexcept {
  auto* __self = static_cast<__blocking_op*>(__b);
  __self->__svc_->__abandon_op(__self);
}

_LIBCPP_HIDE_FROM_ABI inline void __blocking_op::__cancel_cb(__op_base* __b) noexcept {
  auto* __self = static_cast<__blocking_op*>(__b);
  __self->__svc_->__cancel_op(__self);
}

_LIBCPP_HIDE_FROM_ABI inline bool __blocking_op::await_suspend(coroutine_handle<> __h, const io_env* __env) {
  __bind(__h, __env);
  if (__env->stop_token.stop_requested()) {
    __ec_ = make_error_code(error::canceled);
    return false;
  }
  __arm_stop(__env);
  return __svc_->__start(*this);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_REACTOR

#endif // _LIBCPP___IO_CONTEXT_BLOCKING_SERVICE_H
