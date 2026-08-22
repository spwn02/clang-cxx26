//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_STOPPED_AS_ERROR_H
#define _LIBCPP___EXECUTION_STOPPED_AS_ERROR_H

#include <__concepts/constructible.h>
#include <__config>
#include <__execution/just.h>
#include <__execution/let.h>
#include <__execution/movable_value.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__functional/bind_back.h>
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

// [exec.stopped.err]p3: stopped_as_error(sndr, err) is expression-equivalent to
// let_stopped(sndr, [err = forward_like<Sndr>(err)]() mutable noexcept(...) { return
// just_error(std::move(err)); }), unlike <__execution/stopped_as_optional.h>'s adaptor,
// this doesn't need to know Env up front (its result type doesn't depend on the child's
// value completions at all), so -- like <__execution/starts_on.h>'s and <__execution/on.h>'s
// compositions -- it's computed directly at CPO-call time rather than needing its own
// hand-rolled sender type.
//
// **Deviation, same class as <__execution/starts_on.h>'s:** the resulting sender's tag_of_t
// is let_stopped_t's own tag, not stopped_as_error_t; sender_for<decltype(stopped_as_error(sndr,
// err)), stopped_as_error_t> is false. Nothing in scope through M5 inspects tag_of_t/sender-for
// on a stopped_as_error result.
struct stopped_as_error_t {
  template <sender _Sndr, __movable_value _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Err&& __err) const {
    return execution::let_stopped(
        std::forward<_Sndr>(__sndr),
        [__err = std::forward<_Err>(__err)]() mutable
            noexcept(is_nothrow_move_constructible_v<decay_t<_Err>>) { return execution::just_error(std::move(__err)); });
  }

  template <class _Err>
    requires constructible_from<decay_t<_Err>, _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Err&& __err) const
      noexcept(is_nothrow_constructible_v<decay_t<_Err>, _Err>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Err>(__err)));
  }
};

inline constexpr stopped_as_error_t stopped_as_error{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_STOPPED_AS_ERROR_H
