//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___PSTL_BACKENDS_DEFAULT_H
#define _LIBCPP___PSTL_BACKENDS_DEFAULT_H

#include <__algorithm/copy_n.h>
#include <__algorithm/equal.h>
#include <__algorithm/fill_n.h>
#include <__algorithm/for_each_n.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/not_fn.h>
#include <__functional/operations.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/reverse_iterator.h>
#include <__pstl/backend_fwd.h>
#include <__pstl/dispatch.h>
#include <__utility/empty.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__utility/pair.h>
#include <optional>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 17

_LIBCPP_BEGIN_NAMESPACE_STD
namespace __pstl {

//
// This file provides an incomplete PSTL backend that implements all of the PSTL algorithms
// based on a smaller set of basis operations.
//
// It is intended as a building block for other PSTL backends that implement some operations more
// efficiently but may not want to define the full set of PSTL algorithms.
//
// This backend implements all the PSTL algorithms based on the following basis operations:
//
// find_if family
// --------------
// - find
// - find_if_not
// - any_of
// - all_of
// - none_of
// - is_partitioned
//
// for_each family
// ---------------
// - for_each_n
// - fill
// - fill_n
// - replace
// - replace_if
// - generate
// - generate_n
//
// merge family
// ------------
// No other algorithms based on merge
//
// stable_sort family
// ------------------
// - sort
//
// transform_reduce and transform_reduce_binary family
// ---------------------------------------------------
// - count_if
// - count
// - equal(3 legs)
// - equal
// - reduce
//
// transform and transform_binary family
// -------------------------------------
// - replace_copy_if
// - replace_copy
// - move
// - copy
// - copy_n
// - rotate_copy
//

//////////////////////////////////////////////////////////////
// find_if family
//////////////////////////////////////////////////////////////
template <class _Difference>
class __pstl_counting_iterator {
public:
  using iterator_category = random_access_iterator_tag;
  using value_type        = _Difference;
  using difference_type   = _Difference;
  using pointer           = _Difference*;
  using reference         = _Difference;

  constexpr __pstl_counting_iterator() : __value_(0) {}
  constexpr explicit __pstl_counting_iterator(_Difference __value) : __value_(__value) {}
  constexpr reference operator*() const { return __value_; }
  constexpr reference operator[](difference_type __n) const { return __value_ + __n; }
  constexpr __pstl_counting_iterator operator++(int) { auto __copy = *this; ++*this; return __copy; }
  constexpr __pstl_counting_iterator operator--(int) { auto __copy = *this; --*this; return __copy; }
  constexpr __pstl_counting_iterator& operator++() { ++__value_; return *this; }
  constexpr __pstl_counting_iterator& operator--() { --__value_; return *this; }
  constexpr __pstl_counting_iterator& operator+=(difference_type __n) { __value_ += __n; return *this; }
  constexpr __pstl_counting_iterator& operator-=(difference_type __n) { __value_ -= __n; return *this; }
  constexpr friend __pstl_counting_iterator operator+(__pstl_counting_iterator __it, difference_type __n) {
    return __it += __n;
  }
  constexpr friend __pstl_counting_iterator operator+(difference_type __n, __pstl_counting_iterator __it) {
    return __it += __n;
  }
  constexpr friend __pstl_counting_iterator operator-(__pstl_counting_iterator __it, difference_type __n) {
    return __it -= __n;
  }
  constexpr friend difference_type operator-(__pstl_counting_iterator __x, __pstl_counting_iterator __y) {
    return __x.__value_ - __y.__value_;
  }
  constexpr friend bool operator==(__pstl_counting_iterator __x, __pstl_counting_iterator __y) {
    return __x.__value_ == __y.__value_;
  }
  constexpr friend bool operator!=(__pstl_counting_iterator __x, __pstl_counting_iterator __y) { return !(__x == __y); }
  constexpr friend bool operator<(__pstl_counting_iterator __x, __pstl_counting_iterator __y) {
    return __x.__value_ < __y.__value_;
  }
  constexpr friend bool operator>(__pstl_counting_iterator __x, __pstl_counting_iterator __y) { return __y < __x; }
  constexpr friend bool operator<=(__pstl_counting_iterator __x, __pstl_counting_iterator __y) { return !(__y < __x); }
  constexpr friend bool operator>=(__pstl_counting_iterator __x, __pstl_counting_iterator __y) { return !(__x < __y); }

private:
  _Difference __value_;
};

template <class _ExecutionPolicy>
struct __find<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, const _Tp& __value) const noexcept {
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    return _FindIf()(
        __policy, std::move(__first), std::move(__last), [&](__iterator_reference<_ForwardIterator> __element) {
          return __element == __value;
        });
  }
};

