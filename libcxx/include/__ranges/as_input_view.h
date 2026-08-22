// -*- C++ -*-
//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANGES_AS_INPUT_VIEW_H
#define _LIBCPP___RANGES_AS_INPUT_VIEW_H

#include <__config>
#include <concepts>
#include <iterator>
#include <__iterator/concepts.h>
#include <__iterator/iter_move.h>
#include <__iterator/iter_swap.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/view_interface.h>
#include <__type_traits/maybe_const.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26
namespace ranges {
template <input_range _View>
  requires view<_View>
class as_input_view : public view_interface<as_input_view<_View>> {
  _View __base_ = _View();

  template <bool _Const>
  class __iterator;

public:
  as_input_view() requires default_initializable<_View> = default;
  _LIBCPP_HIDE_FROM_ABI constexpr explicit as_input_view(_View __base) : __base_(std::move(__base)) {}

  _LIBCPP_HIDE_FROM_ABI constexpr _View base() const& requires copy_constructible<_View> { return __base_; }
  _LIBCPP_HIDE_FROM_ABI constexpr _View base() && { return std::move(__base_); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto begin() requires(!__simple_view<_View>) { return __iterator<false>(ranges::begin(__base_)); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto begin() const requires range<const _View> { return __iterator<true>(ranges::begin(__base_)); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto end() requires(!__simple_view<_View>) { return ranges::end(__base_); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto end() const requires range<const _View> { return ranges::end(__base_); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto size() requires sized_range<_View> { return ranges::size(__base_); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto size() const requires sized_range<const _View> { return ranges::size(__base_); }
};

template <class _Range>
as_input_view(_Range&&) -> as_input_view<views::all_t<_Range>>;

template <input_range _View>
  requires view<_View>
template <bool _Const>
class as_input_view<_View>::__iterator {
  using _Base = __maybe_const<_Const, _View>;
  iterator_t<_Base> __current_ = iterator_t<_Base>();
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __iterator(iterator_t<_Base> __current) : __current_(std::move(__current)) {}
  friend class as_input_view;
  friend class __iterator<!_Const>;

public:
  using difference_type = range_difference_t<_Base>;
  using value_type = range_value_t<_Base>;
  using iterator_concept = input_iterator_tag;
  __iterator() requires default_initializable<iterator_t<_Base>> = default;
  __iterator(__iterator&&) = default;
  __iterator& operator=(__iterator&&) = default;
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(__iterator<!_Const> __other)
      requires _Const && convertible_to<iterator_t<_View>, iterator_t<_Base>>
      : __current_(std::move(__other.__current_)) {}
  _LIBCPP_HIDE_FROM_ABI constexpr iterator_t<_Base> base() && { return std::move(__current_); }
  _LIBCPP_HIDE_FROM_ABI constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator*() const { return *__current_; }
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() { ++__current_; return *this; }
  _LIBCPP_HIDE_FROM_ABI constexpr void operator++(int) { ++*this; }
  friend _LIBCPP_HIDE_FROM_ABI constexpr bool operator==(const __iterator& __it, const sentinel_t<_Base>& __sent) {
    return __it.__current_ == __sent;
  }
  friend _LIBCPP_HIDE_FROM_ABI constexpr range_difference_t<_Base> operator-(const sentinel_t<_Base>& __sent, const __iterator& __it)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>> { return __sent - __it.__current_; }
  friend _LIBCPP_HIDE_FROM_ABI constexpr range_difference_t<_Base> operator-(const __iterator& __it, const sentinel_t<_Base>& __sent)
      requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>> { return __it.__current_ - __sent; }
  friend _LIBCPP_HIDE_FROM_ABI constexpr range_rvalue_reference_t<_Base> iter_move(const __iterator& __it)
      noexcept(noexcept(ranges::iter_move(__it.__current_))) { return ranges::iter_move(__it.__current_); }
  friend _LIBCPP_HIDE_FROM_ABI constexpr void iter_swap(const __iterator& __x, const __iterator& __y)
      noexcept(noexcept(ranges::iter_swap(__x.__current_, __y.__current_)))
      requires indirectly_swappable<iterator_t<_Base>> { ranges::iter_swap(__x.__current_, __y.__current_); }
};

namespace views {
struct __as_input_fn : range_adaptor_closure<__as_input_fn> {
  template <viewable_range _Range>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range) const {
    if constexpr (input_range<_Range> && !common_range<_Range> && !forward_range<_Range>)
      return views::all(std::forward<_Range>(__range));
    else
      return as_input_view(std::forward<_Range>(__range));
  }
};
inline constexpr __as_input_fn as_input{};
} // namespace views
} // namespace ranges
#endif

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif
