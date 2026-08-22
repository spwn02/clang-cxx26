//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_CONTINUES_ON_H
#define _LIBCPP___EXECUTION_CONTINUES_ON_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/schedule.h>
#include <__execution/schedule_from.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <exception>
#include <tuple>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.continues.on]. continues_on(sndr, sch) starts sndr on the current execution agent, then -- once it
// completes -- transitions onto sch's associated execution resource before forwarding sndr's async result
// to the outer receiver. Per [exec.continues.on]p3, continues_on(sndr, sch) is expression-equivalent to
// make-sender(continues_on, sch, schedule_from(sndr)) -- so the sender this file builds always wraps its
// inner sender in a real <__execution/schedule_from.h> sender (identity-forwarding on this fork, per that
// file's own documented deviation), matching the literal wording rather than collapsing the wrapper away.
//
// Unlike <__execution/then.h>/<__execution/let.h>'s single-child shape, this adaptor's operation state owns
// *two* connected child operations simultaneously: the input sender (connected immediately, started first)
// and `schedule(sch)` (connected immediately but started only once the input sender completes). This is a
// hand-adaptation of the standard's own exposition-only `impls-for<continues_on_t>::get-state`/`::complete`
// and their `state-type`/`receiver-type` -- not routed through the draft's generic basic-sender/impls-for
// machinery, same as every other adaptor in this sub-plan (see the M3 entry in docs/CXX26_GAPS.md for why
// that engine isn't buildable on this fork yet).
struct continues_on_t;

// [exec.continues.on]p9's is-nothrow-decay-copy-sig<Tag(Args...)>: whether decay-copying every argument of
// one completion signature is guaranteed not to throw. Tag itself is not part of the check (a completion
// tag is always a stateless, trivially-constructible type).
template <class _Sig>
inline constexpr bool __continues_on_nothrow_sig = false;
template <class _Tag, class... _Args>
inline constexpr bool __continues_on_nothrow_sig<_Tag(_Args...)> =
    (is_nothrow_constructible_v<decay_t<_Args>, _Args> && ...);

// [exec.continues.on]p9's as-tuple<Tag(Args...)>: decayed-tuple<Tag, Args...>, i.e. the tag itself is
// stored alongside the (decayed) result datums -- unlike <__execution/let.h>'s __decayed_tuple-based
// storage, which never needs the tag since a let_value/let_error/let_stopped opstate only ever intercepts
// one fixed completion function. continues_on captures *any* of the input sender's completions (value,
// error, or stopped) and must remember which one fired to replay it correctly once scheduling completes.
template <class _Sig>
struct __continues_on_as_tuple;
template <class _Tag, class... _Args>
struct __continues_on_as_tuple<_Tag(_Args...)> {
  using type = tuple<_Tag, decay_t<_Args>...>;
};

template <class _List>
struct __continues_on_dedup;
template <class... _Ts>
struct __continues_on_dedup<type_list<_Ts...>> {
  using type = __dedup_type_list_t<_Ts...>;
};

template <class _List>
struct __continues_on_to_sigs;
template <class... _Sigs>
struct __continues_on_to_sigs<type_list<_Sigs...>> {
  using type = completion_signatures<_Sigs...>;
};

// [exec.continues.on]p9's variant_t: variant<monostate, as-tuple<Sigs>..., error-completion...> (deduped),
// where error-completion contributes tuple<set_error_t, exception_ptr> iff *some* completion signature
// isn't guaranteed nothrow-decay-copyable (a single shared alternative for every signature that could throw
// during capture, not one per signature -- this is a storage-layout optimization the standard's own wording
// makes explicitly, distinct from the per-signature nothrow check <__continues_on_child_one> below uses to
// decide whether a *given* signature's own replayed completion needs an exception_ptr alternative).
template <class _Completions>
struct __continues_on_variant_impl;
template <class... _Sigs>
struct __continues_on_variant_impl<completion_signatures<_Sigs...>> {
  using __tuples                    = type_list<typename __continues_on_as_tuple<_Sigs>::type...>;
  static constexpr bool __all_nothrow = (__continues_on_nothrow_sig<_Sigs> && ...);
  using __err_list  = __conditional_t<__all_nothrow, type_list<>, type_list<tuple<set_error_t, exception_ptr>>>;
  using __gathered  = typename __concat_type_lists<__tuples, __err_list>::type;
  using __deduped   = typename __continues_on_dedup<__gathered>::type;

