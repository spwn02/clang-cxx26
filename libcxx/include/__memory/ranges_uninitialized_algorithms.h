// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_RANGES_UNINITIALIZED_ALGORITHMS_H
#define _LIBCPP___MEMORY_RANGES_UNINITIALIZED_ALGORITHMS_H

#include <__algorithm/min.h>
#include <__type_traits/common_type.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/type_identity.h>
#include <__utility/forward.h>
#include <__concepts/destructible.h>
#include <__algorithm/in_out_result.h>
#include <__concepts/constructible.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/incrementable_traits.h>
#include <__iterator/iter_move.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/readable_traits.h>
#include <__memory/concepts.h>
#include <__memory/uninitialized_algorithms.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__type_traits/remove_reference.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>
#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
#  include <__pstl/backend_fwd.h>
#  include <__pstl/dispatch.h>
#  include <__pstl/handle_exception.h>
#  include <__pstl/memory_algorithms.h>
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

namespace ranges {

// uninitialized_default_construct

struct __uninitialized_default_construct {
  template <__nothrow_forward_iterator _ForwardIterator, __nothrow_sentinel_for<_ForwardIterator> _Sentinel>
    requires default_initializable<iter_value_t<_ForwardIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _ForwardIterator
  operator()(_ForwardIterator __first, _Sentinel __last) const {
    using _ValueType = remove_reference_t<iter_reference_t<_ForwardIterator>>;
    return std::__uninitialized_default_construct<_ValueType>(std::move(__first), std::move(__last));
  }

  template <__nothrow_forward_range _ForwardRange>
    requires default_initializable<range_value_t<_ForwardRange>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 borrowed_iterator_t<_ForwardRange>
  operator()(_ForwardRange&& __range) const {
    return (*this)(ranges::begin(__range), ranges::end(__range));
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            __nothrow_sized_sentinel_for<_Iter> _Sent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires default_initializable<iter_value_t<_Iter>>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, _Sent __last) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_default_construct_n, __pstl::__current_configuration, _RawPolicy>;
    auto __n = __last - __first;
    return __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__first),
        __n,
        __type_identity<remove_reference_t<iter_reference_t<_Iter>>>());
  }

  template <class _Ep,
            __nothrow_sized_random_access_range _Range,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires default_initializable<range_value_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(_Ep&& __exec, _Range&& __range) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::begin(__range) + static_cast<range_difference_t<_Range>>(ranges::size(__range)));
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_default_construct = __uninitialized_default_construct{};
} // namespace __cpo

// uninitialized_default_construct_n

struct __uninitialized_default_construct_n {
  template <__nothrow_forward_iterator _ForwardIterator>
    requires default_initializable<iter_value_t<_ForwardIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _ForwardIterator
  operator()(_ForwardIterator __first, iter_difference_t<_ForwardIterator> __n) const {
    using _ValueType = remove_reference_t<iter_reference_t<_ForwardIterator>>;
    return std::__uninitialized_default_construct_n<_ValueType>(std::move(__first), __n);
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires default_initializable<iter_value_t<_Iter>>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, iter_difference_t<_Iter> __n) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_default_construct_n, __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__first),
        __n,
        __type_identity<remove_reference_t<iter_reference_t<_Iter>>>());
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_default_construct_n = __uninitialized_default_construct_n{};
} // namespace __cpo

// uninitialized_value_construct

struct __uninitialized_value_construct {
  template <__nothrow_forward_iterator _ForwardIterator, __nothrow_sentinel_for<_ForwardIterator> _Sentinel>
    requires default_initializable<iter_value_t<_ForwardIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _ForwardIterator
  operator()(_ForwardIterator __first, _Sentinel __last) const {
    using _ValueType = remove_reference_t<iter_reference_t<_ForwardIterator>>;
    return std::__uninitialized_value_construct<_ValueType>(std::move(__first), std::move(__last));
  }

  template <__nothrow_forward_range _ForwardRange>
    requires default_initializable<range_value_t<_ForwardRange>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 borrowed_iterator_t<_ForwardRange>
  operator()(_ForwardRange&& __range) const {
    return (*this)(ranges::begin(__range), ranges::end(__range));
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            __nothrow_sized_sentinel_for<_Iter> _Sent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires default_initializable<iter_value_t<_Iter>>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, _Sent __last) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_value_construct_n, __pstl::__current_configuration, _RawPolicy>;
    auto __n = __last - __first;
    return __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__first),
        __n,
        __type_identity<remove_reference_t<iter_reference_t<_Iter>>>());
  }

  template <class _Ep,
            __nothrow_sized_random_access_range _Range,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires default_initializable<range_value_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(_Ep&& __exec, _Range&& __range) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::begin(__range) + static_cast<range_difference_t<_Range>>(ranges::size(__range)));
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_value_construct = __uninitialized_value_construct{};
} // namespace __cpo

