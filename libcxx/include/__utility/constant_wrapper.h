//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___UTILITY_CONSTANT_WRAPPER_H
#define _LIBCPP___UTILITY_CONSTANT_WRAPPER_H

#include <__config>
#include <__functional/invoke.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

template <auto _Xp, class = decltype(_Xp)>
struct constant_wrapper;

template <class _Tp>
concept __constexpr_param = requires { typename constant_wrapper<_Tp::value>; };

template <const auto& _Callable, class... _Args>
concept __constexpr_callable = (__constexpr_param<remove_cvref_t<_Args>> && ...) && requires {
  typename constant_wrapper<auto(std::invoke(_Callable, remove_cvref_t<_Args>::value...))>;
};

template <const auto& _Object, class... _Args>
concept __constexpr_indexable = (__constexpr_param<remove_cvref_t<_Args>> && ...) && requires {
  typename constant_wrapper<auto(_Object[remove_cvref_t<_Args>::value...])>;
};

struct __constant_wrapper_operators {
  // Unary operators.
  template <__constexpr_param _Tp>
  friend constexpr auto operator+(_Tp) noexcept -> constant_wrapper<(+_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  friend constexpr auto operator-(_Tp) noexcept -> constant_wrapper<(-_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  friend constexpr auto operator~(_Tp) noexcept -> constant_wrapper<(~_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  friend constexpr auto operator!(_Tp) noexcept -> constant_wrapper<(!_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  friend constexpr auto operator&(_Tp) noexcept -> constant_wrapper<(&_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  friend constexpr auto operator*(_Tp) noexcept -> constant_wrapper<(*_Tp::value)> { return {}; }

  // Binary arithmetic.
#  define _LIBCPP_CW_BINARY_ARITHMETIC(__op) \
  template <__constexpr_param _Lp, __constexpr_param _Rp> \
  friend constexpr auto operator __op(_Lp, _Rp) noexcept -> constant_wrapper<(_Lp::value __op _Rp::value)> { return {}; }
  _LIBCPP_CW_BINARY_ARITHMETIC(+)
  _LIBCPP_CW_BINARY_ARITHMETIC(-)
  _LIBCPP_CW_BINARY_ARITHMETIC(*)
  _LIBCPP_CW_BINARY_ARITHMETIC(/)
  _LIBCPP_CW_BINARY_ARITHMETIC(%)
#  undef _LIBCPP_CW_BINARY_ARITHMETIC

  // Binary bitwise operators.
#  define _LIBCPP_CW_BINARY_BITWISE(__op) \
  template <__constexpr_param _Lp, __constexpr_param _Rp> \
  friend constexpr auto operator __op(_Lp, _Rp) noexcept -> constant_wrapper<(_Lp::value __op _Rp::value)> { return {}; }
  _LIBCPP_CW_BINARY_BITWISE(<<)
  _LIBCPP_CW_BINARY_BITWISE(>>)
  _LIBCPP_CW_BINARY_BITWISE(&)
  _LIBCPP_CW_BINARY_BITWISE(|)
  _LIBCPP_CW_BINARY_BITWISE(^)
#  undef _LIBCPP_CW_BINARY_BITWISE

  // Binary logical operators.
  template <__constexpr_param _Lp, __constexpr_param _Rp>
    requires(!is_constructible_v<bool, decltype(_Lp::value)> || !is_constructible_v<bool, decltype(_Rp::value)>)
  friend constexpr auto operator&&(_Lp, _Rp) noexcept -> constant_wrapper<(_Lp::value && _Rp::value)> { return {}; }
  template <__constexpr_param _Lp, __constexpr_param _Rp>
    requires(!is_constructible_v<bool, decltype(_Lp::value)> || !is_constructible_v<bool, decltype(_Rp::value)>)
  friend constexpr auto operator||(_Lp, _Rp) noexcept -> constant_wrapper<(_Lp::value || _Rp::value)> { return {}; }

  // Comparisons.
#  define _LIBCPP_CW_COMPARISON(__op) \
  template <__constexpr_param _Lp, __constexpr_param _Rp> \
  friend constexpr auto operator __op(_Lp, _Rp) noexcept -> constant_wrapper<(_Lp::value __op _Rp::value)> { return {}; }
  _LIBCPP_CW_COMPARISON(<=>)
  _LIBCPP_CW_COMPARISON(<)
  _LIBCPP_CW_COMPARISON(<=)
  _LIBCPP_CW_COMPARISON(==)
  _LIBCPP_CW_COMPARISON(!=)
  _LIBCPP_CW_COMPARISON(>)
  _LIBCPP_CW_COMPARISON(>=)
#  undef _LIBCPP_CW_COMPARISON

  // Pointer-to-member and comma.
  template <__constexpr_param _Lp, __constexpr_param _Rp>
  friend constexpr auto operator->*(_Lp, _Rp) noexcept -> constant_wrapper<(_Lp::value ->* _Rp::value)> { return {}; }
  template <__constexpr_param _Lp, __constexpr_param _Rp>
  friend constexpr auto operator,(_Lp, _Rp) noexcept = delete;

  // Pseudo-mutators. Compound assignment operators are intentionally absent;
  // dependent compound assignments currently trigger a compiler assertion.
  template <__constexpr_param _Tp>
  constexpr auto operator++(this _Tp) noexcept -> constant_wrapper<(++_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  constexpr auto operator++(this _Tp, int) noexcept -> constant_wrapper<(_Tp::value++)> { return {}; }
  template <__constexpr_param _Tp>
  constexpr auto operator--(this _Tp) noexcept -> constant_wrapper<(--_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  constexpr auto operator--(this _Tp, int) noexcept -> constant_wrapper<(_Tp::value--)> { return {}; }

};

template <auto _Xp, class _Tp>
struct constant_wrapper : __constant_wrapper_operators {
  static constexpr decltype(auto) value = (_Xp);
  using type       = constant_wrapper;
  using value_type = decltype(_Xp);

  static_assert(is_same_v<_Tp, value_type>);

  constexpr operator decltype(value)() const noexcept { return value; }

  template <__constexpr_param _Rp>
  constexpr auto operator=(_Rp) const noexcept -> constant_wrapper<(value = _Rp::value)> { return {}; }

  template <class... _Args>
    requires __constexpr_callable<value, _Args...>
  static constexpr auto operator()(_Args&&...) noexcept
      -> constant_wrapper<auto(std::invoke(value, remove_cvref_t<_Args>::value...))> {
    return {};
  }

  template <class... _Args>
    requires(!__constexpr_callable<value, _Args...> && is_invocable_v<const value_type&, _Args&&...>)
  static constexpr decltype(auto) operator()(_Args&&... __args)
      noexcept(noexcept(std::invoke(value, std::forward<_Args>(__args)...))) {
    return std::invoke(value, std::forward<_Args>(__args)...);
  }

  template <class... _Args>
    requires __constexpr_indexable<value, _Args...>
  static constexpr auto operator[](_Args&&...) noexcept
      -> constant_wrapper<auto(value[remove_cvref_t<_Args>::value...])> {
    return {};
  }

  template <class... _Args>
    requires(!__constexpr_indexable<value, _Args...> && requires { value[declval<_Args>()...]; })
  static constexpr decltype(auto) operator[](_Args&&... __args) noexcept(noexcept(value[std::forward<_Args>(__args)...])) {
    return value[std::forward<_Args>(__args)...];
  }

};

template <auto _Xp>
inline constexpr auto cw = constant_wrapper<_Xp>{};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___UTILITY_CONSTANT_WRAPPER_H
