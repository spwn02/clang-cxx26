// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ITERATOR_CONST_ITERATOR_H
#define _LIBCPP___ITERATOR_CONST_ITERATOR_H

#include <__config>
#include <__compare/three_way_comparable.h>
#include <__concepts/common_with.h>
#include <__concepts/convertible_to.h>
#include <__concepts/same_as.h>
#include <__concepts/semiregular.h>
#include <__iterator/concepts.h>
#include <__iterator/iter_move.h>
#include <__iterator/iterator_traits.h>
#include <__memory/addressof.h>
#include <__memory/pointer_traits.h>
#include <__type_traits/common_type.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

template <indirectly_readable _It>
using iter_const_reference_t = common_reference_t<const iter_value_t<_It>&&, iter_reference_t<_It>>;

template <class _It>
concept constant_iterator = input_iterator<_It> && same_as<iter_const_reference_t<_It>, iter_reference_t<_It>>;

template <input_iterator _It>
class basic_const_iterator;

template <class>
struct __const_iterator_category {};

template <forward_iterator _It>
struct __const_iterator_category<_It> {
  using iterator_category = typename iterator_traits<_It>::iterator_category;
};

template <class>
inline constexpr bool __is_basic_const_iterator = false;

template <class _It>
inline constexpr bool __is_basic_const_iterator<class basic_const_iterator<_It>> = true;

template <class _Tp>
concept __not_a_const_iterator = !__is_basic_const_iterator<remove_cvref_t<_Tp>>;

template <indirectly_readable _It>
using __iter_const_rvalue_reference_t = common_reference_t<const iter_value_t<_It>&&, iter_rvalue_reference_t<_It>>;

template <input_iterator _It>
using const_iterator = conditional_t<constant_iterator<_It>, _It, basic_const_iterator<_It>>;

template <class _Sent>
struct __const_sentinel {
  using type = _Sent;
};

template <input_iterator _Sent>
struct __const_sentinel<_Sent> {
  using type = const_iterator<_Sent>;
};

template <semiregular _Sent>
using const_sentinel = typename __const_sentinel<_Sent>::type;

template <input_iterator _It>
class basic_const_iterator : public __const_iterator_category<_It> {
  _It __current_ = _It();
  using __reference       = iter_const_reference_t<_It>;
  using __rvalue_reference = __iter_const_rvalue_reference_t<_It>;

  static auto __iterator_concept() {
    if constexpr (contiguous_iterator<_It>)
      return contiguous_iterator_tag{};
    else if constexpr (random_access_iterator<_It>)
      return random_access_iterator_tag{};
    else if constexpr (bidirectional_iterator<_It>)
      return bidirectional_iterator_tag{};
    else if constexpr (forward_iterator<_It>)
      return forward_iterator_tag{};
    else
      return input_iterator_tag{};
  }

  template <input_iterator>
  friend class basic_const_iterator;

public:
  using iterator_type    = _It;
  using iterator_concept = decltype(__iterator_concept());
  using value_type       = iter_value_t<_It>;
  using difference_type  = iter_difference_t<_It>;

  basic_const_iterator() requires default_initializable<_It> = default;

  constexpr basic_const_iterator(_It __current) noexcept(is_nothrow_move_constructible_v<_It>)
      : __current_(std::move(__current)) {}

  template <convertible_to<_It> _It2>
  constexpr basic_const_iterator(basic_const_iterator<_It2> __current)
      noexcept(is_nothrow_constructible_v<_It, _It2>)
      : __current_(std::move(__current.__current_)) {}

  template <__not_a_const_iterator _Tp>
    requires convertible_to<_Tp, _It>
  constexpr basic_const_iterator(_Tp&& __current) noexcept(is_nothrow_constructible_v<_It, _Tp>)
      : __current_(std::forward<_Tp>(__current)) {}

  constexpr const _It& base() const& noexcept { return __current_; }
  constexpr _It base() && noexcept(is_nothrow_move_constructible_v<_It>) { return std::move(__current_); }

  constexpr __reference operator*() const noexcept(noexcept(static_cast<__reference>(*__current_))) {
    return static_cast<__reference>(*__current_);
  }

  constexpr const auto* operator->() const
      noexcept(contiguous_iterator<_It> || noexcept(*__current_))
      requires is_lvalue_reference_v<iter_reference_t<_It>> &&
               same_as<remove_cvref_t<iter_reference_t<_It>>, value_type>
  {
    if constexpr (contiguous_iterator<_It>)
      return std::to_address(__current_);
    else
      return std::addressof(*__current_);
  }

  constexpr basic_const_iterator& operator++() noexcept(noexcept(++__current_)) {
    ++__current_;
    return *this;
  }
  constexpr void operator++(int) noexcept(noexcept(++__current_)) { ++__current_; }

  constexpr basic_const_iterator operator++(int)
      noexcept(noexcept(++*this) && is_nothrow_copy_constructible_v<basic_const_iterator>)
      requires forward_iterator<_It>
  {
    auto __tmp = *this;
    ++*this;
    return __tmp;
  }

