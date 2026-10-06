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
#include <__type_traits/conditional.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_function.h>
#include <__type_traits/is_object.h>
#include <__utility/constant_wrapper.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD

template <class...>
class function_ref;

template <class _Tp>
inline constexpr bool __is_constant_wrapper_v = false;

template <auto _Cp, class _Fp>
inline constexpr bool __is_constant_wrapper_v<constant_wrapper<_Cp, _Fp>> = true;

// [func.wrap.ref.ctor] is-convertible-from-specialization<F> (P3961R1): _From is function_ref<R(Args...) cv2 noexcept(noex2)>
// with the same R and Args as the specialization being constructed (_Self = R(Args...) noexcept(noex), _SelfConst = cv is const).
template <class _From, class _Self, bool _SelfConst, class _Rp, class... _Args>
inline constexpr bool __function_ref_convertible_from = false;

template <class _Self, bool _SelfConst, class _Rp, class... _Args, bool _Nx2>
inline constexpr bool __function_ref_convertible_from<function_ref<_Rp(_Args...) noexcept(_Nx2)>, _Self, _SelfConst, _Rp, _Args...> =
    is_convertible_v<_Rp (&)(_Args...) noexcept(_Nx2), _Self&> &&
    is_convertible_v<__conditional_t<_SelfConst, const int, int>&, int&>;

template <class _Self, bool _SelfConst, class _Rp, class... _Args, bool _Nx2>
inline constexpr bool
    __function_ref_convertible_from<function_ref<_Rp(_Args...) const noexcept(_Nx2)>, _Self, _SelfConst, _Rp, _Args...> =
        is_convertible_v<_Rp (&)(_Args...) noexcept(_Nx2), _Self&> &&
        is_convertible_v<__conditional_t<_SelfConst, const int, int>&, const int&>;

// [func.wrap.ref.deduct]: the function type deduced for function_ref(constant_wrapper<c, F>, T&&)
template <class _Fp, class _Tp>
struct __function_ref_deduce {};

#  define _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN(_CV, _REF, _NOEX)                                                          \
    template <class _Rp, class _Gp, class... _Ap, class _Tp>                                                           \
    struct __function_ref_deduce<_Rp (_Gp::*)(_Ap...) _CV _REF noexcept(_NOEX), _Tp> {                                \
      using type = _Rp(_Ap...) noexcept(_NOEX);                                                                        \
    };
#  define _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_NOEX(_CV, _REF)                                                            \
    _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN(_CV, _REF, false)                                                                \
    _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN(_CV, _REF, true)
#  define _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_REF(_CV)                                                                   \
    _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_NOEX(_CV, )                                                                      \
    _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_NOEX(_CV, &)
_LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_REF()
_LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_REF(const)
_LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_REF(volatile)
_LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_REF(const volatile)
#  undef _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_REF
#  undef _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN_NOEX
#  undef _LIBCPP_FUNCTION_REF_DEDUCE_MEMFN

template <class _Mp, class _Gp, class _Tp>
  requires is_object_v<_Mp>
struct __function_ref_deduce<_Mp _Gp::*, _Tp> {
  using type = invoke_result_t<_Mp _Gp::*, _Tp&>() noexcept;
};

template <class _Rp, class _Gp, class... _Ap, bool _Ex, class _Tp>
struct __function_ref_deduce<_Rp (*)(_Gp, _Ap...) noexcept(_Ex), _Tp> {
  using type = _Rp(_Ap...) noexcept(_Ex);
};

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
