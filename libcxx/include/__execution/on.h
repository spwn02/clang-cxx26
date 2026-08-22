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
#include <__execution/sender_adaptor_closure.h>
#include <__execution/starts_on.h>
#include <__functional/bind_back.h>
#include <__type_traits/decay.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.on]. The `on` sender adaptor has two forms ([exec.on]p1):
//   on(sch, sndr)          -- start sndr on sch, then transfer back to whichever scheduler was in
//                              effect when the `on` sender itself was started ([exec.on]p1.1/p7).
//   on(sndr, sch, closure) -- upon sndr's completion, transfer to sch, run the pipeable closure
//                              `closure` there, then transfer back to wherever sndr completed
//                              ([exec.on]p1.2/p8).
// `on` is itself a *pipeable* sender adaptor object ([exec.on]p2), so a 2-arg call `on(sch, x)` must
// disambiguate between the two forms based on whether `x` is a sender (form 1, in full) or a pipeable
// sender adaptor closure object (the *partial application* of form 2 -- `on(sch, closure)` produces a
// closure `c` such that `sndr | c` is `on(sndr, sch, closure)`, exactly the way <__execution/then.h>'s
// and <__execution/let.h>'s own single-arg overloads work, just with two bound arguments instead of
// one). [exec.on]p2.2/p2.3 spell this disambiguation out explicitly (ill-formed if the second argument
// is neither or both); here it falls out for free from `sender` and `__sender_adaptor_closure_object`
// (<__execution/sender_adaptor_closure.h>) already being mutually exclusive by construction, so ordinary
// overload-constraint SFINAE does the same job without needing to spell out the exclusion again.
//
// [exec.on]p6's transform_sender body computes both forms as:
//   if constexpr (scheduler<decltype(data)>) {
//     auto orig_sch = call-with-default(get_start_scheduler, not-a-scheduler(), env);
//     return continues_on(starts_on(data, child), std::move(orig_sch));
//   } else {
//     auto& [sch, closure] = data;
//     auto orig_sch = call-with-default(get_completion_scheduler<set_value_t>, not-a-scheduler(),
//                                        get_env(child), env);
//     return continues_on(closure(continues_on(child, sch)), orig_sch);
//   }
// `call-with-default` is a fallback wrapper (answer the query if present, else substitute a stub
// scheduler that fails cleanly) that exists so *forming* a transform_sender rewrite never hard-errors
// even when the query is unanswered -- a compile-time safety net for domain-based dispatch this fork's
// <__execution/domain.h> never invokes in the first place (default_domain::transform_sender always takes
// the identity branch, documented there). Since both sender classes below compute their composition
// directly inside connect() instead (env is available there, from the real receiver, unlike at CPO-call
// time -- this is exactly why `on`, unlike <__execution/starts_on.h>, needs real sender/opstate types
// rather than a pure call-time composition), the query is called directly, matching [exec.on]p7/p8's
// literal (non-transform_sender) operational wording exactly, with no `call-with-default` fallback. If
// nothing in the relevant environment answers the query, this is simply ill-formed -- matching the
// established pattern (e.g. <__execution/let.h>'s let-env, <__execution/domain.h>) of "soft-fail via
// SFINAE where the standard would soft-fail via call-with-default's stub, hard-require where the
// standard's own operational wording has no such fallback anyway".
struct on_t;

// A template parameter purely for the forward-declared-CPO-type ordering trick <__execution/then.h> and
// <__execution/let.h> established (see <__execution/continues_on.h>'s own __continues_on_sndr for the
// identical reasoning) -- only `on_t` ever instantiates either sender template in this file.
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

// [exec.on]p4's `on(sndr, sch, closure)` form: `data` is `product-type{sch, closure}` (here, a
// `tuple<_Sch, _Closure>`) and `child` is `sndr`, matching `make-sender(on, product-type{sch, closure},
// sndr)` literally.
template <class _Tag, class _Sndr, class _Sch, class _Closure>
class __on2_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  tuple<_Sch, _Closure> data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    auto& [__sch, __closure] = data;
    // [exec.on]p8.1: computed from sndr's own env, *not* the outer receiver's -- this is the scheduler
    // sndr itself completes on, unlike form 1's get_start_scheduler(get_env(rcvr)) (the scheduler in
    // effect when the whole `on` operation was started). get_env(child) must happen before child is
    // moved-from below, so this is its own statement rather than inlined into the return expression,
    // where argument evaluation order would be unspecified.
    auto __orig_sch = execution::get_completion_scheduler<set_value_t>(execution::get_env(child), execution::get_env(__rcvr));
    return execution::connect(
        execution::continues_on(
            std::move(__closure)(execution::continues_on(std::move(child), std::move(__sch))), std::move(__orig_sch)),
        std::forward<_Rcvr>(__rcvr));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  // Mirrors connect()'s composition exactly, at the type level -- see __on_sndr::get_completion_signatures
  // above for why this file computes signatures this way instead of the standard's operational spec.
  template <class _Self, class _Env>
    requires requires(const _Sndr& __child, const remove_cvref_t<_Env>& __env) {
      execution::get_completion_scheduler<set_value_t>(execution::get_env(__child), __env);
    }
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __orig_sch_t = decltype(execution::get_completion_scheduler<set_value_t>(
        execution::get_env(std::declval<const _Sndr&>()), std::declval<const remove_cvref_t<_Env>&>()));
    using __inner_t          = decltype(execution::continues_on(std::declval<_Sndr>(), std::declval<_Sch>()));
    using __closure_result_t = decltype(std::declval<_Closure>()(std::declval<__inner_t>()));
    using __composed_t = decltype(execution::continues_on(std::declval<__closure_result_t>(), std::declval<__orig_sch_t>()));
    return completion_signatures_of_t<__composed_t, remove_cvref_t<_Env>>{};
  }
};

struct on_t {
  // [exec.on]p3: on(sch, sndr), sndr a real sender.
  template <scheduler _Sch, sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sch&& __sch, _Sndr&& __sndr) const
      -> __on_sndr<on_t, decay_t<_Sch>, remove_cvref_t<_Sndr>> {
    return __on_sndr<on_t, decay_t<_Sch>, remove_cvref_t<_Sndr>>{
        {}, decay_t<_Sch>(std::forward<_Sch>(__sch)), std::forward<_Sndr>(__sndr)};
  }

  // [exec.on]p2's partial-application form of p4: on(sch, closure) -- closure, not a sender -- produces a
  // pipeable closure `c` such that `sndr | c` is on(sndr, sch, closure).
  template <scheduler _Sch, __sender_adaptor_closure_object _Closure>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sch&& __sch, _Closure&& __closure) const {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Sch>(__sch), std::forward<_Closure>(__closure)));
  }

  // [exec.on]p4: on(sndr, sch, closure), in full.
  template <sender _Sndr, scheduler _Sch, __sender_adaptor_closure_object _Closure>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Sch&& __sch, _Closure&& __closure) const
      -> __on2_sndr<on_t, remove_cvref_t<_Sndr>, decay_t<_Sch>, decay_t<_Closure>> {
    return __on2_sndr<on_t, remove_cvref_t<_Sndr>, decay_t<_Sch>, decay_t<_Closure>>{
        {},
        tuple<decay_t<_Sch>, decay_t<_Closure>>(std::forward<_Sch>(__sch), std::forward<_Closure>(__closure)),
        std::forward<_Sndr>(__sndr)};
  }
};

inline constexpr on_t on{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_ON_H
