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

#if _LIBCPP_HAS_IO_EPOLL

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
struct __blocking_op : __op_base {
  void (*__work_)(__blocking_op*) noexcept = nullptr;
  __blocking_service* __svc_;
  const void* __owner_ = nullptr; // the object whose cancel() reaches this operation
  bool __running_      = false;

  _LIBCPP_HIDE_FROM_ABI static void __cancel_cb(__op_base* __b) noexcept;

  _LIBCPP_HIDE_FROM_ABI __blocking_op(__blocking_service* __s, const void* __owner) noexcept
      : __svc_(__s), __owner_(__owner) {
    __cancel_ = &__cancel_cb;
  }
  // An awaiter is moved only before it is started.
  // NOLINTBEGIN(bugprone-use-after-move): only the __op_base subobject is moved from
  _LIBCPP_HIDE_FROM_ABI __blocking_op(__blocking_op&& __o) noexcept
      : __op_base(std::move(__o)), __work_(__o.__work_), __svc_(__o.__svc_), __owner_(__o.__owner_) {
    __cancel_ = &__cancel_cb;
  }
  // NOLINTEND(bugprone-use-after-move)

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI inline bool await_suspend(coroutine_handle<> __h, const io_env* __env);
};

// The service of an execution_context that runs blocking calls on a small pool of helper threads: the mechanism of the
// resolver (see <net>) for the operations that cannot be waited for with the reactor. The threads are started lazily
// (one more per request, up to __max_threads, when no thread is idle) and joined by shutdown(). Completions are posted
// to the executor of the awaiting coroutine through the reactor of the context. Shutting down the context abandons the
// requests that wait for a thread and those that have completed after the shutdown (their coroutines are destroyed,
// like those of the reactor), and waits for the calls in progress.
_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __blocking_service final : public execution_context::service {
  static constexpr size_t __max_threads = 4;

  __reactor_service* __reactor_;
  mutex __m_;
  condition_variable __cv_;
  __op_list __queue_;   // the requests that wait for a thread
  size_t __queued_ = 0; // their number
  vector<thread> __threads_;
  vector<coroutine_handle<>> __orphans_; // coroutines of calls that finished after the shutdown
  size_t __idle_    = 0;
  bool __shut_down_ = false;

  _LIBCPP_HIDE_FROM_ABI void __worker() {
    unique_lock<mutex> __lk(__m_);
    for (;;) {
      ++__idle_;
      __cv_.wait(__lk, [this] { return __shut_down_ || __queued_ != 0; });
      --__idle_;
      if (__shut_down_)
        return;
      auto* __op = static_cast<__blocking_op*>(__queue_.__front());
      __queue_.__remove(__op);
      --__queued_;
      __op->__running_ = true;
      __lk.unlock();
      __op->__work_(__op);
      __lk.lock();
      __op->__state_ = __op_base::__done;
      if (__shut_down_) {
        __orphans_.push_back(__op->__cont_.h);
      } else {
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
    vector<coroutine_handle<>> __handles;
    vector<thread> __threads;
    {
      lock_guard<mutex> __lk(__m_);
      __shut_down_ = true;
      while (__op_base* __op = __queue_.__front()) {
        __queue_.__remove(__op);
        __op->__state_ = __op_base::__done;
        __handles.push_back(__op->__cont_.h);
      }
      __queued_ = 0;
      __threads = std::move(__threads_);
    }
    __cv_.notify_all();
    for (thread& __t : __threads)
      if (__t.joinable())
        __t.join();
    {
      lock_guard<mutex> __lk(__m_);
      for (coroutine_handle<> __h : __orphans_)
        __handles.push_back(__h);
      __orphans_.clear();
    }
    for (coroutine_handle<> __h : __handles)
      if (__h)
        __h.destroy();
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
      // Another thread when every thread has work and the pool is not full.
      if (__idle_ < __queued_ + 1 && __threads_.size() < __max_threads) {
#  if _LIBCPP_HAS_EXCEPTIONS
        try {
          __threads_.emplace_back([this] { __worker(); });
        } catch (...) {
          if (__threads_.empty()) {
            __op.__ec_    = make_error_code(errc::resource_unavailable_try_again);
            __op.__state_ = __op_base::__done;
            return false;
          }
        }
#  else
        __threads_.emplace_back([this] { __worker(); });
#  endif
      }
      __queue_.__push_back(std::addressof(__op));
      ++__queued_;
      __op.__state_ = __op_base::__pending;
      __reactor_->__op_started();
    }
    __cv_.notify_one();
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
      if (__op->__state_ == __op_base::__done || __op->__running_)
        return;
      __finish_canceled_locked(__op);
    }
    __reactor_->__complete(__op);
  }

  // Cancels every operation that waits for a thread and was started through __owner (cancel() and close()).
  _LIBCPP_HIDE_FROM_ABI void __cancel_owner(const void* __owner) noexcept {
    vector<__blocking_op*> __ops;
    {
      lock_guard<mutex> __lk(__m_);
      for (__op_base* __b = __queue_.__front(); __b;) {
        auto* __op = static_cast<__blocking_op*>(__b);
        __b        = __b->__next_;
        if (__op->__owner_ == __owner) {
          __finish_canceled_locked(__op);
          __ops.push_back(__op);
        }
      }
    }
    for (__blocking_op* __op : __ops)
      __reactor_->__complete(__op);
  }

private:
  _LIBCPP_HIDE_FROM_ABI void __finish_canceled_locked(__blocking_op* __op) noexcept {
    __queue_.__remove(__op);
    --__queued_;
    __op->__state_ = __op_base::__done;
    __op->__ec_    = make_error_code(error::canceled);
  }
};
_LIBCPP_DIAGNOSTIC_POP

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

#endif // _LIBCPP_HAS_IO_EPOLL

#endif // _LIBCPP___IO_CONTEXT_BLOCKING_SERVICE_H
