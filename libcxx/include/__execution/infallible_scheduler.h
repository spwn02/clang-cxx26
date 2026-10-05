//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_INFALLIBLE_SCHEDULER_H
#define _LIBCPP___EXECUTION_INFALLIBLE_SCHEDULER_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_stop_token.h>
#include <__execution/schedule.h>
#include <__execution/scheduler.h>
#include <__stop_token/stoppable_token.h>
#include <__utility/declval.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.snd.expos] infallible-scheduler: a scheduler whose schedule operation can only complete with set_value unless
// stop can be requested.
template <class _Sch, class _Env>
concept __infallible_scheduler =
    scheduler<_Sch> &&
    (same_as<completion_signatures<set_value_t()>,
             completion_signatures_of_t<decltype(execution::schedule(std::declval<_Sch>())), _Env>> ||
     (!unstoppable_token<stop_token_of_t<_Env>> &&
      (same_as<completion_signatures<set_value_t(), set_stopped_t()>,
               completion_signatures_of_t<decltype(execution::schedule(std::declval<_Sch>())), _Env>> ||
       same_as<completion_signatures<set_stopped_t(), set_value_t()>,
               completion_signatures_of_t<decltype(execution::schedule(std::declval<_Sch>())), _Env>>)));

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_INFALLIBLE_SCHEDULER_H
