//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_COMPLETION_FUNCTIONS_H
#define _LIBCPP___EXECUTION_COMPLETION_FUNCTIONS_H

#include <__config>
#include <__type_traits/is_const.h>
#include <__type_traits/is_reference.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.set.value], [exec.set.error], [exec.set.stopped]
// Each of set_value(rcvr, vs...)/set_error(rcvr, err)/set_stopped(rcvr) is ill-formed
// if `rcvr` is an lvalue or an rvalue of const type; the `!is_lvalue_reference_v<_Rcvr>
// && !is_const_v<_Rcvr>` pair on a forwarding-reference parameter is how that's expressed
// as an overload-participation constraint (an lvalue argument deduces _Rcvr as a reference
// type; a const-rvalue argument deduces _Rcvr as const).
struct set_value_t {
  template <class _Rcvr, class... _Args>
    requires(!is_lvalue_reference_v<_Rcvr> && !is_const_v<_Rcvr>) &&
            requires(_Rcvr&& __rcvr, _Args&&... __args) {
              std::forward<_Rcvr>(__rcvr).set_value(std::forward<_Args>(__args)...);
            }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator()(_Rcvr&& __rcvr, _Args&&... __args) const noexcept {
    static_assert(noexcept(std::forward<_Rcvr>(__rcvr).set_value(std::forward<_Args>(__args)...)),
                  "Mandates: the expression rcvr.set_value(vs...) is noexcept.");
    std::forward<_Rcvr>(__rcvr).set_value(std::forward<_Args>(__args)...);
  }
};

struct set_error_t {
  template <class _Rcvr, class _Err>
    requires(!is_lvalue_reference_v<_Rcvr> && !is_const_v<_Rcvr>) &&
            requires(_Rcvr&& __rcvr, _Err&& __err) { std::forward<_Rcvr>(__rcvr).set_error(std::forward<_Err>(__err)); }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator()(_Rcvr&& __rcvr, _Err&& __err) const noexcept {
    static_assert(noexcept(std::forward<_Rcvr>(__rcvr).set_error(std::forward<_Err>(__err))),
                  "Mandates: the expression rcvr.set_error(err) is noexcept.");
    std::forward<_Rcvr>(__rcvr).set_error(std::forward<_Err>(__err));
  }
};

struct set_stopped_t {
  template <class _Rcvr>
    requires(!is_lvalue_reference_v<_Rcvr> && !is_const_v<_Rcvr>) &&
            requires(_Rcvr&& __rcvr) { std::forward<_Rcvr>(__rcvr).set_stopped(); }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator()(_Rcvr&& __rcvr) const noexcept {
    static_assert(noexcept(std::forward<_Rcvr>(__rcvr).set_stopped()),
                  "Mandates: the expression rcvr.set_stopped() is noexcept.");
    std::forward<_Rcvr>(__rcvr).set_stopped();
  }
};

inline constexpr set_value_t set_value{};
inline constexpr set_error_t set_error{};
inline constexpr set_stopped_t set_stopped{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_COMPLETION_FUNCTIONS_H