template <class _ExecutionPolicy>
struct __find_if_not<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred&& __pred) const noexcept {
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    return _FindIf()(__policy, __first, __last, std::not_fn(std::forward<_Pred>(__pred)));
  }
};

template <class _ExecutionPolicy>
struct __any_of<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<bool>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred&& __pred) const noexcept {
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    auto __res    = _FindIf()(__policy, __first, __last, std::forward<_Pred>(__pred));
    if (!__res)
      return nullopt;
    return *__res != __last;
  }
};

template <class _ExecutionPolicy>
struct __all_of<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<bool>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred&& __pred) const noexcept {
    using _AnyOf = __dispatch<__any_of, __current_configuration, _ExecutionPolicy>;
    auto __res   = _AnyOf()(__policy, __first, __last, [&](__iterator_reference<_ForwardIterator> __value) {
      return !__pred(__value);
    });
    if (!__res)
      return nullopt;
    return !*__res;
  }
};

template <class _ExecutionPolicy>
struct __none_of<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<bool>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred&& __pred) const noexcept {
    using _AnyOf = __dispatch<__any_of, __current_configuration, _ExecutionPolicy>;
    auto __res   = _AnyOf()(__policy, __first, __last, std::forward<_Pred>(__pred));
    if (!__res)
      return nullopt;
    return !*__res;
  }
};

template <class _ExecutionPolicy>
struct __is_partitioned<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<bool>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred&& __pred) const noexcept {
    using _FindIfNot   = __dispatch<__find_if_not, __current_configuration, _ExecutionPolicy>;
    auto __maybe_first = _FindIfNot()(__policy, std::move(__first), __last, __pred);
    if (__maybe_first == nullopt)
      return nullopt;

    __first = *__maybe_first;
    if (__first == __last)
      return true;
    ++__first;
    using _NoneOf = __dispatch<__none_of, __current_configuration, _ExecutionPolicy>;
    return _NoneOf()(__policy, std::move(__first), std::move(__last), __pred);
  }
};

template <class _ExecutionPolicy>
struct __adjacent_find<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _RandomAccessIterator, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_RandomAccessIterator> operator()(
      _Policy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last, _Pred&& __pred) const noexcept {
    if (__first == __last || __last - __first == 1)
      return __last;

    using _Difference = typename iterator_traits<_RandomAccessIterator>::difference_type;
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    auto __res = _FindIf()(__policy, __pstl_counting_iterator<_Difference>(0),
                           __pstl_counting_iterator<_Difference>(__last - __first - 1),
                           [=, __pred = std::forward<_Pred>(__pred)](_Difference __i) mutable {
                             return __pred(__first[__i], __first[__i + 1]);
                           });
    if (!__res)
      return nullopt;
    return *__res == __last - __first - 1 ? __last : __first + *__res;
  }
};

template <class _ExecutionPolicy>
struct __mismatch<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _RandomAccessIterator1, class _RandomAccessIterator2, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<pair<_RandomAccessIterator1, _RandomAccessIterator2>> operator()(
      _Policy&& __policy,
      _RandomAccessIterator1 __first1,
      _RandomAccessIterator1 __last1,
      _RandomAccessIterator2 __first2,
      _RandomAccessIterator2 __last2,
      _Pred&& __pred) const noexcept {
    using _Difference1 = typename iterator_traits<_RandomAccessIterator1>::difference_type;
    using _Difference2 = typename iterator_traits<_RandomAccessIterator2>::difference_type;
    _Difference1 __len1 = __last1 - __first1;
    _Difference2 __len2 = __last2 - __first2;
    auto __length = __len1 < __len2 ? __len1 : __len2;
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    auto __res = _FindIf()(__policy, __pstl_counting_iterator<_Difference1>(0),
                           __pstl_counting_iterator<_Difference1>(__length),
                           [=, __pred = std::forward<_Pred>(__pred)](_Difference1 __i) mutable {
                             return !__pred(__first1[__i], __first2[__i]);
                           });
    if (!__res)
      return nullopt;
    auto __i = *__res;
    return pair<_RandomAccessIterator1, _RandomAccessIterator2>{__first1 + __i, __first2 + __i};
  }
};