// uninitialized_value_construct_n

struct __uninitialized_value_construct_n {
  template <__nothrow_forward_iterator _ForwardIterator>
    requires default_initializable<iter_value_t<_ForwardIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _ForwardIterator
  operator()(_ForwardIterator __first, iter_difference_t<_ForwardIterator> __n) const {
    using _ValueType = remove_reference_t<iter_reference_t<_ForwardIterator>>;
    return std::__uninitialized_value_construct_n<_ValueType>(std::move(__first), __n);
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires default_initializable<iter_value_t<_Iter>>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, iter_difference_t<_Iter> __n) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_value_construct_n, __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__first),
        __n,
        __type_identity<remove_reference_t<iter_reference_t<_Iter>>>());
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_value_construct_n = __uninitialized_value_construct_n{};
} // namespace __cpo

// uninitialized_fill

struct __uninitialized_fill {
  template <__nothrow_forward_iterator _ForwardIterator,
            __nothrow_sentinel_for<_ForwardIterator> _Sentinel,
            class _Tp
#if _LIBCPP_STD_VER >= 26
            = iter_value_t<_ForwardIterator> // P2248R8
#endif
            >
    requires constructible_from<iter_value_t<_ForwardIterator>, const _Tp&>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _ForwardIterator
  operator()(_ForwardIterator __first, _Sentinel __last, const _Tp& __x) const {
    using _ValueType = remove_reference_t<iter_reference_t<_ForwardIterator>>;
    return std::__uninitialized_fill<_ValueType>(std::move(__first), std::move(__last), __x);
  }

  template <__nothrow_forward_range _ForwardRange,
            class _Tp
#if _LIBCPP_STD_VER >= 26
            = range_value_t<_ForwardRange> // P2248R8
#endif
            >
    requires constructible_from<range_value_t<_ForwardRange>, const _Tp&>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 borrowed_iterator_t<_ForwardRange>
  operator()(_ForwardRange&& __range, const _Tp& __x) const {
    return (*this)(ranges::begin(__range), ranges::end(__range), __x);
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            __nothrow_sized_sentinel_for<_Iter> _Sent,
            class _Tp = iter_value_t<_Iter>,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires constructible_from<iter_value_t<_Iter>, const _Tp&>
  _LIBCPP_HIDE_FROM_ABI _Iter operator()(_Ep&& __exec, _Iter __first, _Sent __last, const _Tp& __x) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_fill_n, __pstl::__current_configuration, _RawPolicy>;
    auto __n = __last - __first;
    return __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__first),
        __n,
        __x,
        __type_identity<remove_reference_t<iter_reference_t<_Iter>>>());
  }

  template <class _Ep,
            __nothrow_sized_random_access_range _Range,
            class _Tp = range_value_t<_Range>,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires constructible_from<range_value_t<_Range>, const _Tp&>
  _LIBCPP_HIDE_FROM_ABI borrowed_iterator_t<_Range> operator()(_Ep&& __exec, _Range&& __range, const _Tp& __x) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::begin(__range) + static_cast<range_difference_t<_Range>>(ranges::size(__range)), __x);
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_fill = __uninitialized_fill{};
} // namespace __cpo

// uninitialized_fill_n

struct __uninitialized_fill_n {
  template <__nothrow_forward_iterator _ForwardIterator,
            class _Tp
#if _LIBCPP_STD_VER >= 26
            = iter_value_t<_ForwardIterator> // P2248R8
#endif
            >
    requires constructible_from<iter_value_t<_ForwardIterator>, const _Tp&>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _ForwardIterator
  operator()(_ForwardIterator __first, iter_difference_t<_ForwardIterator> __n, const _Tp& __x) const {
    using _ValueType = remove_reference_t<iter_reference_t<_ForwardIterator>>;
    return std::__uninitialized_fill_n<_ValueType>(std::move(__first), __n, __x);
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            __nothrow_random_access_iterator _Iter,
            class _Tp = iter_value_t<_Iter>,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires constructible_from<iter_value_t<_Iter>, const _Tp&>
  _LIBCPP_HIDE_FROM_ABI _Iter
  operator()(_Ep&& __exec, _Iter __first, iter_difference_t<_Iter> __n, const _Tp& __x) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_fill_n, __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__first),
        __n,
        __x,
        __type_identity<remove_reference_t<iter_reference_t<_Iter>>>());
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_fill_n = __uninitialized_fill_n{};
} // namespace __cpo

