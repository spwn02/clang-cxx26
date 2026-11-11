//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_CONTEXT_OP_H
#define _LIBCPP___IO_CONTEXT_OP_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__io/continuation.h>
#include <__io/executor_ref.h>
#include <__io/io_env.h>
#include <__stop_token/stop_callback.h>
#include <__system_error/error_code.h>
#include <optional>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// One pending asynchronous operation of the platform layer (a timer wait, a file descriptor wait, a signal wait). It is
// a base class of the awaiter, so it lives in the frame of the awaiting coroutine and costs no allocation. The
// continuation inside it is what the completion posts to the awaiting coroutine's own executor (taken from io_env), so
// the coroutine resumes where it expects to.
//
// An operation is in one of three states, protected by the lock of the object that owns the pending operation (the
// reactor, the signal service):
//   __idle    - being set up, not yet visible to anybody else;
//   __pending - queued at its owner. Whoever removes it from the queue (completion, cancellation, shutdown) wins and
//               moves it to __done, so an operation completes exactly once;
//   __done    - completed or abandoned. Its owner never touches it again.
// A stop request that arrives while the operation is __idle sets __cancel_requested_, which the owner checks when
// the operation is queued.
//
// An operation that is destroyed while it is still __pending (its frame is destroyed by another context than the one
// that owns the operation, see execution_context::shutdown) is unlinked by the destructor: __abandon_ asks the owner
// to remove it from its queues without completing it (the continuation is not posted), and to uncount it as work. An
// operation whose call is in progress on a thread (a file operation, a request of the ring) cannot be unlinked at
// once: it is marked __orphaned_, and the destructor waits for the call to end, after which the owner drops the
// result instead of completing it.
struct __op_base {
  enum __state_t : unsigned char { __idle, __pending, __done };

  continuation __cont_;
  executor_ref __ex_;
  error_code __ec_;
  __op_base* __prev_                      = nullptr;
  __op_base* __next_                      = nullptr;
  void (*__cancel_)(__op_base*) noexcept  = nullptr; // asks the owner to cancel this operation
  __state_t __state_                      = __idle;
  void (*__abandon_)(__op_base*) noexcept = nullptr; // unlinks this operation if it is pending, see above
  bool __cancel_requested_                = false;
  bool __orphaned_                        = false; // the destructor waits for the owner, which must not complete it

  struct __canceler {
    __op_base* __op_;
    _LIBCPP_HIDE_FROM_ABI void operator()() const noexcept { __op_->__cancel_(__op_); }
  };
  optional<stop_callback<__canceler>> __cb_;

  _LIBCPP_HIDE_FROM_ABI __op_base() noexcept = default;
  // An awaiter is moved only before it is started (when_any and friends take their awaitables by value).
  _LIBCPP_HIDE_FROM_ABI __op_base(__op_base&&) noexcept {}
  __op_base& operator=(__op_base&&) = delete;
  _LIBCPP_HIDE_FROM_ABI ~__op_base() {
    __cb_.reset(); // waits for a running stop callback, which may be calling into the owner
    // Unsynchronized read: an operation that is still pending here is not being completed (the precondition of
    // destroying a chain that is waiting in a context other than the one that destroys it); the owner checks again
    // under its lock.
    if (__abandon_ && __state_ == __pending)
      __abandon_(this);
  }

  // A derived operation sets __cancel_ in its constructors.
  _LIBCPP_HIDE_FROM_ABI void __bind(coroutine_handle<> __h, const io_env* __env) noexcept {
    __cont_.h          = __h;
    __cont_.__chained_ = true; // the awaiting coroutine belongs to a chain
    __ex_              = __env->executor;
  }

  // Connects the stop token. Must be called before the operation is queued, because the callback may run at any time
  // from then on, in any thread. The callback is destroyed by await_resume (__finish), which blocks until a running
  // callback has returned.
  _LIBCPP_HIDE_FROM_ABI void __arm_stop(const io_env* __env) { __cb_.emplace(__env->stop_token, __canceler{this}); }
  _LIBCPP_HIDE_FROM_ABI void __finish() noexcept { __cb_.reset(); }
};

// An intrusive doubly-linked FIFO of operations. Not thread-safe; the owner's lock protects it.
class __op_list {
  __op_base* __head_ = nullptr;
  __op_base* __tail_ = nullptr;

public:
  _LIBCPP_HIDE_FROM_ABI bool __empty() const noexcept { return __head_ == nullptr; }
  _LIBCPP_HIDE_FROM_ABI __op_base* __front() const noexcept { return __head_; }

  _LIBCPP_HIDE_FROM_ABI void __push_back(__op_base* __op) noexcept {
    __op->__next_ = nullptr;
    __op->__prev_ = __tail_;
    if (__tail_)
      __tail_->__next_ = __op;
    else
      __head_ = __op;
    __tail_ = __op;
  }

  _LIBCPP_HIDE_FROM_ABI void __remove(__op_base* __op) noexcept {
    if (__op->__prev_)
      __op->__prev_->__next_ = __op->__next_;
    else
      __head_ = __op->__next_;
    if (__op->__next_)
      __op->__next_->__prev_ = __op->__prev_;
    else
      __tail_ = __op->__prev_;
    __op->__prev_ = nullptr;
    __op->__next_ = nullptr;
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_CONTEXT_OP_H