  template <class>
  struct __to_variant;
  template <class... _Ts>
  struct __to_variant<type_list<_Ts...>> {
    using type = variant<monostate, _Ts...>;
  };
  using type = typename __to_variant<__deduped>::type;
};

template <class _Completions>
using __continues_on_variant_t = typename __continues_on_variant_impl<_Completions>::type;

// The *observable* completion signatures continues_on's own sender advertises, contributed by the input
// sender: each of its signatures Tag(Args...) is replayed (after decay-copy-then-redispatch) as
// Tag(decay_t<Args>...), plus set_error_t(exception_ptr) for that particular signature if it isn't
// statically nothrow-decay-copyable (per-signature, unlike the shared storage alternative above -- mirrors
// <__execution/then.h>'s own TRY-SET-VALUE nothrow-conditional error contribution).
template <class _Sig>
struct __continues_on_child_one;
template <class _Tag, class... _Args>
struct __continues_on_child_one<_Tag(_Args...)> {
  using __value_sig = _Tag(decay_t<_Args>...);
  using type = __conditional_t<__continues_on_nothrow_sig<_Tag(_Args...)>, type_list<__value_sig>,
                                type_list<__value_sig, set_error_t(exception_ptr)>>;
};

// schedule(sch)'s own set_value_t() completion never propagates as-is (it only triggers the captured-result
// redispatch); its set_error_t/set_stopped_t completions (a scheduling failure) propagate directly.
template <class _Sig>
struct __continues_on_sched_one {
  using type = type_list<_Sig>;
};
template <class... _Args>
struct __continues_on_sched_one<set_value_t(_Args...)> {
  using type = type_list<>;
};

template <class _Completions>
struct __continues_on_child_gather;
template <class... _Sigs>
struct __continues_on_child_gather<completion_signatures<_Sigs...>> {
  using type = typename __concat_type_lists<typename __continues_on_child_one<_Sigs>::type...>::type;
};

template <class _Completions>
struct __continues_on_sched_gather;
template <class... _Sigs>
struct __continues_on_sched_gather<completion_signatures<_Sigs...>> {
  using type = typename __concat_type_lists<typename __continues_on_sched_one<_Sigs>::type...>::type;
};

template <class _ChildCompletions, class _SchedCompletions>
using __continues_on_signatures_t = typename __continues_on_to_sigs<typename __continues_on_dedup<
    typename __concat_type_lists<typename __continues_on_child_gather<_ChildCompletions>::type,
                                  typename __continues_on_sched_gather<_SchedCompletions>::type>::type>::type>::type;

template <class _Sch, class _Sndr, class _Rcvr>
class __continues_on_opstate;

// [exec.continues.on]p9's `receiver-type`: the receiver used to connect `schedule(sch)`. Stores a direct
// `_Rcvr&` (not reached through `__state_`) for `get_env()`, matching <__execution/let.h>'s own documented
// incomplete-type fix: `connect_result_t<schedule_result_t<Sch>, receiver_t>`, computed inside the
// still-incomplete `__continues_on_opstate`, transitively *calls* this receiver's `get_env()` body (via
// <__execution/connect.h>'s own auto-returning machinery) as part of forming that type -- a body reaching
// back through `__state_` into the not-yet-complete opstate would hard-error there. `set_value()` (which
// does need `__state_`, to dispatch the captured result) is a non-template ordinary member function whose
// body isn't instantiated until actually called, well after the opstate is complete, so it has no such
// constraint.
template <class _Sch, class _Sndr, class _Rcvr>
class __continues_on_sched_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __continues_on_sched_rcvr(__continues_on_opstate<_Sch, _Sndr, _Rcvr>* __state,
                                                              _Rcvr& __rcvr) noexcept
      : __state_(__state), __rcvr_(__rcvr) {}

  _LIBCPP_HIDE_FROM_ABI constexpr void set_value() && noexcept { __state_->__on_sched_value(); }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(__rcvr_));
  }

private:
  __continues_on_opstate<_Sch, _Sndr, _Rcvr>* __state_;
  _Rcvr& __rcvr_;
};

