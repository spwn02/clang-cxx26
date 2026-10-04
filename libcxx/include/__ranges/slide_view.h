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

#include <__assert>
#include <__compare/three_way_comparable.h>
#include <__concepts/constructible.h>
#include <__concepts/convertible_to.h>
#include <__config>
#include <__functional/bind_back.h>
#include <__iterator/concepts.h>
#include <__iterator/distance.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/next.h>
#include <__iterator/prev.h>
#include <__ranges/access.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/counted.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/non_propagating_cache.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/size.h>
#include <__ranges/view_interface.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/maybe_const.h>
#include <__utility/declval.h>
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

// [range.slide.view]
template <class _View>
concept __slide_caches_nothing = random_access_range<_View> && sized_range<_View>;

template <class _View>
concept __slide_caches_last = !__slide_caches_nothing<_View> && bidirectional_range<_View> && common_range<_View>;

template <class _View>
concept __slide_caches_first = !__slide_caches_nothing<_View> && !__slide_caches_last<_View>;

template <forward_range _View>
  requires view<_View>
class slide_view : public view_interface<slide_view<_View>> {
  using _Cache _LIBCPP_NODEBUG =
      _If<__slide_caches_nothing<_View>, __empty_cache, __non_propagating_cache<iterator_t<_View>>>;

  _LIBCPP_NO_UNIQUE_ADDRESS _View __base_;
  range_difference_t<_View> __n_;
  _LIBCPP_NO_UNIQUE_ADDRESS _Cache __cached_ = _Cache();

