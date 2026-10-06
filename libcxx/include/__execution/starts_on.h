//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_STARTS_ON_H
#define _LIBCPP___EXECUTION_STARTS_ON_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/domain.h>
#include <__execution/env.h>
#include <__execution/continues_on.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/just.h>
#include <__execution/let.h>
#include <__execution/schedule.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/forward_like.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.starts.on]. Per [exec.starts.on]p4, starts_on(sch, sndr) is make-sender(starts_on, sch, sndr): an aggregate with
// public `tag`/`data`/`child` members, which starts_on_t's own transform_sender(set_value, out_sndr, env) rewrites (at
// connect time, via domain dispatch) to:
//   let_value(continues_on(just(), sch),
//             [sndr = std::forward_like<OutSndr>(sndr)]() mutable { return std::move(sndr); });
// Starting `sndr` on sch's execution resource (p5) is what that composition does.
struct starts_on_t;

template <class _Sch, class _Sndr>
class __starts_on_sndr;

struct starts_on_t {
  template <scheduler _Sch, sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sch&& __sch, _Sndr&& __sndr) const;

  // [exec.starts.on]p5: starts_on.transform_sender(set_value, out_sndr, env), for a sender of this tag.
  template <class _OutSndr, class _Env>
    requires __sender_for<_OutSndr, starts_on_t>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto transform_sender(set_value_t, _OutSndr&& __out_sndr, const _Env&) {
    auto&& [__tag, __sch, __sndr] = __out_sndr;
    return execution::let_value(
        execution::continues_on(execution::just(), __sch),
        [__s = std::forward_like<_OutSndr>(__sndr)]() mutable
        noexcept(is_nothrow_move_constructible_v<decay_t<_OutSndr>>) { return std::move(__s); });
  }
};

// The attributes of starts_on(sch, sndr): those of the child, but for the completion queries ([exec.snd.general]).
// starts_on(sch, sndr) starts sndr after a continuation on sch (see transform_sender above, i.e. let_value), so its
// value completions happen where the child completes when it is started on sch: the completion queries of the child
// asked with SCHED-ENV(sch) in front of the (forwarded) environment. Its stopped completions come from two places, the
// child's and the scheduling operation's (on sch), so there is no completion scheduler for them, but the completion
// domain is the common domain of the two. The error completions include the failure of the scheduling operation, which
// happens on an unspecified agent: they are not answered.
template <class _Sch, class _Child, class _ChildAttrs>
class __starts_on_attrs {
  template <class _Env>
  using __joined_env_t = decltype(execution::env(std::declval<const __sched_env<_Sch>&>(), execution::__fwd_env_fn(std::declval<const _Env&>())));

  template <class _Env>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __joined(const _Env& __env) const noexcept(is_nothrow_copy_constructible_v<_Sch>) {
    return execution::env(__sched_env<_Sch>(__sch_), execution::__fwd_env_fn(__env));
  }

  template <class _Sigs>
  static constexpr bool __has_stopped_v = !same_as<type_list<>, __gather_signatures<set_stopped_t, _Sigs, type_list, type_list>>;

  // the stopped completions of the child, of the scheduling operation
  template <class _Env>
  static consteval bool __child_stopped() {
    if constexpr (requires { typename completion_signatures_of_t<_Child, __fwd_env<_Env>>; })
      return __has_stopped_v<completion_signatures_of_t<_Child, __fwd_env<_Env>>>;
    else
      return false;
  }
  template <class _Env>
  static consteval bool __sched_stopped() {
    if constexpr (requires { typename completion_signatures_of_t<schedule_result_t<_Sch>, _Env>; })
      return __has_stopped_v<completion_signatures_of_t<schedule_result_t<_Sch>, _Env>>;
    else
      return false;
  }
  template <class _Env>
  static consteval bool __child_domain_ok() {
    return requires(const _ChildAttrs& __attrs, const __joined_env_t<_Env>& __env) {
      execution::get_completion_domain<set_stopped_t>(__attrs, __env);
    };
  }
  template <class _Env>
  static consteval bool __stopped_domain_ok() {
    return (__child_stopped<_Env>() || __sched_stopped<_Env>()) && (!__child_stopped<_Env>() || __child_domain_ok<_Env>());
  }

public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __starts_on_attrs(_Sch __sch, _ChildAttrs __attrs) noexcept(
      is_nothrow_move_constructible_v<_Sch> && is_nothrow_move_constructible_v<_ChildAttrs>)
      : __sch_(std::move(__sch)), __attrs_(std::move(__attrs)) {}

  template <class _Query, class... _Args>
    requires(std::forwarding_query(_Query())) && (!__is_completion_query_v<_Query>) &&
            requires(const _ChildAttrs& __attrs, _Query __query, _Args&&... __args) {
              __attrs.query(__query, std::forward<_Args>(__args)...);
            }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Query __query, _Args&&... __args) const
      noexcept(noexcept(std::declval<const _ChildAttrs&>().query(__query, std::forward<_Args>(__args)...))) {
    return __attrs_.query(__query, std::forward<_Args>(__args)...);
  }

  template <class _Env>
    requires requires(const _ChildAttrs& __attrs, const __joined_env_t<_Env>& __env) {
      execution::get_completion_scheduler<set_value_t>(__attrs, __env);
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_scheduler_t<set_value_t>, const _Env& __env) const noexcept {
    return execution::get_completion_scheduler<set_value_t>(__attrs_, __joined(__env));
  }

  template <class _Env>
    requires requires(const _ChildAttrs& __attrs, const __joined_env_t<_Env>& __env) {
      execution::get_completion_domain<set_value_t>(__attrs, __env);
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_domain_t<set_value_t>, const _Env& __env) const noexcept {
    return execution::get_completion_domain<set_value_t>(__attrs_, __joined(__env));
  }

  template <class _Env>
    requires(__stopped_domain_ok<_Env>())
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_domain_t<set_stopped_t>, const _Env& __env) const noexcept {
    // the scheduling operation is connected to a receiver without an environment
    using __sched_dom = decltype(execution::get_completion_domain<set_value_t>(std::declval<const _Sch&>(), env<>()));
    using __child_dom = decltype(__child_domain<_Env>(__env));
    if constexpr (__child_stopped<_Env>() && __sched_stopped<_Env>())
      return execution::__common_domain(__child_dom(), __sched_dom());
    else if constexpr (__child_stopped<_Env>())
      return __child_dom();
    else
      return __sched_dom();
  }

private:
  template <class _Env>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __child_domain(const _Env& __env) const noexcept {
    if constexpr (__child_stopped<_Env>())
      return execution::get_completion_domain<set_stopped_t>(__attrs_, __joined(__env));
    else
      return default_domain();
  }

  _Sch __sch_;
  _ChildAttrs __attrs_;
};

template <class _Sch, class _Sndr>
class __starts_on_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS starts_on_t tag;
  _Sch data;
  _Sndr child;

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    using __child_attrs_t = remove_cvref_t<decltype(execution::get_env(child))>;
    return __starts_on_attrs<_Sch, _Sndr, __child_attrs_t>(data, execution::get_env(child));
  }

  // Only reached for a sender that was not transformed, which cannot happen: the transformation has no constraints
  // but the shape of the sender.
  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(_Env) == 0) {
      return execution::__lowered_signatures_without_env<_Self>();
    } else {
      throw __unspecified_exception();
      return completion_signatures<>();
    }
  }
};

template <scheduler _Sch, sender _Sndr>
_LIBCPP_HIDE_FROM_ABI constexpr auto starts_on_t::operator()(_Sch&& __sch, _Sndr&& __sndr) const {
  return __starts_on_sndr<decay_t<_Sch>, remove_cvref_t<_Sndr>>{
      {}, decay_t<_Sch>(std::forward<_Sch>(__sch)), std::forward<_Sndr>(__sndr)};
}

inline constexpr starts_on_t starts_on{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_STARTS_ON_H
