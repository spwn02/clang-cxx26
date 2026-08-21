//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_RUN_LOOP_H
#define _LIBCPP___EXECUTION_RUN_LOOP_H

#include <__condition_variable/condition_variable.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_forward_progress_guarantee.h>
#include <__execution/get_scheduler.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
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
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

class run_loop;
class __run_loop_scheduler;
class __run_loop_sender;

// [exec.run.loop.types]
// Not an aggregate (unlike the exposition text's bare-struct sketch): a class with a pure
// virtual member function cannot be one, so a small constructor takes its place -- an
// implementation detail invisible to anything outside this header.
struct __run_loop_opstate_base {
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __run_loop_opstate_base(run_loop* __loop) noexcept
      : __loop(__loop), __next(nullptr) {}

  virtual void __execute() noexcept = 0;
  run_loop* __loop;
  __run_loop_opstate_base* __next;

protected:
  ~__run_loop_opstate_base() = default;
};

template <class _Rcvr>
class __run_loop_opstate;

// [exec.run.loop.types]: run-loop-scheduler.
class __run_loop_scheduler {
public:
  using scheduler_concept = scheduler_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __run_loop_scheduler(run_loop* __loop) noexcept : __loop_(__loop) {}

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __run_loop_scheduler& __x,
                                                          const __run_loop_scheduler& __y) noexcept {
    return __x.__loop_ == __y.__loop_;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __run_loop_sender schedule() const noexcept;

  // [exec.run.loop] does not pin down a forward progress guarantee for run-loop-scheduler.
  // run_loop drives every scheduled item to completion on a single thread of execution (the
  // one that calls run()), which is exactly the guarantee [intro.progress] describes as
  // "parallel" (as opposed to "concurrent", which would require independent forward
  // progress across agents that could block on each other) -- matching the answer other
  // single-thread-executor sender implementations give for an equivalent scheduler.
  _LIBCPP_HIDE_FROM_ABI constexpr forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::parallel;
  }

private:
  friend class __run_loop_sender;
  run_loop* __loop_;
};

// [exec.run.loop.types]p5: the environment associated with schedule(run-loop-scheduler)
// answers get_completion_scheduler<set_value_t>/get_completion_scheduler<set_stopped_t> with
// the run-loop-scheduler instance schedule() was called on (run-loop-sender never completes
// with set_error, so that tag is intentionally not answered here). Stores the scheduler by
// value, so it's defined after __run_loop_scheduler rather than before it.
class __run_loop_sndr_env {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __run_loop_sndr_env(__run_loop_scheduler __sch) noexcept : __sch_(__sch) {}

  template <class _Tag>
    requires(is_same_v<_Tag, set_value_t> || is_same_v<_Tag, set_stopped_t>)
  _LIBCPP_HIDE_FROM_ABI constexpr __run_loop_scheduler query(get_completion_scheduler_t<_Tag>) const noexcept {
    return __sch_;
  }

private:
  __run_loop_scheduler __sch_;
};

// [exec.run.loop.types]: run-loop-sender.
class __run_loop_sender {
public:
  using sender_concept = sender_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __run_loop_sender(run_loop* __loop) noexcept : __loop_(__loop) {}

  _LIBCPP_HIDE_FROM_ABI constexpr __run_loop_sndr_env get_env() const noexcept {
    return __run_loop_sndr_env(__run_loop_scheduler(__loop_));
  }

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr __run_loop_opstate<remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) &&;

  // [exec.run.loop.types]p6: completion_signatures_of_t<run-loop-sender, E> is
  // completion_signatures<set_value_t()> if unstoppable_token<stop_token_of_t<E>> is true,
  // otherwise completion_signatures<set_value_t(), set_stopped_t()>. Genuinely
  // Env-dependent, so -- matching <__execution/read_env.h>'s documented "dependent-sender-
  // as-soft-failure" deviation (docs/CXX26_GAPS.md, M2) -- a single, non-variadic _Env
  // parameter (rather than 0-or-1) means the zero-Env case simply has no viable overload
  // instead of reporting dependent_sender_error.
  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (unstoppable_token<stop_token_of_t<_Env>>) {
      return completion_signatures<set_value_t()>{};
    } else {
      return completion_signatures<set_value_t(), set_stopped_t()>{};
    }
  }

private:
  run_loop* __loop_;
};

_LIBCPP_HIDE_FROM_ABI constexpr __run_loop_sender __run_loop_scheduler::schedule() const noexcept {
  return __run_loop_sender(__loop_);
}

// [exec.run.loop.types]: run-loop-opstate<Rcvr>.
template <class _Rcvr>
class __run_loop_opstate final : private __run_loop_opstate_base {
public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __run_loop_opstate(run_loop* __loop, _Rcvr&& __rcvr)
      : __run_loop_opstate_base(__loop), __rcvr_(std::move(__rcvr)) {}

  __run_loop_opstate(const __run_loop_opstate&)            = delete;
  __run_loop_opstate& operator=(const __run_loop_opstate&) = delete;

