//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FUNCTIONAL_FUNCTION_REF_COMMON_H
#define _LIBCPP___FUNCTIONAL_FUNCTION_REF_COMMON_H

#include <__config>
#include <__type_traits/is_function.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD

template <auto _Vp>
struct nontype_t {
  _LIBCPP_HIDE_FROM_ABI explicit nontype_t() = default;
};

template <auto _Vp>
inline constexpr nontype_t<_Vp> nontype{};

template <class _Tp>
inline constexpr bool __is_nontype_t_v = false;

template <auto _Vp>
inline constexpr bool __is_nontype_t_v<nontype_t<_Vp>> = true;

// Exposition-only `bound-entity`: a trivially copyable object capable of
// storing either a pointer to an object or a pointer to a function, per
// [func.wrap.ref]. Reads must use the accessor matching how the value was
// stored (object vs. function) — the two members are not layout-compatible,
// so reading the wrong one is undefined behavior.
union __function_ref_bound_entity {
  _LIBCPP_HIDE_FROM_ABI constexpr __function_ref_bound_entity() noexcept : __obj_(nullptr) {}

  template <class _Tp>
    requires(!is_function_v<_Tp>)
  _LIBCPP_HIDE_FROM_ABI constexpr __function_ref_bound_entity(_Tp* __p) noexcept
      : __obj_(const_cast<void*>(static_cast<const volatile void*>(__p))) {}

  template <class _Fp>
    requires is_function_v<_Fp>
  _LIBCPP_HIDE_FROM_ABI __function_ref_bound_entity(_Fp* __f) noexcept : __func_(reinterpret_cast<void (*)()>(__f)) {}

  template <class _Tp>
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp* __get_object() const noexcept {
    return static_cast<_Tp*>(__obj_);
  }

  template <class _Fp>
  _LIBCPP_HIDE_FROM_ABI _Fp* __get_function() const noexcept {
    return reinterpret_cast<_Fp*>(__func_);
  }

  void* __obj_;
  void (*__func_)();
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP___FUNCTIONAL_FUNCTION_REF_COMMON_H
