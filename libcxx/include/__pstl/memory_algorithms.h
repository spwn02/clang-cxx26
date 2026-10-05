// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___PSTL_MEMORY_ALGORITHMS_H
#define _LIBCPP___PSTL_MEMORY_ALGORITHMS_H

#include <__config>
#include <__memory/destroy.h>
#include <__memory/uninitialized_algorithms.h>
#include <__pstl/backend_fwd.h>
#include <__type_traits/type_identity.h>
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

// [algorithms.parallel.overloads]: the semantics of a parallel algorithm overload are those of the algorithm without the
// execution policy, so the default backend runs the sequential algorithms. If an element's constructor throws, the
// elements constructed so far are destroyed by the sequential algorithm, and the exception reaches the caller's
// noexcept helper ([algorithms.parallel.exceptions]: std::terminate).

template <class _ExecutionPolicy>
struct __memory_copy_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _InIter, class _Size, class _OutIter, class _ValueType>
  _LIBCPP_HIDE_FROM_ABI optional<pair<_InIter, _OutIter>>
  operator()(_Policy&&, _InIter __first, _Size __n, _OutIter __result, __type_identity<_ValueType>) const {
    return std::__uninitialized_copy_n<_ValueType>(
        std::move(__first), __n, std::move(__result), [](auto&&) { return false; });
  }
};

template <class _ExecutionPolicy>
struct __memory_move_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _InIter, class _Size, class _OutIter, class _ValueType, class _IterMove>
  _LIBCPP_HIDE_FROM_ABI optional<pair<_InIter, _OutIter>> operator()(
      _Policy&&, _InIter __first, _Size __n, _OutIter __result, __type_identity<_ValueType>, _IterMove __iter_move)
      const {
    return std::__uninitialized_move_n<_ValueType>(
        std::move(__first), __n, std::move(__result), [](auto&&) { return false; }, std::move(__iter_move));
  }
};

template <class _ExecutionPolicy>
struct __memory_fill_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Size, class _Tp, class _ValueType>
  _LIBCPP_HIDE_FROM_ABI optional<_Iter>
  operator()(_Policy&&, _Iter __first, _Size __n, const _Tp& __x, __type_identity<_ValueType>) const {
    return std::__uninitialized_fill_n<_ValueType>(std::move(__first), __n, __x);
  }
};

template <class _ExecutionPolicy>
struct __memory_default_construct_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Size, class _ValueType>
  _LIBCPP_HIDE_FROM_ABI optional<_Iter>
  operator()(_Policy&&, _Iter __first, _Size __n, __type_identity<_ValueType>) const {
    return std::__uninitialized_default_construct_n<_ValueType>(std::move(__first), __n);
  }
};

template <class _ExecutionPolicy>
struct __memory_value_construct_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Size, class _ValueType>
  _LIBCPP_HIDE_FROM_ABI optional<_Iter>
  operator()(_Policy&&, _Iter __first, _Size __n, __type_identity<_ValueType>) const {
    return std::__uninitialized_value_construct_n<_ValueType>(std::move(__first), __n);
  }
};

template <class _ExecutionPolicy>
struct __memory_destroy_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Size>
  _LIBCPP_HIDE_FROM_ABI optional<_Iter> operator()(_Policy&&, _Iter __first, _Size __n) const {
    for (; __n > 0; (void)++__first, --__n)
      std::__destroy_at(std::addressof(*__first));
    return __first;
  }
};

} // namespace __pstl
_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_POP_MACROS

#endif // _LIBCPP___PSTL_MEMORY_ALGORITHMS_H
