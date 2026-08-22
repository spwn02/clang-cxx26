//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_ON_H
#define _LIBCPP___EXECUTION_ON_H

#include <__config>
#include <__execution/continues_on.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_scheduler.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__execution/starts_on.h>
#include <__type_traits/decay.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.on]. Only the `on(sch, sndr)` form ([exec.on]p1.1: run sndr on sch, then transfer back to the
// scheduler that was in effect when the `on` sender was started) is implemented. The pipeable-closure form,
// `on(sndr, sch, closure)` ([exec.on]p1.2), and the argument-disambiguation rules that let a single 2-arg
// call resolve to either form ([exec.on]p2.1-2.3) are not -- deferred to a future M5 session; see
// docs/CXX26_GAPS.md's session log for the exact cut point.
//
// [exec.on]p6's transform_sender body for the scheduler-argument branch is:
//   auto orig_sch = call-with-default(get_start_scheduler, not-a-scheduler(), env);
//   return continues_on(starts_on(data, child), std::move(orig_sch));
// `call-with-default` is a fallback wrapper (answer the query if present, else substitute a stub scheduler
// that fails cleanly) that exists so *forming* a transform_sender rewrite never hard-errors even when
// get_start_scheduler is unanswered -- a compile-time safety net for domain-based dispatch this fork's
// <__execution/domain.h> never invokes in the first place (default_domain::transform_sender always takes
// the identity branch, documented there). Since this file computes the composition directly inside
// connect() instead (env is available there, from the real receiver, unlike at CPO-call time -- this is
// exactly why `on`, unlike <__execution/starts_on.h>, needs a real sender/opstate rather than a pure
// call-time composition: starts_on's rewrite needs no env at all), get_start_scheduler(get_env(rcvr)) is
// called directly, matching [exec.on]p7's literal (non-transform_sender) operational wording exactly. If
// nothing in the receiver's environment chain answers get_start_scheduler, this is simply ill-formed --
// matching the currently-established pattern (e.g. <__execution/let.h>'s let-env, <__execution/domain.h>)
// of "soft-fail via SFINAE where the standard would soft-fail via call-with-default's stub, hard-require
// where the standard's own operational wording has no such fallback anyway".
struct on_t;

// A template parameter purely for the forward-declared-CPO-type ordering trick <__execution/then.h> and
// <__execution/let.h> established (see <__execution/continues_on.h>'s own __continues_on_sndr for the
// identical reasoning) -- only `on_t` ever instantiates this.
template <class _Tag, class _Sch, class _Sndr>
class __on_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  _Sch data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    auto __orig_sch = execution::get_start_scheduler(execution::get_env(__rcvr));
    return execution::connect(
        execution::continues_on(execution::starts_on(std::move(data), std::move(child)), std::move(__orig_sch)),
        std::forward<_Rcvr>(__rcvr));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated attribute object
  // equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  // Mirrors connect()'s composition exactly, at the type level: get_start_scheduler is looked up against
  // the plain (not FWD-ENV-wrapped) Env, matching [exec.on]p7's literal get_start_scheduler(get_env(rcvr))
  // -- connect() above forwards the outer receiver straight into the composed sender with no intermediate
  // FWD-ENV-wrapping receiver of its own (unlike every single-child adaptor elsewhere in this sub-plan), so
  // the composed sender's own env_of_t genuinely is the outer Rcvr's plain env, not a filtered view of it.
  template <class _Self, class _Env>
    requires requires(const remove_cvref_t<_Env>& __env) { execution::get_start_scheduler(__env); }
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __orig_sch_t = decltype(execution::get_start_scheduler(std::declval<const remove_cvref_t<_Env>&>()));
    using __composed_t = decltype(execution::continues_on(
        execution::starts_on(std::declval<_Sch>(), std::declval<_Sndr>()), std::declval<__orig_sch_t>()));
    return completion_signatures_of_t<__composed_t, remove_cvref_t<_Env>>{};
  }
};

struct on_t {
  template <scheduler _Sch, sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sch&& __sch, _Sndr&& __sndr) const
      -> __on_sndr<on_t, decay_t<_Sch>, remove_cvref_t<_Sndr>> {
    return __on_sndr<on_t, decay_t<_Sch>, remove_cvref_t<_Sndr>>{
        {}, decay_t<_Sch>(std::forward<_Sch>(__sch)), std::forward<_Sndr>(__sndr)};
  }
};

inline constexpr on_t on{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_ON_H
