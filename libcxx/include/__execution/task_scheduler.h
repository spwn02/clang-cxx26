//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_TASK_SCHEDULER_H
#define _LIBCPP___EXECUTION_TASK_SCHEDULER_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/domain.h>
#include <__execution/env.h>
#include <__execution/get_forward_progress_guarantee.h>
#include <__execution/inline_scheduler.h>
#include <__execution/get_scheduler.h>
#include <__execution/get_stop_token.h>
#include <__execution/infallible_scheduler.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/schedule.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <exception>
#include <memory>
#include <typeinfo>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.task.scheduler]: task_scheduler. A type-erasing wrapper holding any type satisfying
// `scheduler`, used as execution::task<T, Environment>::scheduler_type's default. Per the
// adopted text (verified directly, not re-derived from memory -- see
// docs/design/execution_task_p3552.md), task_scheduler has exactly one constructor --
// `explicit task_scheduler(Sch&&, Allocator = {})`, constrained on scheduler<Sch> -- and is
// NOT default-constructible: there is no implicit "empty" state, so every task<T,Environment>
// whose Environment doesn't supply its own scheduler_type must explicitly construct
// `task_scheduler(inline_scheduler{})` wherever it needs a default value (see task.h's
// promise_type).
//
// The erased schedule() sender always advertises this fixed completion-signature shape,
// regardless of what the wrapped real scheduler's own schedule() sender declares: erasing to
// a common shape is what makes a single concrete task_scheduler type possible at all.
// inline_scheduler's own schedule() only ever produces set_value_t(), a strict subset of this
// -- the error_t/stopped_t alternatives exist for a future non-inline scheduler whose
// schedule() can genuinely fail or be cancelled (P2079R10). A real limitation, documented
// rather than hidden: __task_scheduler_bridge_rcvr (below) answers get_env() with an empty
// environment, so the wrapped scheduler's own schedule() sender never sees whatever
// environment the outer receiver connected through task_scheduler carried (allocator/stop
// token/etc.) -- acceptable today because inline_scheduler's schedule() doesn't consult its
// receiver's environment at all; revisit once a real scheduler that does exists.
using __task_scheduler_sigs = completion_signatures<set_value_t(), set_error_t(exception_ptr), set_stopped_t()>;

// Owned only through unique_ptr<__task_scheduler_rcvr_base> (see __task_scheduler_bridge_rcvr
// below), so the destructor must be both public (default_delete<Base> deletes through the
// base pointer) and virtual (so the owning model's own members/base are actually torn down).
struct __task_scheduler_rcvr_base {
  _LIBCPP_HIDE_FROM_ABI virtual ~__task_scheduler_rcvr_base()                 = default;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_value() noexcept             = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_error(exception_ptr) noexcept = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_stopped() noexcept           = 0;
};

template <class _Rcvr>
struct __task_scheduler_rcvr_model final : __task_scheduler_rcvr_base {
  _Rcvr __rcvr_;

  _LIBCPP_HIDE_FROM_ABI explicit __task_scheduler_rcvr_model(_Rcvr&& __rcvr) : __rcvr_(std::move(__rcvr)) {}

  _LIBCPP_HIDE_FROM_ABI void __complete_value() noexcept override { execution::set_value(std::move(__rcvr_)); }
  _LIBCPP_HIDE_FROM_ABI void __complete_error(exception_ptr __ep) noexcept override {
    (void)__ep;
    std::terminate();
  }
  _LIBCPP_HIDE_FROM_ABI void __complete_stopped() noexcept override {
    if constexpr (requires { execution::set_stopped(std::move(__rcvr_)); })
      execution::set_stopped(std::move(__rcvr_));
    else
      std::terminate();
  }
};

// Concrete (non-template) receiver connected to the *real*, wrapped scheduler's own
// schedule() sender -- bridges its (arbitrary) completion shape back to the fixed
// __task_scheduler_sigs shape via the erased sink above. Only set_value()/set_stopped() are
// spelled directly (matching every real scheduler this fork has); a real error completion
// would need a bridging overload too, added when a fallible scheduler actually exists.
struct __task_scheduler_bridge_rcvr {
  using receiver_concept = receiver_tag;

