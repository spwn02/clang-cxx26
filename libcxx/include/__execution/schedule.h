//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_SCHEDULE_H
#define _LIBCPP___EXECUTION_SCHEDULE_H

#include <__config>
#include <__execution/sender.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.schedule]
// Deliberately does not reference the `scheduler` concept (<__execution/scheduler.h>): `scheduler`'s own
// requires-clause calls schedule(sch) to check itself, so constraining schedule_t on `scheduler` would make
// the two mutually recursive. Constraining on syntax alone (sch.schedule() well-formed) and enforcing
// "satisfies sender" as a Mandates-style static_assert -- matching connect_t's and start_t's existing
// pattern -- keeps this self-contained.
struct schedule_t {
  template <class _Sch>
    requires requires(_Sch&& __sch) { std::forward<_Sch>(__sch).schedule(); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sch&& __sch) const
      noexcept(noexcept(std::forward<_Sch>(__sch).schedule())) {
    static_assert(sender<decltype(std::forward<_Sch>(__sch).schedule())>,
                  "Mandates: the type of sch.schedule() satisfies sender.");
    return std::forward<_Sch>(__sch).schedule();
  }
};

inline constexpr schedule_t schedule{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_SCHEDULE_H
