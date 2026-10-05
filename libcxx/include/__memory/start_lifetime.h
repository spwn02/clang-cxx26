// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_START_LIFETIME_H
#define _LIBCPP___MEMORY_START_LIFETIME_H

#include <__config>
#include <__memory/addressof.h>
#include <__type_traits/is_aggregate.h>
#include <__type_traits/is_implicit_lifetime.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [obj.lifetime]: if the object referenced by r is not within its lifetime, begins its lifetime. No initialization is
// performed and no subobject begins its lifetime; for a member of a union, it becomes the active member. Only a
// constant evaluation can tell the difference, so the work is done by a compiler builtin.
template <class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr void start_lifetime(_Tp& __r) noexcept {
  static_assert(is_aggregate_v<_Tp> && is_implicit_lifetime_v<_Tp>,
                "std::start_lifetime(r) requires an implicit-lifetime aggregate type");
#  if __has_builtin(__builtin_start_lifetime)
  __builtin_start_lifetime(std::addressof(__r));
#  endif
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MEMORY_START_LIFETIME_H