// The receiver used to connect the input sender (`schedule_from(sndr)`, per this file's own top note).
// Same direct-`_Rcvr&`-for-get_env() shape as __continues_on_sched_rcvr above, for the same reason:
// `connect_result_t<Sndr, child_rcvr_t>` is also computed inside the still-incomplete opstate.
template <class _Sch, class _Sndr, class _Rcvr>
class __continues_on_child_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __continues_on_child_rcvr(__continues_on_opstate<_Sch, _Sndr, _Rcvr>* __state,
                                                              _Rcvr& __rcvr) noexcept
      : __state_(__state), __rcvr_(__rcvr) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    __state_->__on_child_complete(set_value_t{}, std::forward<_Args>(__args)...);
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    __state_->__on_child_complete(set_error_t{}, std::forward<_Err>(__err));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { __state_->__on_child_complete(set_stopped_t{}); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(__rcvr_));
  }

private:
  __continues_on_opstate<_Sch, _Sndr, _Rcvr>* __state_;
  _Rcvr& __rcvr_;
};

template <class _Sch, class _Sndr, class _Rcvr>
class __continues_on_opstate {
  using __sched_rcvr_t  = __continues_on_sched_rcvr<_Sch, _Sndr, _Rcvr>;
  using __child_rcvr_t  = __continues_on_child_rcvr<_Sch, _Sndr, _Rcvr>;
  using __sched_sndr_t  = schedule_result_t<_Sch>;
  using __sched_op_t    = connect_result_t<__sched_sndr_t, __sched_rcvr_t>;
  using __child_op_t    = connect_result_t<_Sndr, __child_rcvr_t>;
  using __child_sigs    = completion_signatures_of_t<_Sndr, __fwd_env<env_of_t<_Rcvr>>>;
  using __async_result_t = __continues_on_variant_t<__child_sigs>;

  _Rcvr __rcvr_;
  __async_result_t __async_result_;
  __child_op_t __child_op_;
  __sched_op_t __sched_op_;

  friend class __continues_on_sched_rcvr<_Sch, _Sndr, _Rcvr>;
  friend class __continues_on_child_rcvr<_Sch, _Sndr, _Rcvr>;

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __continues_on_opstate(_Sch&& __sch, _Sndr&& __sndr, _Rcvr&& __rcvr)
      : __rcvr_(std::move(__rcvr)),
        __child_op_(execution::connect(std::move(__sndr), __child_rcvr_t(this, __rcvr_))),
        __sched_op_(execution::connect(execution::schedule(__sch), __sched_rcvr_t(this, __rcvr_))) {}

  __continues_on_opstate(const __continues_on_opstate&)            = delete;
  __continues_on_opstate& operator=(const __continues_on_opstate&) = delete;

  // [exec.continues.on]p12: start sndr on the current execution agent.
  _LIBCPP_HIDE_FROM_ABI constexpr void start() & noexcept { execution::start(__child_op_); }

  // Called by __continues_on_child_rcvr (a friend, above) when the input sender completes: capture its
  // result (or, if capturing throws, an exception_ptr) into __async_result_, then start scheduling.
  //
  // The `if constexpr (requires{...})` guard below is load-bearing, not defensive styling: this fork's
  // <__execution/run_loop.h> `__run_loop_opstate::__execute()` decides at *runtime* whether to call
  // set_value() or set_stopped() on its receiver, but the decision is a plain `if`, not `if constexpr` --
  // so both branches must be well-formed *at compile time* for whatever receiver type ends up connected to
  // schedule(sch), regardless of whether the environment's stop token could ever actually report
  // stop_requested(). When continues_on's own sched_rcvr (below) sits, transitively, in front of *another*
  // adaptor whose own scheduling hop also runs through run-loop-scheduler (e.g. <__execution/starts_on.h>'s
  // internal continues_on(just(), sch) composition, reached through <__execution/on.h>), that forced
  // set_stopped() call propagates outward through every plain-forwarding receiver in between (then.h's and
  // let.h's own "not intercepted" branches are exactly this: a type-independent direct forward, always
  // well-formed) until it reaches *this* receiver -- the first one in the chain whose set_stopped() handling
  // is genuinely type-dependent (it captures into __async_result_, sized only for the tag/arg combinations
  // the child sender actually advertises in its own completion signatures). For a child like just()/just(42)
  // that never advertises set_stopped_t() at all, `tuple<set_stopped_t>` simply isn't an alternative of
  // __async_result_t, and emplacing it would be ill-formed. This is provably unreachable at runtime for that
  // case (unstoppable_token<never_stop_token> is true, so __execute()'s stopped branch never actually
  // executes) -- so where the "normal" capture-then-schedule path isn't well-formed, skip the scheduling hop
  // entirely and complete directly on the current agent instead of hard-erroring the whole program.
  template <class _Tag, class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __on_child_complete(_Tag __tag, _Args&&... __args) {
    using __result_t = tuple<_Tag, decay_t<_Args>...>;
    if constexpr (requires { __async_result_.template emplace<__result_t>(__tag, std::forward<_Args>(__args)...); }) {
      if constexpr (__continues_on_nothrow_sig<_Tag(_Args...)>) {
        __async_result_.template emplace<__result_t>(__tag, std::forward<_Args>(__args)...);
      } else {
        try {
          __async_result_.template emplace<__result_t>(__tag, std::forward<_Args>(__args)...);
        } catch (...) {
          __async_result_.template emplace<tuple<set_error_t, exception_ptr>>(set_error_t{}, std::current_exception());
        }
      }
      execution::start(__sched_op_);
    } else {
      __tag(std::move(__rcvr_), std::forward<_Args>(__args)...);
    }
  }

