//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_MOVABLE_VALUE_H
#define _LIBCPP___EXECUTION_MOVABLE_VALUE_H

#include <__concepts/constructible.h>
#include <__concepts/movable.h>
#include <__config>
#include <__type_traits/decay.h>
#include <__type_traits/is_array.h>
#include <__type_traits/remove_reference.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [exec.general]
// Declared directly in namespace std (not std::execution) per [execution.syn], matching
// forwarding_query_t/__queryable/get_allocator_t/get_stop_token_t.
template <class _Tp>
concept __movable_value =
    move_constructible<decay_t<_Tp>> && constructible_from<decay_t<_Tp>, _Tp> && !is_array_v<remove_reference_t<_Tp>>;

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_MOVABLE_VALUE_H
