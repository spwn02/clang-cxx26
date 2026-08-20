//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_ALLOCATOR_H
#define _LIBCPP___EXECUTION_GET_ALLOCATOR_H

#include <__concepts/constructible.h>
#include <__concepts/equality_comparable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__execution/forwarding_query.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// [allocator.requirements.general]'s exposition-only `simple-allocator`: the minimal
// allocator interface `get_allocator`'s Mandates require of a query's result.
template <class _Alloc>
concept __simple_allocator =
    requires(_Alloc __alloc, size_t __n) {
      { *__alloc.allocate(__n) } -> same_as<typename _Alloc::value_type&>;
      { __alloc.deallocate(__alloc.allocate(__n), __n) };
    } && copy_constructible<_Alloc> && equality_comparable<_Alloc>;

// [exec.get.allocator]
// Declared directly in namespace std (not std::execution) per [execution.syn].
struct get_allocator_t : forwarding_query_t {
  template <class _Env>
    requires requires(const _Env& __env, const get_allocator_t& __self) {
      { __env.query(__self) } -> __simple_allocator;
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept(noexcept(__env.query(*this))) {
    return __env.query(*this);
  }
};

inline constexpr get_allocator_t get_allocator{};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_ALLOCATOR_H
