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
#include <__execution/domain.h>
#include <__execution/env.h>
#include <__execution/fwd_env.h>
#include <__execution/get_allocator.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_scheduler.h>
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
#include <__utility/forward_like.h>
#include <__utility/in_place.h>
#include <__utility/move.h>
#include <exception>
#include <tuple>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

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
// every possibility -- mirroring the standard's own exposition-only `let-state`. This sender
// is a concrete implementation with environment-dependent completion signatures.
//
// [exec.let]p2's `let-env(sndr, env)` is the environment the continuation sender is connected through, besides
// FWD-ENV(env): the first well-formed of the SCHED-ENV of the completion scheduler of the child for set-cpo (the
// continuation starts where the child completed), a MAKE-ENV of its completion domain, and env<>{}. [exec.let]p8's
// `receiver2::get_env()` is `let-env.query(q, ...)` if valid, else `get_env(rcvr).query(q, ...)` if q is a forwarding
// query: the let-env joined in front of FWD-ENV(get_env(rcvr)).
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
  struct __to_variant<__exec_type_list<_Us...>> {
    using type = variant<monostate, _Us...>;
  };
  using type = typename __to_variant<__dedup_type_list_t<_Ts...>>::type;
};
template <class... _Ts>
using __let_args_variant = typename __let_args_variant_impl<_Ts...>::type;

// [exec.snd.expos] SCHED-ENV(sch): an environment whose get_start_scheduler is sch and whose get_domain is the one of sch.
template <class _Sch>
class __sched_env {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __sched_env(_Sch __sch) noexcept(is_nothrow_move_constructible_v<_Sch>)
      : __sch_(std::move(__sch)) {}

  _LIBCPP_HIDE_FROM_ABI constexpr _Sch query(get_start_scheduler_t) const noexcept(is_nothrow_copy_constructible_v<_Sch>) {
    return __sch_;
  }

  template <class _Tag = get_domain_t>
    requires requires(const _Sch& __sch, _Tag __tag) { __sch.query(__tag); }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(get_domain_t __tag) const
      noexcept(noexcept(std::declval<const _Sch&>().query(__tag))) {
    return __sch_.query(__tag);
  }

private:
  _Sch __sch_;
};

// [exec.let]p2: let-env(sndr, env) for the completion function set-cpo, given the attributes of sndr.
template <class _SetCpo, class _Attrs, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __let_env_of_attrs(const _Attrs& __attrs, const _Env& __env) noexcept {
  if constexpr (requires { execution::get_completion_scheduler<_SetCpo>(__attrs, execution::__fwd_env_fn(__env)); }) {
    return __sched_env<remove_cvref_t<decltype(execution::get_completion_scheduler<_SetCpo>(
        __attrs, execution::__fwd_env_fn(__env)))>>(
        execution::get_completion_scheduler<_SetCpo>(__attrs, execution::__fwd_env_fn(__env)));
  } else if constexpr (requires { execution::get_completion_domain<_SetCpo>(__attrs, execution::__fwd_env_fn(__env)); }) {
    return execution::prop(
        get_domain, execution::get_completion_domain<_SetCpo>(__attrs, execution::__fwd_env_fn(__env)));
  } else {
    return env<>{};
  }
}

template <class _SetCpo, class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __let_env(const _Sndr& __sndr, const _Env& __env) noexcept {
  return execution::__let_env_of_attrs<_SetCpo>(execution::get_env(__sndr), __env);
}

template <class _SetCpo, class _Sndr, class _Env>
using __let_env_t = decltype(execution::__let_env<_SetCpo>(std::declval<const _Sndr&>(), std::declval<const _Env&>()));

// [exec.let]p5 receiver2's environment: JOIN-ENV(let-env, FWD-ENV(get_env(rcvr))), that is let-env first.
template <class _LetEnv, class _Env>
using __let_joined_env_t = decltype(execution::env(std::declval<const _LetEnv&>(), std::declval<__fwd_env<_Env>>()));

// A receiver of the environment `_Env` that accepts every completion, standing in for the receiver of
// get_completion_signatures<Sndr, Env> ([exec.snd.expos]: "the type of a receiver whose environment has type E").
template <class _Env>
struct __let_probe_rcvr {
  using receiver_concept = receiver_tag;
  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI void set_value(_Args&&...) && noexcept {}
  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI void set_error(_Err&&) && noexcept {}
  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept {}
  _LIBCPP_HIDE_FROM_ABI _Env get_env() const noexcept { __builtin_unreachable(); } // only named in unevaluated operands
};

// [exec.let]p9/p12's completion-signature transform: every non-intercepted signature passes
// through unchanged; an intercepted `set-cpo(Args...)` is replaced by the *continuation*
// sender's own completion signatures (invoke_result_t<Fn, decay_t<Args>&...>, computed
// against FWD-ENV(Env) per this file's let-env simplification above), plus
// set_error_t(exception_ptr) unless both decay-copying Args and invoking Fn are statically
// nothrow (mirrors <__execution/then.h>'s TRY-SET-VALUE nothrow check; see __let_opstate's
// __intercept below for why the *runtime* path doesn't mirror this exactly).
template <class _Rcvr, class _LetEnv>
class __let_cont_rcvr;

template <class _SetCpo, class _Fn, class _Child, class _Env>
class __let_sig_transform {
public:
  // The environment the continuation sender is connected through, for a receiver with the environment _Env.
  using __let_env_type = __let_env_t<_SetCpo, _Child, _Env>;
  using __cont_env     = __let_joined_env_t<__let_env_type, _Env>;
  using __probe_rcvr   = __let_probe_rcvr<_Env>;

  // [exec.let]p5 let-state::impl: no exception completion if decay-copying the datums, invoking the function and
  // connecting the continuation sender are all noexcept. The runtime path (__let_opstate::__intercept) uses the same
  // value, so that it never completes with an error that is not advertised.
  template <class... _Args>
  static constexpr bool __nothrow_for =
      is_nothrow_constructible_v<__decayed_tuple<_Args...>, _Args...> &&
      is_nothrow_invocable_v<_Fn, decay_t<_Args>&...> &&
      noexcept(execution::connect(std::declval<invoke_result_t<_Fn, decay_t<_Args>&...>>(),
                                  std::declval<__let_cont_rcvr<__probe_rcvr, __let_env_type>>()));

  template <class _Sig>
  struct __one {
    using type = __exec_type_list<_Sig>;
  };

  template <class _Sigs>
  struct __sigs_to_list;
  template <class... _Ss>
  struct __sigs_to_list<completion_signatures<_Ss...>> {
    using type = __exec_type_list<_Ss...>;
  };

  template <class... _Args>
  struct __one<_SetCpo(_Args...)> {
    using __cont_sndr = invoke_result_t<_Fn, decay_t<_Args>&...>;
    using __cont_list = typename __sigs_to_list<completion_signatures_of_t<__cont_sndr, __cont_env>>::type;
    static constexpr bool __nothrow = __nothrow_for<_Args...>;
    using type = __conditional_t<__nothrow,
                                  __cont_list,
                                  typename __concat_type_lists<__cont_list, __exec_type_list<set_error_t(exception_ptr)>>::type>;
  };

  template <class _List>
  struct __dedup;
  template <class... _Ts>
  struct __dedup<__exec_type_list<_Ts...>> {
    using type = __dedup_type_list_t<_Ts...>;
  };

  template <class _List>
  struct __to_completion_signatures;
  template <class... _Sigs>
  struct __to_completion_signatures<__exec_type_list<_Sigs...>> {
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

// check-types ([exec.let]p9, is-valid-let-sender): for the datums `_Ts` of an intercepted completion the function can be
// called with decayed copies, returns a sender, and that sender is a sender_in the environment of the continuation.
template <class _Fn, class _ContEnv, class... _Ts>
concept __let_valid_args =
    (constructible_from<decay_t<_Ts>, _Ts> && ...) && invocable<_Fn, decay_t<_Ts>&...> &&
    sender<invoke_result_t<_Fn, decay_t<_Ts>&...>> && sender_in<invoke_result_t<_Fn, decay_t<_Ts>&...>, _ContEnv>;

template <class _Fn, class _ContEnv, class _List>
inline constexpr bool __let_valid_v = false;
template <class _Fn, class _ContEnv, class... _Ts>
inline constexpr bool __let_valid_v<_Fn, _ContEnv, __exec_type_list<_Ts...>> = __let_valid_args<_Fn, _ContEnv, _Ts...>;

template <class _Fn, class _ContEnv, class _Lists>
inline constexpr bool __let_all_valid_v = false;
template <class _Fn, class _ContEnv, class... _Lists>
inline constexpr bool __let_all_valid_v<_Fn, _ContEnv, __exec_type_list<_Lists...>> = (__let_valid_v<_Fn, _ContEnv, _Lists> && ...);

template <class _Tag, class _Fn, class _Child, class _Env, class _Completions>
using __let_signatures_t =
    typename __let_sig_transform<__let_set_cpo_t<_Tag>, _Fn, _Child, _Env>::template __impl<_Completions>::type;

// [exec.let]p8's `receiver2`: the receiver used to connect the sender `fn` returns, forwarding
// every completion straight through to the outer receiver, with the let-env joined in front of
// FWD-ENV(get_env(rcvr)) as its environment. Stores its own reference to the *outer* receiver (not a pointer to
// __let_opstate) since it outlives the child operation state entirely -- unlike
// __let_child_rcvr below, it has no need to reach back into __let_opstate's other storage.
template <class _Rcvr, class _LetEnv>
class __let_cont_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __let_cont_rcvr(_Rcvr& __rcvr, const _LetEnv& __env) noexcept
      : __rcvr_(__rcvr), __env_(__env) {}

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
    return execution::env(__env_, execution::__fwd_env_fn(execution::get_env(__rcvr_)));
  }

private:
  _Rcvr& __rcvr_;
  _LetEnv __env_;
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
  using __let_env_type = __let_env_t<__set_cpo, _Sndr, env_of_t<_Rcvr>>;
  using __cont_rcvr_t  = __let_cont_rcvr<_Rcvr, __let_env_type>;
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
  using __cont_ops_list = __gather_signatures<__set_cpo, __child_sigs, __cont_op_for, __exec_type_list>;

  template <class _List>
  struct __ops_variant_from;
  template <class... _Ts>
  struct __ops_variant_from<__exec_type_list<_Ts...>> {
    template <class>
    struct __to_variant;
    template <class... _Us>
    struct __to_variant<__exec_type_list<_Us...>> {
      using type = variant<_Us...>;
    };
    using type = typename __to_variant<__dedup_type_list_t<monostate, __child_op_t, _Ts...>>::type;
  };
  using __ops_variant_t = typename __ops_variant_from<__cont_ops_list>::type;

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __intercept(_Args&&... __args) {
    // The error completion is only reachable (and only advertised by get_completion_signatures, through
    // __let_sig_transform::__nothrow_for) if decay-copying __args, invoking __fn_ or connecting the continuation
    // sender can throw.
    using __transform_t      = __let_sig_transform<__set_cpo, _Fn, _Sndr, env_of_t<_Rcvr>>;
    constexpr bool __nothrow = __transform_t::template __nothrow_for<_Args...>;
    if constexpr (__nothrow) {
      __start_continuation(std::forward<_Args>(__args)...);
    } else {
      try {
        __start_continuation(std::forward<_Args>(__args)...);
      } catch (...) {
        execution::set_error(std::move(__rcvr_), std::current_exception());
      }
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
      return execution::connect(std::forward<__cont_sndr_t>(__sndr2), __cont_rcvr_t(__rcvr_, __env_));
    }});
    execution::start(std::get<__cont_op_t>(__ops_));
  }

  _Fn __fn_;
  _Rcvr __rcvr_;
  __args_variant_t __args_;
  __let_env_type __env_;
  __ops_variant_t __ops_;

  template <class, class>
  friend class __let_child_rcvr;

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __let_opstate(_Fn&& __fn, _Sndr&& __sndr, _Rcvr&& __rcvr)
      : __fn_(std::move(__fn)),
        __rcvr_(std::move(__rcvr)),
        __env_(execution::__let_env<__set_cpo>(__sndr, execution::get_env(__rcvr_))),
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
// shape tag_of_t (<__execution/sender.h>) decomposes via structured bindings. This sender
// stores its tag, function, and child and implements connection directly.
// The domains of the continuation senders ([exec.let]) for the completions with tag _Cpo, accumulated over the lists of
// datum types of the child's set-cpo completions: the continuation sender of a list _Args... is
// invoke_result_t<_Fn, decay_t<_Args>&...>, connected through _ContEnv.
template <class _Cpo, class _Fn, class _ContEnv, class... _Args>
inline constexpr bool __let_cont_has = false;
template <class _Cpo, class _Fn, class _ContEnv, class... _Args>
  requires requires { typename completion_signatures_of_t<invoke_result_t<_Fn, decay_t<_Args>&...>, _ContEnv>; }
