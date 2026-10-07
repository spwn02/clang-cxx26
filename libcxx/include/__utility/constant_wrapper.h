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
#include <__cstddef/size_t.h>
#include <__functional/invoke.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [const.wrap.class] (C++26, before P4206R0): cw-fixed-value<T> wraps the value; the array specialization lets a string
// literal (or any array) be the template argument.
template <class _Tp>
struct __cw_fixed_value {
  using type = _Tp;
  _LIBCPP_HIDE_FROM_ABI constexpr __cw_fixed_value(type __v) noexcept : __data(__v) {}
  _Tp __data;
};

template <class _Tp, size_t _Extent>
struct __cw_fixed_value<_Tp[_Extent]> {
  using type = _Tp[_Extent];
  _LIBCPP_HIDE_FROM_ABI constexpr __cw_fixed_value(_Tp (&__arr)[_Extent]) noexcept
      : __cw_fixed_value(__arr, make_index_sequence<_Extent>()) {}
  _Tp __data[_Extent];

private:
  template <size_t... _Is>
  _LIBCPP_HIDE_FROM_ABI constexpr __cw_fixed_value(_Tp (&__arr)[_Extent], index_sequence<_Is...>) noexcept
      : __data{__arr[_Is]...} {}
};

template <class _Tp, size_t _Extent>
__cw_fixed_value(_Tp (&)[_Extent]) -> __cw_fixed_value<_Tp[_Extent]>;

template <__cw_fixed_value _Xp, class = typename decltype(_Xp)::type>
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

  // Pseudo-mutators.
#  define _LIBCPP_CW_COMPOUND_ASSIGN(__op) \
  template <__constexpr_param _Tp, __constexpr_param _Rp> \
  constexpr auto operator __op(this _Tp, _Rp) noexcept -> constant_wrapper<(_Tp::value __op _Rp::value)> { \
    return {}; \
  }
  _LIBCPP_CW_COMPOUND_ASSIGN(+=)
  _LIBCPP_CW_COMPOUND_ASSIGN(-=)
  _LIBCPP_CW_COMPOUND_ASSIGN(*=)
  _LIBCPP_CW_COMPOUND_ASSIGN(/=)
  _LIBCPP_CW_COMPOUND_ASSIGN(%=)
  _LIBCPP_CW_COMPOUND_ASSIGN(&=)
  _LIBCPP_CW_COMPOUND_ASSIGN(|=)
  _LIBCPP_CW_COMPOUND_ASSIGN(^=)
  _LIBCPP_CW_COMPOUND_ASSIGN(<<=)
  _LIBCPP_CW_COMPOUND_ASSIGN(>>=)
#  undef _LIBCPP_CW_COMPOUND_ASSIGN

  template <__constexpr_param _Tp>
  constexpr auto operator++(this _Tp) noexcept -> constant_wrapper<(++_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  constexpr auto operator++(this _Tp, int) noexcept -> constant_wrapper<(_Tp::value++)> { return {}; }
  template <__constexpr_param _Tp>
  constexpr auto operator--(this _Tp) noexcept -> constant_wrapper<(--_Tp::value)> { return {}; }
  template <__constexpr_param _Tp>
  constexpr auto operator--(this _Tp, int) noexcept -> constant_wrapper<(_Tp::value--)> { return {}; }

};

template <__cw_fixed_value _Xp, class _Tp>
struct constant_wrapper : __constant_wrapper_operators {
  static constexpr const auto& value = _Xp.__data;
  using type       = constant_wrapper;
  using value_type = typename decltype(_Xp)::type;

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

template <__cw_fixed_value _Xp>
inline constexpr auto cw = constant_wrapper<_Xp>{};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___UTILITY_CONSTANT_WRAPPER_H
