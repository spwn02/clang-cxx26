//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_AFFINE_ON_H
#define _LIBCPP___EXECUTION_AFFINE_ON_H

#include <__config>
#include <__execution/continues_on.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.affine.on]. Per the adopted text (verified directly -- see
// docs/design/execution_task_p3552.md): start(op) "will start sndr on the current execution
// agent and execute completion operations on out_rcvr on an execution agent of the execution
// resource associated with sch. If the current execution resource is the same as the
// execution resource associated with sch, the completion operation on out_rcvr *may* be
// called before start(op) completes." The "may" makes the same-resource fast path a permitted
// optimization, not a correctness requirement -- <__execution/continues_on.h> already
// implements exactly the required behavior (run the child sender, then unconditionally
// transition onto sch before forwarding the result), so affine_on is a thin, fully-conforming
// wrapper around it. The only thing left on the table is the fast-path optimization (skipping
// the scheduling hop when already on sch's resource); revisit if that ever proves observable
// (e.g. reentrancy-depth-sensitive code) rather than purely a scheduling-cost concern.
struct affine_on_t {
  template <sender _Sndr, scheduler _Sch>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Sch&& __sch) const
      -> decltype(execution::continues_on(std::forward<_Sndr>(__sndr), std::forward<_Sch>(__sch))) {
    return execution::continues_on(std::forward<_Sndr>(__sndr), std::forward<_Sch>(__sch));
  }
};

inline constexpr affine_on_t affine_on{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_AFFINE_ON_H
