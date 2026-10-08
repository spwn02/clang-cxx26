//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_PARALLEL_SCHEDULER_H
#define _LIBCPP___EXECUTION_PARALLEL_SCHEDULER_H

#include <__assert>
#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_forward_progress_guarantee.h>
#include <__execution/get_scheduler.h>
#include <__exception/terminate.h>
#include <__execution/domain.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__execution/system_context_replaceability.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__mutex/unique_lock.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <span>
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
struct bulk_chunked_t;
struct bulk_unchunked_t;

// The completion domain of schedule(parallel_scheduler). Bulk senders retain their
// shape for the domain transform; their connect operation uses the bound backend.
struct __parallel_scheduler_domain {
  template <class _Tag, class _Sndr, class _Env>
    requires(is_same_v<_Tag, bulk_chunked_t> || is_same_v<_Tag, bulk_unchunked_t>)
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) transform_sender(_Tag, _Sndr&& __sndr, const _Env&) const noexcept {
    return std::forward<_Sndr>(__sndr);
  }
};

// Intrusive singly-linked task list, matching <__execution/run_loop.h>'s __run_loop_opstate_base shape
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
  __parallel_task_base& operator=(__parallel_task_base&&)                               = delete;

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
  static __parallel_pool* __pool = new __parallel_pool(
      std::thread::hardware_concurrency() == 0 ? 1 : static_cast<size_t>(std::thread::hardware_concurrency()));
  return *__pool;
}

// [exec.par.scheduler]: retain the backend queried when the scheduler is obtained.
// Copies and senders share that backend; equality compares the backend object identity.
class parallel_scheduler {
public:
  using scheduler_concept = scheduler_tag;

  parallel_scheduler() = delete;

  _LIBCPP_HIDE_FROM_ABI friend bool operator==(const parallel_scheduler& __x, const parallel_scheduler& __y) noexcept {
    return __x.__backend_ == __y.__backend_;
  }

  _LIBCPP_HIDE_FROM_ABI __parallel_sender schedule() const noexcept;

  // [exec.parallel.scheduler]p2: query(get_forward_progress_guarantee_t) is parallel --
  // independent worker threads, but no work-stealing/helping guarantee that would
  // justify the stronger "concurrent" answer inline_scheduler gives for its own, different
  // reason (completing synchronously on the caller's own agent).
  _LIBCPP_HIDE_FROM_ABI constexpr forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::parallel;
  }

  // [exec.sched]p6: the scheduler answers the completion queries of the attributes of schedule(*this) (also without an
  // environment).
  _LIBCPP_HIDE_FROM_ABI parallel_scheduler query(get_completion_scheduler_t<set_value_t>) const noexcept { return *this; }
  _LIBCPP_HIDE_FROM_ABI __parallel_scheduler_domain query(get_completion_domain_t<set_value_t>) const noexcept { return {}; }

  _LIBCPP_HIDE_FROM_ABI shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend>
  __get_backend() const noexcept { return __backend_; }

private:
  friend _LIBCPP_HIDE_FROM_ABI parallel_scheduler get_parallel_scheduler();
  friend class __parallel_sndr_env;

  _LIBCPP_HIDE_FROM_ABI explicit parallel_scheduler(
      shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend) noexcept
      : __backend_(std::move(__backend)) {}

  shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend_;
};

// The schedule sender's completion attributes identify the scheduler whose
// backend must receive subsequent bulk work.
class __parallel_sndr_env {
public:
  _LIBCPP_HIDE_FROM_ABI explicit __parallel_sndr_env(parallel_scheduler __sch) noexcept;

  template <class _Tag>
    requires is_same_v<_Tag, set_value_t>
  _LIBCPP_HIDE_FROM_ABI parallel_scheduler query(get_completion_scheduler_t<_Tag>) const noexcept;

  // The sender completes with set_stopped only for an environment with a stoppable token (its completion signatures).
  template <class _Tag, class _Env>
    requires(is_same_v<_Tag, set_stopped_t> && !unstoppable_token<stop_token_of_t<_Env>>)
  _LIBCPP_HIDE_FROM_ABI parallel_scheduler query(get_completion_scheduler_t<_Tag>, const _Env&) const noexcept {
    return __sch_;
  }

private:
  parallel_scheduler __sch_;
};

// Dispatch through the bound backend, keeping it alive through operation completion.
template <class _Rcvr>
class __parallel_opstate final : public execution::parallel_scheduler_replacement::receiver_proxy {
  using _StopToken = decltype(std::get_stop_token(execution::get_env(std::declval<_Rcvr&>())));

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __parallel_opstate(
      shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend, _Rcvr&& __rcvr) noexcept
      : __backend_(std::move(__backend)), __rcvr_(std::move(__rcvr)) {}

