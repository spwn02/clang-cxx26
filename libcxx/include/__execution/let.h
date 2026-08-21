//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_LET_H
#define _LIBCPP___EXECUTION_LET_H

#include <__concepts/constructible.h>
#include <__concepts/invocable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/movable_value.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__functional/bind_back.h>
#include <__functional/invoke.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
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

// [exec.let]. let_value/let_error/let_stopped transform a sender's value/error/stopped
// completion, respectively, into a *new child asynchronous operation*: the intercepted
// completion's result datums are passed to `fn`, which returns a new sender that is
// connected and started, and it's *that* sender's completion the overall operation waits
// on. Unlike <__execution/then.h>'s family (which just invokes `fn` and completes
// synchronously with its result), this needs real operation-state lifetime management: the
// type of both the args and the continuation operation state depend on *which* of possibly
// several signatures the child completes with, so both are stored in a `variant` sized to
// every possibility -- mirroring the standard's own exposition-only `let-state` (this is a
// hand-written, concrete adaptation of it, not routed through impls-for/basic-sender, same
// as every other adaptor in this sub-plan; see the M3 entry in docs/CXX26_GAPS.md for why).
//
// **Deliberate simplification, documented once here (applies to all three CPOs):**
// [exec.let]p2's `let-env(sndr, env)` is the first well-formed of three branches:
// (2.1) SCHED-ENV(get_completion_scheduler<set-cpo>(get_env(sndr), FWD-ENV(env))) -- giving
//       the continuation sender the *same scheduler* the child completed on;
// (2.2) MAKE-ENV(get_domain, get_completion_domain<set-cpo>(...)) -- same idea, for domains;
// (2.3) (void(sndr), env<>{}) -- the unconditionally-well-formed fallback.
// This implementation always takes (2.3). Branch 2.2 needs `MAKE-ENV`/`get_completion_domain`
// (the latter is a query-only stub in <__execution/domain.h> with no `operator()`, per the M2
// deviation) -- genuinely not buildable yet. Branch 2.1 is *not* similarly blocked: M4 already
// built `get_completion_scheduler_t`, and `SCHED-ENV(sched)` is nothing more than
// `prop<get_scheduler_t, Sched>` (M1) -- a future session wiring up `continues_on`/`on`/
// `schedule_from` later in this same milestone (which *do* need scheduler affinity to be
// meaningful) should reach for branch 2.1 there rather than assuming it's blocked. It's
// skipped here only because nothing in let_value/let_error/let_stopped's own contract
// requires it. With let-env always env<>{}, [exec.let]p8's `receiver2::get_env()` --
// "env.query(q,...) if valid, else get_env(rcvr).query(q,...) if q is forwarding" -- collapses
// exactly to <__execution/fwd_env.h>'s FWD-ENV(get_env(rcvr)): env<>{} never has a valid
// query, so the first branch is always ill-formed and every call falls through to the
// forwarding-query-gated second branch, which is FWD-ENV's entire definition. Reused directly
// below as `__let_cont_rcvr`, rather than reimplementing the two-branch dispatch.
struct let_value_t;
struct let_error_t;
struct let_stopped_t;

// [exec.let]p2's `set-cpo`: which completion-function tag (set_value_t/set_error_t/
// set_stopped_t) `_Tag` (let_value_t/let_error_t/let_stopped_t) intercepts.
template <class _Tag>
using __let_set_cpo_t =
    __conditional_t<same_as<_Tag, let_value_t>,
                     set_value_t,
                     __conditional_t<same_as<_Tag, let_error_t>, set_error_t, set_stopped_t>>;

// Exposition-only `emplace-from` ([exec.let]p10's own usage of this exact idiom): operation
// states are neither movable nor copyable (per [exec.opstate]), so `variant::emplace<T>(args)`
// -- which materializes `args` into a temporary bound to a forwarding-reference parameter,
// then move-constructs T from it -- cannot construct one in place from a factory call
// returning T by value (the intervening materialization defeats guaranteed copy elision).
// Wrapping the factory in a type with an `operator T()` conversion sidesteps this: `T(ef)`
// where `ef` converts to a *prvalue* T is direct-initialization from a prvalue of the same
// type, which mandatory elision *does* cover.
template <class _Fn>
struct __emplace_from {
  _Fn __fn_;
  using __result_t = invoke_result_t<_Fn&>;
  _LIBCPP_HIDE_FROM_ABI constexpr operator __result_t() && { return std::move(__fn_)(); }
};
template <class _Fn>
__emplace_from(_Fn) -> __emplace_from<_Fn>;

