//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_PARALLEL_SCHEDULER_H
#define _LIBCPP___EXECUTION_PARALLEL_SCHEDULER_H

#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_forward_progress_guarantee.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <thread>
#include <vector>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

class parallel_scheduler;
class __parallel_sender;

// See docs/design/parallel_scheduler_p2079.md for the full design rationale. Intrusive
// singly-linked task list, matching <__execution/run_loop.h>'s __run_loop_opstate_base shape
// (a class with a pure virtual member can't be an aggregate, hence the small constructor).
struct __parallel_task_base {
  _LIBCPP_HIDE_FROM_ABI constexpr __parallel_task_base() noexcept = default;

  // A user-declared destructor (even defaulted) suppresses the implicit move constructor, so
  // without this, __parallel_opstate's own defaulted move constructor would fall back to
  // copy-constructing this base subobject -- deprecated (-Wdeprecated-copy-with-dtor) and, under
  // this codebase's -Werror, a hard build failure. Declared explicitly instead.
  _LIBCPP_HIDE_FROM_ABI constexpr __parallel_task_base(__parallel_task_base&&) noexcept = default;
  __parallel_task_base(const __parallel_task_base&)                                     = delete;
  __parallel_task_base& operator=(const __parallel_task_base&)                          = delete;
  __parallel_task_base& operator=(__parallel_task_base&&)                                = delete;

  virtual void __execute() noexcept = 0;
  __parallel_task_base* __next      = nullptr;

protected:
  ~__parallel_task_base() = default;
};

// [exec.parallel.scheduler]: the process-wide worker pool backing every parallel_scheduler
// handle. Heap-allocated exactly once, by __get_parallel_pool() below, and deliberately never
// destroyed: joining worker threads from a static destructor is a well-known deadlock/UB
// source (destruction order against other statics, including the C++ runtime's own, is
// unspecified), and the paper itself permits -- via its Phoenix-singleton suggestion -- the
// scheduler to outlive main(). Every worker thread loops forever over one shared, mutex-
// guarded intrusive queue (the same shape as run_loop's, generalized from one draining thread
// to N) and is detached immediately; the pool's `this` stays valid for the rest of the
// process's lifetime because the pool is never freed.
class __parallel_pool {
public:
  _LIBCPP_HIDE_FROM_ABI explicit __parallel_pool(size_t __n) {
    __workers_.reserve(__n);
    for (size_t __i = 0; __i < __n; ++__i) {
      __workers_.emplace_back([this] { __worker_loop(); });
    }
    for (auto& __t : __workers_) {
      __t.detach();
    }
  }

  __parallel_pool(const __parallel_pool&)            = delete;
  __parallel_pool& operator=(const __parallel_pool&) = delete;

  // [exec.parallel.scheduler]: schedule(parallel-scheduler)'s start(o) pushes the operation
  // state onto the pool's queue instead of running it inline -- this is the one property that
  // actually distinguishes parallel_scheduler from inline_scheduler.
  _LIBCPP_HIDE_FROM_ABI void __schedule(__parallel_task_base* __task) {
    {
      lock_guard<mutex> __lock(__mtx_);
      __task->__next = nullptr;
      if (__tail_ != nullptr) {
        __tail_->__next = __task;
      } else {
        __head_ = __task;
      }
      __tail_ = __task;
    }
    __cv_.notify_one();
  }

private:
  _LIBCPP_HIDE_FROM_ABI void __worker_loop() {
    while (true) {
      __parallel_task_base* __task;
      {
        unique_lock<mutex> __lock(__mtx_);
        __cv_.wait(__lock, [this] { return __head_ != nullptr; });
        __task  = __head_;
        __head_ = __task->__next;
        if (__head_ == nullptr) {
          __tail_ = nullptr;
        }
      }
      __task->__execute();
    }
  }

  mutex __mtx_;
  condition_variable __cv_;
  __parallel_task_base* __head_ = nullptr;
  __parallel_task_base* __tail_ = nullptr;
  vector<thread> __workers_;
};

