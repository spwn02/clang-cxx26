//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_FORWARD_PROGRESS_GUARANTEE_H
#define _LIBCPP___EXECUTION_GET_FORWARD_PROGRESS_GUARANTEE_H

#include <__concepts/same_as.h>
#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.get.fwd.progress]
enum class forward_progress_guarantee {
  concurrent,
  parallel,
  weakly_parallel,
};

// [exec.get.fwd.progress]p2: "If Sch does not satisfy scheduler, get_forward_progress_guarantee is
// ill-formed." Transcribed literally, that would make get_forward_progress_guarantee and the `scheduler`
// concept (<__execution/scheduler.h>) mutually recursive: `scheduler`'s own requires-clause calls
// get_forward_progress_guarantee(sch) to check itself. Constraining operator() directly on
// "sch.query(*this) is well-formed and returns forward_progress_guarantee" (rather than on the scheduler
// concept) breaks the cycle without changing observable behavior: every Sch for which this query is
// well-formed and correctly typed is exactly the set of types the standard intends to accept here, and
// it's the same style already used for forwarding_query_t/get_domain_t. Note this uses `*this`, not a
// freshly-constructed `get_forward_progress_guarantee_t{}`: the latter would require this class to be a
// complete type at the point its own trailing requires-clause is checked, which it isn't yet (a member
// function template's requires-clause is not a complete-class context the way a member function body is).
struct get_forward_progress_guarantee_t {
  template <class _Sch>
    requires requires(const _Sch& __sch, const get_forward_progress_guarantee_t& __self) {
      { __sch.query(__self) } -> same_as<forward_progress_guarantee>;
    }
  _LIBCPP_HIDE_FROM_ABI constexpr forward_progress_guarantee operator()(const _Sch& __sch) const noexcept {
    static_assert(noexcept(__sch.query(*this)), "Mandates: the expression sch.query(get_forward_progress_guarantee) is noexcept.");
    return __sch.query(*this);
  }
};

inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_FORWARD_PROGRESS_GUARANTEE_H
