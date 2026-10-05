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
#include <__execution/continues_on.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/just.h>
#include <__execution/let.h>
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

template <class _Sch, class _Sndr>
class __starts_on_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS starts_on_t tag;
  _Sch data;
  _Sndr child;

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__sender_attrs_fn(execution::get_env(child));
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