template <class _ExecutionPolicy>
struct __search<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _RandomAccessIterator1, class _RandomAccessIterator2, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_RandomAccessIterator1> operator()(
      _Policy&& __policy,
      _RandomAccessIterator1 __first1,
      _RandomAccessIterator1 __last1,
      _RandomAccessIterator2 __first2,
      _RandomAccessIterator2 __last2,
      _Pred&& __pred) const noexcept {
    using _Difference1 = typename iterator_traits<_RandomAccessIterator1>::difference_type;
    using _Difference2 = typename iterator_traits<_RandomAccessIterator2>::difference_type;
    _Difference1 __len1 = __last1 - __first1;
    _Difference2 __len2 = __last2 - __first2;
    if (__len2 == 0)
      return __first1;
    if (__len2 > __len1)
      return __last1;
    using _Difference = typename iterator_traits<_RandomAccessIterator1>::difference_type;
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    auto __res = _FindIf()(__policy, __pstl_counting_iterator<_Difference>(0),
                           __pstl_counting_iterator<_Difference>(__len1 - __len2 + 1),
                           [=, __pred = std::forward<_Pred>(__pred)](_Difference __candidate) mutable {
      for (_Difference2 __i = 0; __i != __len2; ++__i)
        if (!__pred(__first1[__candidate + __i], __first2[__i]))
          return false;
      return true;
    });
    if (!__res)
      return nullopt;
    return *__res == __len1 - __len2 + 1 ? __last1 : __first1 + *__res;
  }
};

template <class _ExecutionPolicy>
struct __find_first_of<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _RandomAccessIterator1, class _RandomAccessIterator2, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_RandomAccessIterator1> operator()(
      _Policy&& __policy,
      _RandomAccessIterator1 __first1,
      _RandomAccessIterator1 __last1,
      _RandomAccessIterator2 __first2,
      _RandomAccessIterator2 __last2,
      _Pred&& __pred) const noexcept {
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    auto __res = _FindIf()(__policy, __first1, __last1, [=, __pred = std::forward<_Pred>(__pred)](auto&& __value) mutable {
      for (auto __it = __first2; __it != __last2; ++__it)
        if (__pred(__value, *__it))
          return true;
      return false;
    });
    return __res;
  }
};

template <class _ExecutionPolicy>
struct __find_end<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _RandomAccessIterator1, class _RandomAccessIterator2, class _Pred>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_RandomAccessIterator1> operator()(
      _Policy&& __policy,
      _RandomAccessIterator1 __first1,
      _RandomAccessIterator1 __last1,
      _RandomAccessIterator2 __first2,
      _RandomAccessIterator2 __last2,
      _Pred&& __pred) const noexcept {
    using _Difference1 = typename iterator_traits<_RandomAccessIterator1>::difference_type;
    using _Difference2 = typename iterator_traits<_RandomAccessIterator2>::difference_type;
    _Difference1 __len1 = __last1 - __first1;
    _Difference2 __len2 = __last2 - __first2;
    if (__len2 == 0)
      return __last1;
    if (__len2 > __len1)
      return __last1;
    using _Difference = typename iterator_traits<_RandomAccessIterator1>::difference_type;
    using _FindIf = __dispatch<__find_if, __current_configuration, _ExecutionPolicy>;
    auto __res = _FindIf()(__policy, __pstl_counting_iterator<_Difference>(0),
                           __pstl_counting_iterator<_Difference>(__len1 - __len2 + 1),
                           [=, __pred = std::forward<_Pred>(__pred)](_Difference __candidate) mutable {
                             for (_Difference2 __i = 0; __i != __len2; ++__i)
                               if (!__pred(__first1[__len1 - __len2 - __candidate + __i], __first2[__i]))
                                 return false;
                             return true;
                           });
    if (!__res)
      return nullopt;
    return *__res == __len1 - __len2 + 1 ? __last1 : __first1 + (__len1 - __len2 - *__res);
  }
};

//////////////////////////////////////////////////////////////
// for_each family
//////////////////////////////////////////////////////////////
template <class _ExecutionPolicy>
struct __for_each_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Size, class _Function>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy, _ForwardIterator __first, _Size __size, _Function __func) const noexcept {
    if constexpr (__has_random_access_iterator_category_or_concept<_ForwardIterator>::value) {
      using _ForEach          = __dispatch<__for_each, __current_configuration, _ExecutionPolicy>;
      _ForwardIterator __last = __first + __size;
      return _ForEach()(__policy, std::move(__first), std::move(__last), std::move(__func));
    } else {
      // Otherwise, use the serial algorithm to avoid doing two passes over the input
      std::for_each_n(std::move(__first), __size, std::move(__func));
      return __empty{};
    }
  }
};

template <class _ExecutionPolicy>
struct __fill<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Tp const& __value) const noexcept {
    using _ForEach = __dispatch<__for_each, __current_configuration, _ExecutionPolicy>;
    using _Ref     = __iterator_reference<_ForwardIterator>;
    return _ForEach()(__policy, std::move(__first), std::move(__last), [&](_Ref __element) { __element = __value; });
  }
};

template <class _ExecutionPolicy>
struct __fill_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Size, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy, _ForwardIterator __first, _Size __n, _Tp const& __value) const noexcept {
    if constexpr (__has_random_access_iterator_category_or_concept<_ForwardIterator>::value) {
      using _Fill             = __dispatch<__fill, __current_configuration, _ExecutionPolicy>;
      _ForwardIterator __last = __first + __n;
      return _Fill()(__policy, std::move(__first), std::move(__last), __value);
    } else {
      // Otherwise, use the serial algorithm to avoid doing two passes over the input
      std::fill_n(std::move(__first), __n, __value);
      return optional<__empty>{__empty{}};
    }
  }
};

template <class _ExecutionPolicy>
struct __replace<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Tp const& __old, _Tp const& __new)
      const noexcept {
    using _ReplaceIf = __dispatch<__replace_if, __current_configuration, _ExecutionPolicy>;
    using _Ref       = __iterator_reference<_ForwardIterator>;
    return _ReplaceIf()(
        __policy, std::move(__first), std::move(__last), [&](_Ref __element) { return __element == __old; }, __new);
  }
};

template <class _ExecutionPolicy>
struct __replace_if<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Pred, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty> operator()(
      _Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Pred&& __pred, _Tp const& __new_value)
      const noexcept {
    using _ForEach = __dispatch<__for_each, __current_configuration, _ExecutionPolicy>;
    using _Ref     = __iterator_reference<_ForwardIterator>;
    return _ForEach()(__policy, std::move(__first), std::move(__last), [&](_Ref __element) {
      if (__pred(__element))
        __element = __new_value;
    });
  }
};

template <class _ExecutionPolicy>
struct __generate<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Generator>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Generator&& __gen) const noexcept {
    using _ForEach = __dispatch<__for_each, __current_configuration, _ExecutionPolicy>;
    using _Ref     = __iterator_reference<_ForwardIterator>;
    return _ForEach()(__policy, std::move(__first), std::move(__last), [&](_Ref __element) { __element = __gen(); });
  }
};

template <class _ExecutionPolicy>
struct __generate_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Size, class _Generator>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy, _ForwardIterator __first, _Size __n, _Generator&& __gen) const noexcept {
    using _ForEachN = __dispatch<__for_each_n, __current_configuration, _ExecutionPolicy>;
    using _Ref      = __iterator_reference<_ForwardIterator>;
    return _ForEachN()(__policy, std::move(__first), __n, [&](_Ref __element) { __element = __gen(); });
  }
};

//////////////////////////////////////////////////////////////
// stable_sort family
//////////////////////////////////////////////////////////////
template <class _ExecutionPolicy>
struct __sort<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _RandomAccessIterator, class _Comp>
  _LIBCPP_HIDE_FROM_ABI optional<__empty> operator()(
      _Policy&& __policy, _RandomAccessIterator __first, _RandomAccessIterator __last, _Comp&& __comp) const noexcept {
    using _StableSort = __dispatch<__stable_sort, __current_configuration, _ExecutionPolicy>;
    return _StableSort()(__policy, std::move(__first), std::move(__last), std::forward<_Comp>(__comp));
  }
};

