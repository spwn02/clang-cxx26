// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_RANGES_DESTROY_H
#define _LIBCPP___MEMORY_RANGES_DESTROY_H

#include <__algorithm/min.h>
#include <__pstl/backend_fwd.h>
#include <__pstl/dispatch.h>
#include <__pstl/handle_exception.h>
#include <__pstl/memory_algorithms.h>
#include <__type_traits/common_type.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/remove_reference.h>
#include <__type_traits/type_identity.h>
#include <__utility/forward.h>
#include <__iterator/iter_move.h>
#include <__concepts/constructible.h>
#include <__iterator/concepts.h>
#include <__concepts/destructible.h>
#include <__config>
#include <__iterator/incrementable_traits.h>
#include <__iterator/iterator_traits.h>
#include <__memory/concepts.h>
#include <__memory/destroy.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20
namespace ranges {

// destroy

struct __destroy {
  template <__nothrow_input_iterator _InputIterator, __nothrow_sentinel_for<_InputIterator> _Sentinel>
    requires destructible<iter_value_t<_InputIterator>>
  _LIBCPP_HIDE_FROM_ABI constexpr _InputIterator operator()(_InputIterator __first, _Sentinel __last) const noexcept {
    return std::__destroy(std::move(__first), std::move(__last));
  }

  template <__nothrow_input_range _InputRange>
    requires destructible<range_value_t<_InputRange>>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_InputRange> operator()(_InputRange&& __range) const noexcept {
    return (*this)(ranges::begin(__range), ranges::end(__range));
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            __nothrow_sized_sentinel_for<_Iter> _Sent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires destructible<iter_value_t<_Iter>>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, _Sent __last) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_destroy_n, __pstl::__current_configuration, _RawPolicy>;
    auto __n = __last - __first;
    return __pstl::__handle_exception<_Implementation>(std::forward<_Ep>(__exec), std::move(__first), __n);
  }

  template <class _Ep,
            __nothrow_sized_random_access_range _Range,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires destructible<range_value_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(_Ep&& __exec, _Range&& __range) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::begin(__range) + static_cast<range_difference_t<_Range>>(ranges::size(__range)));
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto destroy = __destroy{};
} // namespace __cpo

// destroy_n

struct __destroy_n {
  template <__nothrow_input_iterator _InputIterator>
    requires destructible<iter_value_t<_InputIterator>>
  _LIBCPP_HIDE_FROM_ABI constexpr _InputIterator
  operator()(_InputIterator __first, iter_difference_t<_InputIterator> __n) const noexcept {
    return std::destroy_n(std::move(__first), __n);
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires destructible<iter_value_t<_Iter>>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, iter_difference_t<_Iter> __n) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_destroy_n, __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(std::forward<_Ep>(__exec), std::move(__first), __n);
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto destroy_n = __destroy_n{};
} // namespace __cpo

} // namespace ranges

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MEMORY_RANGES_DESTROY_H
