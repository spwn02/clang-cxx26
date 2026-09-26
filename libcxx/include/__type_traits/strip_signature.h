//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_STRIP_SIGNATURE_H
#define _LIBCPP___TYPE_TRAITS_STRIP_SIGNATURE_H

#include <__config>
#include <__type_traits/is_class.h>
#include <__type_traits/is_union.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 17

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Fp>
struct __strip_signature;

#  if defined(__cpp_static_call_operator) && __cpp_static_call_operator >= 202207L

template <class _Rp, class... _Args>
struct __strip_signature<_Rp (*)(_Args...)> {
  using type _LIBCPP_NODEBUG = _Rp(_Args...);
};

template <class _Rp, class... _Args>
struct __strip_signature<_Rp (*)(_Args...) noexcept> {
  using type _LIBCPP_NODEBUG = _Rp(_Args...);
};

#  endif // defined(__cpp_static_call_operator) && __cpp_static_call_operator >= 202207L

#  if defined(__cpp_explicit_this_parameter) && __cpp_explicit_this_parameter >= 202110L

// LWG3617: an operator() with an explicit object parameter has the type R(*)(G, A...), where G is (a reference to) a
// class type; the deduced signature does not include the object parameter.
template <class _Rp, class _Gp, class... _Args>
  requires(is_class_v<__remove_cvref_t<_Gp>> || is_union_v<__remove_cvref_t<_Gp>>)
struct __strip_signature<_Rp (*)(_Gp, _Args...)> {
  using type _LIBCPP_NODEBUG = _Rp(_Args...);
};

template <class _Rp, class _Gp, class... _Args>
  requires(is_class_v<__remove_cvref_t<_Gp>> || is_union_v<__remove_cvref_t<_Gp>>)
struct __strip_signature<_Rp (*)(_Gp, _Args...) noexcept> {
  using type _LIBCPP_NODEBUG = _Rp(_Args...);
};

#  endif // defined(__cpp_explicit_this_parameter) && __cpp_explicit_this_parameter >= 202110L

// clang-format off
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...)> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) volatile> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const volatile> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };

template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) &> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const &> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) volatile &> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const volatile &> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };

template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) volatile noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const volatile noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };

template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) & noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const & noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) volatile & noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct __strip_signature<_Rp (_Gp::*) (_Ap...) const volatile & noexcept> { using type _LIBCPP_NODEBUG = _Rp(_Ap...); };
// clang-format on

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 17

#endif // _LIBCPP___TYPE_TRAITS_STRIP_SIGNATURE_H
