//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_SENDER_ADAPTOR_CLOSURE_H
#define _LIBCPP___EXECUTION_SENDER_ADAPTOR_CLOSURE_H

#include <__concepts/constructible.h>
#include <__concepts/invocable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__execution/sender.h>
#include <__functional/compose.h>
#include <__functional/invoke.h>
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

// [exec.adapt.obj]: sender_adaptor_closure. Mirrors <__ranges/range_adaptor.h>'s
// range_adaptor_closure/__range_adaptor_closure split, substituting "is a sender" for
// "is a range" as the disqualifying case (p2: "T does not satisfy sender").
template <class _Tp>
struct sender_adaptor_closure {};

// Wraps an arbitrary function object to make it a pipeable sender adaptor closure, i.e.
// something usable via `sndr | f` notation -- used by the `operator|(closure, closure)`
// composition overload below, and by every M5 adaptor's single-argument (partial
// application) overload via std::__bind_back, matching views::transform's own
// __pipeable(std::__bind_back(*this, ...)) pattern in <__ranges/transform_view.h>.
template <class _Fn>
struct __pipeable : _Fn, sender_adaptor_closure<__pipeable<_Fn>> {
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __pipeable(_Fn&& __f) : _Fn(std::move(__f)) {}
};
_LIBCPP_CTAD_SUPPORTED_FOR_TYPE(__pipeable);

template <class _Tp>
_Tp __derived_from_sender_adaptor_closure(sender_adaptor_closure<_Tp>*);

template <class _Tp>
concept __sender_adaptor_closure_object = !sender<remove_cvref_t<_Tp>> && requires {
  { execution::__derived_from_sender_adaptor_closure((remove_cvref_t<_Tp>*)nullptr) } -> same_as<remove_cvref_t<_Tp>>;
};

// [exec.adapt.obj]p1: `sndr | c` is equivalent to `c(sndr)`.
template <sender _Sndr, __sender_adaptor_closure_object _Closure>
  requires invocable<_Closure, _Sndr>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator|(_Sndr&& __sndr, _Closure&& __closure) noexcept(
    is_nothrow_invocable_v<_Closure, _Sndr>) {
  return std::invoke(std::forward<_Closure>(__closure), std::forward<_Sndr>(__sndr));
}

// [exec.adapt.obj]p1: `c | d` produces a closure `e` such that `e(sndr)` is `d(c(sndr))`.
template <__sender_adaptor_closure_object _C1, __sender_adaptor_closure_object _C2>
  requires constructible_from<decay_t<_C1>, _C1> && constructible_from<decay_t<_C2>, _C2>
_LIBCPP_HIDE_FROM_ABI constexpr auto operator|(_C1&& __c1, _C2&& __c2) noexcept(
    is_nothrow_constructible_v<decay_t<_C1>, _C1> && is_nothrow_constructible_v<decay_t<_C2>, _C2>) {
  return execution::__pipeable(std::__compose(std::forward<_C2>(__c2), std::forward<_C1>(__c1)));
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_SENDER_ADAPTOR_CLOSURE_H