// Thread count is explicitly implementation-defined per the paper (no configuration API
// exists) -- hardware_concurrency(), floored at 1 since it may report 0 when undetectable.
_LIBCPP_HIDE_FROM_ABI inline __parallel_pool& __get_parallel_pool() {
  static __parallel_pool* __pool = new __parallel_pool(std::thread::hardware_concurrency() == 0
                                                             ? 1
                                                             : static_cast<size_t>(std::thread::hardware_concurrency()));
  return *__pool;
}

// [exec.parallel.scheduler]: parallel_scheduler. A handle, not an owner -- copyable, not
// default-constructible (no implicit "empty" state; only get_parallel_scheduler() below
// produces one), equality-comparable by backend identity. Holds a raw, non-owning pointer to
// the process-wide __parallel_pool singleton, which is safe precisely because that singleton
// is deliberately leaked (see __get_parallel_pool() above) -- no shared_ptr/refcounting needed
// since the pointee never goes away. Defined here (immediately after __parallel_pool, before
// anything that embeds a parallel_scheduler by value) rather than after __parallel_sender, the
// same ordering <__execution/run_loop.h> uses for __run_loop_scheduler: schedule()'s return
// type is only forward-declared at this point, so its body is defined out-of-line below, once
// __parallel_sender is complete -- matching run_loop.h's own __run_loop_scheduler::schedule().
class parallel_scheduler {
public:
  using scheduler_concept = scheduler_tag;

  parallel_scheduler() = delete;

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const parallel_scheduler& __x, const parallel_scheduler& __y) noexcept {
    return __x.__pool_ == __y.__pool_;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __parallel_sender schedule() const noexcept;

  // [exec.parallel.scheduler]p2: query(get_forward_progress_guarantee_t) is parallel --
  // independent worker threads, but (Pass 1) no work-stealing/helping guarantee that would
  // justify the stronger "concurrent" answer inline_scheduler gives for its own, different
  // reason (completing synchronously on the caller's own agent).
  _LIBCPP_HIDE_FROM_ABI constexpr forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::parallel;
  }

private:
  friend _LIBCPP_HIDE_FROM_ABI parallel_scheduler get_parallel_scheduler() noexcept;
  friend class __parallel_sndr_env;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit parallel_scheduler(__parallel_pool* __pool) noexcept : __pool_(__pool) {}

  __parallel_pool* __pool_;
};

// [exec.parallel.scheduler]p3: the environment of schedule(parallel-scheduler) answers
// get_completion_scheduler<set_value_t> with the parallel_scheduler instance schedule() was
// called on. <__execution/bulk.h>'s bulk_chunked_t probes this (Pass 2, see the design note)
// to detect when it should dispatch across the pool instead of running inline -- the standard's
// own domain-based transform_sender customization mechanism for this is permanently disabled on
// this fork (see <__execution/domain.h>'s M2 deviation and <__execution/bulk.h>'s own comment),
// so this direct probe is the documented, pragmatic replacement for it.
class __parallel_sndr_env {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __parallel_sndr_env(parallel_scheduler __sch) noexcept;

  template <class _Tag>
    requires is_same_v<_Tag, set_value_t>
  _LIBCPP_HIDE_FROM_ABI constexpr parallel_scheduler query(get_completion_scheduler_t<_Tag>) const noexcept;

private:
  parallel_scheduler __sch_;
};

// [exec.parallel.scheduler]: parallel-scheduler's operation state. Unlike run_loop's opstate
// (single draining thread, chosen by whoever calls run()), __execute() here runs on whichever
// worker thread pops this item off the pool's queue -- genuine concurrency, the first in this
// fork. Mirrors run_loop's stop-token check: a schedule() sender can complete set_stopped_t()
// if the receiver's environment already carries a requested stop, matching
// [exec.parallel.scheduler]'s completion-signature set; nothing in Pass 1 can produce
// set_error_t (no user code runs as part of schedule()'s own completion), so -- matching
// run_loop.h's and task_scheduler.h's own precedent of only declaring what's reachable -- it
// is intentionally not declared here.
template <class _Rcvr>
class __parallel_opstate final : public __parallel_task_base {
public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __parallel_opstate(__parallel_pool* __pool, _Rcvr&& __rcvr) noexcept
      : __pool_(__pool), __rcvr_(std::move(__rcvr)) {}

