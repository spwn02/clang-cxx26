//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_QUERYABLE_H
#define _LIBCPP___EXECUTION_QUERYABLE_H

#include <__concepts/destructible.h>
#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [exec.queryable]
// Exposition-only, declared directly in namespace std (not std::execution) per
// the synopsis in [execution.syn]. A queryable object is any destructible type;
// the interesting requirements (which queries are supported, constness,
// equality-preservation) are semantic, checked ad hoc by each query's own
// constraints, not encoded here.
template <class _Tp>
concept __queryable = destructible<_Tp>;

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_QUERYABLE_H
