// -*- C++ -*-
#ifndef _LIBCPP___MDSPAN_LAYOUT_LEFT_PADDED_H
#define _LIBCPP___MDSPAN_LAYOUT_LEFT_PADDED_H

#include <__assert>
#include <__config>
#include <__fwd/mdspan.h>
#include <__mdspan/extents.h>
#include <__mdspan/layout_left.h>
#include <__mdspan/layout_stride.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__utility/integer_sequence.h>

_LIBCPP_BEGIN_NAMESPACE_STD
#if _LIBCPP_STD_VER >= 26

template <size_t _PaddingValue>
template <class _Extents>
class layout_left_padded<_PaddingValue>::mapping {
  static constexpr size_t __rank = _Extents::rank();
  using _Index = typename _Extents::index_type;
  _Extents __extents_{};
  _Index __stride1_{};
  static constexpr size_t __round(size_t __p, size_t __n) {
    return __p == dynamic_extent ? __n : (__n == 0 ? 0 : ((__n + __p - 1) / __p) * __p);
  }
  constexpr _Index __stride1(const _Extents& __e) const {
    if constexpr (__rank <= 1) return 0;
    return static_cast<_Index>(__round(_PaddingValue, static_cast<size_t>(__e.extent(0))));
  }
public:
  static constexpr size_t padding_value = _PaddingValue;
  using extents_type = _Extents; using index_type = _Index;
  using size_type = typename _Extents::size_type; using rank_type = typename _Extents::rank_type;
  using layout_type = layout_left_padded;
  constexpr mapping() noexcept : mapping(_Extents{}) {}
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const _Extents& __e) noexcept : __extents_(__e), __stride1_(__stride1(__e)) {}
  template<class _Other>
    requires(is_convertible_v<_Other, _Index> && is_nothrow_constructible_v<_Index, _Other>)
  constexpr mapping(const _Extents& __e, _Other __p) noexcept
      : __extents_(__e), __stride1_(__rank <= 1 ? 0 : static_cast<_Index>(__round(static_cast<size_t>(__p), __e.extent(0)))) {}
  template<class _OtherExtents>
    requires(is_constructible_v<_Extents, _OtherExtents>)
  constexpr explicit(!is_convertible_v<_OtherExtents, _Extents>) mapping(const layout_left::mapping<_OtherExtents>& __m) noexcept
      : mapping(_Extents(__m.extents())) {}
  template<class _OtherExtents>
    requires(is_constructible_v<_Extents, _OtherExtents>)
  constexpr explicit mapping(const layout_stride::mapping<_OtherExtents>& __m) noexcept
      : __extents_(__m.extents()), __stride1_(__rank <= 1 ? 0 : __m.stride(1)) {}
  constexpr mapping& operator=(const mapping&) noexcept = default;
  constexpr const _Extents& extents() const noexcept { return __extents_; }
  constexpr index_type stride(rank_type __r) const noexcept {
    if (__r == 0) return 1; index_type __s = __stride1_;
    for (rank_type __i = 1; __i < __r; ++__i) __s *= __extents_.extent(__i); return __s;
  }
  constexpr index_type required_span_size() const noexcept {
    if constexpr (__rank == 0) return 1;
    if (__extents_.extent(0) == 0) return 0;
    index_type __r = 0;
    for (size_t __i = 0; __i < __rank; ++__i)
      __r += (__extents_.extent(__i) - 1) * stride(__i);
    return __r + 1;
  }
  template<class... _I> requires(sizeof...(_I) == __rank)
  constexpr index_type operator()(_I... __i) const noexcept {
    index_type __r = 0; index_type __a[] = {static_cast<index_type>(__i)...};
    for (size_t __n = __rank; __n-- > 0;) __r = __a[__n] + __extents_.extent(__n) * __r;
    if constexpr (__rank > 1) { __r = 0; for (size_t __n = 0; __n < __rank; ++__n) __r += __a[__n] * stride(__n); }
    return __r;
  }
  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_strided() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept { return _PaddingValue == dynamic_extent || __rank <= 1; }
  static constexpr bool is_unique() noexcept { return true; }
  static constexpr bool is_strided() noexcept { return true; }
  constexpr bool is_exhaustive() const noexcept { return __rank <= 1 || stride(1) == __extents_.extent(0); }
};

#endif
_LIBCPP_END_NAMESPACE_STD
#endif
