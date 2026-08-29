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
#include <__type_traits/is_function.h>
#include <__type_traits/is_member_pointer.h>
#include <__type_traits/is_pointer.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/is_same.h>
#include <__type_traits/invoke.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/remove_reference.h>
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

  using _Thunk _LIBCPP_NODEBUG = _Rp (*)(__function_ref_bound_entity, _ArgTypes&&...) noexcept(_Np);

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
        }) {}

  template <auto _Fp>
    requires __is_invocable_using<decltype(_Fp)>
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(nontype_t<_Fp>) noexcept
      : __bound_(), __thunk_([](__function_ref_bound_entity, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          return std::invoke_r<_Rp>(_Fp, std::forward<_ArgTypes>(__args)...);
        }) {
    using _Fn _LIBCPP_NODEBUG = decltype(_Fp);
    if constexpr (is_pointer_v<_Fn> || is_member_pointer_v<_Fn>) {
      static_assert(_Fp != nullptr, "function_ref cannot be constructed from a null function pointer");
    }
  }

  template <auto _Fp, class _Up>
    requires(!is_rvalue_reference_v<_Up&&> &&
             __is_invocable_using<decltype(_Fp), _LIBCPP_FUNCTION_REF_CV remove_reference_t<_Up>&>)
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(nontype_t<_Fp>, _Up&& __obj) noexcept
      : __bound_(std::addressof(__obj)),
        __thunk_([](__function_ref_bound_entity __bound_entity_, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          using _Tp _LIBCPP_NODEBUG = remove_reference_t<_Up>;
          return std::invoke_r<_Rp>(_Fp,
                                     static_cast<_LIBCPP_FUNCTION_REF_CV _Tp&>(*__bound_entity_.template __get_object<_Tp>()),
                                     std::forward<_ArgTypes>(__args)...);
        }) {
    using _Fn _LIBCPP_NODEBUG = decltype(_Fp);
    if constexpr (is_pointer_v<_Fn> || is_member_pointer_v<_Fn>) {
      static_assert(_Fp != nullptr, "function_ref cannot be constructed from a null function pointer");
    }
  }

  template <auto _Fp, class _Tp>
    requires __is_invocable_using<decltype(_Fp), _LIBCPP_FUNCTION_REF_CV _Tp*>
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(nontype_t<_Fp>, _LIBCPP_FUNCTION_REF_CV _Tp* __obj) noexcept
      : __bound_(__obj),
        __thunk_([](__function_ref_bound_entity __bound_entity_, _ArgTypes&&... __args) noexcept(_Np) -> _Rp {
          return std::invoke_r<_Rp>(_Fp,
                                     __bound_entity_.template __get_object<_LIBCPP_FUNCTION_REF_CV _Tp>(),
                                     std::forward<_ArgTypes>(__args)...);
        }) {
    using _Fn _LIBCPP_NODEBUG = decltype(_Fp);
    if constexpr (is_member_pointer_v<_Fn>) {
      _LIBCPP_ASSERT_NON_NULL(
          __obj != nullptr, "function_ref cannot be constructed from a null object pointer when f is a member pointer");
    }
    if constexpr (is_pointer_v<_Fn> || is_member_pointer_v<_Fn>) {
      static_assert(_Fp != nullptr, "function_ref cannot be constructed from a null function pointer");
    }
  }

  _LIBCPP_HIDE_FROM_ABI constexpr function_ref(const function_ref&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI constexpr function_ref& operator=(const function_ref&) noexcept = default;

  template <class _Tp>
    requires(!is_same_v<_Tp, function_ref> && !is_pointer_v<_Tp> && !__is_nontype_t_v<_Tp>)
  function_ref& operator=(_Tp) = delete;

  _LIBCPP_HIDE_FROM_ABI _Rp operator()(_ArgTypes... __args) const noexcept(_Np) {
    return __thunk_(__bound_, std::forward<_ArgTypes>(__args)...);
  }
};

#undef _LIBCPP_FUNCTION_REF_CV

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS
