//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_UNWRAP_ITER_H
#define _LIBCPP___ALGORITHM_UNWRAP_ITER_H

#include <__config>
#include <__iterator/bounded_iter.h>
#include <__iterator/iterator_traits.h>
#include <__cstddef/size_t.h>
#include <__iterator/static_bounded_iter.h>
#include <__memory/pointer_traits.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_constructible.h>
#include <__utility/declval.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

// TODO: Change the name of __unwrap_iter_impl to something more appropriate
// The job of __unwrap_iter is to remove iterator wrappers (like reverse_iterator or __wrap_iter),
// to reduce the number of template instantiations and to enable pointer-based optimizations e.g. in std::copy.
//
// Some algorithms (e.g. std::copy, but not std::sort) need to convert an
// "unwrapped" result back into the original iterator type. Doing that is the job of __rewrap_iter.

// Default case - we can't unwrap anything
template <class _Iter, bool = __libcpp_is_contiguous_iterator<_Iter>::value>
struct __unwrap_iter_impl {
  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR _Iter __rewrap(_Iter, _Iter __iter) { return __iter; }
  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR _Iter __unwrap(_Iter __i) _NOEXCEPT { return __i; }
};

// A bounded iterator carries the bounds of its container so that accesses through it are checked. Unwrapping it to a raw
// pointer would drop them: a memmove-based algorithm such as std::copy would write through the whole output range
// before any check could run. Bounded iterators are therefore never unwrapped (the algorithms fall back to the
// element-wise loops, which dereference the iterator).
template <class _Iter>
struct __unwrap_iter_impl<__bounded_iter<_Iter>, true> {
  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR __bounded_iter<_Iter>
  __rewrap(__bounded_iter<_Iter>, __bounded_iter<_Iter> __iter) {
    return __iter;
  }
  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR __bounded_iter<_Iter> __unwrap(__bounded_iter<_Iter> __i) _NOEXCEPT {
    return __i;
  }
};

template <class _Iter, size_t _Size>
struct __unwrap_iter_impl<__static_bounded_iter<_Iter, _Size>, true> {
  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR __static_bounded_iter<_Iter, _Size>
  __rewrap(__static_bounded_iter<_Iter, _Size>, __static_bounded_iter<_Iter, _Size> __iter) {
    return __iter;
  }
  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR __static_bounded_iter<_Iter, _Size>
  __unwrap(__static_bounded_iter<_Iter, _Size> __i) _NOEXCEPT {
    return __i;
  }
};

// It's a contiguous iterator, so we can use a raw pointer instead
template <class _Iter>
struct __unwrap_iter_impl<_Iter, true> {
  using _ToAddressT _LIBCPP_NODEBUG = decltype(std::__to_address(std::declval<_Iter>()));

  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR _Iter __rewrap(_Iter __orig_iter, _ToAddressT __unwrapped_iter) {
    return __orig_iter + (__unwrapped_iter - std::__to_address(__orig_iter));
  }

  static _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR _ToAddressT __unwrap(_Iter __i) _NOEXCEPT {
    return std::__to_address(__i);
  }
};

template <class _Iter,
          class _Impl                                             = __unwrap_iter_impl<_Iter>,
          __enable_if_t<is_copy_constructible<_Iter>::value, int> = 0>
inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX14 decltype(_Impl::__unwrap(std::declval<_Iter>()))
__unwrap_iter(_Iter __i) _NOEXCEPT {
  return _Impl::__unwrap(__i);
}

// Allow input_iterators to be passed to __unwrap_iter (but not __rewrap_iter)
#if _LIBCPP_STD_VER >= 20
template <class _Iter, __enable_if_t<!is_copy_constructible<_Iter>::value, int> = 0>
inline _LIBCPP_HIDE_FROM_ABI constexpr _Iter __unwrap_iter(_Iter __i) noexcept {
  return __i;
}
#endif

template <class _OrigIter, class _Iter, class _Impl = __unwrap_iter_impl<_OrigIter> >
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR _OrigIter __rewrap_iter(_OrigIter __orig_iter, _Iter __iter) _NOEXCEPT {
  return _Impl::__rewrap(std::move(__orig_iter), std::move(__iter));
}

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_UNWRAP_ITER_H