inline constexpr bool __let_cont_has<_Cpo, _Fn, _ContEnv, _Args...> =
    !same_as<__exec_type_list<>,
             __gather_signatures<_Cpo,
                                 completion_signatures_of_t<invoke_result_t<_Fn, decay_t<_Args>&...>, _ContEnv>,
                                 __exec_type_list,
                                 __exec_type_list>>;

template <class _Cpo, class _Fn, class _ContEnv, class _Acc, class... _Lists>
struct __let_cont_domains;
template <class _Cpo, class _Fn, class _ContEnv, class _Acc>
struct __let_cont_domains<_Cpo, _Fn, _ContEnv, _Acc> {
  static constexpr bool __ok = true;
  using type                 = _Acc;
};
template <class _Cpo, class _Fn, class _ContEnv, class... _Acc, class... _Args, class... _Rest>
  requires(!__let_cont_has<_Cpo, _Fn, _ContEnv, _Args...>)
struct __let_cont_domains<_Cpo, _Fn, _ContEnv, __exec_type_list<_Acc...>, __exec_type_list<_Args...>, _Rest...>
    : __let_cont_domains<_Cpo, _Fn, _ContEnv, __exec_type_list<_Acc...>, _Rest...> {};
template <class _Cpo, class _Fn, class _ContEnv, class... _Acc, class... _Args, class... _Rest>
  requires(__let_cont_has<_Cpo, _Fn, _ContEnv, _Args...> &&
           requires(const invoke_result_t<_Fn, decay_t<_Args>&...>& __cont, const _ContEnv& __env) {
             execution::get_completion_domain<_Cpo>(execution::get_env(__cont), __env);
           })
struct __let_cont_domains<_Cpo, _Fn, _ContEnv, __exec_type_list<_Acc...>, __exec_type_list<_Args...>, _Rest...>
    : __let_cont_domains<
          _Cpo,
          _Fn,
          _ContEnv,
          __exec_type_list<_Acc...,
                    decltype(execution::get_completion_domain<_Cpo>(
                        execution::get_env(std::declval<const invoke_result_t<_Fn, decay_t<_Args>&...>&>()),
                        std::declval<const _ContEnv&>()))>,
          _Rest...> {};
template <class _Cpo, class _Fn, class _ContEnv, class... _Acc, class... _Args, class... _Rest>
  requires(__let_cont_has<_Cpo, _Fn, _ContEnv, _Args...> &&
           !requires(const invoke_result_t<_Fn, decay_t<_Args>&...>& __cont, const _ContEnv& __env) {
             execution::get_completion_domain<_Cpo>(execution::get_env(__cont), __env);
           })
struct __let_cont_domains<_Cpo, _Fn, _ContEnv, __exec_type_list<_Acc...>, __exec_type_list<_Args...>, _Rest...> {
  static constexpr bool __ok = false;
  using type                 = __exec_type_list<>;
};

// The attributes of a let_value/let_error/let_stopped sender: those of the child, but for the completion queries
// ([exec.snd.general]). The completions of the continuation senders happen wherever those say, which is only known
// at run time for a scheduler (so no completion scheduler is answered), but the completion domain is the common domain
// of the domains of every place a completion with that tag can happen in: the continuation senders (for every tag),
// the forwarded completions of the child (the tags other than set-cpo) and, for the error completion that a throwing
// function or datum copy produces, the place where the child completed with set-cpo.
template <class _Tag, class _Fn, class _Child, class _ChildAttrs>
class __let_attrs {
  using __set_cpo = __let_set_cpo_t<_Tag>;

