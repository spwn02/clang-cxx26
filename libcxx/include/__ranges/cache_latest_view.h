// -*- C++ -*-
//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANGES_CACHE_LATEST_VIEW_H
#define _LIBCPP___RANGES_CACHE_LATEST_VIEW_H

#include <__config>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <__memory/addressof.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/non_propagating_cache.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/view_interface.h>
#include <__type_traits/conditional.h>
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
class cache_latest_view : public view_interface<cache_latest_view<_View>> {
  _View __base_ = _View();
  using __cache_t = _If<is_reference_v<range_reference_t<_View>>,
                        add_pointer_t<range_reference_t<_View>>, range_reference_t<_View>>;
  mutable __non_propagating_cache<__cache_t> __cache_;
  class __iterator;
  class __sentinel;

public:
  cache_latest_view() requires default_initializable<_View> = default;
  constexpr explicit cache_latest_view(_View __base) : __base_(std::move(__base)) {}
  constexpr _View base() const& requires copy_constructible<_View> { return __base_; }
  constexpr _View base() && { return std::move(__base_); }
  constexpr auto begin() { return __iterator(*this); }
  constexpr auto end() { return __sentinel(*this); }
  constexpr auto size() requires sized_range<_View> { return ranges::size(__base_); }
  constexpr auto size() const requires sized_range<const _View> { return ranges::size(__base_); }

private:
  friend class __iterator;
  friend class __sentinel;
};

template <class _Range>
cache_latest_view(_Range&&) -> cache_latest_view<views::all_t<_Range>>;

template <input_range _View>
  requires view<_View>
class cache_latest_view<_View>::__iterator {
  cache_latest_view* __parent_;
  iterator_t<_View> __current_;
  explicit __iterator(cache_latest_view& __parent)
      : __parent_(std::addressof(__parent)), __current_(ranges::begin(__parent.__base_)) {}
  friend class cache_latest_view;

public:
  using difference_type = range_difference_t<_View>;
  using value_type = range_value_t<_View>;
  using iterator_concept = input_iterator_tag;
  __iterator(__iterator&&) = default;
  __iterator& operator=(__iterator&&) = default;
  constexpr iterator_t<_View> base() && { return std::move(__current_); }
  constexpr const iterator_t<_View>& base() const& noexcept { return __current_; }
  constexpr range_reference_t<_View>& operator*() const {
    if constexpr (is_reference_v<range_reference_t<_View>>) {
      if (!__parent_->__cache_.__has_value())
        __parent_->__cache_.__emplace(std::addressof(*__current_));
      return **__parent_->__cache_;
    } else {
      if (!__parent_->__cache_.__has_value())
        __parent_->__cache_.__emplace_from([&] { return *__current_; });
      return *__parent_->__cache_;
    }
  }
  constexpr __iterator& operator++() { __parent_->__cache_ = {}; ++__current_; return *this; }
  constexpr void operator++(int) { ++*this; }
  friend constexpr range_rvalue_reference_t<_View> iter_move(const __iterator& __it)
      noexcept(noexcept(ranges::iter_move(__it.__current_))) { return ranges::iter_move(__it.__current_); }
  friend constexpr void iter_swap(const __iterator& __x, const __iterator& __y)
      noexcept(noexcept(ranges::iter_swap(__x.__current_, __y.__current_)))
      requires indirectly_swappable<iterator_t<_View>> { ranges::iter_swap(__x.__current_, __y.__current_); }
};

template <input_range _View>
  requires view<_View>
class cache_latest_view<_View>::__sentinel {
  sentinel_t<_View> __end_ = sentinel_t<_View>();
  explicit __sentinel(cache_latest_view& __parent) : __end_(ranges::end(__parent.__base_)) {}
  friend class cache_latest_view;
public:
  __sentinel() = default;
  constexpr sentinel_t<_View> base() const { return __end_; }
  friend constexpr bool operator==(const __iterator& __it, const __sentinel& __sent) {
    return __it.__current_ == __sent.__end_;
  }
  friend constexpr range_difference_t<_View> operator-(const __iterator& __it, const __sentinel& __sent)
      requires sized_sentinel_for<sentinel_t<_View>, iterator_t<_View>> { return __it.__current_ - __sent.__end_; }
  friend constexpr range_difference_t<_View> operator-(const __sentinel& __sent, const __iterator& __it)
      requires sized_sentinel_for<sentinel_t<_View>, iterator_t<_View>> { return __sent.__end_ - __it.__current_; }
};

template <class _View>
inline constexpr bool enable_borrowed_range<cache_latest_view<_View>> = enable_borrowed_range<_View>;

namespace views {
struct __cache_latest_fn : range_adaptor_closure<__cache_latest_fn> {
  template <viewable_range _Range>
  constexpr auto operator()(_Range&& __range) const { return cache_latest_view(std::forward<_Range>(__range)); }
};
inline constexpr __cache_latest_fn cache_latest{};
} // namespace views
} // namespace ranges
#endif

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif
