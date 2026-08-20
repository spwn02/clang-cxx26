//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_DOMAIN_H
#define _LIBCPP___EXECUTION_DOMAIN_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/forwarding_query.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/queryable.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.get.compl.domain] is out of scope for this sub-plan (it's meaningful only once a
// scheduler/completion-scheduler dispatch story exists, which lands with real schedulers).
// It's declared here -- with no operator() -- purely so that `completion-domain(s)` below
// (used by transform_sender's fixed-point recursion) has something to name in a
// `requires{}` probe: every `get_completion_domain<...>(...)` call is then a soft,
// SFINAE'd failure rather than a hard "no such name in namespace" error, and
// completion-domain falls back to `default_domain` unconditionally, which is correct for
// every sender/env in scope through at least M5 (none of them provide a completion
// scheduler). operator() lands whenever completion-scheduler-driven domain dispatch is
// implemented for real.
template <class _Cpo = void>
struct get_completion_domain_t {};
template <class _Cpo = void>
inline constexpr get_completion_domain_t<_Cpo> get_completion_domain{};

// [exec.domain.default]
struct default_domain {
  // Per [exec.domain.default]p2: `tag_of_t<Sndr>().transform_sender(Tag(), forward<Sndr>
  // (sndr), env)` if that expression is well-formed, else `static_cast<Sndr>(forward<Sndr>
  // (sndr))`. The "if well-formed" half is deliberately not implemented: `tag_of_t` is
  // computed via a structured-binding decomposition inside a *separate* helper function
  // ([__execution/sender.h]'s __sender_tag_of), and instantiating that helper's body to
  // deduce its `auto` return type is not in the "immediate context" of substitution --
  // verified empirically that a `requires{ tag_of_t<Sndr>()...; }` probe hard-errors
  // (rather than soundly evaluating false) for a Sndr that doesn't decompose into at least
  // (tag, data), instead of being SFINAE-safe the way the standard's prose implies.
  // Rather than build a from-scratch "is this aggregate decomposable into >=2 members"
  // trait (fragile, and non-aggregate-with-public-members senders would still need
  // separate handling) purely to guard a branch that, per the transform-recurse fixed
  // point, is only ever reached for a sender whose *own tag type* defines a per-tag
  // `.transform_sender` member -- something nothing in scope through at least M5 does --
  // this always takes the "otherwise" branch. Revisit once a real sender needs per-tag
  // domain customization.
  template <class _Tag, sender _Sndr, __queryable _Env>
  _LIBCPP_HIDE_FROM_ABI static constexpr decltype(auto) transform_sender(_Tag, _Sndr&& __sndr, const _Env&) noexcept {
    return static_cast<_Sndr>(std::forward<_Sndr>(__sndr));
  }

  template <class _Tag, sender _Sndr, class... _Args>
    requires requires(_Sndr&& __sndr, _Args&&... __args) { _Tag().apply_sender(std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...); }
  _LIBCPP_HIDE_FROM_ABI static constexpr decltype(auto) apply_sender(_Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(
      noexcept(_Tag().apply_sender(std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...))) {
    return _Tag().apply_sender(std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
  }
};

// [exec.get.domain]: only branches (2.1) (env.query(get_domain)) and (2.3) (fall back to
// default_domain()) are implemented; branch (2.2) (derive the domain from
// get_scheduler(env)'s completion domain) needs get_scheduler/get_completion_scheduler,
// which land with real schedulers -- until then, nothing in scope provides a scheduler via
// this path, so the branch would never fire.
struct get_domain_t : forwarding_query_t {
  template <class _Env>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept {
    if constexpr (requires { auto(__env.query(*this)); }) {
      static_assert(noexcept(auto(__env.query(*this))), "Mandates: env.query(get_domain) is noexcept.");
      return auto(__env.query(*this));
    } else {
      static_assert(noexcept(default_domain()));
      return default_domain();
    }
  }
};
inline constexpr get_domain_t get_domain{};

// These internal helpers are noexcept unconditionally: today every path through them
// bottoms out in `default_domain` (get_domain/get_completion_domain have no other
// answerer in scope, and default_domain's own transform_sender/apply_sender are
// noexcept), so this is accurate, not just optimistic. Revisit with a computed
// noexcept(...) once a real domain can flow through here.
template <class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __start_domain(const _Env& __env) noexcept {
  if constexpr (requires { execution::get_domain(__env); }) {
    return decay_t<decltype(execution::get_domain(__env))>();
  } else {
    return default_domain();
  }
}

template <class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __completion_domain(_Sndr&& __sndr, const _Env& __env) noexcept {
  if constexpr (requires { execution::get_completion_domain<>(execution::get_env(__sndr), __env); }) {
    return decay_t<decltype(execution::get_completion_domain<>(execution::get_env(__sndr), __env))>();
  } else {
    return default_domain();
  }
}

template <class _Dom, class _Tag, class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto)
__transformed_sndr(_Dom __dom, _Tag __tag, _Sndr&& __sndr, const _Env& __env) noexcept {
  if constexpr (requires { __dom.transform_sender(__tag, std::forward<_Sndr>(__sndr), __env); }) {
    return __dom.transform_sender(__tag, std::forward<_Sndr>(__sndr), __env);
  } else {
    return default_domain().transform_sender(__tag, std::forward<_Sndr>(__sndr), __env);
  }
}

// [exec.snd.transform]: transform-recurse. For every sender/env in scope through at least
// M5 (none of which define a per-tag `.transform_sender` member, and no env provides
// get_domain/get_completion_domain), transformed-sndr(dom, tag, s) always yields the same
// type as s, so this terminates on the first call -- the recursive branch is real but
// currently unexercised.
template <class _Dom, class _Tag, class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto)
__transform_recurse(_Dom __dom, _Tag __tag, _Sndr&& __sndr, const _Env& __env) noexcept {
  using __s2_t = decltype(execution::__transformed_sndr(__dom, __tag, std::forward<_Sndr>(__sndr), __env));
  if constexpr (is_same_v<remove_cvref_t<__s2_t>, remove_cvref_t<_Sndr>>) {
    return execution::__transformed_sndr(__dom, __tag, std::forward<_Sndr>(__sndr), __env);
  } else {
    decltype(auto) __s2 = execution::__transformed_sndr(__dom, __tag, std::forward<_Sndr>(__sndr), __env);
    if constexpr (is_same_v<_Tag, start_t>) {
      return execution::__transform_recurse(execution::__start_domain(__env), __tag, std::forward<decltype(__s2)>(__s2), __env);
    } else {
      return execution::__transform_recurse(
          execution::__completion_domain(__s2, __env), __tag, std::forward<decltype(__s2)>(__s2), __env);
    }
  }
}

template <class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __transform_sender_impl(_Sndr&& __sndr, const _Env& __env) noexcept {
  decltype(auto) __tmp_sndr =
      execution::__transform_recurse(execution::__completion_domain(__sndr, __env), set_value_t{}, std::forward<_Sndr>(__sndr), __env);
  return execution::__transform_recurse(
      execution::__start_domain(__env), start_t{}, std::forward<decltype(__tmp_sndr)>(__tmp_sndr), __env);
}

// [exec.snd.transform]
template <sender _Sndr, __queryable _Env>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) transform_sender(_Sndr&& __sndr, const _Env& __env) noexcept(
    noexcept(execution::__transform_sender_impl(std::forward<_Sndr>(__sndr), __env))) {
  return execution::__transform_sender_impl(std::forward<_Sndr>(__sndr), __env);
}

template <class _Domain, class _Tag, class _Sndr, class... _Args>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __apply_sender_impl(_Domain __dom, _Tag, _Sndr&& __sndr, _Args&&... __args) {
  if constexpr (requires { __dom.apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...); }) {
    return __dom.apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
  } else {
    return default_domain().apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
  }
}

// [exec.snd.apply]
template <class _Domain, class _Tag, sender _Sndr, class... _Args>
  requires requires(_Domain __dom, _Sndr&& __sndr, _Args&&... __args) {
    execution::__apply_sender_impl(__dom, _Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
  }
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) apply_sender(_Domain __dom, _Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(
    noexcept(execution::__apply_sender_impl(__dom, _Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...))) {
  return execution::__apply_sender_impl(__dom, _Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_DOMAIN_H
