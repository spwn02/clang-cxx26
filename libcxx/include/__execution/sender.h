//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_SENDER_H
#define _LIBCPP___EXECUTION_SENDER_H

#include <__concepts/constructible.h>
#include <__concepts/derived_from.h>
#include <__concepts/movable.h>
#include <__config>
#include <__execution/awaitable.h>
#include <__execution/env.h>
#include <__execution/get_env.h>
#include <__execution/queryable.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.snd.concepts]
struct sender_tag {};

template <class _Sndr>
concept __is_sender = derived_from<typename _Sndr::sender_concept, sender_tag>;

// [exec.snd.concepts]p enable-sender: is-sender<Sndr> || is-awaitable<Sndr,
// env-promise<env<>>>. __is_awaitable's own SFINAE-safety (see awaitable.h) keeps this from
// hard-erroring on ordinary non-sender, non-awaitable types.
template <class _Sndr>
inline constexpr bool enable_sender = __is_sender<_Sndr> || __is_awaitable<_Sndr, __env_promise<env<>>>;

template <class _Sndr>
concept sender = enable_sender<remove_cvref_t<_Sndr>> && requires(const remove_cvref_t<_Sndr>& __sndr) {
  { execution::get_env(__sndr) } -> __queryable;
} && move_constructible<remove_cvref_t<_Sndr>> && constructible_from<remove_cvref_t<_Sndr>, _Sndr>;

// [exec.snd.concepts]p6: tag_of_t<Sndr> is decltype(auto(tag)) where `auto&& [tag, data,
// ...children] = sndr;` would be well-formed; otherwise it's ill-formed (not required to
// be SFINAE-friendly).
template <class _Sndr>
_LIBCPP_HIDE_FROM_ABI constexpr auto __sender_tag_of(_Sndr&& __sndr) {
  auto&& [__tag, __data, ...__children] = std::forward<_Sndr>(__sndr);
  return auto(__tag);
}

template <class _Sndr>
using tag_of_t = decltype(execution::__sender_tag_of(std::declval<_Sndr>()));

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_SENDER_H