// args_variant_t's element-building step ([exec.let]p12): variant<monostate, Ts...> with Ts
// deduped. A standalone alias template (not nested in __let_opstate) so it can be passed
// directly as __gather_signatures's `_Variant` template-template argument, the same way
// <__execution/get_completion_signatures.h>'s own __variant_or_empty is.
template <class... _Ts>
struct __let_args_variant_impl {
  template <class>
  struct __to_variant;
  template <class... _Us>
  struct __to_variant<type_list<_Us...>> {
    using type = variant<monostate, _Us...>;
  };
  using type = typename __to_variant<__dedup_type_list_t<_Ts...>>::type;
};
template <class... _Ts>
using __let_args_variant = typename __let_args_variant_impl<_Ts...>::type;

// [exec.let]p9/p12's completion-signature transform: every non-intercepted signature passes
// through unchanged; an intercepted `set-cpo(Args...)` is replaced by the *continuation*
// sender's own completion signatures (invoke_result_t<Fn, decay_t<Args>&...>, computed
// against FWD-ENV(Env) per this file's let-env simplification above), plus
// set_error_t(exception_ptr) unless both decay-copying Args and invoking Fn are statically
// nothrow (mirrors <__execution/then.h>'s TRY-SET-VALUE nothrow check; see __let_opstate's
// __intercept below for why the *runtime* path doesn't mirror this exactly).
template <class _SetCpo, class _Fn, class _Env>
class __let_sig_transform {
public:
  template <class _Sig>
  struct __one {
    using type = type_list<_Sig>;
  };

  template <class _Sigs>
  struct __sigs_to_list;
  template <class... _Ss>
  struct __sigs_to_list<completion_signatures<_Ss...>> {
    using type = type_list<_Ss...>;
  };

  template <class... _Args>
  struct __one<_SetCpo(_Args...)> {
    using __cont_sndr = invoke_result_t<_Fn, decay_t<_Args>&...>;
    using __cont_list = typename __sigs_to_list<completion_signatures_of_t<__cont_sndr, __fwd_env<_Env>>>::type;
    static constexpr bool __nothrow =
        is_nothrow_constructible_v<__decayed_tuple<_Args...>, _Args...> && is_nothrow_invocable_v<_Fn, decay_t<_Args>&...>;
    using type = __conditional_t<__nothrow,
                                  __cont_list,
                                  typename __concat_type_lists<__cont_list, type_list<set_error_t(exception_ptr)>>::type>;
  };

  template <class _List>
  struct __dedup;
  template <class... _Ts>
  struct __dedup<type_list<_Ts...>> {
    using type = __dedup_type_list_t<_Ts...>;
  };

  template <class _List>
  struct __to_completion_signatures;
  template <class... _Sigs>
  struct __to_completion_signatures<type_list<_Sigs...>> {
    using type = completion_signatures<_Sigs...>;
  };

  template <class _Completions>
  struct __impl;
  template <class... _Fns>
  struct __impl<completion_signatures<_Fns...>> {
    using __gathered = typename __concat_type_lists<typename __one<_Fns>::type...>::type;
    using type        = typename __to_completion_signatures<typename __dedup<__gathered>::type>::type;
  };
};

template <class _Tag, class _Fn, class _Env, class _Completions>
using __let_signatures_t =
    typename __let_sig_transform<__let_set_cpo_t<_Tag>, _Fn, _Env>::template __impl<_Completions>::type;

// [exec.let]p8's `receiver2`, specialized to this implementation's env<>{}-only let-env (see
// the deviation note above): the receiver used to connect the sender `fn` returns, forwarding
// every completion straight through to the outer receiver, with FWD-ENV(get_env(rcvr)) as its
// environment. Stores its own reference to the *outer* receiver (not a pointer to
// __let_opstate) since it outlives the child operation state entirely -- unlike
// __let_child_rcvr below, it has no need to reach back into __let_opstate's other storage.
template <class _Rcvr>
class __let_cont_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __let_cont_rcvr(_Rcvr& __rcvr) noexcept : __rcvr_(__rcvr) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    execution::set_value(std::move(__rcvr_), std::forward<_Args>(__args)...);
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(__rcvr_));
  }

private:
  _Rcvr& __rcvr_;
};

// [exec.let]p8's `let-state::receiver`: the receiver used to connect the *original* child
// sender `sndr`. Per the standard's literal wording this one's `get_env()` returns
// `execution::get_env(rcvr)` **unwrapped** -- not FWD-ENV-wrapped like `__let_cont_rcvr`
// above -- because let-state overrides `impls-for`'s `get-state` entirely rather than using
// default-impls' generic single-child-adaptor behavior ([exec.adapt.general]p3.4, the rule
// <__execution/then.h>'s own `__then_rcvr::get_env` follows), so that general rule simply
// doesn't apply here.
//
// Stores its own direct `_Rcvr&` (mirroring the standard's own `receiver` class, which has
// both a `let-state&` *and* a `Rcvr&` member, not just the former) rather than reaching for
// `__state_->__rcvr_` -- this matters, not just mirrors: `connect_result_t<_Sndr,
// __let_child_rcvr<...>>` is computed inside `__let_opstate` while that class is still being
// defined, and doing so calls `execution::get_env` on a *value* of this receiver type deep
// inside <__execution/connect.h>'s own `auto`-returning (not trailing-decltype) machinery --
// which genuinely compiles this class's `get_env()` body right then, not merely probes its
// signature. A body that touches `_OpState` (even via an explicit, non-placeholder return
// type) would need `_OpState` complete at that point, which it isn't yet. A body that only
// touches this receiver's own already-complete `_Rcvr&` member has no such dependency.
// `__state_` remains solely for the interception dispatch in set_value/set_error/set_stopped,
// which -- being member *function templates* -- are only instantiated when actually called,
// well after __let_opstate is complete.
template <class _OpState, class _Rcvr>
class __let_child_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __let_child_rcvr(_OpState* __state, _Rcvr& __rcvr) noexcept
      : __state_(__state), __rcvr_(__rcvr) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    __state_->__on_value(std::forward<_Args>(__args)...);
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    __state_->__on_error(std::forward<_Err>(__err));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { __state_->__on_stopped(); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept { return execution::get_env(__rcvr_); }

private:
  _OpState* __state_;
  _Rcvr& __rcvr_;
};

template <class _Tag, class _Fn, class _Sndr, class _Rcvr>
class __let_opstate {
  using __set_cpo      = __let_set_cpo_t<_Tag>;
  using __child_rcvr_t = __let_child_rcvr<__let_opstate, _Rcvr>;
  using __cont_rcvr_t  = __let_cont_rcvr<_Rcvr>;
  using __child_op_t   = connect_result_t<_Sndr, __child_rcvr_t>;

  // completion_signatures_of_t<Sndr, FWD-ENV-T(env_of_t<Rcvr>)>, per [exec.let]p12 -- note
  // this is FWD-ENV-wrapped even though the *runtime* __child_rcvr_t::get_env() above is not;
  // the standard specifies these two independently (the type-level enumeration of "what could
  // the child complete with" uses the conservative forwarding-only view, matching every other
  // adaptor's completion-signature computation in this sub-plan, while the concrete receiver
  // actually connected exposes the full outer environment per p8's literal wording).
  using __child_sigs = completion_signatures_of_t<_Sndr, __fwd_env<env_of_t<_Rcvr>>>;

  template <class... _Args>
  using __cont_sndr_for = invoke_result_t<_Fn, decay_t<_Args>&...>;
  template <class... _Args>
  using __cont_op_for = connect_result_t<__cont_sndr_for<_Args...>, __cont_rcvr_t>;

  // args_variant_t ([exec.let]p12): variant<monostate, decayed-tuple<Args>...> over every
  // __set_cpo(Args...) signature the child might complete with (deduped).
  using __args_variant_t = __gather_signatures<__set_cpo, __child_sigs, __decayed_tuple, __let_args_variant>;

  // ops_variant_t ([exec.let]p13): variant<monostate, child_op_t, continuation-op-per-
  // intercepted-signature...> (deduped -- required for correctness, not just compactness:
  // __intercept below emplaces by *type*, which is ill-formed if two alternatives collide).
  using __cont_ops_list = __gather_signatures<__set_cpo, __child_sigs, __cont_op_for, type_list>;