  template <class _Env>
  using __child_sigs_t = completion_signatures_of_t<_Child, __fwd_env<_Env>>;
  template <class _Env>
  using __lists_t = __gather_signatures<__set_cpo, __child_sigs_t<_Env>, __exec_type_list, __exec_type_list>;
  template <class _Env>
  using __cont_env_t =
      __let_joined_env_t<decltype(execution::__let_env_of_attrs<__set_cpo>(std::declval<const _ChildAttrs&>(),
                                                                         std::declval<const _Env&>())),
                         _Env>;

  template <class _Cpo, class _Env>
  static constexpr bool __child_has =
      !same_as<__exec_type_list<>, __gather_signatures<_Cpo, __child_sigs_t<_Env>, __exec_type_list, __exec_type_list>>;

  // Does any continuation (or the decay copy of the datums, or the function) possibly throw?
  template <class _Env, class _Lists>
  struct __may_throw;
  template <class _Env, class... _Lists>
  struct __may_throw<_Env, __exec_type_list<_Lists...>> {
    template <class _List>
    struct __one;
    template <class... _Args>
    struct __one<__exec_type_list<_Args...>> {
      static constexpr bool value = !__let_sig_transform<__set_cpo, _Fn, _Child, _Env>::template __nothrow_for<_Args...>;
    };
    static constexpr bool value = (__one<_Lists>::value || ... || false);
  };

  template <class _Cpo, class _Env, class _Lists>
  struct __plan;
  template <class _Cpo, class _Env, class... _Lists>
  struct __plan<_Cpo, _Env, __exec_type_list<_Lists...>> {
    using __cont = __let_cont_domains<_Cpo, _Fn, __cont_env_t<_Env>, __exec_type_list<>, _Lists...>;

    template <class _Tg>
    static constexpr bool __child_domain_ok = requires(const _ChildAttrs& __attrs, const __fwd_env<_Env>& __env) {
      execution::get_completion_domain<_Tg>(__attrs, __env);
    };
    template <class _Tg>
    using __child_domain_t = decltype(execution::get_completion_domain<_Tg>(
        std::declval<const _ChildAttrs&>(), std::declval<const __fwd_env<_Env>&>()));

    // the forwarded completions of the child
    static constexpr bool __forward = !same_as<_Cpo, __set_cpo> && __child_has<_Cpo, _Env>;
    // the exception of the function or of a datum copy, an error completion in the place of set-cpo
    static consteval bool __exception_fn() {
      if constexpr (same_as<_Cpo, set_error_t> && __child_has<__set_cpo, _Env>)
        return __may_throw<_Env, __lists_t<_Env>>::value;
      else
        return false;
    }
    static constexpr bool __exception = __exception_fn();

    static consteval bool __ok() {
      if constexpr (!__cont::__ok) {
        return false;
      } else {
        if constexpr (__forward && !__child_domain_ok<_Cpo>)
          return false;
        if constexpr (__exception && !__child_domain_ok<__set_cpo>)
          return false;
        return true;
      }
    }

    template <class... _Ds>
    static constexpr auto __common(__exec_type_list<_Ds...>*) noexcept {
      return execution::__common_domain(_Ds()...);
    }

    template <bool _Use, class _Tg>
    struct __child_list {
      using type = __exec_type_list<>;
    };
    template <class _Tg>
    struct __child_list<true, _Tg> {
      using type = __exec_type_list<__child_domain_t<_Tg>>;
    };
    using __forward_list   = typename __child_list<__forward && __child_domain_ok<_Cpo>, _Cpo>::type;
    using __exception_list = typename __child_list<__exception && __child_domain_ok<__set_cpo>, __set_cpo>::type;
    using __all =
        typename __concat_type_lists<typename __cont::type, __forward_list, __exception_list>::type;
  };

  template <class _Cpo, class... _Envs>
  static consteval bool __answers() {
    if constexpr (sizeof...(_Envs) <= 1 && (is_same_v<_Cpo, set_value_t> || is_same_v<_Cpo, set_error_t> ||
                                            is_same_v<_Cpo, set_stopped_t>)) {
      using _Env = typename __first_env_or<env<>, remove_cvref_t<_Envs>...>::type;
      if constexpr (requires { typename __lists_t<_Env>; typename __cont_env_t<_Env>; }) {
        // only for a valid sender (the check-types of get_completion_signatures)
        if constexpr (__let_all_valid_v<_Fn, __cont_env_t<_Env>, __lists_t<_Env>>) {
          using _Plan = __plan<_Cpo, _Env, __lists_t<_Env>>;
          if constexpr (_Plan::__ok())
            return !same_as<__exec_type_list<>, typename _Plan::__all>;
        }
      }
    }
    return false;
  }

