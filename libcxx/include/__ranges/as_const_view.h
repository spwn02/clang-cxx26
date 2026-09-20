// -*- C++ -*-
//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___RANGES_AS_CONST_VIEW_H
#define _LIBCPP___RANGES_AS_CONST_VIEW_H

#include <__config>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/empty_view.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/range_adaptor.h>
#include <span>
#include <__ranges/size.h>
#include <__ranges/view_interface.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <optional>

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
      noexcept(is_nothrow_move_constructible_v<_View>) : __base_(std::forward<_View>(__base)) {}

  _LIBCPP_HIDE_FROM_ABI constexpr _View base() const&
      noexcept(is_nothrow_copy_constructible_v<_View>) requires copy_constructible<_View> { return __base_; }
  _LIBCPP_HIDE_FROM_ABI constexpr _View base() && noexcept(is_nothrow_move_constructible_v<_View>) {
    return std::forward<_View>(__base_);
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
template <class _Tp>
struct __as_const_empty_view_type;
template <class _Tp>
struct __as_const_empty_view_type<empty_view<_Tp>> {
  using type = _Tp;
};

template <class _Tp>
inline constexpr bool __as_const_is_empty_view = requires { typename __as_const_empty_view_type<_Tp>::type; };

template <class _Tp>
struct __as_const_span_type;
template <class _Tp, size_t _Extent>
struct __as_const_span_type<span<_Tp, _Extent>> {
    using type = span<const _Tp, _Extent>;
};

template <class _Tp>
struct __as_const_ref_view_type;
template <class _Tp>
struct __as_const_ref_view_type<ref_view<_Tp>> {
  using type = ref_view<const _Tp>;
  using base_type = _Tp;
};

template <class _Tp>
struct __optional_reference {
  static constexpr bool value = false;
};

template <class _Tp>
struct __optional_reference<optional<_Tp&>> {
  static constexpr bool value = true;
  using type                     = _Tp;
};

struct __as_const : range_adaptor_closure<__as_const> {
  template <viewable_range _Range>
    requires constant_range<views::all_t<_Range>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range) const
      noexcept(noexcept(views::all(std::forward<_Range>(__range))))
          -> decltype(views::all(std::forward<_Range>(__range))) {
    return views::all(std::forward<_Range>(__range));
  }

  template <class _Range, class _RawRange = remove_cvref_t<_Range>>
    requires __optional_reference<_RawRange>::value &&
             (!(viewable_range<_Range> && constant_range<views::all_t<_Range>>))
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range) const
      noexcept(noexcept(optional<const typename __optional_reference<_RawRange>::type&>(
          std::forward<_Range>(__range))))
          -> decltype(optional<const typename __optional_reference<_RawRange>::type&>(std::forward<_Range>(__range))) {
    return optional<const typename __optional_reference<_RawRange>::type&>(std::forward<_Range>(__range));
  }

  template <class _Range, class _RawRange = remove_cvref_t<_Range>>
    requires __as_const_is_empty_view<_RawRange> &&
             (!(viewable_range<_Range> && constant_range<views::all_t<_Range>>)) &&
             (!__optional_reference<_RawRange>::value)
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&&) const
      noexcept
      -> empty_view<const typename __as_const_empty_view_type<_RawRange>::type> {
    return {};
  }

  template <class _Range, class _RawRange = remove_cvref_t<_Range>>
    requires requires { typename __as_const_span_type<_RawRange>::type; } &&
             (!(viewable_range<_Range> && constant_range<views::all_t<_Range>>)) &&
             (!__as_const_is_empty_view<_RawRange>) &&
             (!__optional_reference<_RawRange>::value)
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range) const
      noexcept(noexcept(typename __as_const_span_type<_RawRange>::type(__range)))
          -> typename __as_const_span_type<_RawRange>::type {
    return typename __as_const_span_type<_RawRange>::type(__range);
  }

  template <class _Range, class _RawRange = remove_cvref_t<_Range>>
    requires requires { typename __as_const_ref_view_type<_RawRange>::type; } &&
             constant_range<const typename __as_const_ref_view_type<_RawRange>::base_type> &&
             (!(viewable_range<_Range> && constant_range<views::all_t<_Range>>)) &&
             (!__as_const_is_empty_view<_RawRange>) &&
             (!__optional_reference<_RawRange>::value) &&
             (!(requires { typename __as_const_span_type<_RawRange>::type; }))
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range) const
      -> typename __as_const_ref_view_type<_RawRange>::type {
    return typename __as_const_ref_view_type<_RawRange>::type(__range.base());
  }

  template <class _Range, class _RawRange = remove_cvref_t<_Range>>
    requires is_lvalue_reference_v<_Range> && (!view<_RawRange>) &&
             constant_range<const _RawRange> &&
             (!(viewable_range<_Range> && constant_range<views::all_t<_Range>>)) &&
             (!__optional_reference<_RawRange>::value) &&
             (!__as_const_is_empty_view<_RawRange>) &&
             (!(requires { typename __as_const_span_type<_RawRange>::type; })) &&
             (!(requires { typename __as_const_ref_view_type<_RawRange>::type; }))
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range) const
      noexcept(noexcept(ref_view<const _RawRange>(static_cast<const _RawRange&>(__range))))
          -> ref_view<const _RawRange> {
    return ref_view<const _RawRange>(static_cast<const _RawRange&>(__range));
  }

  template <viewable_range _Range>
    requires(!__optional_reference<remove_cvref_t<_Range>>::value) &&
            (!(constant_range<views::all_t<_Range>>)) &&
            (!__as_const_is_empty_view<remove_cvref_t<_Range>>) &&
            (!(requires { typename __as_const_span_type<remove_cvref_t<_Range>>::type; })) &&
            (!(requires { typename __as_const_ref_view_type<remove_cvref_t<_Range>>::type; } &&
              constant_range<const typename __as_const_ref_view_type<remove_cvref_t<_Range>>::base_type>))
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