  constexpr basic_const_iterator& operator--() noexcept(noexcept(--__current_)) requires bidirectional_iterator<_It> {
    --__current_;
    return *this;
  }
  constexpr basic_const_iterator operator--(int)
      noexcept(noexcept(--*this) && is_nothrow_copy_constructible_v<basic_const_iterator>)
      requires bidirectional_iterator<_It>
  {
    auto __tmp = *this;
    --*this;
    return __tmp;
  }

  constexpr basic_const_iterator& operator+=(difference_type __n) noexcept(noexcept(__current_ += __n))
      requires random_access_iterator<_It> {
    __current_ += __n;
    return *this;
  }
  constexpr basic_const_iterator& operator-=(difference_type __n) noexcept(noexcept(__current_ -= __n))
      requires random_access_iterator<_It> {
    __current_ -= __n;
    return *this;
  }
  constexpr __reference operator[](difference_type __n) const
      noexcept(noexcept(static_cast<__reference>(__current_[__n]))) requires random_access_iterator<_It> {
    return static_cast<__reference>(__current_[__n]);
  }

  template <sentinel_for<_It> _Sent>
  constexpr bool operator==(const _Sent& __s) const noexcept(noexcept(__current_ == __s)) {
    return __current_ == __s;
  }

  constexpr bool operator<(const basic_const_iterator& __y) const requires random_access_iterator<_It> {
    return __current_ < __y.__current_;
  }
  constexpr bool operator>(const basic_const_iterator& __y) const requires random_access_iterator<_It> {
    return __current_ > __y.__current_;
  }
  constexpr bool operator<=(const basic_const_iterator& __y) const requires random_access_iterator<_It> {
    return __current_ <= __y.__current_;
  }
  constexpr bool operator>=(const basic_const_iterator& __y) const requires random_access_iterator<_It> {
    return __current_ >= __y.__current_;
  }
  constexpr auto operator<=>(const basic_const_iterator& __y) const
      requires random_access_iterator<_It> && three_way_comparable<_It> {
    return __current_ <=> __y.__current_;
  }

  template <__not_a_const_iterator _It2>
  constexpr bool operator<(const _It2& __y) const
      requires random_access_iterator<_It> && totally_ordered_with<_It, _It2> { return __current_ < __y; }
  template <__not_a_const_iterator _It2>
  constexpr bool operator>(const _It2& __y) const
      requires random_access_iterator<_It> && totally_ordered_with<_It, _It2> { return __current_ > __y; }
  template <__not_a_const_iterator _It2>
  constexpr bool operator<=(const _It2& __y) const
      requires random_access_iterator<_It> && totally_ordered_with<_It, _It2> { return __current_ <= __y; }
  template <__not_a_const_iterator _It2>
  constexpr bool operator>=(const _It2& __y) const
      requires random_access_iterator<_It> && totally_ordered_with<_It, _It2> { return __current_ >= __y; }

  friend constexpr basic_const_iterator operator+(const basic_const_iterator& __i, difference_type __n)
      requires random_access_iterator<_It> { return basic_const_iterator(__i.__current_ + __n); }
  friend constexpr basic_const_iterator operator+(difference_type __n, const basic_const_iterator& __i)
      requires random_access_iterator<_It> { return basic_const_iterator(__i.__current_ + __n); }
  friend constexpr basic_const_iterator operator-(const basic_const_iterator& __i, difference_type __n)
      requires random_access_iterator<_It> { return basic_const_iterator(__i.__current_ - __n); }

  template <sized_sentinel_for<_It> _Sent>
  constexpr difference_type operator-(const _Sent& __s) const { return __current_ - __s; }
  template <__not_a_const_iterator _Sent>
    requires sized_sentinel_for<_Sent, _It>
  friend constexpr difference_type operator-(const _Sent& __s, const basic_const_iterator& __i) {
    return __s - __i.__current_;
  }

  friend constexpr __rvalue_reference iter_move(const basic_const_iterator& __i)
      noexcept(noexcept(static_cast<__rvalue_reference>(ranges::iter_move(__i.__current_)))) {
    return static_cast<__rvalue_reference>(ranges::iter_move(__i.__current_));
  }
};

template <class _Tp, common_with<_Tp> _Up>
  requires input_iterator<common_type_t<_Tp, _Up>>
struct common_type<basic_const_iterator<_Tp>, _Up> {
  using type = basic_const_iterator<common_type_t<_Tp, _Up>>;
};
template <class _Tp, common_with<_Tp> _Up>
  requires input_iterator<common_type_t<_Tp, _Up>>
struct common_type<_Up, basic_const_iterator<_Tp>> {
  using type = basic_const_iterator<common_type_t<_Tp, _Up>>;
};
template <class _Tp, common_with<_Tp> _Up>
  requires input_iterator<common_type_t<_Tp, _Up>>
struct common_type<basic_const_iterator<_Tp>, basic_const_iterator<_Up>> {
  using type = basic_const_iterator<common_type_t<_Tp, _Up>>;
};

template <input_iterator _It>
constexpr const_iterator<_It> make_const_iterator(_It __it)
    noexcept(is_nothrow_convertible_v<_It, const_iterator<_It>>) { return __it; }

template <semiregular _Sent>
constexpr const_sentinel<_Sent> make_const_sentinel(_Sent __s)
    noexcept(is_nothrow_convertible_v<_Sent, const_sentinel<_Sent>>) { return __s; }

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___ITERATOR_CONST_ITERATOR_H
