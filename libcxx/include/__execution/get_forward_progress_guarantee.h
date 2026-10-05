//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_FORWARD_PROGRESS_GUARANTEE_H
#define _LIBCPP___EXECUTION_GET_FORWARD_PROGRESS_GUARANTEE_H

#include <__concepts/copyable.h>
#include <__concepts/derived_from.h>
#include <__concepts/equality_comparable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__execution/queryable.h>
#include <__execution/schedule.h>
#include <__execution/sender.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>

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
struct scheduler_tag {};

// All the requirements of [exec.sched] but the one on get_forward_progress_guarantee. [exec.get.fwd.progress] makes
// get_forward_progress_guarantee ill-formed for a type that does not satisfy scheduler, which is itself defined in
// terms of get_forward_progress_guarantee: the query is constrained with this concept, which is what breaks the cycle.
template <class _Sch>
concept __scheduler_without_progress =
    derived_from<typename remove_cvref_t<_Sch>::scheduler_concept, scheduler_tag> && __queryable<_Sch> &&
    requires(_Sch&& __sch) {
      { execution::schedule(std::forward<_Sch>(__sch)) } -> sender;
    } && equality_comparable<remove_cvref_t<_Sch>> && copyable<remove_cvref_t<_Sch>>;

struct get_forward_progress_guarantee_t {
  template <class _Sch>
    requires __scheduler_without_progress<_Sch> &&
             requires(const _Sch& __sch, const get_forward_progress_guarantee_t& __self) { __sch.query(__self); }
  _LIBCPP_HIDE_FROM_ABI constexpr forward_progress_guarantee operator()(const _Sch& __sch) const noexcept {
    static_assert(noexcept(__sch.query(*this)),
                  "Mandates: the expression sch.query(get_forward_progress_guarantee) is noexcept.");
    static_assert(same_as<decltype(__sch.query(*this)), forward_progress_guarantee>,
                  "Mandates: the type of sch.query(get_forward_progress_guarantee) is forward_progress_guarantee.");
    return __sch.query(*this);
  }
};

inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_FORWARD_PROGRESS_GUARANTEE_H