  template <class _List>
  struct __ops_variant_from;
  template <class... _Ts>
  struct __ops_variant_from<type_list<_Ts...>> {
    template <class>
    struct __to_variant;
    template <class... _Us>
    struct __to_variant<type_list<_Us...>> {
      using type = variant<_Us...>;
    };
    using type = typename __to_variant<__dedup_type_list_t<monostate, __child_op_t, _Ts...>>::type;
  };
  using __ops_variant_t = typename __ops_variant_from<__cont_ops_list>::type;

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __intercept(_Args&&... __args) {
    // Unconditionally guarded, unlike <__execution/then.h>'s nothrow fast path: besides
    // decay-copying __args and invoking __fn_ (which then.h's equivalent check already
    // covers), this also calls execution::connect() on the continuation sender, and no
    // sender's connect() in this tree is declared noexcept (confirmed: just.h, then.h,
    // read_env.h, run_loop.h all return plain `auto`/a named type with no noexcept-specifier)
    // -- so a static nothrow check here would almost never trigger the fast path anyway, and
    // *would* be a real soundness gap for any future sender whose connect() can genuinely
    // throw. get_completion_signatures (via __let_sig_transform above) still uses the
    // then-style static nothrow check (args-construction + fn-invocation only) to decide
    // whether to advertise set_error_t(exception_ptr); this makes the runtime strictly safer
    // than what's advertised (it always catches), never the reverse, which is the direction
    // that matters.
    try {
      __start_continuation(std::forward<_Args>(__args)...);
    } catch (...) {
      execution::set_error(std::move(__rcvr_), std::current_exception());
    }
  }

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __start_continuation(_Args&&... __args) {
    using __args_t      = __decayed_tuple<_Args...>;
    using __cont_sndr_t = __cont_sndr_for<_Args...>;
    using __cont_op_t   = __cont_op_for<_Args...>;
    // Decay-copy the child's result datums into __args_ *before* tearing down the child
    // operation state below: __args (this function's parameter pack) are references into
    // storage owned by that operation state (e.g. a __just_opstate's own tuple), which
    // __ops_.emplace<monostate>() is about to destroy.
    auto& __tuple = __args_.template emplace<__args_t>(std::forward<_Args>(__args)...);
    // Drop the (now-finished) child operation state. Safe even though we're still inside a
    // call chain that originated from *its* set_value/set_error/set_stopped: that call landed
    // on __child_rcvr_t, a value stored inside the operation state being destroyed here, not
    // on `this` (__let_opstate itself lives independently) -- nothing below touches the
    // destroyed receiver or operation state again.
    __ops_.template emplace<monostate>();
    auto&& __sndr2 = std::apply(std::move(__fn_), __tuple);
    __ops_.template emplace<__cont_op_t>(__emplace_from{[&] {
      return execution::connect(std::forward<__cont_sndr_t>(__sndr2), __cont_rcvr_t(__rcvr_));
    }});
    execution::start(std::get<__cont_op_t>(__ops_));
  }

  _Fn __fn_;
  _Rcvr __rcvr_;
  __args_variant_t __args_;
  __ops_variant_t __ops_;

  template <class, class>
  friend class __let_child_rcvr;

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __let_opstate(_Fn&& __fn, _Sndr&& __sndr, _Rcvr&& __rcvr)
      : __fn_(std::move(__fn)),
        __rcvr_(std::move(__rcvr)),
        __ops_(in_place_type<__child_op_t>, __emplace_from{[&] {
                 return execution::connect(std::move(__sndr), __child_rcvr_t(this, __rcvr_));
               }}) {}

