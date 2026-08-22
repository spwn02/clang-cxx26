//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_UNSTOPPABLE_H
#define _LIBCPP___EXECUTION_UNSTOPPABLE_H

#include <__config>
#include <__execution/env.h>
#include <__execution/get_stop_token.h>
#include <__execution/sender.h>
#include <__execution/write_env.h>
#include <__stop_token/never_stop_token.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

// get_stop_token/never_stop_token (<__execution/get_stop_token.h>) are only declared under
// `_LIBCPP_HAS_THREADS`, so unstoppable -- expressed entirely in terms of them -- follows suit.
#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// unstoppable's type is left unspecified by [execution.syn] (like write_env's, unlike
// then_t/on_t/starts_on_t), so __unstoppable_t is this fork's own name, not a standard one --
// only the CPO object itself is exported (see libcxx/modules/std/execution.inc).
//
// [exec.unstoppable]p2: "unstoppable(sndr) is expression-equivalent to
// write_env(sndr, prop(get_stop_token, never_stop_token{}))." Unlike [exec.write.env],
// [exec.unstoppable] never uses the phrase "pipeable sender adaptor object" (the trigger for a
// `adaptor(args...)` partial-application overload per [exec.adapt.obj]p4-5, used by
// then_t/on_t/let_value_t) -- so __unstoppable_t is call-only, no `sndr | unstoppable` form. See
// the longer note at the top of <__execution/write_env.h>.
struct __unstoppable_t {
  template <sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr) const {
    return execution::write_env(std::forward<_Sndr>(__sndr), execution::prop(std::get_stop_token, never_stop_token{}));
  }
};

inline constexpr __unstoppable_t unstoppable{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_UNSTOPPABLE_H