// uninitialized_copy

template <class _InputIterator, class _OutputIterator>
using uninitialized_copy_result = in_out_result<_InputIterator, _OutputIterator>;

struct __uninitialized_copy {
  template <input_iterator _InputIterator,
            sentinel_for<_InputIterator> _Sentinel1,
            __nothrow_forward_iterator _OutputIterator,
            __nothrow_sentinel_for<_OutputIterator> _Sentinel2>
    requires constructible_from<iter_value_t<_OutputIterator>, iter_reference_t<_InputIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 uninitialized_copy_result<_InputIterator, _OutputIterator>
  operator()(_InputIterator __ifirst, _Sentinel1 __ilast, _OutputIterator __ofirst, _Sentinel2 __olast) const {
    using _ValueType = remove_reference_t<iter_reference_t<_OutputIterator>>;

    auto __stop_copying = [&__olast](auto&& __out_iter) -> bool { return __out_iter == __olast; };
    auto __result       = std::__uninitialized_copy<_ValueType>(
        std::move(__ifirst), std::move(__ilast), std::move(__ofirst), __stop_copying);
    return {std::move(__result.first), std::move(__result.second)};
  }

  template <input_range _InputRange, __nothrow_forward_range _OutputRange>
    requires constructible_from<range_value_t<_OutputRange>, range_reference_t<_InputRange>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26
      uninitialized_copy_result<borrowed_iterator_t<_InputRange>, borrowed_iterator_t<_OutputRange>>
      operator()(_InputRange&& __in_range, _OutputRange&& __out_range) const {
    return (*this)(
        ranges::begin(__in_range), ranges::end(__in_range), ranges::begin(__out_range), ranges::end(__out_range));
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _InIter,
            sized_sentinel_for<_InIter> _InSent,
            __nothrow_random_access_iterator _OutIter,
            __nothrow_sized_sentinel_for<_OutIter> _OutSent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires constructible_from<iter_value_t<_OutIter>, iter_reference_t<_InIter>>
  _LIBCPP_HIDE_FROM_ABI uninitialized_copy_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __ifirst, _InSent __ilast, _OutIter __ofirst, _OutSent __olast) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_copy_n, __pstl::__current_configuration, _RawPolicy>;
    using _Diff = common_type_t<iter_difference_t<_InIter>, iter_difference_t<_OutIter>>;
    _Diff __n   = std::min<_Diff>(__ilast - __ifirst, __olast - __ofirst);
    auto __r    = __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__ifirst),
        __n,
        std::move(__ofirst),
        __type_identity<remove_reference_t<iter_reference_t<_OutIter>>>()
);
    return {std::move(__r.first), std::move(__r.second)};
  }

  template <class _Ep,
            class _InRange,
            class _OutRange,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires random_access_range<_InRange> && sized_range<_InRange> && __nothrow_sized_random_access_range<_OutRange> &&
             constructible_from<range_value_t<_OutRange>, range_reference_t<_InRange>>
  _LIBCPP_HIDE_FROM_ABI uninitialized_copy_result<borrowed_iterator_t<_InRange>, borrowed_iterator_t<_OutRange>>
  operator()(_Ep&& __exec, _InRange&& __in_range, _OutRange&& __out_range) const {
    return (*this)(
        std::forward<_Ep>(__exec),
        ranges::begin(__in_range),
        ranges::begin(__in_range) + static_cast<range_difference_t<_InRange>>(ranges::size(__in_range)),
        ranges::begin(__out_range),
        ranges::begin(__out_range) + static_cast<range_difference_t<_OutRange>>(ranges::size(__out_range)));
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_copy = __uninitialized_copy{};
} // namespace __cpo

// uninitialized_copy_n

template <class _InputIterator, class _OutputIterator>
using uninitialized_copy_n_result = in_out_result<_InputIterator, _OutputIterator>;

