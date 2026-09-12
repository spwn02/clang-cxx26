// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANGES_CARTESIAN_PRODUCT_VIEW_H
#define _LIBCPP___RANGES_CARTESIAN_PRODUCT_VIEW_H

#include <__concepts/convertible_to.h>
#include <__concepts/equality_comparable.h>
#include <__config>
#include <__iterator/concepts.h>
#include <__iterator/default_sentinel.h>
#include <__iterator/iter_move.h>
#include <__iterator/iter_swap.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/empty_view.h>
#include <__ranges/view_interface.h>
#include <__tuple/tuple_transform.h>
#include <__type_traits/conditional.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>
#include <__utility/move.h>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

namespace ranges {

template <bool _Const, class _First, class... _Vs>
concept __cartesian_product_random_access =
    random_access_range<__maybe_const<_Const, _First>> &&
    (random_access_range<__maybe_const<_Const, _Vs>> && ...) &&
    (sized_range<__maybe_const<_Const, _Vs>> && ...);

template <bool _Const, class _First, class... _Vs>
concept __cartesian_product_bidirectional =
    bidirectional_range<__maybe_const<_Const, _First>> &&
    (bidirectional_range<__maybe_const<_Const, _Vs>> && ...) &&
    ((common_range<__maybe_const<_Const, _Vs>> ||
      (random_access_range<__maybe_const<_Const, _Vs>> && sized_range<__maybe_const<_Const, _Vs>>)) && ...);

template <class _R>
concept __cartesian_product_common_arg =
    common_range<_R> || (random_access_range<_R> && sized_range<_R>);

template <class _R>
  requires __cartesian_product_common_arg<_R>
_LIBCPP_HIDE_FROM_ABI constexpr auto __cartesian_product_end(_R& __r) {
  if constexpr (common_range<_R>)
    return ranges::end(__r);
  else
    return ranges::begin(__r) + ranges::distance(__r);
}

template <input_range _First, forward_range... _Vs>
  requires(view<_First> && ... && view<_Vs>)
class cartesian_product_view : public view_interface<cartesian_product_view<_First, _Vs...>> {
  tuple<_First, _Vs...> __bases_;
  template <bool> class __iterator;

  template <bool _Const>
  using __base = __maybe_const<_Const, cartesian_product_view>;

public:
  _LIBCPP_HIDE_FROM_ABI cartesian_product_view() = default;
  _LIBCPP_HIDE_FROM_ABI constexpr explicit cartesian_product_view(_First __first, _Vs... __vs)
      : __bases_(std::move(__first), std::move(__vs)...) {}