  unique_ptr<__task_scheduler_rcvr_base> __sink_;

  _LIBCPP_HIDE_FROM_ABI void set_value() && noexcept { __sink_->__complete_value(); }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI void set_error(_Err&& __err) && noexcept {
    __sink_->__complete_error(execution::__as_except_ptr(std::forward<_Err>(__err)));
  }

  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { __sink_->__complete_stopped(); }

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return env<>{}; }
};

// Owned only through unique_ptr<__task_scheduler_opstate_base> (see __task_scheduler_opstate
// below) -- same public+virtual destructor reasoning as __task_scheduler_rcvr_base above.
struct __task_scheduler_opstate_base {
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI virtual ~__task_scheduler_opstate_base() = default;
  _LIBCPP_HIDE_FROM_ABI virtual void __start() noexcept          = 0;
  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { __start(); }
};

template <class _RealOp>
struct __task_scheduler_opstate_model final : __task_scheduler_opstate_base {
  _RealOp __op_;

  template <class _Factory>
  _LIBCPP_HIDE_FROM_ABI explicit __task_scheduler_opstate_model(_Factory __make) : __op_(__make()) {}
  _LIBCPP_HIDE_FROM_ABI void __start() noexcept override { execution::start(__op_); }
};

class __task_scheduler_opstate {
public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __task_scheduler_opstate(unique_ptr<__task_scheduler_opstate_base> __op) noexcept
      : __op_(std::move(__op)) {}

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { __op_->start(); }

private:
  unique_ptr<__task_scheduler_opstate_base> __op_;
};

struct __task_scheduler_concept {
  _LIBCPP_HIDE_FROM_ABI virtual ~__task_scheduler_concept()                                    = default;
  _LIBCPP_HIDE_FROM_ABI virtual unique_ptr<__task_scheduler_opstate_base>
  __connect(unique_ptr<__task_scheduler_rcvr_base>) const                                      = 0;
  _LIBCPP_HIDE_FROM_ABI virtual bool __equals(const __task_scheduler_concept&) const noexcept   = 0;
  _LIBCPP_HIDE_FROM_ABI virtual forward_progress_guarantee __fwd_progress() const noexcept      = 0;
};

template <class _Sch>
struct __task_scheduler_model final : __task_scheduler_concept {
  _Sch __sch_;

  _LIBCPP_HIDE_FROM_ABI explicit __task_scheduler_model(_Sch __sch) : __sch_(std::move(__sch)) {}

  _LIBCPP_HIDE_FROM_ABI unique_ptr<__task_scheduler_opstate_base>
  __connect(unique_ptr<__task_scheduler_rcvr_base> __rcvr) const override {
    using __real_op_t = connect_result_t<schedule_result_t<const _Sch&>, __task_scheduler_bridge_rcvr>;
    return std::make_unique<__task_scheduler_opstate_model<__real_op_t>>(
        [&] { return execution::connect(execution::schedule(__sch_), __task_scheduler_bridge_rcvr{std::move(__rcvr)}); });
  }

  _LIBCPP_HIDE_FROM_ABI bool __equals(const __task_scheduler_concept& __other) const noexcept override {
    auto* __p = dynamic_cast<const __task_scheduler_model*>(&__other);
    return __p != nullptr && __sch_ == __p->__sch_;
  }

  _LIBCPP_HIDE_FROM_ABI forward_progress_guarantee __fwd_progress() const noexcept override {
    return execution::get_forward_progress_guarantee(__sch_);
  }
};

class __task_scheduler_domain : public default_domain {};

class __task_scheduler_sender {
public:
  using sender_concept = sender_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __task_scheduler_sender(shared_ptr<const __task_scheduler_concept> __holder) noexcept
      : __holder_(std::move(__holder)) {}

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI __task_scheduler_opstate connect(_Rcvr&& __rcvr) const {
    auto __erased_rcvr =
        std::make_unique<__task_scheduler_rcvr_model<remove_cvref_t<_Rcvr>>>(std::forward<_Rcvr>(__rcvr));
    return __task_scheduler_opstate(__holder_->__connect(std::move(__erased_rcvr)));
  }

  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (unstoppable_token<stop_token_of_t<_Env>>)
      return completion_signatures<set_value_t()>{};
    else
      return completion_signatures<set_value_t(), set_stopped_t()>{};
  }