  // [exec.run.loop.types]p10.3: start(o) is equivalent to `o.loop->push-back(addressof(o));`
  // -- defined out-of-line below, after run_loop itself (a complete type here needs
  // run_loop::__push_back to be a real, callable member).
  _LIBCPP_HIDE_FROM_ABI constexpr void start() & noexcept;

private:
  // [exec.run.loop.types]p10.2. The clause literally writes `get_stop_token(REC(o))`, but
  // get_stop_token ([exec.get.stop.token]) is defined in terms of a *queryable environment*
  // (`env.query(get_stop_token)`), not a receiver -- a receiver only becomes queryable via
  // its own get_env(). Read as shorthand for `get_stop_token(get_env(REC(o)))`, matching how
  // every other stop-token consumer in the draft (and every other query CPO applied "to a
  // receiver" throughout [exec]) is actually spelled.
  _LIBCPP_HIDE_FROM_ABI void __execute() noexcept override {
    if (std::get_stop_token(execution::get_env(__rcvr_)).stop_requested()) {
      execution::set_stopped(std::move(__rcvr_));
    } else {
      execution::set_value(std::move(__rcvr_));
    }
  }

  _Rcvr __rcvr_;
};

template <class _Rcvr>
_LIBCPP_HIDE_FROM_ABI constexpr __run_loop_opstate<remove_cvref_t<_Rcvr>> __run_loop_sender::connect(_Rcvr&& __rcvr) && {
  return __run_loop_opstate<remove_cvref_t<_Rcvr>>(__loop_, std::forward<_Rcvr>(__rcvr));
}

// [exec.run.loop]
class run_loop {
public:
  // [exec.run.loop.ctor]p1: postconditions are count == 0, state == starting -- both are
  // the default member initializers below, so the constructor body is empty. Written out
  // (rather than `= default`) because `mutex`/`condition_variable`'s own default
  // constructors are not themselves declared noexcept, and an explicitly-defaulted special
  // member function's exception specification must match the implicit one exactly --
  // an ordinary constructor with an explicit `noexcept` has no such constraint (a throw from
  // a member's constructor here would call std::terminate, matching what `noexcept` means
  // everywhere else).
  _LIBCPP_HIDE_FROM_ABI run_loop() noexcept {}

  run_loop(run_loop&&) = delete;

  // [exec.run.loop.ctor]p2: terminate() unless count == 0 and state != running.
  _LIBCPP_HIDE_FROM_ABI ~run_loop() {
    if (__count_ != 0 || __state_ == __running) {
      std::terminate();
    }
  }

  _LIBCPP_HIDE_FROM_ABI __run_loop_scheduler get_scheduler() noexcept { return __run_loop_scheduler(this); }

  // [exec.run.loop.members]p5-7.
  _LIBCPP_HIDE_FROM_ABI void run() noexcept {
    {
      lock_guard<mutex> __lock(__mtx_);
      if (__state_ == __starting) {
        __state_ = __running;
      }
    }
    while (__run_loop_opstate_base* __op = __pop_front()) {
      __op->__execute();
    }
  }

  // [exec.run.loop.members]p8-10.
  _LIBCPP_HIDE_FROM_ABI void finish() noexcept {
    {
      lock_guard<mutex> __lock(__mtx_);
      __state_ = __finishing;
    }
    __cv_.notify_one();
  }

private:
  template <class>
  friend class __run_loop_opstate;

  // [exec.run.loop.members]p1: blocks until either (count == 0 && state == finishing) --
  // sets state to finished and returns nullptr -- or count > 0 -- pops and returns the front
  // item, decrementing count.
  _LIBCPP_HIDE_FROM_ABI __run_loop_opstate_base* __pop_front() noexcept {
    unique_lock<mutex> __lock(__mtx_);
    __cv_.wait(__lock, [this] { return __count_ > 0 || __state_ == __finishing; });
    if (__count_ == 0) {
      __state_ = __finished;
      return nullptr;
    }
    __run_loop_opstate_base* __item = __head_;
    __head_                         = __item->__next;
    if (__head_ == nullptr) {
      __tail_ = nullptr;
    }
    --__count_;
    return __item;
  }

  // [exec.run.loop.members]p2-3: adds to the back, increments count, and synchronizes with
  // the pop-front call that obtains this item.
  _LIBCPP_HIDE_FROM_ABI void __push_back(__run_loop_opstate_base* __item) noexcept {
    {
      lock_guard<mutex> __lock(__mtx_);
      __item->__next = nullptr;
      if (__tail_ != nullptr) {
        __tail_->__next = __item;
      } else {
        __head_ = __item;
      }
      __tail_ = __item;
      ++__count_;
    }
    __cv_.notify_one();
  }

  enum __state_t { __starting, __running, __finishing, __finished };

  mutex __mtx_;
  condition_variable __cv_;
  __run_loop_opstate_base* __head_ = nullptr;
  __run_loop_opstate_base* __tail_ = nullptr;
  size_t __count_                  = 0;
  __state_t __state_                = __starting;
};

template <class _Rcvr>
_LIBCPP_HIDE_FROM_ABI constexpr void __run_loop_opstate<_Rcvr>::start() & noexcept {
  __loop->__push_back(this);
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_RUN_LOOP_H