//////////////////////////////////////////////////////////////
// transform_reduce family
//////////////////////////////////////////////////////////////
template <class _ExecutionPolicy>
struct __count_if<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Predicate>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__iterator_difference_type<_ForwardIterator>> operator()(
      _Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Predicate&& __pred) const noexcept {
    using _TransformReduce = __dispatch<__transform_reduce, __current_configuration, _ExecutionPolicy>;
    using _DiffT           = __iterator_difference_type<_ForwardIterator>;
    using _Ref             = __iterator_reference<_ForwardIterator>;
    return _TransformReduce()(
        __policy, std::move(__first), std::move(__last), _DiffT{}, std::plus{}, [&](_Ref __element) -> _DiffT {
          return __pred(__element) ? _DiffT(1) : _DiffT(0);
        });
  }
};

template <class _ExecutionPolicy>
struct __count<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__iterator_difference_type<_ForwardIterator>>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Tp const& __value) const noexcept {
    using _CountIf = __dispatch<__count_if, __current_configuration, _ExecutionPolicy>;
    using _Ref     = __iterator_reference<_ForwardIterator>;
    return _CountIf()(__policy, std::move(__first), std::move(__last), [&](_Ref __element) -> bool {
      return __element == __value;
    });
  }
};

template <class _ExecutionPolicy>
struct __equal_3leg<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _Predicate>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<bool>
  operator()(_Policy&& __policy,
             _ForwardIterator1 __first1,
             _ForwardIterator1 __last1,
             _ForwardIterator2 __first2,
             _Predicate&& __pred) const noexcept {
    using _TransformReduce = __dispatch<__transform_reduce_binary, __current_configuration, _ExecutionPolicy>;
    return _TransformReduce()(
        __policy,
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        true,
        std::logical_and{},
        std::forward<_Predicate>(__pred));
  }
};

template <class _ExecutionPolicy>
struct __equal<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _Predicate>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<bool>
  operator()(_Policy&& __policy,
             _ForwardIterator1 __first1,
             _ForwardIterator1 __last1,
             _ForwardIterator2 __first2,
             _ForwardIterator2 __last2,
             _Predicate&& __pred) const noexcept {
    if constexpr (__has_random_access_iterator_category<_ForwardIterator1>::value &&
                  __has_random_access_iterator_category<_ForwardIterator2>::value) {
      if (__last1 - __first1 != __last2 - __first2)
        return false;
      // Fall back to the 3 legged algorithm
      using _Equal3Leg = __dispatch<__equal_3leg, __current_configuration, _ExecutionPolicy>;
      return _Equal3Leg()(
          __policy, std::move(__first1), std::move(__last1), std::move(__first2), std::forward<_Predicate>(__pred));
    } else {
      // If we don't have random access, fall back to the serial algorithm cause we can't do much
      return std::equal(
          std::move(__first1),
          std::move(__last1),
          std::move(__first2),
          std::move(__last2),
          std::forward<_Predicate>(__pred));
    }
  }
};

template <class _ExecutionPolicy>
struct __reduce<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Tp, class _BinaryOperation>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_Tp>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _Tp __init, _BinaryOperation&& __op)
      const noexcept {
    using _TransformReduce = __dispatch<__transform_reduce, __current_configuration, _ExecutionPolicy>;
    return _TransformReduce()(
        __policy,
        std::move(__first),
        std::move(__last),
        std::move(__init),
        std::forward<_BinaryOperation>(__op),
        __identity{});
  }
};

//////////////////////////////////////////////////////////////
// transform family
//////////////////////////////////////////////////////////////
template <class _ExecutionPolicy>
struct __replace_copy_if<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _ForwardOutIterator, class _Pred, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy,
             _ForwardIterator __first,
             _ForwardIterator __last,
             _ForwardOutIterator __out_it,
             _Pred&& __pred,
             _Tp const& __new_value) const noexcept {
    using _Transform = __dispatch<__transform, __current_configuration, _ExecutionPolicy>;
    using _Ref       = __iterator_reference<_ForwardIterator>;
    auto __res =
        _Transform()(__policy, std::move(__first), std::move(__last), std::move(__out_it), [&](_Ref __element) {
          return __pred(__element) ? __new_value : __element;
        });
    if (__res == nullopt)
      return nullopt;
    return __empty{};
  }
};