private:
  shared_ptr<const __task_scheduler_concept> __holder_;
};

class task_scheduler {
public:
  using scheduler_concept = scheduler_tag;

  template <class _Sch, class _Allocator = allocator<byte>>
    requires(!same_as<task_scheduler, remove_cvref_t<_Sch>>) && scheduler<remove_cvref_t<_Sch>>
  _LIBCPP_HIDE_FROM_ABI explicit task_scheduler(_Sch&& __sch, const _Allocator& __alloc = {})
      : __holder_(std::allocate_shared<__task_scheduler_model<remove_cvref_t<_Sch>>>(
            __alloc, std::forward<_Sch>(__sch))) {
    // [exec.task.scheduler]: Mandates: Sch satisfies infallible-scheduler<env<>>.
    static_assert(__infallible_scheduler<remove_cvref_t<_Sch>, env<>>,
                  "Mandates: Sch satisfies infallible-scheduler<env<>>.");
  }

  _LIBCPP_HIDE_FROM_ABI friend bool operator==(const task_scheduler& __x, const task_scheduler& __y) noexcept {
    return __x.__holder_ == __y.__holder_ || __x.__holder_->__equals(*__y.__holder_);
  }

  template <scheduler _Sch>
    requires (!same_as<task_scheduler, _Sch>)
  _LIBCPP_HIDE_FROM_ABI friend bool operator==(const task_scheduler& __lhs, const _Sch& __rhs) noexcept {
    auto* __p = dynamic_cast<const __task_scheduler_model<_Sch>*>(__lhs.__holder_.get());
    return __p != nullptr && __p->__sch_ == __rhs;
  }

  _LIBCPP_HIDE_FROM_ABI __task_scheduler_sender schedule() const noexcept { return __task_scheduler_sender(__holder_); }

  _LIBCPP_HIDE_FROM_ABI forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return __holder_->__fwd_progress();
  }
  _LIBCPP_HIDE_FROM_ABI __task_scheduler_domain query(get_completion_domain_t<set_value_t>) const noexcept {
    return {};
  }
  // [exec.sched]p6: the same completion scheduler as the attributes of schedule(*this), also without an environment.
  _LIBCPP_HIDE_FROM_ABI task_scheduler query(get_completion_scheduler_t<set_value_t>) const noexcept { return *this; }

private:
  friend class __task_scheduler_sender;
  _LIBCPP_HIDE_FROM_ABI explicit task_scheduler(shared_ptr<const __task_scheduler_concept> __holder)
      : __holder_(std::move(__holder)) {}
  shared_ptr<const __task_scheduler_concept> __holder_;
};

// The attributes of schedule(task_scheduler): the completion scheduler of the value completion is the scheduler itself,
// the completion domain its ts-domain; the sender only completes with set_stopped for an environment with a stoppable
// token.
struct __task_scheduler_sender_env {
  task_scheduler __scheduler;
  _LIBCPP_HIDE_FROM_ABI task_scheduler query(get_completion_scheduler_t<set_value_t>) const noexcept {
    return __scheduler;
  }
  template <class _Env>
    requires(!unstoppable_token<stop_token_of_t<_Env>>)
  _LIBCPP_HIDE_FROM_ABI task_scheduler query(get_completion_scheduler_t<set_stopped_t>, const _Env&) const noexcept {
    return __scheduler;
  }
  _LIBCPP_HIDE_FROM_ABI __task_scheduler_domain query(get_completion_domain_t<set_value_t>) const noexcept {
    return {};
  }
};

_LIBCPP_HIDE_FROM_ABI inline auto __task_scheduler_sender::get_env() const noexcept {
  return __task_scheduler_sender_env{task_scheduler(__holder_)};
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_TASK_SCHEDULER_H
