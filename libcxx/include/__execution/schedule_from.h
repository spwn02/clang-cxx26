//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_SCHEDULE_FROM_H
#define _LIBCPP___EXECUTION_SCHEDULE_FROM_H

#include <__config>
#include <__execution/connect.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.schedule.from]. Per the standard's own wording, schedule_from(sndr) is nothing more than
// make-sender(schedule_from, {}, sndr) -- unlike every other [exec.adapt] entity, this clause defines *no*
// impls-for specialization and *no* connect/completion-signature algorithm of its own ([Note 1]:
// "schedule_from is used by schedulers to control how to transition off of their schedulers' associated
// execution contexts"). Its entire behavior comes from domain-based customization: a real scheduler's
// domain would specialize `schedule_from_t`'s own `.transform_sender()` to insert real transition logic.
// This fork's <__execution/domain.h> documents that default_domain's transform_sender always takes the
// identity "otherwise" branch (never checks a tag's own transform_sender) -- so on this fork,
// schedule_from(sndr) is unconditionally identity-forwarding: same completion signatures as sndr, same
// attributes, and connect() just relays straight through to sndr's own connect(), wrapping the receiver
// only to apply FWD-ENV (matching every other single-child adaptor's [exec.adapt.general]p3.4 obligation).
// Still implemented as a real, distinctly-typed sender (not literally `return sndr;`), per
// <__execution/continues_on.h>'s own use of it as a real child -- advisor guidance: collapsing schedule_from
// away entirely at continues_on's call site would be an extra, unnecessary deviation on top of the one
// already documented here.
template <class _Rcvr>
class __schedule_from_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __schedule_from_rcvr(_Rcvr&& __rcvr) noexcept : __rcvr_(std::move(__rcvr)) {}

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
  _Rcvr __rcvr_;
};

// A single public `child` member, rather than the usual `tag`/`data`/`child` triple used by adaptors with
// real per-instance data (<__execution/then.h>, <__execution/let.h>) or at least a stateless tag
// (<__execution/continues_on.h>'s own `__continues_on_sndr`): schedule_from's own `data` is always `{}` per
// [exec.schedule.from]p1, and nothing in scope through M5 decomposes a schedule_from sender via
// tag_of_t/structured bindings, so there's nothing to gain from carrying a `tag` field just for shape
// symmetry with those other adaptors.
template <class _Sndr>
class __schedule_from_sndr {
public:
  using sender_concept = sender_tag;

  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    return execution::connect(std::move(child), __schedule_from_rcvr<remove_cvref_t<_Rcvr>>(std::forward<_Rcvr>(__rcvr)));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  template <class _Self, class _Env>
    requires sender_in<_Sndr, __fwd_env<remove_cvref_t<_Env>>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return completion_signatures_of_t<_Sndr, __fwd_env<remove_cvref_t<_Env>>>{};
  }
};

struct schedule_from_t {
  template <sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr) const
      -> __schedule_from_sndr<remove_cvref_t<_Sndr>> {
    return __schedule_from_sndr<remove_cvref_t<_Sndr>>{std::forward<_Sndr>(__sndr)};
  }
};

inline constexpr schedule_from_t schedule_from{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_SCHEDULE_FROM_H