  // Movable, matching <__execution/task_scheduler.h>'s __inline_opstate: __task_scheduler_model
  // ::__connect receives an already-materialized opstate through a forwarding-reference
  // constructor parameter (`_RealOp&& __op`) and move-constructs a member from it -- one layer
  // removed from the guaranteed-copy-elision-eligible "single prvalue return statement" shape
  // most other opstates in this fork rely on to get away with deleting move entirely (e.g.
  // <__execution/run_loop.h>'s __run_loop_opstate, whose connect() returns a bare prvalue).
  // Any scheduler wrapped by task_scheduler (as parallel_scheduler will be) needs this.
  _LIBCPP_HIDE_FROM_ABI __parallel_opstate(__parallel_opstate&&) noexcept = default;
  __parallel_opstate(const __parallel_opstate&)                           = delete;
  __parallel_opstate& operator=(const __parallel_opstate&)                = delete;
  __parallel_opstate& operator=(__parallel_opstate&&)                     = delete;

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept {
    __backend_->schedule(*this, span<byte>(__backend_storage_, sizeof(__backend_storage_)));
  }

private:
  _LIBCPP_HIDE_FROM_ABI void set_value() noexcept override { execution::set_value(std::move(__rcvr_)); }
  _LIBCPP_HIDE_FROM_ABI void set_stopped() noexcept override { execution::set_stopped(std::move(__rcvr_)); }
  // [exec.par.scheduler]: "r.set_error(e) has effects equivalent to set_error(std::move(rcvr), std::move(e))", but the
  // draft gives the schedule sender no error completion (task_scheduler needs an infallible-scheduler; LWG candidate
  // on #263, #270): the library advertises none, so a receiver cannot be assumed to handle one (the forwarding
  // receivers of the adaptors would fail to compile for a receiver that does not). A backend that reports a scheduling
  // error is therefore a contract violation of the advertised signatures: terminate deterministically (before, the
  // receiver was never completed unless hardening was enabled).
  [[noreturn]] _LIBCPP_HIDE_FROM_ABI void set_error(std::exception_ptr) noexcept override { std::terminate(); }

  _LIBCPP_HIDE_FROM_ABI bool
  __query_env(const type_info& __query_type, const type_info& __result_type, const void*, void* __result_storage)
      const noexcept override {
    if (__query_type == typeid(get_stop_token_t) && __result_type == typeid(_StopToken)) {
      ::new (__result_storage) _StopToken(std::get_stop_token(execution::get_env(__rcvr_)));
      return true;
    }
    return false;
  }

  shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend_;
  _Rcvr __rcvr_;
  // Preallocated scratch storage for the bound backend.
  alignas(max_align_t) byte __backend_storage_[64];
};

// [exec.parallel.scheduler]: parallel-scheduler-sender.
class __parallel_sender {
public:
  using sender_concept = sender_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __parallel_sender(
      shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend,
      parallel_scheduler __sch) noexcept
      : __backend_(std::move(__backend)), __sch_(std::move(__sch)) {}

  _LIBCPP_HIDE_FROM_ABI __parallel_sndr_env get_env() const noexcept { return __parallel_sndr_env(__sch_); }

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI __parallel_opstate<remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) const {
    return __parallel_opstate<remove_cvref_t<_Rcvr>>(__backend_, std::forward<_Rcvr>(__rcvr));
  }

  // Same Env-dependent shape as run_loop.h's own get_completion_signatures -- see that file's
  // comment on why a single non-variadic _Env parameter (rather than 0-or-1) is the right
  // match for the dependent-sender-as-soft-failure behavior: an absent environment makes
  // the overload non-viable rather than producing a dependent error result.
  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (unstoppable_token<stop_token_of_t<_Env>>) {
      return completion_signatures<set_value_t()>{};
    } else {
      return completion_signatures<set_value_t(), set_stopped_t()>{};
    }
  }

private:
  shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend_;
  parallel_scheduler __sch_;
};

inline _LIBCPP_HIDE_FROM_ABI __parallel_sndr_env::__parallel_sndr_env(parallel_scheduler __sch) noexcept : __sch_(__sch) {}

template <class _Tag>
  requires is_same_v<_Tag, set_value_t>
_LIBCPP_HIDE_FROM_ABI parallel_scheduler __parallel_sndr_env::query(get_completion_scheduler_t<_Tag>) const noexcept {
  return __sch_;
}

inline _LIBCPP_HIDE_FROM_ABI __parallel_sender parallel_scheduler::schedule() const noexcept {
  return __parallel_sender(__backend_, *this);
}

// [exec.par.scheduler]: query once and terminate if no backend is available.
_LIBCPP_HIDE_FROM_ABI inline parallel_scheduler get_parallel_scheduler() {
  auto __backend = parallel_scheduler_replacement::query_parallel_scheduler_backend();
  if (!__backend)
    std::terminate();
  return parallel_scheduler(std::move(__backend));
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_PARALLEL_SCHEDULER_H
