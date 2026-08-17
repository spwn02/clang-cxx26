// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SIMD_FLAGS_H
#define _LIBCPP___SIMD_FLAGS_H

#include <__bit/has_single_bit.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__type_traits/is_same.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace simd {

// [simd.expos]: convert-flag, aligned-flag and overaligned-flag<N> are exposition-only.
struct __convert_flag {};
struct __aligned_flag {};

template <size_t _Np>
struct __overaligned_flag {
  static constexpr size_t __alignment = _Np;
};

template <class _Tp>
inline constexpr bool __is_load_store_flag = false;

template <>
inline constexpr bool __is_load_store_flag<__convert_flag> = true;
template <>
inline constexpr bool __is_load_store_flag<__aligned_flag> = true;
template <size_t _Np>
inline constexpr bool __is_load_store_flag<__overaligned_flag<_Np>> = true;

// [simd.flags.overview]/2 -- Constraints: every type in the pack Flags is one of convert-flag,
// aligned-flag, or overaligned-flag<N>. Spelling this as a requires-clause on the primary template
// means flags<int> simply does not exist, which is what "Constraints" means for a class template.
template <class... _Flags>
  requires(__is_load_store_flag<_Flags> && ...)
struct flags {
  // [simd.flags.oper]/1: the result need only be set-equal to the union of the two packs, so plain
  // concatenation is conforming -- deduplication is permitted but not required.
  template <class... _Other>
  friend consteval auto operator|(flags, flags<_Other...>) {
    return flags<_Flags..., _Other...>{};
  }
};

inline constexpr flags<> flag_default{};
inline constexpr flags<__convert_flag> flag_convert{};
inline constexpr flags<__aligned_flag> flag_aligned{};

template <size_t _Np>
  requires(std::has_single_bit(_Np))
inline constexpr flags<__overaligned_flag<_Np>> flag_overaligned{};

// Query helpers used by the load/store and constructor machinery. These are what turn the flags
// from an inert tag pack into the Mandates and Preconditions of [simd.loadstore].
template <class... _Flags>
inline constexpr bool __flags_have_convert = (is_same_v<_Flags, __convert_flag> || ...);

template <class... _Flags>
inline constexpr bool __flags_have_aligned = (is_same_v<_Flags, __aligned_flag> || ...);

template <class _Tp>
inline constexpr size_t __overaligned_value = 0;

template <size_t _Np>
inline constexpr size_t __overaligned_value<__overaligned_flag<_Np>> = _Np;

// The strongest overalignment requested, or 0 when none is.
template <class... _Flags>
inline constexpr size_t __flags_overalignment = [] consteval {
  size_t __result = 0;
  ((__result = __overaligned_value<_Flags> > __result ? __overaligned_value<_Flags> : __result), ...);
  return __result;
}();

} // namespace simd

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___SIMD_FLAGS_H