  template <class _Cpo, class _Env>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto __domain() noexcept {
    using _Plan = __plan<_Cpo, _Env, __lists_t<_Env>>;
    return _Plan::__common(static_cast<typename _Plan::__all*>(nullptr));
  }

public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __let_attrs(_ChildAttrs __attrs) noexcept(
      is_nothrow_move_constructible_v<_ChildAttrs>)
      : __attrs_(std::move(__attrs)) {}

  template <class _Query, class... _Args>
    requires(std::forwarding_query(_Query())) && (!__is_completion_query_v<_Query>) &&
            requires(const _ChildAttrs& __attrs, _Query __query, _Args&&... __args) {
              __attrs.query(__query, std::forward<_Args>(__args)...);
            }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Query __query, _Args&&... __args) const
      noexcept(noexcept(std::declval<const _ChildAttrs&>().query(__query, std::forward<_Args>(__args)...))) {
    return __attrs_.query(__query, std::forward<_Args>(__args)...);
  }

  template <class _Cpo, class... _Envs>
    requires(__answers<_Cpo, _Envs...>())
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_domain_t<_Cpo>, const _Envs&...) const noexcept {
    return __domain<_Cpo, typename __first_env_or<env<>, remove_cvref_t<_Envs>...>::type>();
  }

private:
  _ChildAttrs __attrs_;
};

template <class _Tag, class _Fn, class _Sndr>
class __let_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  _Fn data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && noexcept(
      noexcept(__connect_with(std::move(*this), std::declval<_Rcvr>()))) {
    return __connect_with(std::move(*this), std::forward<_Rcvr>(__rcvr));
  }

  template <class _Rcvr>
    requires copy_constructible<_Fn> && copy_constructible<_Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) const& noexcept(
      noexcept(__connect_with(*this, std::declval<_Rcvr>()))) {
    return __connect_with(*this, std::forward<_Rcvr>(__rcvr));
  }

private:
  template <class _Self, class _Rcvr>
  // (potentially throwing: the operation state connects the child in its constructor)
  _LIBCPP_HIDE_FROM_ABI static constexpr auto __connect_with(_Self&& __self, _Rcvr&& __rcvr)
      -> __let_opstate<_Tag, _Fn, _Sndr, remove_cvref_t<_Rcvr>> {
    return __let_opstate<_Tag, _Fn, _Sndr, remove_cvref_t<_Rcvr>>(
        _Fn(std::__allocator_aware_forward(std::forward_like<_Self>(__self.data), __rcvr)),
        _Sndr(std::forward_like<_Self>(__self.child)),
        std::forward<_Rcvr>(__rcvr));
  }

public:

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    using __child_attrs_t = remove_cvref_t<decltype(execution::get_env(child))>;
    return __let_attrs<_Tag, _Fn, _Sndr, __child_attrs_t>(execution::get_env(child));
  }

  // [exec.let]p9 (check-types) is the requires-clause below: an `Fn` that isn't invocable with
  // the intercepted datums, or whose result isn't itself a sender, makes this overload not
  // viable, so the generic [exec.getcomplsigs] fallback throws and sender_in is false.
  template <class _Self, class... _Env>
    requires sender_in<_Sndr, __fwd_env<typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type>> &&
             __let_all_valid_v<
                 _Fn,
                 __let_joined_env_t<
                     __let_env_t<__let_set_cpo_t<_Tag>, _Sndr, typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type>,
                     typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type>,
                 __gather_signatures<
                     __let_set_cpo_t<_Tag>,
                     completion_signatures_of_t<_Sndr, __fwd_env<typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type>>,
                     __exec_type_list,
                     __exec_type_list>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __env_t      = typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type;
    using __child_sigs = completion_signatures_of_t<_Sndr, __fwd_env<__env_t>>;
    return __let_signatures_t<_Tag, _Fn, _Sndr, __env_t, __child_sigs>{};
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

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_LET_H
