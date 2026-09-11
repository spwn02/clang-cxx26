// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANGES_STRIDE_VIEW_H
#define _LIBCPP___RANGES_STRIDE_VIEW_H

#include <__concepts/constructible.h>
#include <__concepts/convertible_to.h>
#include <__concepts/copyable.h>
#include <__concepts/equality_comparable.h>
#include <__config>
#include <__functional/bind_back.h>
#include <__iterator/concepts.h>
#include <__iterator/default_sentinel.h>
#include <__iterator/incrementable_traits.h>
#include <__iterator/iter_move.h>
#include <__iterator/iter_swap.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/size.h>
#include <__ranges/view_interface.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/maybe_const.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstdlib>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

namespace ranges {

template <class _View>
concept __stride_view_can_size = requires(_View& __v) { ranges::size(__v); };

template <input_range _View>
  requires view<_View>
class stride_view : public view_interface<stride_view<_View>> {
public:
  _LIBCPP_HIDE_FROM_ABI stride_view()
    requires default_initializable<_View>
  = default;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit stride_view(_View __base, range_difference_t<_View> __stride)
      : __base_(std::move(__base)), __stride_(__stride) {
    _LIBCPP_ASSERT_UNCATEGORIZED(__stride > 0, "stride must be greater than 0");
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _View base() const&
    requires copy_constructible<_View>
  {
    return __base_;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _View base() && { return std::move(__base_); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr range_difference_t<_View> stride() const noexcept { return __stride_; }

  template <bool _Const>
  class __iterator;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr __iterator<false> begin()
    requires(!__simple_view<_View>)
  {
    return __iterator<false>(this, ranges::begin(__base_), __missing_from_partial_stride());
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr __iterator<true> begin() const
    requires range<const _View>
  {
    return __iterator<true>(this, ranges::begin(__base_), __missing_from_partial_stride());
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto end()
    requires(!__simple_view<_View>)
  {
    if constexpr (common_range<_View> && sized_range<_View> && forward_range<_View>)
      return __iterator<false>(this, ranges::end(__base_), (ranges::distance(__base_) % __stride_ != 0)
                                                                 ? __stride_ - (ranges::distance(__base_) % __stride_)
                                                                 : 0);
    else if constexpr (common_range<_View> && !bidirectional_range<_View>)
      return __iterator<false>(this, ranges::end(__base_), 0);
    else
      return default_sentinel;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto end() const
    requires range<const _View>
  {
    using __const_iter = __iterator<true>;
    if constexpr (common_range<const _View> && sized_range<const _View> && forward_range<const _View>)
      return __const_iter(this, ranges::end(__base_), (ranges::distance(__base_) % __stride_ != 0)
                                                            ? __stride_ - (ranges::distance(__base_) % __stride_)
                                                            : 0);
    else if constexpr (common_range<const _View> && !bidirectional_range<const _View>)
      return __const_iter(this, ranges::end(__base_), 0);
    else
      return default_sentinel;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto size()
    requires __stride_view_can_size<_View>
  {
    return __size(*this);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto size() const
    requires __stride_view_can_size<const _View>
  {
    return __size(*this);
  }

private:
  _LIBCPP_HIDE_FROM_ABI constexpr range_difference_t<_View> __missing_from_partial_stride() const { return 0; }

  _LIBCPP_HIDE_FROM_ABI static constexpr auto __size(auto& __self) {
    auto __s = ranges::size(__self.__base_);
    auto __n = std::__to_unsigned_like(__self.__stride_);
    return static_cast<decltype(__s)>(__s == 0 ? 0 : 1 + (__s - 1) / __n);
  }

  _View __base_                          = _View();
  range_difference_t<_View> __stride_    = 1;
};

template <class _Range>
stride_view(_Range&&, range_difference_t<_Range>) -> stride_view<views::all_t<_Range>>;

template <class _View>
struct __stride_view_iterator_category {};

template <forward_range _View>
struct __stride_view_iterator_category<_View> {
  using _Cat _LIBCPP_NODEBUG = typename iterator_traits<iterator_t<_View>>::iterator_category;
  using iterator_category =
      _If<derived_from<_Cat, random_access_iterator_tag>, random_access_iterator_tag, _Cat>;
};

template <input_range _View>
  requires view<_View>
template <bool _Const>
class stride_view<_View>::__iterator : public __stride_view_iterator_category<__maybe_const<_Const, _View>> {
  using _Parent = __maybe_const<_Const, stride_view>;
  using _Base   = __maybe_const<_Const, _View>;

  _Parent* __parent_                    = nullptr;
  iterator_t<_Base> __current_          = iterator_t<_Base>();
  range_difference_t<_Base> __stride_   = 0;
  range_difference_t<_Base> __missing_  = 0;

  friend class stride_view<_View>;

  template <bool _OtherConst>
  friend class stride_view<_View>::__iterator;

public:
  using difference_type = range_difference_t<_Base>;
  using value_type       = range_value_t<_Base>;
  using iterator_concept =
      _If<random_access_range<_Base>,
          random_access_iterator_tag,
          _If<bidirectional_range<_Base>,
              bidirectional_iterator_tag,
              _If<forward_range<_Base>, forward_iterator_tag, input_iterator_tag>>>;
  // iterator_category is inherited from __stride_view_iterator_category, and only present for forward_range.

  _LIBCPP_HIDE_FROM_ABI __iterator()
    requires default_initializable<iterator_t<_Base>>
  = default;

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(_Parent* __parent, iterator_t<_Base> __current,
                                             range_difference_t<_Base> __missing)
      : __parent_(__parent), __current_(std::move(__current)), __stride_(__parent->__stride_), __missing_(__missing) {
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(__iterator<!_Const> __i)
    requires _Const && convertible_to<iterator_t<_View>, iterator_t<_Base>>
      : __parent_(__i.__parent_), __current_(std::move(__i.__current_)), __stride_(__i.__stride_),
        __missing_(__i.__missing_) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const iterator_t<_Base>& base() const& noexcept { return __current_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr iterator_t<_Base> base() && { return std::move(__current_); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator*() const { return *__current_; }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() {
    __missing_ = ranges::advance(__current_, __stride_, ranges::end(__parent_->__base_));
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void operator++(int) { ++*this; }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator++(int)
    requires forward_range<_Base>
  {
    auto __tmp = *this;
    ++*this;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator--()
    requires bidirectional_range<_Base>
  {
    ranges::advance(__current_, __missing_ - __stride_);
    __missing_ = 0;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator--(int)
    requires bidirectional_range<_Base>
  {
    auto __tmp = *this;
    --*this;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator+=(difference_type __n)
    requires random_access_range<_Base>
  {
    if (__n > 0) {
      _LIBCPP_ASSERT_UNCATEGORIZED(ranges::distance(__current_, ranges::end(__parent_->__base_)) > __stride_ * (__n - 1),
                                   "Advancing past the end of stride_view's underlying range is undefined behavior");
      __missing_ = ranges::advance(__current_, __stride_ * __n, ranges::end(__parent_->__base_));
    } else if (__n < 0) {
      ranges::advance(__current_, __stride_ * __n + __missing_);
      __missing_ = 0;
    }
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator-=(difference_type __n)
    requires random_access_range<_Base>
  {
    return *this += -__n;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) operator[](difference_type __n) const
    requires random_access_range<_Base>
  {
    return *(*this + __n);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, const __iterator& __y)
    requires equality_comparable<iterator_t<_Base>>
  {
    return __x.__current_ == __y.__current_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, default_sentinel_t) {
    return __x.__current_ == ranges::end(__x.__parent_->__base_);
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

#  if _LIBCPP_STD_VER >= 20 && defined(__cpp_lib_three_way_comparison)
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(const __iterator& __x, const __iterator& __y)
    requires random_access_range<_Base> && three_way_comparable<iterator_t<_Base>>
  {
    return __x.__current_ <=> __y.__current_;
  }
#  endif

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
    return __i + __n;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr __iterator operator-(const __iterator& __i, difference_type __n)
    requires random_access_range<_Base>
  {
    auto __r = __i;
    __r -= __n;
    return __r;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type operator-(const __iterator& __x,
                                                                                  const __iterator& __y)
    requires sized_sentinel_for<iterator_t<_Base>, iterator_t<_Base>>
  {
    auto __n = static_cast<difference_type>(__x.__stride_);
    return (__x.__current_ - __y.__current_ + __x.__missing_ - __y.__missing_) / __n;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type
  operator-(default_sentinel_t, const __iterator& __x)
    requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
  {
    auto __dist = ranges::end(__x.__parent_->__base_) - __x.__current_;
    return (__dist + __x.__missing_ + __x.__stride_ - 1) / __x.__stride_;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr difference_type
  operator-(const __iterator& __x, default_sentinel_t __y)
    requires sized_sentinel_for<sentinel_t<_Base>, iterator_t<_Base>>
  {
    return -(__y - __x);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr range_rvalue_reference_t<_Base>
  iter_move(const __iterator& __i) noexcept(noexcept(ranges::iter_move(__i.__current_))) {
    return ranges::iter_move(__i.__current_);
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr void iter_swap(const __iterator& __x, const __iterator& __y) noexcept(
      noexcept(ranges::iter_swap(__x.__current_, __y.__current_)))
    requires indirectly_swappable<iterator_t<_Base>>
  {
    ranges::iter_swap(__x.__current_, __y.__current_);
  }
};

template <class _Tp>
inline constexpr bool enable_borrowed_range<stride_view<_Tp>> = enable_borrowed_range<_Tp>;

namespace views {
namespace __stride {
struct __fn {
  template <class _Range, class _Np>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range, _Np&& __n) const
      noexcept(noexcept(stride_view(std::forward<_Range>(__range), std::forward<_Np>(__n))))
          -> decltype(stride_view(std::forward<_Range>(__range), std::forward<_Np>(__n))) {
    return stride_view(std::forward<_Range>(__range), std::forward<_Np>(__n));
  }

  template <class _Np>
    requires constructible_from<decay_t<_Np>, _Np>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Np&& __n) const
      noexcept(is_nothrow_constructible_v<decay_t<_Np>, _Np>) {
    return __pipeable(std::__bind_back(*this, std::forward<_Np>(__n)));
  }
};
} // namespace __stride

inline namespace __cpo {
inline constexpr auto stride = __stride::__fn{};
} // namespace __cpo
} // namespace views

} // namespace ranges

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___RANGES_STRIDE_VIEW_H