  template <bool>
  class __iterator;
  class __sentinel;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit slide_view(_View __base, range_difference_t<_View> __n)
      : __base_(std::move(__base)), __n_(__n) {
    _LIBCPP_ASSERT_UNCATEGORIZED(__n > 0, "slide_view window size must be greater than zero");
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _View base() const&
    requires copy_constructible<_View>
  {
    return __base_;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _View base() && { return std::move(__base_); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto begin()
    requires(!(__simple_view<_View> && __slide_caches_nothing<const _View>))
  {
    if constexpr (__slide_caches_first<_View>) {
      if (!__cached_.__has_value())
        __cached_.__emplace(ranges::next(ranges::begin(__base_), __n_ - 1, ranges::end(__base_)));
      return __iterator<false>(ranges::begin(__base_), *__cached_, __n_);
    } else {
      return __iterator<false>(ranges::begin(__base_), __n_);
    }
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto begin() const
    requires __slide_caches_nothing<const _View>
  {
    return __iterator<true>(ranges::begin(__base_), __n_);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto end()
    requires(!(__simple_view<_View> && __slide_caches_nothing<const _View>))
  {
    if constexpr (__slide_caches_nothing<_View>) {
      return __iterator<false>(ranges::begin(__base_) + range_difference_t<_View>(size()), __n_);
    } else if constexpr (__slide_caches_last<_View>) {
      if (!__cached_.__has_value())
        __cached_.__emplace(ranges::prev(ranges::end(__base_), __n_ - 1, ranges::begin(__base_)));
      return __iterator<false>(*__cached_, __n_);
    } else if constexpr (common_range<_View>) {
      return __iterator<false>(ranges::end(__base_), ranges::end(__base_), __n_);
    } else {
      return __sentinel(ranges::end(__base_));
    }
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto end() const
    requires __slide_caches_nothing<const _View>
  {
    return begin() + range_difference_t<const _View>(size());
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto size()
    requires sized_range<_View>
  {
    auto __sz = ranges::distance(__base_) - __n_ + 1;
    if (__sz < 0)
      __sz = 0;
    return std::__to_unsigned_like(__sz);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto size() const
    requires sized_range<const _View>
  {
    auto __sz = ranges::distance(__base_) - __n_ + 1;
    if (__sz < 0)
      __sz = 0;
    return std::__to_unsigned_like(__sz);
  }

#  if _LIBCPP_STD_VER >= 26
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto reserve_hint()
    requires approximately_sized_range<_View>
  {
    auto __sz = static_cast<range_difference_t<decltype((__base_))>>(ranges::reserve_hint(__base_)) - __n_ + 1;
    if (__sz < 0)
      __sz = 0;
    return std::__to_unsigned_like(__sz);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto reserve_hint() const
    requires approximately_sized_range<const _View>
  {
    auto __sz = static_cast<range_difference_t<decltype((__base_))>>(ranges::reserve_hint(__base_)) - __n_ + 1;
    if (__sz < 0)
      __sz = 0;
    return std::__to_unsigned_like(__sz);
  }
#  endif // _LIBCPP_STD_VER >= 26
};

template <class _Range>
slide_view(_Range&&, range_difference_t<_Range>) -> slide_view<views::all_t<_Range>>;

template <class _View>
inline constexpr bool enable_borrowed_range<slide_view<_View>> = enable_borrowed_range<_View>;

// [range.slide.iterator]
template <forward_range _View>
  requires view<_View>
template <bool _Const>
class slide_view<_View>::__iterator {
  friend slide_view;
  friend class slide_view<_View>::__sentinel;
  friend __iterator<!_Const>;

  using _Base _LIBCPP_NODEBUG = __maybe_const<_Const, _View>;

  struct __empty_last_ele {};
  using _LastEle _LIBCPP_NODEBUG =
      _If<__slide_caches_first<_Base>, iterator_t<_Base>, __empty_last_ele>;

  iterator_t<_Base> __current_ = iterator_t<_Base>();
  _LIBCPP_NO_UNIQUE_ADDRESS _LastEle __last_ele_ = _LastEle();
  range_difference_t<_Base> __n_ = 0;

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(iterator_t<_Base> __current, range_difference_t<_Base> __n)
    requires(!__slide_caches_first<_Base>)
      : __current_(std::move(__current)), __n_(__n) {}

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(
      iterator_t<_Base> __current, iterator_t<_Base> __last_ele, range_difference_t<_Base> __n)
    requires __slide_caches_first<_Base>
      : __current_(std::move(__current)), __last_ele_(std::move(__last_ele)), __n_(__n) {}

  static consteval auto __get_iterator_concept() {
    if constexpr (random_access_range<_Base>)
      return random_access_iterator_tag{};
    else if constexpr (bidirectional_range<_Base>)
      return bidirectional_iterator_tag{};
    else
      return forward_iterator_tag{};
  }

public:
  using iterator_category = input_iterator_tag;
  using iterator_concept  = decltype(__get_iterator_concept());
  using value_type        = decltype(views::counted(std::declval<iterator_t<_Base>>(), std::declval<range_difference_t<_Base>>()));
  using difference_type   = range_difference_t<_Base>;

  _LIBCPP_HIDE_FROM_ABI __iterator() = default;

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(__iterator<!_Const> __i)
    requires _Const && convertible_to<iterator_t<_View>, iterator_t<_Base>>
      : __current_(std::move(__i.__current_)), __n_(__i.__n_) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator*() const { return views::counted(__current_, __n_); }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() {
    ++__current_;
    if constexpr (__slide_caches_first<_Base>)
      ++__last_ele_;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator++(int) {
    auto __tmp = *this;
    ++*this;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator--()
    requires bidirectional_range<_Base>
  {
    --__current_;
    if constexpr (__slide_caches_first<_Base>)
      --__last_ele_;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator--(int)
    requires bidirectional_range<_Base>
  {
    auto __tmp = *this;
    --*this;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator+=(difference_type __x)
    requires random_access_range<_Base>
  {
    __current_ += __x;
    if constexpr (__slide_caches_first<_Base>)
      __last_ele_ += __x;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator-=(difference_type __x)
    requires random_access_range<_Base>
  {
    __current_ -= __x;
    if constexpr (__slide_caches_first<_Base>)
      __last_ele_ -= __x;
    return *this;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator[](difference_type __n) const
    requires random_access_range<_Base>
  {
    return views::counted(__current_ + __n, __n_);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, const __iterator& __y) {
    if constexpr (__slide_caches_first<_Base>)
      return __x.__last_ele_ == __y.__last_ele_;
    else
      return __x.__current_ == __y.__current_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator<(const __iterator& __x, const __iterator& __y)
    requires random_access_range<_Base>
  {
    return __x.__current_ < __y.__current_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator>(const __iterator& __x, const __iterator& __y)
    requires random_access_range<_Base>
  {
    return __y < __x;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator<=(const __iterator& __x, const __iterator& __y)
    requires random_access_range<_Base>
  {
    return !(__y < __x);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator>=(const __iterator& __x, const __iterator& __y)
    requires random_access_range<_Base>
  {
    return !(__x < __y);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(const __iterator& __x, const __iterator& __y)
    requires random_access_range<_Base> && three_way_comparable<iterator_t<_Base>>
  {
    return __x.__current_ <=> __y.__current_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator+(const __iterator& __i, difference_type __n)
    requires random_access_range<_Base>
  {
    auto __r = __i;
    __r += __n;
    return __r;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator+(difference_type __n, const __iterator& __i)
    requires random_access_range<_Base>
  {
    auto __r = __i;
    __r += __n;
    return __r;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator-(const __iterator& __i, difference_type __n)
    requires random_access_range<_Base>
  {
    auto __r = __i;
    __r -= __n;
    return __r;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type
  operator-(const __iterator& __x, const __iterator& __y)
    requires sized_sentinel_for<iterator_t<_Base>, iterator_t<_Base>>
  {
    if constexpr (__slide_caches_first<_Base>)
      return __x.__last_ele_ - __y.__last_ele_;
    else
      return __x.__current_ - __y.__current_;
  }
};

// [range.slide.sentinel]
template <forward_range _View>
  requires view<_View>
class slide_view<_View>::__sentinel {
  friend slide_view;

  sentinel_t<_View> __end_ = sentinel_t<_View>();

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __sentinel(sentinel_t<_View> __end) : __end_(std::move(__end)) {}

public:
  _LIBCPP_HIDE_FROM_ABI __sentinel() = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator<false>& __x, const __sentinel& __y) {
    return __x.__last_ele_ == __y.__end_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr range_difference_t<_View>
  operator-(const __iterator<false>& __x, const __sentinel& __y)
    requires sized_sentinel_for<sentinel_t<_View>, iterator_t<_View>>
  {
    return __x.__last_ele_ - __y.__end_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr range_difference_t<_View>
  operator-(const __sentinel& __y, const __iterator<false>& __x)
    requires sized_sentinel_for<sentinel_t<_View>, iterator_t<_View>>
  {
    return __y.__end_ - __x.__last_ele_;
  }
};

namespace views {
namespace __slide {
struct __fn {
  template <viewable_range _Range>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range, range_difference_t<_Range> __n) const
      noexcept(noexcept(slide_view(std::forward<_Range>(__range), __n)))
          -> decltype(slide_view(std::forward<_Range>(__range), __n)) {
    return slide_view(std::forward<_Range>(__range), __n);
  }

  template <class _Np>
    requires constructible_from<decay_t<_Np>, _Np>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Np&& __n) const
      noexcept(is_nothrow_constructible_v<decay_t<_Np>, _Np>) {
    return __pipeable(std::__bind_back(*this, std::forward<_Np>(__n)));
  }
};
} // namespace __slide

inline namespace __cpo {
inline constexpr auto slide = __slide::__fn{};
} // namespace __cpo
} // namespace views

} // namespace ranges

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___RANGES_SLIDE_VIEW_H
