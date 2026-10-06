//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FUNCTIONAL_FUNCTION_REF_H
#define _LIBCPP___FUNCTIONAL_FUNCTION_REF_H

#include <__config>
#include <__functional/function_ref_common.h>
#include <__type_traits/is_function.h>
#include <__type_traits/remove_pointer.h>
#include <__utility/constant_wrapper.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

// Generate both cv-qualified specializations. The noexcept variants are
// represented by the boolean noexcept template argument in each
// specialization (deduced from the abominable function type's
// noexcept-specifier, same trick as std::move_only_function).
#  define _LIBCPP_IN_FUNCTION_REF_H

#  include <__functional/function_ref_impl.h>

#  define _LIBCPP_FUNCTION_REF_CV const
#  include <__functional/function_ref_impl.h>

#  undef _LIBCPP_IN_FUNCTION_REF_H

_LIBCPP_BEGIN_NAMESPACE_STD

// [func.wrap.ref.deduct]
template <class _Fp>
  requires is_function_v<_Fp>
function_ref(_Fp*) -> function_ref<_Fp>;

template <auto _Cp, class _F0>
  requires is_function_v<remove_pointer_t<_F0>>
function_ref(constant_wrapper<_Cp, _F0>) -> function_ref<remove_pointer_t<_F0>>;

template <auto _Cp, class _Fp, class _Tp>
  requires requires { typename __function_ref_deduce<_Fp, _Tp>::type; }
function_ref(constant_wrapper<_Cp, _Fp>, _Tp&&) -> function_ref<typename __function_ref_deduce<_Fp, _Tp>::type>;

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP___FUNCTIONAL_FUNCTION_REF_H