  __let_opstate(const __let_opstate&)            = delete;
  __let_opstate& operator=(const __let_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI constexpr void start() & noexcept { execution::start(std::get<__child_op_t>(__ops_)); }

  // Called by __let_child_rcvr (a friend, above), which lands set_value/set_error/set_stopped
  // from the child sender here and needs to decide whether to intercept.
  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __on_value(_Args&&... __args) {
    if constexpr (same_as<__set_cpo, set_value_t>) {
      __intercept(std::forward<_Args>(__args)...);
    } else {
      execution::set_value(std::move(__rcvr_), std::forward<_Args>(__args)...);
    }
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void __on_error(_Err&& __err) {
    if constexpr (same_as<__set_cpo, set_error_t>) {
      __intercept(std::forward<_Err>(__err));
    } else {
      execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
    }
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void __on_stopped() {
    if constexpr (same_as<__set_cpo, set_stopped_t>) {
      __intercept();
    } else {
      execution::set_stopped(std::move(__rcvr_));
    }
  }
};

// An aggregate with public `tag`/`data`/`child` members, matching the (tag, data, ...children)
// shape tag_of_t (<__execution/sender.h>) decomposes via structured bindings. Not routed
// through the draft's generic basic-sender/impls-for machinery: see the M3 entry in
// docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet.
template <class _Tag, class _Fn, class _Sndr>
class __let_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  _Fn data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) &&
      -> __let_opstate<_Tag, _Fn, _Sndr, remove_cvref_t<_Rcvr>> {
    return __let_opstate<_Tag, _Fn, _Sndr, remove_cvref_t<_Rcvr>>(
        std::move(data), std::move(child), std::forward<_Rcvr>(__rcvr));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  // [exec.let]p9 (check-types) is not implemented -- same P3068 constexpr-exceptions gap as
  // <__execution/then.h>'s deviation for the same reason; an `Fn` that isn't invocable with
  // the intercepted datums, or whose result isn't itself a sender, simply makes this whole
  // overload not participate rather than reporting a dedicated diagnostic.
  template <class _Self, class _Env>
    requires sender_in<_Sndr, __fwd_env<remove_cvref_t<_Env>>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __child_sigs = completion_signatures_of_t<_Sndr, __fwd_env<remove_cvref_t<_Env>>>;
    return __let_signatures_t<_Tag, _Fn, remove_cvref_t<_Env>, __child_sigs>{};
  }
};

template <class _Tag, class _Sndr, class _Fn>
_LIBCPP_HIDE_FROM_ABI constexpr auto __let_make_sndr(_Sndr&& __sndr, _Fn&& __fn) {
  return __let_sndr<_Tag, decay_t<_Fn>, remove_cvref_t<_Sndr>>{
      {}, decay_t<_Fn>(std::forward<_Fn>(__fn)), std::forward<_Sndr>(__sndr)};
}

struct let_value_t {
  template <sender _Sndr, __movable_value _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Fn&& __fn) const
      -> __let_sndr<let_value_t, decay_t<_Fn>, remove_cvref_t<_Sndr>> {
    return execution::__let_make_sndr<let_value_t>(std::forward<_Sndr>(__sndr), std::forward<_Fn>(__fn));
  }

  template <class _Fn>
    requires constructible_from<decay_t<_Fn>, _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Fn&& __fn) const
      noexcept(is_nothrow_constructible_v<decay_t<_Fn>, _Fn>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Fn>(__fn)));
  }
};

struct let_error_t {
  template <sender _Sndr, __movable_value _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Fn&& __fn) const
      -> __let_sndr<let_error_t, decay_t<_Fn>, remove_cvref_t<_Sndr>> {
    return execution::__let_make_sndr<let_error_t>(std::forward<_Sndr>(__sndr), std::forward<_Fn>(__fn));
  }

  template <class _Fn>
    requires constructible_from<decay_t<_Fn>, _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Fn&& __fn) const
      noexcept(is_nothrow_constructible_v<decay_t<_Fn>, _Fn>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Fn>(__fn)));
  }
};

struct let_stopped_t {
  // [exec.let]p3: unlike let_value/let_error (whose Fn-invocability can only be checked once
  // the child's value/error datums are known), let_stopped(sndr, f) is ill-formed up front
  // unless F satisfies invocable<F> -- fn takes no arguments here, so there's nothing to defer.
  template <sender _Sndr, __movable_value _Fn>
    requires invocable<decay_t<_Fn>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Fn&& __fn) const
      -> __let_sndr<let_stopped_t, decay_t<_Fn>, remove_cvref_t<_Sndr>> {
    return execution::__let_make_sndr<let_stopped_t>(std::forward<_Sndr>(__sndr), std::forward<_Fn>(__fn));
  }

  template <class _Fn>
    requires constructible_from<decay_t<_Fn>, _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Fn&& __fn) const
      noexcept(is_nothrow_constructible_v<decay_t<_Fn>, _Fn>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Fn>(__fn)));
  }
};

inline constexpr let_value_t let_value{};
inline constexpr let_error_t let_error{};
inline constexpr let_stopped_t let_stopped{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_LET_H
