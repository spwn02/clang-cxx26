// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANGES_SLIDE_VIEW_H
#define _LIBCPP___RANGES_SLIDE_VIEW_H

#include <__concepts/constructible.h>
#include <__concepts/convertible_to.h>
#include <__config>
#include <__functional/bind_back.h>
#include <__iterator/concepts.h>
#include <__iterator/distance.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/next.h>
#include <__ranges/access.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/size.h>
#include <__ranges/subrange.h>
#include <__ranges/view_interface.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/maybe_const.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif
_LIBCPP_PUSH_MACROS
#include <__undef_macros>
_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23
namespace ranges {
template <forward_range _View>
  requires view<_View>
class slide_view : public view_interface<slide_view<_View>> {
  _LIBCPP_NO_UNIQUE_ADDRESS _View __base_ = _View();
  range_difference_t<_View> __n_ = 0;
  template <bool> class __iterator;
  template <bool> class __sentinel;
public:
  _LIBCPP_HIDE_FROM_ABI slide_view() requires default_initializable<_View> = default;
  _LIBCPP_HIDE_FROM_ABI constexpr explicit slide_view(_View __base, range_difference_t<_View> __n)
      : __base_(std::move(__base)), __n_(__n) {
    _LIBCPP_ASSERT_PEDANTIC(__n > 0, "slide window size must be greater than zero");
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _View base() const& requires copy_constructible<_View> { return __base_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _View base() && { return std::move(__base_); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto begin() requires(!__simple_view<_View>) {
    return __iterator<false>(this, ranges::begin(__base_));
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto begin() const requires forward_range<const _View> {
    return __iterator<true>(this, ranges::begin(__base_));
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto end() requires(!__simple_view<_View>) {
    if constexpr (common_range<_View>) return __iterator<false>(this, ranges::end(__base_), true);
    else return __sentinel<false>(ranges::end(__base_));
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto end() const requires forward_range<const _View> {
    if constexpr (common_range<const _View>) return __iterator<true>(this, ranges::end(__base_), true);
    else return __sentinel<true>(ranges::end(__base_));
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto size() requires sized_range<_View> {
    auto __s = ranges::distance(__base_) - __n_ + 1;
    return std::__to_unsigned_like(__s < 0 ? 0 : __s);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto size() const requires sized_range<const _View> {
    auto __s = ranges::distance(__base_) - __n_ + 1;
    return std::__to_unsigned_like(__s < 0 ? 0 : __s);
  }
};

template <class _Range>
slide_view(_Range&&, range_difference_t<_Range>) -> slide_view<views::all_t<_Range>>;

template <forward_range _View> requires view<_View>
template <bool _Const>
class slide_view<_View>::__iterator {
  friend slide_view;
  using _Base = __maybe_const<_Const, _View>;
  using _Parent = __maybe_const<_Const, slide_view>;
  iterator_t<_Base> __current_ = iterator_t<_Base>();
  iterator_t<_Base> __last_ = iterator_t<_Base>();
  sentinel_t<_Base> __end_ = sentinel_t<_Base>();
  range_difference_t<_Base> __n_ = 0;
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(_Parent* __p, iterator_t<_Base> __i)
      : __current_(__i), __last_(ranges::next(__i, __p->__n_ - 1, ranges::end(__p->__base_))),
        __end_(ranges::end(__p->__base_)), __n_(__p->__n_) {}
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(_Parent* __p, iterator_t<_Base> __i, bool)
      : __current_(__i), __last_(__i), __end_(ranges::end(__p->__base_)), __n_(__p->__n_) {}
  static consteval auto __iterator_concept() {
    if constexpr (random_access_range<_Base>) return random_access_iterator_tag{};
    else if constexpr (bidirectional_range<_Base>) return bidirectional_iterator_tag{};
    else return forward_iterator_tag{};
  }
public:
  using iterator_category = input_iterator_tag;
  using iterator_concept = decltype(__iterator::__iterator_concept());
  using difference_type = range_difference_t<_Base>;
  using value_type = subrange<iterator_t<_Base>>;
  _LIBCPP_HIDE_FROM_ABI __iterator() = default;
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(__iterator<!_Const> __i)
    requires _Const && convertible_to<iterator_t<_View>, iterator_t<_Base>>
      : __current_(std::move(__i.__current_)), __last_(std::move(__i.__last_)),
        __end_(std::move(__i.__end_)), __n_(__i.__n_) {}
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr value_type operator*() const { return {__current_, ranges::next(__last_)}; }
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() { ++__current_; ++__last_; return *this; }
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator++(int) { auto __t = *this; ++*this; return __t; }
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator--() requires bidirectional_range<_Base> { --__current_; --__last_; return *this; }
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator--(int) requires bidirectional_range<_Base> { auto __t=*this; --*this; return __t; }
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator+=(difference_type __n) requires random_access_range<_Base> { __current_ += __n; __last_ += __n; return *this; }
  _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator+(const __iterator& __i, difference_type __n) requires random_access_range<_Base> { auto __r=__i; return __r += __n; }
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, const __iterator& __y) {
    return __x.__current_ == __y.__current_ ||
           (__x.__last_ == __x.__end_ && __y.__last_ == __y.__end_);
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type operator-(const __iterator& __x, const __iterator& __y) requires sized_sentinel_for<iterator_t<_Base>, iterator_t<_Base>> { return __x.__current_ - __y.__current_; }
};

template <forward_range _View> requires view<_View>
template <bool _Const>
class slide_view<_View>::__sentinel {
  using _Base = __maybe_const<_Const, _View>;
  sentinel_t<_Base> __end_;
  friend slide_view;
  _LIBCPP_HIDE_FROM_ABI explicit __sentinel(sentinel_t<_Base> __e) : __end_(std::move(__e)) {}
public:
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator<_Const>& __i, const __sentinel& __s) { return __i.__last_ == __s.__end_; }
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __sentinel& __s, const __iterator<_Const>& __i) { return __i == __s; }
};

template <class _View>
inline constexpr bool enable_borrowed_range<slide_view<_View>> = enable_borrowed_range<_View>;

namespace views { namespace __slide {
struct __fn {
  template <viewable_range _Range>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __r, range_difference_t<_Range> __n) const
      noexcept(noexcept(slide_view(std::forward<_Range>(__r), __n)))
      -> decltype(slide_view(std::forward<_Range>(__r), __n)) { return slide_view(std::forward<_Range>(__r), __n); }
  template <class _Np> requires constructible_from<decay_t<_Np>, _Np>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Np&& __n) const noexcept(is_nothrow_constructible_v<decay_t<_Np>, _Np>) {
    return __pipeable(std::__bind_back(*this, std::forward<_Np>(__n)));
  }
};
} inline namespace __cpo { inline constexpr auto slide = __slide::__fn{}; } }
} // namespace ranges
#endif
_LIBCPP_END_NAMESPACE_STD
_LIBCPP_POP_MACROS
#endif
