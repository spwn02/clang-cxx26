//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_EXECUTOR_H
#define _LIBCPP___IO_EXECUTOR_H

#include <__concepts/convertible_to.h>
#include <__concepts/derived_from.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__io/continuation.h>
#include <__io/execution_context.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/remove_reference.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4003R3 [4.3]. `dispatch` may run the continuation inline (it then returns the handle for symmetric transfer, or
// noop_coroutine() when it queued the continuation instead); `post` always defers.
template <class _Ex>
concept Executor =
    is_nothrow_copy_constructible_v<_Ex> && is_nothrow_move_constructible_v<_Ex> &&
    requires(_Ex& __e, const _Ex& __ce, const _Ex& __ce2, continuation& __c) {
      { __ce == __ce2 } noexcept -> convertible_to<bool>;
      { __ce.context() } noexcept;
      requires is_lvalue_reference_v<decltype(__ce.context())> &&
                   derived_from<remove_reference_t<decltype(__ce.context())>, execution_context>;
      { __ce.on_work_started() } noexcept;
      { __ce.on_work_finished() } noexcept;
      { __ce.dispatch(__c) } -> same_as<coroutine_handle<>>;
      { __ce.post(__c) };
    };

// P4003R3 [4.4]
template <class _Xp>
concept ExecutionContext = derived_from<_Xp, execution_context> && requires(_Xp& __x) {
  typename _Xp::executor_type;
  requires Executor<typename _Xp::executor_type>;
  { __x.get_executor() } noexcept -> same_as<typename _Xp::executor_type>;
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_EXECUTOR_H