  _LIBCPP_HIDE_FROM_ABI constexpr auto begin() requires (!__simple_view<_First> || ... || !__simple_view<_Vs>) {
    return __iterator<false>(*this, std::__tuple_transform(ranges::begin, __bases_));
  }
  _LIBCPP_HIDE_FROM_ABI constexpr auto begin() const requires (range<const _First> && ... && range<const _Vs>) {
    return __iterator<true>(*this, std::__tuple_transform(ranges::begin, __bases_));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto end()
    requires (!__simple_view<_First> || ... || !__simple_view<_Vs>)
  {
    if constexpr (__cartesian_product_common_arg<_First>)
      return __end<false>();
    else
      return default_sentinel;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr auto end() const requires (range<const _First> && ... && range<const _Vs>) {
    if constexpr (__cartesian_product_common_arg<const _First>)
      return const_cast<cartesian_product_view*>(this)->template __end<true>();
    else
      return default_sentinel;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto size() requires (sized_range<_First> && ... && sized_range<_Vs>) {
    return __size<false>();
  }
  _LIBCPP_HIDE_FROM_ABI constexpr auto size() const requires (sized_range<const _First> && ... && sized_range<const _Vs>) {
    return __size<true>();
  }

private:
  template <bool _Const>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __end() {
    auto __current = std::__tuple_transform(ranges::begin, __bases_);
    bool __empty = false;
    [&]<size_t... _Is>(index_sequence<_Is...>) {
      ((void)(_Is == 0 ? 0 : (__empty = __empty || ranges::begin(std::get<_Is>(__bases_)) == ranges::end(std::get<_Is>(__bases_)), 0)), ...);
    }(make_index_sequence<sizeof...(_Vs) + 1>{});
    if (__empty)
      std::get<0>(__current) = ranges::begin(std::get<0>(__bases_));
    else
      std::get<0>(__current) = __cartesian_product_end(std::get<0>(__bases_));
    return __iterator<_Const>(*this, std::move(__current));
  }
  template <bool _Const>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __size() const {
    return std::apply([](auto const&... __r) { return (std::__to_unsigned_like(ranges::size(__r)) * ... * 1); }, __bases_);
  }

  template <bool _Const>
  class __iterator {
    using _Parent = __maybe_const<_Const, cartesian_product_view>;
    using _Bases = tuple<__maybe_const<_Const, _First>, __maybe_const<_Const, _Vs>...>;
    _Parent* __parent_ = nullptr;
    tuple<iterator_t<__maybe_const<_Const, _First>>, iterator_t<__maybe_const<_Const, _Vs>>...> __current_;
    friend class cartesian_product_view;
    template <bool> friend class __iterator;

    _LIBCPP_HIDE_FROM_ABI constexpr __iterator(_Parent& __p, decltype(__current_) __current)
        : __parent_(std::addressof(__p)), __current_(std::move(__current)) {}
    template <size_t _N = sizeof...(_Vs)>
    _LIBCPP_HIDE_FROM_ABI constexpr void __next() {
      auto& __it = std::get<_N>(__current_);
      ++__it;
      if constexpr (_N > 0)
        if (__it == ranges::end(std::get<_N>(__parent_->__bases_))) {
          __it = ranges::begin(std::get<_N>(__parent_->__bases_));
          __next<_N - 1>();
        }
    }
    template <size_t _N = sizeof...(_Vs)>
    _LIBCPP_HIDE_FROM_ABI constexpr void __prev() {
      auto& __it = std::get<_N>(__current_);
      if constexpr (_N > 0)
        if (__it == ranges::begin(std::get<_N>(__parent_->__bases_))) {
          __it = __cartesian_product_end(std::get<_N>(__parent_->__bases_));
          __prev<_N - 1>();
        }
      --__it;
    }
    template <size_t... _Is>
    _LIBCPP_HIDE_FROM_ABI constexpr bool __at_end(index_sequence<_Is...>) const {
      return ((std::get<_Is>(__current_) == ranges::end(std::get<_Is>(__parent_->__bases_))) || ...);
    }
    template <size_t... _Is>
    _LIBCPP_HIDE_FROM_ABI constexpr common_type_t<range_difference_t<__maybe_const<_Const, _First>>, range_difference_t<__maybe_const<_Const, _Vs>>...>
    __distance_from(const __iterator& __other, index_sequence<_Is...>) const {
      common_type_t<range_difference_t<__maybe_const<_Const, _First>>, range_difference_t<__maybe_const<_Const, _Vs>>...> __result = 0;
      ((void)(_Is == 0 ? (__result = std::get<0>(__current_) - std::get<0>(__other.__current_), 0)
                        : (__result = __result * static_cast<difference_type>(ranges::size(std::get<_Is>(__parent_->__bases_))) +
                                      (std::get<_Is>(__current_) - std::get<_Is>(__other.__current_)), 0)), ...);
      return __result;
    }
  public:
    using iterator_category = input_iterator_tag;
    using iterator_concept = conditional_t<__cartesian_product_random_access<_Const, _First, _Vs...>, random_access_iterator_tag,
        conditional_t<__cartesian_product_bidirectional<_Const, _First, _Vs...>, bidirectional_iterator_tag,
        conditional_t<forward_range<__maybe_const<_Const, _First>>, forward_iterator_tag, input_iterator_tag>>>;
    using value_type = tuple<range_value_t<__maybe_const<_Const, _First>>, range_value_t<__maybe_const<_Const, _Vs>>...>;
    using difference_type = common_type_t<range_difference_t<__maybe_const<_Const, _First>>, range_difference_t<__maybe_const<_Const, _Vs>>...>;
    _LIBCPP_HIDE_FROM_ABI __iterator() = default;
    _LIBCPP_HIDE_FROM_ABI constexpr __iterator(__iterator<!_Const> __i)
      requires _Const && (convertible_to<iterator_t<_First>, iterator_t<const _First>> && ... && convertible_to<iterator_t<_Vs>, iterator_t<const _Vs>>)
        : __parent_(__i.__parent_), __current_(std::move(__i.__current_)) {}
    _LIBCPP_HIDE_FROM_ABI constexpr auto operator*() const { return std::__tuple_transform([](auto& __i) -> decltype(auto) { return *__i; }, __current_); }
    _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() { __next(); return *this; }
    _LIBCPP_HIDE_FROM_ABI constexpr void operator++(int) { ++*this; }
    _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator++(int) requires forward_range<__maybe_const<_Const, _First>> { auto __r = *this; ++*this; return __r; }
    _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator--() requires __cartesian_product_bidirectional<_Const, _First, _Vs...> { __prev(); return *this; }
    _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator--(int) requires __cartesian_product_bidirectional<_Const, _First, _Vs...> { auto __r = *this; --*this; return __r; }
    _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator+=(difference_type __n) requires __cartesian_product_random_access<_Const, _First, _Vs...> { if (__n >= 0) while (__n--) ++*this; else while (__n++) --*this; return *this; }
    _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator-=(difference_type __n) requires __cartesian_product_random_access<_Const, _First, _Vs...> { return *this += -__n; }
    _LIBCPP_HIDE_FROM_ABI constexpr auto operator[](difference_type __n) const requires __cartesian_product_random_access<_Const, _First, _Vs...> { return *(*this + __n); }
    _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, const __iterator& __y) requires equality_comparable<iterator_t<__maybe_const<_Const, _First>>> { return __x.__current_ == __y.__current_; }
    _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, default_sentinel_t) { return __x.__at_end(make_index_sequence<sizeof...(_Vs) + 1>{}); }
    _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator+(const __iterator& __i, difference_type __n) requires __cartesian_product_random_access<_Const, _First, _Vs...> { auto __r = __i; return __r += __n; }
    _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator+(difference_type __n, const __iterator& __i) requires __cartesian_product_random_access<_Const, _First, _Vs...> { return __i + __n; }
    _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator-(const __iterator& __i, difference_type __n) requires __cartesian_product_random_access<_Const, _First, _Vs...> { auto __r = __i; return __r -= __n; }
    _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type operator-(const __iterator& __x, const __iterator& __y) requires __cartesian_product_random_access<_Const, _First, _Vs...> { return __x.__distance_from(__y, make_index_sequence<sizeof...(_Vs) + 1>{}); }
    _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(const __iterator& __x, const __iterator& __y) requires __cartesian_product_random_access<_Const, _First, _Vs...> { return __x.__current_ <=> __y.__current_; }
  };
};

template <class... _Ranges>
cartesian_product_view(_Ranges&&...) -> cartesian_product_view<views::all_t<_Ranges>...>;

namespace views { namespace __cartesian_product {
struct __fn {
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()() const { return single_view<tuple<>>{tuple<>{}}; }
  template <class... _Ranges>
    requires (sizeof...(_Ranges) > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Ranges&&... __ranges) const
      -> decltype(cartesian_product_view<all_t<_Ranges>...>(std::forward<_Ranges>(__ranges)...)) {
    return cartesian_product_view<all_t<_Ranges>...>(std::forward<_Ranges>(__ranges)...);
  }
};
} inline namespace __cpo { inline constexpr auto cartesian_product = __cartesian_product::__fn{}; } }

} // namespace ranges
#endif

_LIBCPP_END_NAMESPACE_STD
_LIBCPP_POP_MACROS
#endif