template <class _ExecutionPolicy>
struct __replace_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _ForwardOutIterator, class _Tp>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&& __policy,
             _ForwardIterator __first,
             _ForwardIterator __last,
             _ForwardOutIterator __out_it,
             _Tp const& __old_value,
             _Tp const& __new_value) const noexcept {
    using _ReplaceCopyIf = __dispatch<__replace_copy_if, __current_configuration, _ExecutionPolicy>;
    using _Ref           = __iterator_reference<_ForwardIterator>;
    return _ReplaceCopyIf()(
        __policy,
        std::move(__first),
        std::move(__last),
        std::move(__out_it),
        [&](_Ref __element) { return __element == __old_value; },
        __new_value);
  }
};

// TODO: Use the std::copy/move shenanigans to forward to std::memmove
//       Investigate whether we want to still forward to std::transform(policy)
//       in that case for the execution::par part, or whether we actually want
//       to run everything serially in that case.
template <class _ExecutionPolicy>
struct __move<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _ForwardOutIterator>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __out_it)
      const noexcept {
    using _Transform = __dispatch<__transform, __current_configuration, _ExecutionPolicy>;
    return _Transform()(__policy, std::move(__first), std::move(__last), std::move(__out_it), [&](auto&& __element) {
      return std::move(__element);
    });
  }
};

// TODO: Use the std::copy/move shenanigans to forward to std::memmove
template <class _ExecutionPolicy>
struct __copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _ForwardOutIterator>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator>
  operator()(_Policy&& __policy, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __out_it)
      const noexcept {
    using _Transform = __dispatch<__transform, __current_configuration, _ExecutionPolicy>;
    return _Transform()(__policy, std::move(__first), std::move(__last), std::move(__out_it), __identity());
  }
};

template <class _ExecutionPolicy>
struct __copy_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Size, class _ForwardOutIterator>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator>
  operator()(_Policy&& __policy, _ForwardIterator __first, _Size __n, _ForwardOutIterator __out_it) const noexcept {
    if constexpr (__has_random_access_iterator_category_or_concept<_ForwardIterator>::value) {
      using _Copy             = __dispatch<__copy, __current_configuration, _ExecutionPolicy>;
      _ForwardIterator __last = __first + __n;
      return _Copy()(__policy, std::move(__first), std::move(__last), std::move(__out_it));
    } else {
      // Otherwise, use the serial algorithm to avoid doing two passes over the input
      return std::copy_n(std::move(__first), __n, std::move(__out_it));
    }
  }
};

template <class _ExecutionPolicy>
struct __rotate_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _ForwardOutIterator>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator>
  operator()(_Policy&& __policy,
             _ForwardIterator __first,
             _ForwardIterator __middle,
             _ForwardIterator __last,
             _ForwardOutIterator __out_it) const noexcept {
    using _Copy       = __dispatch<__copy, __current_configuration, _ExecutionPolicy>;
    auto __result_mid = _Copy()(__policy, __middle, std::move(__last), std::move(__out_it));
    if (__result_mid == nullopt)
      return nullopt;
    return _Copy()(__policy, std::move(__first), std::move(__middle), *std::move(__result_mid));
  }
};

template <class _ExecutionPolicy>
struct __reverse_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _BidirectionalIterator, class _ForwardOutIterator>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator> operator()(
      _Policy&& __policy, _BidirectionalIterator __first, _BidirectionalIterator __last, _ForwardOutIterator __out_it)
      const noexcept {
    using _Copy = __dispatch<__copy, __current_configuration, _ExecutionPolicy>;
    return _Copy()(
        __policy,
        std::make_reverse_iterator(std::move(__last)),
        std::make_reverse_iterator(std::move(__first)),
        std::move(__out_it));
  }
};

} // namespace __pstl
_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_POP_MACROS

#endif // _LIBCPP___PSTL_BACKENDS_DEFAULT_H