  // Movable, matching <__execution/task_scheduler.h>'s __inline_opstate: __task_scheduler_model
  // ::__connect receives an already-materialized opstate through a forwarding-reference
  // constructor parameter (`_RealOp&& __op`) and move-constructs a member from it -- one layer
  // removed from the guaranteed-copy-elision-eligible "single prvalue return statement" shape
  // most other opstates in this fork rely on to get away with deleting move entirely (e.g.
  // <__execution/run_loop.h>'s __run_loop_opstate, whose connect() returns a bare prvalue).
  // Any scheduler wrapped by task_scheduler (as parallel_scheduler will be) needs this.
  _LIBCPP_HIDE_FROM_ABI __parallel_opstate(__parallel_opstate&&) noexcept = default;
  __parallel_opstate(const __parallel_opstate&)                          = delete;
  __parallel_opstate& operator=(const __parallel_opstate&)               = delete;
  __parallel_opstate& operator=(__parallel_opstate&&)                    = delete;

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { __pool_->__schedule(this); }

private:
  _LIBCPP_HIDE_FROM_ABI void __execute() noexcept override {
    if (std::get_stop_token(execution::get_env(__rcvr_)).stop_requested()) {
      execution::set_stopped(std::move(__rcvr_));
    } else {
      execution::set_value(std::move(__rcvr_));
    }
  }

  __parallel_pool* __pool_;
  _Rcvr __rcvr_;
};

// [exec.parallel.scheduler]: parallel-scheduler-sender.
class __parallel_sender {
public:
  using sender_concept = sender_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __parallel_sender(__parallel_pool* __pool, parallel_scheduler __sch) noexcept
      : __pool_(__pool), __sch_(__sch) {}

  _LIBCPP_HIDE_FROM_ABI constexpr __parallel_sndr_env get_env() const noexcept { return __parallel_sndr_env(__sch_); }

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI __parallel_opstate<remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) const {
    return __parallel_opstate<remove_cvref_t<_Rcvr>>(__pool_, std::forward<_Rcvr>(__rcvr));
  }

  // Same Env-dependent shape as run_loop.h's own get_completion_signatures -- see that file's
  // comment on why a single non-variadic _Env parameter (rather than 0-or-1) is the right
  // match for this fork's "dependent-sender-as-soft-failure" deviation (docs/CXX26_GAPS.md, M2).
  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (unstoppable_token<stop_token_of_t<_Env>>) {
      return completion_signatures<set_value_t()>{};
    } else {
      return completion_signatures<set_value_t(), set_stopped_t()>{};
    }
  }

private:
  __parallel_pool* __pool_;
  parallel_scheduler __sch_;
};

_LIBCPP_HIDE_FROM_ABI constexpr __parallel_sndr_env::__parallel_sndr_env(parallel_scheduler __sch) noexcept
    : __sch_(__sch) {}

template <class _Tag>
  requires is_same_v<_Tag, set_value_t>
_LIBCPP_HIDE_FROM_ABI constexpr parallel_scheduler __parallel_sndr_env::query(get_completion_scheduler_t<_Tag>) const noexcept {
  return __sch_;
}

_LIBCPP_HIDE_FROM_ABI constexpr __parallel_sender parallel_scheduler::schedule() const noexcept {
  return __parallel_sender(__pool_, *this);
}

// [exec.parallel.scheduler]p1: the only way to obtain a parallel_scheduler. Every call returns
// a handle to the same process-wide pool (verified by operator== comparing the pool pointer),
// matching the paper's own description of a typically-shared backend.
_LIBCPP_HIDE_FROM_ABI inline parallel_scheduler get_parallel_scheduler() noexcept {
  return parallel_scheduler(&__get_parallel_pool());
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_PARALLEL_SCHEDULER_H
