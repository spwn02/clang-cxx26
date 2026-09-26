// -*- C++ -*-
#ifndef _LIBCPP___MDSPAN_LAYOUT_RIGHT_PADDED_H
#define _LIBCPP___MDSPAN_LAYOUT_RIGHT_PADDED_H
#include <__config>
#include <__fwd/mdspan.h>
#include <__mdspan/layout_right.h>
#include <__mdspan/layout_stride.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_nothrow_constructible.h>
#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD
#if _LIBCPP_STD_VER >= 26
template <size_t _PaddingValue> template <class _Extents>
class layout_right_padded<_PaddingValue>::mapping {
  static constexpr size_t __rank = _Extents::rank(); using _Index = typename _Extents::index_type;
  _Extents __extents_{}; _Index __stride_{};
  static constexpr size_t __round(size_t p, size_t n) { return p == dynamic_extent ? n : (n == 0 ? 0 : ((n+p-1)/p)*p); }
public:
  static constexpr size_t padding_value = _PaddingValue; using extents_type = _Extents; using index_type = _Index;
  using size_type = typename _Extents::size_type; using rank_type = typename _Extents::rank_type; using layout_type = layout_right_padded;
  constexpr mapping() noexcept : mapping(_Extents{}) {} constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const _Extents& e) noexcept : __extents_(e), __stride_(__rank <= 1 ? 0 : static_cast<_Index>(__round(_PaddingValue, e.extent(__rank-1)))) {}
  template<class O> requires(is_convertible_v<O,_Index> && is_nothrow_constructible_v<_Index,O>)
  constexpr mapping(const _Extents& e, O p) noexcept : __extents_(e), __stride_(__rank <= 1 ? 0 : static_cast<_Index>(__round(static_cast<size_t>(p),e.extent(__rank-1)))) {}
  template<class OE> requires(is_constructible_v<_Extents,OE>)
  constexpr explicit(!is_convertible_v<OE,_Extents>) mapping(const layout_right::mapping<OE>& m) noexcept : mapping(_Extents(m.extents())) {}
  template<class OE> requires(is_constructible_v<_Extents,OE>)
  constexpr explicit mapping(const layout_stride::mapping<OE>& m) noexcept : __extents_(m.extents()), __stride_(__rank <= 1 ? 0 : m.stride(__rank-2)) {}
  constexpr mapping& operator=(const mapping&) noexcept = default; constexpr const _Extents& extents() const noexcept { return __extents_; }
  constexpr index_type stride(rank_type r) const noexcept { if (r == __rank-1) return 1; index_type s = __stride_; for (rank_type i=__rank-2; i>r; --i) s *= __extents_.extent(i); return s; }
  constexpr index_type required_span_size() const noexcept { if constexpr (__rank==0) return 1; if (__extents_.extent(__rank-1)==0) return 0; index_type r=0; index_type a[__rank]{}; for(size_t i=0;i<__rank;++i)a[i]=__extents_.extent(i)-1; for(size_t i=0;i<__rank;++i)r+=a[i]*stride(i); return r+1; }
  template<class... I> requires(sizeof...(I)==__rank) constexpr index_type operator()(I... i) const noexcept { index_type a[] = {static_cast<index_type>(i)...}; index_type r=0; for(size_t n=0;n<__rank;++n)r+=a[n]*stride(n); return r; }
  static constexpr bool is_always_unique() noexcept{return true;} static constexpr bool is_always_strided() noexcept{return true;} static constexpr bool is_always_exhaustive() noexcept{return _PaddingValue==dynamic_extent||__rank<=1;} static constexpr bool is_unique() noexcept{return true;} static constexpr bool is_strided() noexcept{return true;} constexpr bool is_exhaustive() const noexcept{return __rank<=1||stride(__rank-2)==__extents_.extent(__rank-1);}
};
#endif
_LIBCPP_END_NAMESPACE_STD
#endif