struct __uninitialized_copy_n {
  template <input_iterator _InputIterator,
            __nothrow_forward_iterator _OutputIterator,
            __nothrow_sentinel_for<_OutputIterator> _Sentinel>
    requires constructible_from<iter_value_t<_OutputIterator>, iter_reference_t<_InputIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 uninitialized_copy_n_result<_InputIterator, _OutputIterator>
  operator()(_InputIterator __ifirst,
             iter_difference_t<_InputIterator> __n,
             _OutputIterator __ofirst,
             _Sentinel __olast) const {
    using _ValueType    = remove_reference_t<iter_reference_t<_OutputIterator>>;
    auto __stop_copying = [&__olast](auto&& __out_iter) -> bool { return __out_iter == __olast; };
    auto __result =
        std::__uninitialized_copy_n<_ValueType>(std::move(__ifirst), __n, std::move(__ofirst), __stop_copying);
    return {std::move(__result.first), std::move(__result.second)};
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _InIter,
            __nothrow_random_access_iterator _OutIter,
            __nothrow_sized_sentinel_for<_OutIter> _OutSent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires constructible_from<iter_value_t<_OutIter>, iter_reference_t<_InIter>>
  _LIBCPP_HIDE_FROM_ABI uninitialized_copy_n_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __ifirst, iter_difference_t<_InIter> __n, _OutIter __ofirst, _OutSent __olast) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_copy_n, __pstl::__current_configuration, _RawPolicy>;
    using _Diff = common_type_t<iter_difference_t<_InIter>, iter_difference_t<_OutIter>>;
    _Diff __count = std::min<_Diff>(__n, __olast - __ofirst);
    auto __r      = __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__ifirst),
        __count,
        std::move(__ofirst),
        __type_identity<remove_reference_t<iter_reference_t<_OutIter>>>());
    return {std::move(__r.first), std::move(__r.second)};
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_copy_n = __uninitialized_copy_n{};
} // namespace __cpo

// uninitialized_move

template <class _InputIterator, class _OutputIterator>
using uninitialized_move_result = in_out_result<_InputIterator, _OutputIterator>;

struct __uninitialized_move {
  template <input_iterator _InputIterator,
            sentinel_for<_InputIterator> _Sentinel1,
            __nothrow_forward_iterator _OutputIterator,
            __nothrow_sentinel_for<_OutputIterator> _Sentinel2>
    requires constructible_from<iter_value_t<_OutputIterator>, iter_rvalue_reference_t<_InputIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 uninitialized_move_result<_InputIterator, _OutputIterator>
  operator()(_InputIterator __ifirst, _Sentinel1 __ilast, _OutputIterator __ofirst, _Sentinel2 __olast) const {
    using _ValueType   = remove_reference_t<iter_reference_t<_OutputIterator>>;
    auto __iter_move   = [](auto&& __iter) -> decltype(auto) { return ranges::iter_move(__iter); };
    auto __stop_moving = [&__olast](auto&& __out_iter) -> bool { return __out_iter == __olast; };
    auto __result      = std::__uninitialized_move<_ValueType>(
        std::move(__ifirst), std::move(__ilast), std::move(__ofirst), __stop_moving, __iter_move);
    return {std::move(__result.first), std::move(__result.second)};
  }

