//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_FORWARDING_QUERY_H
#define _LIBCPP___EXECUTION_FORWARDING_QUERY_H

#include <__concepts/derived_from.h>
#include <__concepts/same_as.h>
#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.fwd.env]
// A query `q` is a "forwarding query" if, when an environment adaptor combines several
// queryable objects into one, `q` should be looked up in the *inner* environment rather
// than answered by the adaptor itself. Query CPOs that want to be forwarding queries
// derive publicly from `forwarding_query_t` (checked below via `derived_from`); the
// member-function form lets a query object override that default on a case-by-case basis.
struct forwarding_query_t {
  template <class _Query>
  _LIBCPP_HIDE_FROM_ABI constexpr bool operator()(const _Query& __query) const noexcept {
    if constexpr (requires {
                    { __query.query(*this) } -> same_as<bool>;
                  }) {
      return __query.query(*this);
    } else {
      return derived_from<_Query, forwarding_query_t>;
    }
  }
};

inline constexpr forwarding_query_t forwarding_query{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_FORWARDING_QUERY_H
