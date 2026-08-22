//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_WRITE_ENV_H
#define _LIBCPP___EXECUTION_WRITE_ENV_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/connect.h>
#include <__execution/env.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/queryable.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.write.env]. write_env's type is left unspecified by [execution.syn] (`inline constexpr
// unspecified write_env;` -- unlike then_t/on_t/starts_on_t/schedule_from_t/continues_on_t,
// which the synopsis names outright), so, matching <__execution/read_env.h>'s own precedent,
// __write_env_t below is this fork's own name for it, not a standard one, and only the CPO
// object itself is exported (see libcxx/modules/std/execution.inc).
//
// Also unlike then_t/on_t/let_value_t, [exec.write.env]p2 says only "write_env is a
// customization point object" -- not "write_env denotes a pipeable sender adaptor object" (the
// phrase [exec.then]p2/[exec.on]p2/[exec.let]p2 use for the CPOs that get a partial-application
// `adaptor(args...)` overload per [exec.adapt.obj]p4-5). [exec.adapt.obj]p4 *defines* "pipeable
// sender adaptor object" as "a customization point object that accepts a sender as its first
// argument and returns a sender" -- read in isolation, write_env(sndr, env) matches that shape,
// which could suggest the classification (and p5's partial-application grant) applies
// automatically to anything shaped this way. It doesn't: [exec.schedule.from]p1 uses the exact
// same "denotes a customization point object" phrasing (not "pipeable sender adaptor object")
// for schedule_from(sndr) -- a *single-argument*, sender-first CPO that would trivially satisfy
// p4's shape and would therefore have to be unconditionally pipeable (p4's own last sentence:
// a one-argument pipeable sender adaptor object *is* a pipeable sender adaptor closure object,
// no partial application even needed) if the shape alone were sufficient. It isn't implemented
// that way (see <__execution/schedule_from.h>, an earlier M5 session, no pipe support), which
// settles the reading: p4 defines the *term*, and a clause's own "denotes a pipeable sender
// adaptor object" is the actual per-CPO grant, not an automatic consequence of matching the
// shape. write_env and unstoppable don't have that phrase, so __write_env_t below has only the
// 2-arg direct-call form -- no `write_env(env)` closure, no `sndr | write_env(env)`. Same for
// __unstoppable_t in <__execution/unstoppable.h>: [exec.unstoppable] never uses either phrase,
// so it too is call-only (`unstoppable(sndr)`, no pipe form).
//
// __write_env_t is defined (in full) below, *before* __write_env_sndr -- unlike
// then_t/on_t/schedule_from_t, which forward-declare their CPO type and use it as a template
// parameter (_Tag) in their sender's `tag` member, write_env has only one CPO, so
// __write_env_sndr's `tag` member below names __write_env_t directly, non-dependently. A
// non-dependent by-value member must be a complete type at the point its enclosing class
// *template* is defined (this check isn't deferred to instantiation the way it would be for a
// dependent/template-parameter member type) -- so __write_env_t has to come first. Matches
// <__execution/read_env.h>'s own ordering for exactly this reason.
template <class _Env, class _Sndr>
class __write_env_sndr;

// [exec.write.env]p4: impls-for<write-env-t>::join-env(state, env) returns a queryable object
// `e` such that `e.query(q)` is `state.query(q)` if that's valid, else `env.query(q)` --
// exactly the priority order execution::env<_Envs...> (<__execution/env.h>) already implements
// (query forwards to the *first* element for which the call is well-formed). So join-env is
// just `execution::env{state, env}` -- no new queryable-combinator type needed here.
template <class _State, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __write_env_join(const _State& __state, _Env&& __env) noexcept(
    is_nothrow_constructible_v<decay_t<_Env>, _Env>) {
  return execution::env(__state, std::forward<_Env>(__env));
}

// [exec.write.env]p4's `get-env` lambda: `join-env(state, FWD-ENV(get_env(rcvr)))`, the
// environment write_env connects its child through -- the one place a single-child adaptor
// in this fork customizes [exec.adapt.general]p3.4's default `FWD-ENV(get_env(rcvr))`.
template <class _Env, class _Rcvr>
class __write_env_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __write_env_rcvr(_Env&& __env, _Rcvr&& __rcvr)
      : __env_(std::move(__env)), __rcvr_(std::move(__rcvr)) {}

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
    return execution::__write_env_join(__env_, execution::__fwd_env_fn(execution::get_env(__rcvr_)));
  }

private:
  _Env __env_;
  _Rcvr __rcvr_;
};

struct __write_env_t {
  template <sender _Sndr, class _Env>
    requires __queryable<decay_t<_Env>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Env&& __env) const
      -> __write_env_sndr<decay_t<_Env>, remove_cvref_t<_Sndr>> {
    return __write_env_sndr<decay_t<_Env>, remove_cvref_t<_Sndr>>{
        {}, decay_t<_Env>(std::forward<_Env>(__env)), std::forward<_Sndr>(__sndr)};
  }
};

// An aggregate with public `tag`/`data`/`child` members, matching the (tag, data, ...children)
// shape tag_of_t (<__execution/sender.h>) decomposes via structured bindings. Not routed
// through the draft's generic basic-sender/impls-for machinery: see the M3 entry in
// docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet.
template <class _Env, class _Sndr>
class __write_env_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS __write_env_t tag;
  _Env data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    return execution::connect(
        std::move(child), __write_env_rcvr<_Env, remove_cvref_t<_Rcvr>>(std::move(data), std::forward<_Rcvr>(__rcvr)));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)) -- write_env doesn't customize its own
  // attributes (only the environment its child is connected through), same as then/read_env.
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept { return execution::__fwd_env_fn(execution::get_env(child)); }

  // [exec.write.env]p5 (check-types): with State = data-type<Sndr> and
  // JoinEnv = decltype(join-env(declval<State>(), FWD-ENV(declval<Env>()))), this is
  // expression-equivalent to get_completion_signatures<child-type<Sndr>, JoinEnv>() -- i.e.
  // the child's own signatures, computed against the joined environment type instead of Env
  // directly. `_Self` is accepted (matching [exec.getcomplsigs]'s call shape) but not used to
  // vary behavior, same as every other M5 adaptor's get_completion_signatures.
  template <class _Self, class _Env2>
    requires sender_in<_Sndr, decltype(execution::__write_env_join(
                                   std::declval<const _Env&>(), std::declval<__fwd_env<remove_cvref_t<_Env2>>>()))>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __joined_env =
        decltype(execution::__write_env_join(std::declval<const _Env&>(), std::declval<__fwd_env<remove_cvref_t<_Env2>>>()));
    return completion_signatures_of_t<_Sndr, __joined_env>{};
  }
};

inline constexpr __write_env_t write_env{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_WRITE_ENV_H