  template <input_range _InputRange, __nothrow_forward_range _OutputRange>
    requires constructible_from<range_value_t<_OutputRange>, range_rvalue_reference_t<_InputRange>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26
      uninitialized_move_result<borrowed_iterator_t<_InputRange>, borrowed_iterator_t<_OutputRange>>
      operator()(_InputRange&& __in_range, _OutputRange&& __out_range) const {
    return (*this)(
        ranges::begin(__in_range), ranges::end(__in_range), ranges::begin(__out_range), ranges::end(__out_range));
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _InIter,
            sized_sentinel_for<_InIter> _InSent,
            __nothrow_random_access_iterator _OutIter,
            __nothrow_sized_sentinel_for<_OutIter> _OutSent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires constructible_from<iter_value_t<_OutIter>, iter_rvalue_reference_t<_InIter>>
  _LIBCPP_HIDE_FROM_ABI uninitialized_move_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __ifirst, _InSent __ilast, _OutIter __ofirst, _OutSent __olast) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_move_n, __pstl::__current_configuration, _RawPolicy>;
    using _Diff = common_type_t<iter_difference_t<_InIter>, iter_difference_t<_OutIter>>;
    _Diff __n   = std::min<_Diff>(__ilast - __ifirst, __olast - __ofirst);
    auto __r    = __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__ifirst),
        __n,
        std::move(__ofirst),
        __type_identity<remove_reference_t<iter_reference_t<_OutIter>>>(),
        [](auto&& __iter) -> decltype(auto) { return ranges::iter_move(__iter); });
    return {std::move(__r.first), std::move(__r.second)};
  }

  template <class _Ep,
            class _InRange,
            class _OutRange,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires random_access_range<_InRange> && sized_range<_InRange> && __nothrow_sized_random_access_range<_OutRange> &&
             constructible_from<range_value_t<_OutRange>, range_rvalue_reference_t<_InRange>>
  _LIBCPP_HIDE_FROM_ABI uninitialized_move_result<borrowed_iterator_t<_InRange>, borrowed_iterator_t<_OutRange>>
  operator()(_Ep&& __exec, _InRange&& __in_range, _OutRange&& __out_range) const {
    return (*this)(
        std::forward<_Ep>(__exec),
        ranges::begin(__in_range),
        ranges::begin(__in_range) + static_cast<range_difference_t<_InRange>>(ranges::size(__in_range)),
        ranges::begin(__out_range),
        ranges::begin(__out_range) + static_cast<range_difference_t<_OutRange>>(ranges::size(__out_range)));
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_move = __uninitialized_move{};
} // namespace __cpo

// uninitialized_move_n

template <class _InputIterator, class _OutputIterator>
using uninitialized_move_n_result = in_out_result<_InputIterator, _OutputIterator>;

struct __uninitialized_move_n {
  template <input_iterator _InputIterator,
            __nothrow_forward_iterator _OutputIterator,
            __nothrow_sentinel_for<_OutputIterator> _Sentinel>
    requires constructible_from<iter_value_t<_OutputIterator>, iter_rvalue_reference_t<_InputIterator>>
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 uninitialized_move_n_result<_InputIterator, _OutputIterator>
  operator()(_InputIterator __ifirst,
             iter_difference_t<_InputIterator> __n,
             _OutputIterator __ofirst,
             _Sentinel __olast) const {
    using _ValueType   = remove_reference_t<iter_reference_t<_OutputIterator>>;
    auto __iter_move   = [](auto&& __iter) -> decltype(auto) { return ranges::iter_move(__iter); };
    auto __stop_moving = [&__olast](auto&& __out_iter) -> bool { return __out_iter == __olast; };
    auto __result      = std::__uninitialized_move_n<_ValueType>(
        std::move(__ifirst), __n, std::move(__ofirst), __stop_moving, __iter_move);
    return {std::move(__result.first), std::move(__result.second)};
  }
#  if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep,
            random_access_iterator _InIter,
            __nothrow_random_access_iterator _OutIter,
            __nothrow_sized_sentinel_for<_OutIter> _OutSent,
            class _RawPolicy = __remove_cvref_t<_Ep>,
            enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires constructible_from<iter_value_t<_OutIter>, iter_rvalue_reference_t<_InIter>>
  _LIBCPP_HIDE_FROM_ABI uninitialized_move_n_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __ifirst, iter_difference_t<_InIter> __n, _OutIter __ofirst, _OutSent __olast) const {
    using _Implementation = __pstl::__dispatch<__pstl::__memory_move_n, __pstl::__current_configuration, _RawPolicy>;
    using _Diff = common_type_t<iter_difference_t<_InIter>, iter_difference_t<_OutIter>>;
    _Diff __count = std::min<_Diff>(__n, __olast - __ofirst);
    auto __r      = __pstl::__handle_exception<_Implementation>(
        std::forward<_Ep>(__exec),
        std::move(__ifirst),
        __count,
        std::move(__ofirst),
        __type_identity<remove_reference_t<iter_reference_t<_OutIter>>>(),
        [](auto&& __iter) -> decltype(auto) { return ranges::iter_move(__iter); });
    return {std::move(__r.first), std::move(__r.second)};
  }
#  endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_EXPERIMENTAL_PSTL && !defined(_LIBCPP_FREESTANDING)
};

inline namespace __cpo {
inline constexpr auto uninitialized_move_n = __uninitialized_move_n{};
} // namespace __cpo

} // namespace ranges

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MEMORY_RANGES_UNINITIALIZED_ALGORITHMS_H
