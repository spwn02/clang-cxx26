//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_IS_WITHIN_LIFETIME_H
#define _LIBCPP___TYPE_TRAITS_IS_WITHIN_LIFETIME_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && __has_builtin(__builtin_is_within_lifetime)

// [meta.const.eval]: is_within_lifetime<U>(p), P3450R1.
//
// Returns: true if p points to an object that is within its lifetime and static_cast<const volatile U*>(p) is a constant
// subexpression. The cast is always a constant subexpression for U = void, for U the type of the pointee and for a base
// class of it; only a conversion to a derived class (the object has to be a complete object of that type) can fail to
// be one, and library code cannot ask the compiler whether that conversion is a constant subexpression: this
// implementation returns the answer of the builtin for those (an object that is not of the derived type is reported
// as within its lifetime).
template <class _Up = void, class _Tp>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI consteval bool is_within_lifetime(const _Tp* __p) noexcept {
  static_assert(requires(const _Tp* __q) { static_cast<const volatile _Up*>(__q); },
                "is_within_lifetime requires static_cast<const volatile U*>(p) to be well-formed");
  return __builtin_is_within_lifetime(__p);
}

#endif // _LIBCPP_STD_VER >= 26 && __has_builtin(__builtin_is_within_lifetime)

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_IS_WITHIN_LIFETIME_H
