//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// This header is unguarded on purpose. It generates the cv-qualified
// specializations of std::function_ref. Unlike std::move_only_function,
// function_ref has no ref-qualified specializations ([func.wrap.ref]).

#include <__assert>
#include <__config>
#include <__functional/function_ref_common.h>
#include <__functional/invoke.h>
#include <__memory/addressof.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_function.h>
#include <__type_traits/is_member_pointer.h>
#include <__type_traits/is_pointer.h>
#include <__type_traits/is_const.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/is_same.h>
#include <__type_traits/invoke.h>
#include <__type_traits/remove_cv.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/remove_pointer.h>
#include <__type_traits/remove_reference.h>
#include <__utility/constant_wrapper.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#ifndef _LIBCPP_IN_FUNCTION_REF_H
#  error This header should only be included from function_ref.h
#endif

#ifndef _LIBCPP_FUNCTION_REF_CV
#  define _LIBCPP_FUNCTION_REF_CV
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

template <class...>
class function_ref;

template <class _Rp, class... _ArgTypes, bool _Np>
class function_ref<_Rp(_ArgTypes...) _LIBCPP_FUNCTION_REF_CV noexcept(_Np)> {
private:
  template <class... _Tp>
  static constexpr bool __is_invocable_using =
      _Np ? is_nothrow_invocable_r_v<_Rp, _Tp..., _ArgTypes...> : is_invocable_r_v<_Rp, _Tp..., _ArgTypes...>;

  // [func.wrap.ref.ctor]: is-convertible-from-specialization<F> (P3961R1)
  template <class _Fp>
  static constexpr bool __is_convertible_from_specialization = __function_ref_convertible_from<
      _Fp,
      _Rp(_ArgTypes...) noexcept(_Np),
      is_const_v<int _LIBCPP_FUNCTION_REF_CV>,
      _Rp,
      _ArgTypes...>;

  using _Thunk _LIBCPP_NODEBUG = _Rp (*)(__function_ref_bound_entity, _ArgTypes&&...) noexcept(_Np);

  template <class...>
  friend class function_ref;

  __function_ref_bound_entity __bound_;
  _Thunk __thunk_;

public:
  template <class _Fp>
    requires(is_function_v<_Fp> && __is_invocable_using<_Fp&>)
  _LIBCPP_HIDE_FROM_ABI function_ref(_Fp* __f) noexcept
      : __bound_(__f), __thunk_([](__function_ref_bound_entity __bound_entity_, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          return std::invoke_r<_Rp>(*__bound_entity_.template __get_function<_Fp>(), std::forward<_ArgTypes>(__args)...);
        }) {
    _LIBCPP_ASSERT_NON_NULL(__f != nullptr, "function_ref cannot be constructed from a null function pointer");
  }

  template <class _Fp>
    requires(!is_same_v<remove_cvref_t<_Fp>, function_ref> && !is_member_pointer_v<remove_reference_t<_Fp>> &&
             __is_invocable_using<_LIBCPP_FUNCTION_REF_CV remove_reference_t<_Fp>&>)
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(_Fp&& __f) noexcept
      : __bound_(std::addressof(__f)),
        __thunk_([](__function_ref_bound_entity __bound_entity_, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          using _Tp _LIBCPP_NODEBUG = remove_reference_t<_Fp>;
          if constexpr (is_function_v<_Tp>) {
            return std::invoke_r<_Rp>(*__bound_entity_.template __get_function<_Tp>(), std::forward<_ArgTypes>(__args)...);
          } else {
            return std::invoke_r<_Rp>(static_cast<_LIBCPP_FUNCTION_REF_CV _Tp&>(*__bound_entity_.template __get_object<_Tp>()),
                                       std::forward<_ArgTypes>(__args)...);
          }
        }) {
    // P3961R1: a function_ref that converts to this one is copied rather than referenced (no double indirection).
    using _Tp _LIBCPP_NODEBUG = remove_cv_t<remove_reference_t<_Fp>>;
    if constexpr (__is_convertible_from_specialization<_Tp>) {
      __bound_ = __f.__bound_;
      __thunk_ = __f.__thunk_;
    }
  }

