// -*- C++ -*-
//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___RANGES_AS_CONST_VIEW_H
#define _LIBCPP___RANGES_AS_CONST_VIEW_H

#include <__config>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/size.h>
#include <__ranges/view_interface.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD
#if _LIBCPP_STD_VER >= 23
namespace ranges {

template <input_range _View>
  requires view<_View>
class as_const_view : public view_interface<as_const_view<_View>> {
  _View __base_ = _View();

public:
  as_const_view() requires default_initializable<_View> = default;
  _LIBCPP_HIDE_FROM_ABI constexpr explicit as_const_view(_View __base)
      noexcept(is_nothrow_move_constructible_v<_View>) : __base_(std::move(__base)) {}

  _LIBCPP_HIDE_FROM_ABI constexpr _View base() const&
      noexcept(is_nothrow_copy_constructible_v<_View>) requires copy_constructible<_View> { return __base_; }
  _LIBCPP_HIDE_FROM_ABI constexpr _View base() && noexcept(is_nothrow_move_constructible_v<_View>) {
    return std::move(__base_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto begin() requires (!__simple_view<_View>) { return ranges::cbegin(__base_); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto begin() const requires range<const _View> {
    return ranges::cbegin(__base_);
  }
  _LIBCPP_HIDE_FROM_ABI constexpr auto end() requires (!__simple_view<_View>) { return ranges::cend(__base_); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto end() const requires range<const _View> {
    return ranges::cend(__base_);
  }
  _LIBCPP_HIDE_FROM_ABI constexpr auto size() requires sized_range<_View> { return ranges::size(__base_); }
  _LIBCPP_HIDE_FROM_ABI constexpr auto size() const requires sized_range<const _View> {
    return ranges::size(__base_);
  }
};

template <class _Range>
as_const_view(_Range&&) -> as_const_view<views::all_t<_Range>>;

template <class _View>
inline constexpr bool enable_borrowed_range<as_const_view<_View>> = enable_borrowed_range<_View>;

namespace views {
struct __as_const : range_adaptor_closure<__as_const> {
  template <viewable_range _Range>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range) const
      noexcept(noexcept(as_const_view(std::forward<_Range>(__range))))
      requires requires { as_const_view(std::forward<_Range>(__range)); }
  { return as_const_view(std::forward<_Range>(__range)); }
};
inline constexpr __as_const as_const{};
} // namespace views
} // namespace ranges
#endif
_LIBCPP_END_NAMESPACE_STD
#endif