  // Called by __continues_on_sched_rcvr::set_value() (defined below, after this class, since it needs
  // __async_result_ complete) once scheduling completes: replay the captured result on the real receiver.
  _LIBCPP_HIDE_FROM_ABI constexpr void __on_sched_value() {
    std::visit(
        [this]<class _Tuple>(_Tuple& __result) noexcept {
          if constexpr (!same_as<_Tuple, monostate>) {
            auto& [__tag, ...__args] = __result;
            __tag(std::move(__rcvr_), std::move(__args)...);
          }
        },
        __async_result_);
  }
};

// An aggregate with public `tag`/`data`/`child` members, matching the (tag, data, ...children) shape
// tag_of_t (<__execution/sender.h>) decomposes via structured bindings. `_Tag` is a template parameter
// purely so this class can be fully defined -- and named in `continues_on_t::operator()`'s trailing return
// type -- before `continues_on_t` itself is complete, mirroring <__execution/then.h>'s and
// <__execution/let.h>'s identical forward-declared-CPO-type ordering trick; only `continues_on_t` ever
// instantiates it (unlike then.h/let.h, which share one sender template across three CPOs).
template <class _Tag, class _Sch, class _Sndr>
class __continues_on_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  _Sch data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) &&
      -> __continues_on_opstate<_Sch, _Sndr, remove_cvref_t<_Rcvr>> {
    return __continues_on_opstate<_Sch, _Sndr, remove_cvref_t<_Rcvr>>(
        std::move(data), std::move(child), std::forward<_Rcvr>(__rcvr));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated attribute object
  // equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  // [exec.continues.on]p6 (check-types) is not implemented -- same P3068 constexpr-exceptions gap as every
  // other adaptor in this sub-plan; a Sch/Sndr combination that doesn't actually work simply makes this
  // whole overload not participate (via the requires-clause below) rather than reporting a dedicated
  // diagnostic.
  template <class _Self, class _Env>
    requires sender_in<_Sndr, __fwd_env<remove_cvref_t<_Env>>> &&
             sender_in<schedule_result_t<_Sch>, __fwd_env<remove_cvref_t<_Env>>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __child_sigs = completion_signatures_of_t<_Sndr, __fwd_env<remove_cvref_t<_Env>>>;
    using __sched_sigs = completion_signatures_of_t<schedule_result_t<_Sch>, __fwd_env<remove_cvref_t<_Env>>>;
    return __continues_on_signatures_t<__child_sigs, __sched_sigs>{};
  }
};

struct continues_on_t {
  template <sender _Sndr, scheduler _Sch>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Sch&& __sch) const
      -> __continues_on_sndr<continues_on_t, decay_t<_Sch>, __schedule_from_sndr<remove_cvref_t<_Sndr>>> {
    return __continues_on_sndr<continues_on_t, decay_t<_Sch>, __schedule_from_sndr<remove_cvref_t<_Sndr>>>{
        {}, decay_t<_Sch>(std::forward<_Sch>(__sch)), execution::schedule_from(std::forward<_Sndr>(__sndr))};
  }
};

inline constexpr continues_on_t continues_on{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_CONTINUES_ON_H
