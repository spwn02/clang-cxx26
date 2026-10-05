//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_AFFINE_H
#define _LIBCPP___EXECUTION_AFFINE_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/continues_on.h>
#include <__execution/env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_scheduler.h>
#include <__execution/get_stop_token.h>
#include <__execution/infallible_scheduler.h>
#include <__execution/schedule.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__execution/unstoppable.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/decay.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/forward_like.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.affine]p4: UNSTOPPABLE-SCHEDULER(sch) is a scheduler whose schedule(e) is unstoppable(schedule(sch)), that
// forwards every query to sch and that compares equal to the one of an equal scheduler.
template <class _Sch>
class __unstoppable_scheduler {
public:
  using scheduler_concept = scheduler_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __unstoppable_scheduler(_Sch __sch) noexcept(
      is_nothrow_move_constructible_v<_Sch>)
      : __sch_(std::move(__sch)) {}

  _LIBCPP_HIDE_FROM_ABI constexpr auto schedule() const
      noexcept(noexcept(execution::unstoppable(execution::schedule(std::declval<const _Sch&>())))) {
    return execution::unstoppable(execution::schedule(__sch_));
  }

  template <class _Query, class... _Args>
    requires requires(const _Sch& __sch, _Query __query, _Args&&... __args) {
      __sch.query(__query, std::forward<_Args>(__args)...);
    }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Query __query, _Args&&... __args) const
      noexcept(noexcept(std::declval<const _Sch&>().query(__query, std::forward<_Args>(__args)...))) {
    return __sch_.query(__query, std::forward<_Args>(__args)...);
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const __unstoppable_scheduler& __x, const __unstoppable_scheduler& __y) noexcept {
    return __x.__sch_ == __y.__sch_;
  }

private:
  _Sch __sch_;
};

struct affine_t;

// [exec.affine]: affine adapts a sender into one that completes on the receiver's scheduler.
struct affine_t : sender_adaptor_closure<affine_t> {
  template <sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr) const;

  // [exec.affine]p6: if sender-for<Sndr, affine_t> is false the expression is ill-formed; otherwise child.affine() if
  // that is well-formed and continues_on(child, UNSTOPPABLE-SCHEDULER(get_start_scheduler(ev))) if not.
  // Like the transformations of the other adaptors it is called with the completion tag (set_value_t) by
  // default_domain; the draft writes the call without the tag, which cannot be the call [exec.snd.transform] makes.
  template <class _Sndr, class _Env>
    requires same_as<tag_of_t<_Sndr>, affine_t> && requires(const _Env& __ev) { execution::get_start_scheduler(__ev); } &&
             __infallible_scheduler<remove_cvref_t<decltype(execution::get_start_scheduler(std::declval<const _Env&>()))>,
                                    _Env>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env& __ev) {
    auto& [__tag, __data, __child] = __sndr;
    if constexpr (requires { std::forward_like<_Sndr>(__child).affine(); }) {
      return std::forward_like<_Sndr>(__child).affine();
    } else {
      return execution::continues_on(
          std::forward_like<_Sndr>(__child),
          __unstoppable_scheduler<remove_cvref_t<decltype(execution::get_start_scheduler(__ev))>>(
              execution::get_start_scheduler(__ev)));
    }
  }
};

// make-sender(affine, env<>(), sndr): an aggregate with public `tag`/`data`/`child` members, matching the
// (tag, data, ...children) shape tag_of_t decomposes. It is always transformed (affine_t::transform_sender) before it
// is connected; one that cannot be transformed (the environment has no infallible start scheduler) has no signatures.
template <class _Sndr>
class __affine_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS affine_t tag;
  env<> data;
  _Sndr child;

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__sender_attrs_fn(execution::get_env(child));
  }

  // [exec.affine]p9: if get_start_scheduler(get_env(rcvr)) is ill-formed or is not an infallible-scheduler<Env>, the
  // evaluation of get_completion_signatures<OutSndr, Env>() exits with an exception. This is only reached when the
  // sender was not transformed (see transform_sender below).
  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    throw __unspecified_exception();
    return completion_signatures<>();
  }
};

template <sender _Sndr>
_LIBCPP_HIDE_FROM_ABI constexpr auto affine_t::operator()(_Sndr&& __sndr) const {
  return __affine_sndr<remove_cvref_t<_Sndr>>{{}, env<>(), std::forward<_Sndr>(__sndr)};
}

inline constexpr affine_t affine{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_AFFINE_H
