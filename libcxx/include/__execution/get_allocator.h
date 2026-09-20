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
#include <__execution/get_env.h>
#include <__memory/uses_allocator_construction.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward_like.h>
#include <tuple>

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
  // The constraint checks the *decayed* query result against simple-allocator, not the raw
  // `decltype((__env.query(__self)))` a compound-requirement would otherwise use: many
  // queryable environments (e.g. prop, per its own [exec.prop]-matching design) return their
  // stored value via `const Value&` for efficiency, which is fine for read-only queries but
  // would make simple-allocator's own `alloc.allocate(n)` check (a non-const member function
  // on every standard allocator, including pmr::polymorphic_allocator) fail on the
  // const-qualified reference type -- not because the allocator itself is unusable, only
  // because the *reference* is const. operator() itself already returns by decayed `auto`
  // (a fresh copy), so only the constraint needed to match that intent.
  template <class _Env>
    requires requires(const _Env& __env, const get_allocator_t& __self) {
      { __env.query(__self) };
      requires __simple_allocator<remove_cvref_t<decltype(__env.query(__self))>>;
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept(noexcept(__env.query(*this))) {
    return __env.query(*this);
  }
};

inline constexpr get_allocator_t get_allocator{};

// [exec.snd.expos] allocator-aware-forward. The generic basic-sender machinery is not
// present in this fork, so hand-written sender adaptors call this helper from their
// connect() implementations. std::tuple is the product-type used by those adaptors.
template <class _Tp>
struct __is_execution_product_type : false_type {};

template <class... _Ts>
struct __is_execution_product_type<tuple<_Ts...>> : true_type {};

template <class _Tp, class _Alloc, size_t... _Is>
_LIBCPP_HIDE_FROM_ABI constexpr auto __allocator_aware_product(_Tp&& __obj,
                                                                const _Alloc& __alloc,
                                                                index_sequence<_Is...>) {
  return _Tp(std::make_obj_using_allocator<tuple_element_t<_Is, remove_cvref_t<_Tp>>>(
      __alloc, std::forward_like<_Tp>(std::get<_Is>(std::forward<_Tp>(__obj))))...);
}

template <class _Tp, class _Context>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __allocator_aware_forward(_Tp&& __obj, _Context&& __context) {
  if constexpr (requires { std::get_allocator(execution::get_env(__context)); }) {
    auto __alloc = std::get_allocator(execution::get_env(__context));
    using __product_type     = remove_cvref_t<_Tp>;
    if constexpr (__is_execution_product_type<__product_type>::value) {
      return std::__allocator_aware_product(
          std::forward<_Tp>(__obj), __alloc, make_index_sequence<tuple_size_v<__product_type>>{});
    } else {
      return std::make_obj_using_allocator<__product_type>(__alloc, std::forward<_Tp>(__obj));
    }
  } else {
    return std::forward<_Tp>(__obj);
  }
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_ALLOCATOR_H
