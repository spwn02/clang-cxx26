//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_SCHEDULER_H
#define _LIBCPP___EXECUTION_SCHEDULER_H

#include <__concepts/copyable.h>
#include <__concepts/derived_from.h>
#include <__concepts/equality_comparable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__execution/get_forward_progress_guarantee.h>
#include <__execution/queryable.h>
#include <__execution/schedule.h>
#include <__execution/sender.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.sched]
// scheduler_tag and __scheduler_without_progress are in <__execution/get_forward_progress_guarantee.h>.
template <class _Sch>
concept scheduler = __scheduler_without_progress<_Sch> && requires(_Sch&& __sch) {
  { execution::get_forward_progress_guarantee(__sch) } -> same_as<forward_progress_guarantee>;
};

template <scheduler _Sch>
using schedule_result_t = decltype(execution::schedule(std::declval<_Sch>()));

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_SCHEDULER_H
