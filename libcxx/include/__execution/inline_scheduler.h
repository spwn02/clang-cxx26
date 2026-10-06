//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_INLINE_SCHEDULER_H
#define _LIBCPP___EXECUTION_INLINE_SCHEDULER_H

#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/domain.h>
#include <__execution/get_forward_progress_guarantee.h>
#include <__execution/get_scheduler.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/schedule.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.task.scheduler]'s inline-scheduler: a scheduler whose schedule() sender completes
// set_value_t() synchronously, on the calling execution agent, from within start(). This is
// the only scheduler this fork implements today (P2079R10's parallel scheduler is a separate,
// not-yet-designed facility) -- it exists so that execution::task<T, Environment> has a real,
// working default scheduler_type value rather than a stub.
class __inline_sender;

// [exec.snd.expos]: inline-attrs<Tag>. The completion scheduler of an inline operation is the scheduler of the
// environment it is started in, its completion domain is the domain of that environment.
template <class _Tag>
struct __inline_attrs {
  template <class _Env>
    requires requires(const _Env& __env) { execution::get_scheduler(__env); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_scheduler_t<_Tag>, const _Env& __env) const
      noexcept(noexcept(execution::get_scheduler(__env))) {
    return execution::get_scheduler(__env);
  }

  template <class _Env>
    requires requires(const _Env& __env) { execution::get_domain(__env); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_domain_t<_Tag>, const _Env& __env) const
      noexcept(noexcept(execution::get_domain(__env))) {
    return execution::get_domain(__env);
  }
};

class inline_scheduler {
public:
  using scheduler_concept = scheduler_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr inline_scheduler() noexcept = default;

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const inline_scheduler&, const inline_scheduler&) noexcept =
      default;

  _LIBCPP_HIDE_FROM_ABI constexpr __inline_sender schedule() const noexcept;

  // Completes inline on whatever agent called start() -- the strongest guarantee
  // [intro.progress] defines, so this reports concurrent rather than run-loop-scheduler's
  // more conservative parallel (see <__execution/run_loop.h>'s identical query, which
  // genuinely queues work for a single dedicated draining thread instead of running it
  // wherever start() happens to be called).
  _LIBCPP_HIDE_FROM_ABI constexpr forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::concurrent;
  }

  // [exec.inline.scheduler]p2: sch.query(q, args...) is expression-equivalent to inline-attrs<set_value_t>().query(q, args...).
  template <class _Query, class... _Args>
    requires requires(_Query __q, _Args&&... __args) {
      __inline_attrs<set_value_t>().query(__q, std::forward<_Args>(__args)...);
    }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Query __q, _Args&&... __args) const
      noexcept(noexcept(__inline_attrs<set_value_t>().query(__q, std::forward<_Args>(__args)...))) {
    return __inline_attrs<set_value_t>().query(__q, std::forward<_Args>(__args)...);
  }
};

template <class _Rcvr>
class __inline_opstate {
public:
  using operation_state_concept = operation_state_tag;

  template <class _Receiver>
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __inline_opstate(_Receiver&& __rcvr)
      noexcept(is_nothrow_constructible_v<_Rcvr, _Receiver>) : __rcvr_(std::forward<_Receiver>(__rcvr)) {}

  // Movable (not just in-place-constructible): __task_scheduler_opstate_model (below) receives
  // an already-materialized opstate object through a forwarding-reference constructor
  // parameter, which is one layer removed from the guaranteed-copy-elision-eligible
  // "single prvalue return statement" shape most other operation states in this fork rely on
  // to get away with deleting move entirely (e.g. <__execution/continues_on.h>'s opstate) --
  // so an actual move constructor call happens here, not elision.
  _LIBCPP_HIDE_FROM_ABI constexpr __inline_opstate(__inline_opstate&&) noexcept = default;
  __inline_opstate(const __inline_opstate&)            = delete;
  __inline_opstate& operator=(const __inline_opstate&) = delete;
  __inline_opstate& operator=(__inline_opstate&&)      = delete;

  _LIBCPP_HIDE_FROM_ABI constexpr void start() & noexcept { execution::set_value(std::move(__rcvr_)); }

private:
  _Rcvr __rcvr_;
};

class __inline_sender {
public:
  using sender_concept = sender_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __inline_attrs<set_value_t> get_env() const noexcept { return {}; }

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr __inline_opstate<remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) const
      noexcept(is_nothrow_constructible_v<remove_cvref_t<_Rcvr>, _Rcvr>) {
    return __inline_opstate<remove_cvref_t<_Rcvr>>(std::forward<_Rcvr>(__rcvr));
  }

  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t()>{};
  }
};

_LIBCPP_HIDE_FROM_ABI constexpr __inline_sender inline_scheduler::schedule() const noexcept { return {}; }

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_INLINE_SCHEDULER_H
