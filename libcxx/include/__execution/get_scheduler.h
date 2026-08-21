//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_SCHEDULER_H
#define _LIBCPP___EXECUTION_GET_SCHEDULER_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/domain.h>
#include <__execution/forwarding_query.h>
#include <__execution/queryable.h>
#include <__execution/scheduler.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/as_const.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// get_scheduler_t is defined further down (it needs get_completion_scheduler, defined below,
// to implement [exec.get.scheduler]); forward-declared here so that __hide_sched (immediately
// below) can name it in a mere type comparison (`is_same_v`, which doesn't need a complete
// type), matching the forward-declare-for-type-identity-only pattern <__execution/just.h>
// already established for just_t/just_error_t/just_stopped_t.
struct get_scheduler_t;

// [exec.queries.expos]: HIDE-SCHED(q). An adaptor over a queryable object q that makes the
// get_scheduler_t and get_domain_t queries ill-formed, forwarding everything else to q
// unchanged. get_scheduler's own definition (below) uses this to keep
// get_completion_scheduler's TRY-QUERY step from looping back into get_scheduler/get_domain
// on the same environment it was itself invoked on.
template <class _Env>
struct __hide_sched {
  const _Env* __env_;

  template <class _Tag, class... _Args>
    requires(!is_same_v<decay_t<_Tag>, get_scheduler_t> && !is_same_v<decay_t<_Tag>, get_domain_t>) &&
            requires(const _Env& __env, _Tag __tag, _Args&&... __args) {
              __env.query(__tag, std::forward<_Args>(__args)...);
            }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Tag __tag, _Args&&... __args) const
      noexcept(noexcept(__env_->query(__tag, std::forward<_Args>(__args)...))) {
    return __env_->query(__tag, std::forward<_Args>(__args)...);
  }
};

template <class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __hide_sched_fn(const _Env& __env) noexcept {
  return __hide_sched<_Env>{&__env};
}

// [exec.queries.expos]: TRY-QUERY(q, tag, args...). Split into two separately-constrained
// overloads (rather than one function with an internal `if constexpr`) so that probing
// callability via `requires { __try_query(...); }` never needs to instantiate the body of a
// branch that doesn't apply to the caller's types: a single `decltype(auto)`-returning
// function with an `if constexpr` inside would need its return type deduced -- i.e. its body
// instantiated -- to determine whether the *call itself* is well-formed, and body
// instantiation is not protected by SFINAE the way a trailing requires-clause substitution
// failure is (the exact pitfall <__execution/sender.h>'s `__sender_tag_of`/`tag_of_t` hit,
// recorded as the M2 "deviation 4" finding in docs/CXX26_GAPS.md). Two overloads, each valid
// only where its own requires-clause holds, sidesteps this: overload resolution rejects the
// inapplicable one via its (SFINAE-safe) constraint, without ever deducing its return type.
template <class _Q, class _Tag, class... _Args>
  requires requires(const _Q& __q, _Tag __tag, _Args&&... __args) {
    std::as_const(__q).query(__tag, std::forward<_Args>(__args)...);
  }
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __try_query(const _Q& __q, _Tag __tag, _Args&&... __args) {
  return std::as_const(__q).query(__tag, std::forward<_Args>(__args)...);
}

template <class _Q, class _Tag, class... _Args>
  requires(!requires(const _Q& __q, _Tag __tag, _Args&&... __args) {
    std::as_const(__q).query(__tag, std::forward<_Args>(__args)...);
  }) && requires(const _Q& __q, _Tag __tag) { std::as_const(__q).query(__tag); }
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __try_query(const _Q& __q, _Tag __tag, _Args&&...) {
  return std::as_const(__q).query(__tag);
}

// [exec.queries.expos]: RECURSE-QUERY(sch1, envs...). Forward-declared here (defined further
// down, after get_completion_scheduler is complete, since its body needs to name that
// variable template) so that get_completion_scheduler_t::operator() below can call it.
//
// Per spec, when sch1 and the freshly re-queried sch2 have the *same type*, the choice
// between "stop, return sch1" and "recurse again with sch2" is a runtime equality
// comparison; when they differ in type, recursion is mandatory. Nothing in scope through at
// least M5 ever has TRY-QUERY produce a same-typed-but-unequal scheduler (the only scheduler
// that answers this query at all right now is run-loop-scheduler, <__execution/run_loop.h>,
// whose own completion scheduler is itself, so the very first re-query is already ill-formed
// and recursion terminates at the base case) -- so, matching the documented simplifications
// already made for domain resolution in <__execution/domain.h>, the same-type case here
// always returns sch1 without modeling the runtime comparison. Revisit if a future
// scheduler's completion-scheduler chain needs it. Deliberately a single function with an
// internal `if constexpr` (unlike __try_query's split overloads above): this function is
// only ever called unconditionally, after the caller has already confirmed via a `requires`
// probe that the TRY-QUERY step it depends on succeeds, so its own `decltype(auto)` return
// is never itself the subject of a callability probe -- the M2 "deviation 4" body-
// instantiation hazard that motivated splitting __try_query does not apply here.
template <class _Sch1, class... _Envs>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __recurse_query(_Sch1&& __sch1, const _Envs&... __envs) noexcept;

// [exec.get.compl.sched]
template <class _Cpo>
struct get_completion_scheduler_t {
  template <class _Q, class... _Envs>
    requires(is_same_v<_Cpo, set_value_t> || is_same_v<_Cpo, set_error_t> || is_same_v<_Cpo, set_stopped_t>)
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Q& __q, const _Envs&... __envs) const noexcept {
    if constexpr (requires { execution::__try_query(__q, *this, __envs...); }) {
      decltype(auto) __sch1 = execution::__try_query(__q, *this, __envs...);
      return execution::__recurse_query(std::forward<decltype(__sch1)>(__sch1), __envs...);
    } else {
      static_assert(scheduler<_Q>, "Mandates: the type of q satisfies scheduler.");
      static_assert(sizeof...(_Envs) > 0, "Mandates: envs is not an empty pack.");
      return auto(__q);
    }
  }
};

template <class _Cpo>
inline constexpr get_completion_scheduler_t<_Cpo> get_completion_scheduler{};

template <class _Sch1, class... _Envs>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __recurse_query(_Sch1&& __sch1, const _Envs&... __envs) noexcept {
  if constexpr (requires { execution::__try_query(__sch1, execution::get_completion_scheduler<set_value_t>, __envs...); }) {
    decltype(auto) __sch2 = execution::__try_query(__sch1, execution::get_completion_scheduler<set_value_t>, __envs...);
    if constexpr (is_same_v<remove_cvref_t<decltype(__sch2)>, remove_cvref_t<_Sch1>>) {
      return static_cast<_Sch1&&>(__sch1);
    } else {
      return execution::__recurse_query(std::forward<decltype(__sch2)>(__sch2), __envs...);
    }
  } else {
    return static_cast<_Sch1&&>(__sch1);
  }
}

// [exec.get.scheduler]
struct get_scheduler_t : forwarding_query_t {
  template <class _Env>
    requires requires(const _Env& __env, const get_scheduler_t& __self) { __env.query(__self); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept {
    static_assert(noexcept(__env.query(*this)), "Mandates: the expression env.query(get_scheduler) is noexcept.");
    return execution::get_completion_scheduler<set_value_t>(__env.query(*this), execution::__hide_sched_fn(__env));
  }
};

inline constexpr get_scheduler_t get_scheduler{};

// [exec.get.start.scheduler]
struct get_start_scheduler_t : forwarding_query_t {
  template <class _Env>
    requires requires(const _Env& __env, const get_start_scheduler_t& __self) {
      { __env.query(__self) } -> scheduler;
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept(noexcept(__env.query(*this))) {
    return __env.query(*this);
  }
};

inline constexpr get_start_scheduler_t get_start_scheduler{};

// [exec.get.delegation.scheduler]
struct get_delegation_scheduler_t : forwarding_query_t {
  template <class _Env>
    requires requires(const _Env& __env, const get_delegation_scheduler_t& __self) {
      { __env.query(__self) } -> scheduler;
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept(noexcept(__env.query(*this))) {
    return __env.query(*this);
  }
};

inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_SCHEDULER_H