  // P3948R1: constant_wrapper replaces constant_arg_t.
  template <auto _Cp, class _Fp>
    requires __is_invocable_using<const _Fp&>
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(constant_wrapper<_Cp, _Fp>) noexcept
      : __bound_(), __thunk_([](__function_ref_bound_entity, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          return std::invoke_r<_Rp>(constant_wrapper<_Cp, _Fp>::value, std::forward<_ArgTypes>(__args)...);
        }) {
    if constexpr (is_pointer_v<_Fp> || is_member_pointer_v<_Fp>) {
      static_assert(constant_wrapper<_Cp, _Fp>::value != nullptr, "function_ref cannot be constructed from a null function pointer");
    }
    if constexpr (sizeof...(_ArgTypes) != 0 && (__constexpr_param<remove_cvref_t<_ArgTypes>> && ...)) {
      static_assert(
          !requires { typename constant_wrapper<std::invoke(constant_wrapper<_Cp, _Fp>::value, remove_cvref_t<_ArgTypes>::value...)>; },
          "function_ref cannot be constructed from a constant_wrapper whose invocation with constant arguments is a constant "
          "expression");
    }
  }

  template <auto _Cp, class _Fp, class _Up>
    requires(!is_rvalue_reference_v<_Up&&> &&
             __is_invocable_using<const _Fp&, _LIBCPP_FUNCTION_REF_CV remove_reference_t<_Up>&>)
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(constant_wrapper<_Cp, _Fp>, _Up&& __obj) noexcept
      : __bound_(std::addressof(__obj)),
        __thunk_([](__function_ref_bound_entity __bound_entity_, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          using _Tp _LIBCPP_NODEBUG = remove_reference_t<_Up>;
          return std::invoke_r<_Rp>(constant_wrapper<_Cp, _Fp>::value,
                                     static_cast<_LIBCPP_FUNCTION_REF_CV _Tp&>(*__bound_entity_.template __get_object<_Tp>()),
                                     std::forward<_ArgTypes>(__args)...);
        }) {
    if constexpr (is_pointer_v<_Fp> || is_member_pointer_v<_Fp>) {
      static_assert(constant_wrapper<_Cp, _Fp>::value != nullptr, "function_ref cannot be constructed from a null function pointer");
    }
  }

  template <auto _Cp, class _Fp, class _Tp>
    requires __is_invocable_using<const _Fp&, _LIBCPP_FUNCTION_REF_CV _Tp*>
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(constant_wrapper<_Cp, _Fp>, _LIBCPP_FUNCTION_REF_CV _Tp* __obj) noexcept
      : __bound_(__obj),
        __thunk_([](__function_ref_bound_entity __bound_entity_, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          return std::invoke_r<_Rp>(constant_wrapper<_Cp, _Fp>::value,
                                     __bound_entity_.template __get_object<_LIBCPP_FUNCTION_REF_CV _Tp>(),
                                     std::forward<_ArgTypes>(__args)...);
        }) {
    if constexpr (is_member_pointer_v<_Fp>) {
      _LIBCPP_ASSERT_NON_NULL(
          __obj != nullptr, "function_ref cannot be constructed from a null object pointer when f is a member pointer");
    }
    if constexpr (is_pointer_v<_Fp> || is_member_pointer_v<_Fp>) {
      static_assert(constant_wrapper<_Cp, _Fp>::value != nullptr, "function_ref cannot be constructed from a null function pointer");
    }
  }

  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(const function_ref&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref& operator=(const function_ref&) noexcept = default;

  template <class _Tp>
    requires(!__is_convertible_from_specialization<_Tp> && !is_pointer_v<_Tp> && !__is_constant_wrapper_v<_Tp>)
  function_ref& operator=(_Tp) = delete;

  _LIBCPP_HIDE_FROM_ABI _Rp operator()(_ArgTypes... __args) const noexcept(_Np) {
    return __thunk_(__bound_, std::forward<_ArgTypes>(__args)...);
  }
};

#undef _LIBCPP_FUNCTION_REF_CV

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS
