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
#include <__execution/continues_on.h>
#include <__execution/just.h>
#include <__execution/let.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.starts.on]. Per [exec.starts.on]p4, starts_on(sch, sndr) is expression-equivalent to
// make-sender(starts_on, sch, sndr), and starts_on_t's own `.transform_sender()` then rewrites that sender
// (at connect time, via domain dispatch) to:
//   let_value(continues_on(just(), sch),
//             [sndr = std::forward_like<OutSndr>(sndr)]() mutable { return std::move(sndr); });
// This fork's <__execution/domain.h> documents that default_domain::transform_sender always takes the
// identity "otherwise" branch (it never checks whether a sender's own tag type defines a per-tag
// transform_sender) -- so a starts_on_t-tagged sender built the usual way would never actually be rewritten
// into working code on this fork. Rather than build that dead-on-arrival intermediate sender, this computes
// the *result* of the rewrite directly, at CPO-call time: starts_on(sch, sndr) here literally returns
// let_value(continues_on(just(), sch), ...), matching [exec.starts.on]p4's algorithm body exactly (unlike
// starts_on's own scheduling contract -- p5's "start sndr on sch's execution resource" -- which is genuinely
// implemented by that composition, not merely approximated).
//
// **Deviation, same class as <__execution/let.h>'s let-env 2.3 fallback:** the resulting sender's tag_of_t
// is let_value_t's own tag, not starts_on_t; sender_for<decltype(starts_on(sch, sndr)), starts_on_t> is
// false. Nothing in scope through M5 inspects tag_of_t/sender-for on a starts_on result. <__execution/on.h>
// -- which itself composes starts_on with continues_on -- relies on this same composed-at-call-time shape.
struct starts_on_t {
  template <scheduler _Sch, sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sch&& __sch, _Sndr&& __sndr) const {
    return execution::let_value(
        execution::continues_on(execution::just(), std::forward<_Sch>(__sch)),
        [__sndr = std::forward<_Sndr>(__sndr)]() mutable
            noexcept(is_nothrow_move_constructible_v<decay_t<_Sndr>>) { return std::move(__sndr); });
  }
};

inline constexpr starts_on_t starts_on{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_STARTS_ON_H
