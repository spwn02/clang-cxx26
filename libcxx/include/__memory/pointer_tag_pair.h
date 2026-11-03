// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_POINTER_TAG_PAIR_H
#define _LIBCPP___MEMORY_POINTER_TAG_PAIR_H

#include <__assert>
#include <__bit/bit_width.h>
#include <__bit/countr.h>
#include <__compare/common_comparison_category.h>
#include <__compare/compare_three_way_result.h>
#include <__compare/ordering.h>
#include <__compare/three_way_comparable.h>
#include <__concepts/convertible_to.h>
#include <__concepts/equality_comparable.h>
#include <__config>
#include <__cstddef/nullptr_t.h>
#include <__cstddef/size_t.h>
#include <__memory/is_sufficiently_aligned.h>
#include <__memory/pointer_traits.h>
#include <__tuple/tuple_element.h>
#include <__tuple/tuple_size.h>
#include <__type_traits/add_pointer.h>
#include <__type_traits/copy_cv.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_enum.h>
#include <__type_traits/is_function.h>
#include <__type_traits/is_pointer.h>
#include <__type_traits/is_pointer_interconvertible_base_of.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_scalar.h>
#include <__type_traits/is_union.h>
#include <__type_traits/is_unsigned.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/remove_pointer.h>
#include <__type_traits/underlying_type.h>
#include <__utility/to_underlying.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 29

// [ptrtag.bits]
//
// A tagged pointer is a pointer whose low bits hold the tag. __builtin_pointer_tag_insert, _get and _strip build and take
// it apart: at run time with address arithmetic, in constant expressions with a tag the evaluator keeps next to the
// pointer without observing an address. A tagged pointer must only be copied, compared, converted to void* or passed to
// these builtins; stripping it gives the pointer back.

// Tagging must never overflow into the page number.
inline constexpr unsigned max_pointer_bits_available = 12;

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr unsigned pointer_bits_available(size_t __alignment) {
  // The precondition is part of the constant expression requirements whatever the hardening mode is.
  if consteval {
    if (__alignment == 0 || (__alignment & (__alignment - 1)) != 0)
      __builtin_unreachable();
  } else {
    _LIBCPP_ASSERT_UNCATEGORIZED(
        __alignment != 0 && (__alignment & (__alignment - 1)) == 0, "pointer_bits_available requires a power of two");
  }
  unsigned __bits = static_cast<unsigned>(std::countr_zero(__alignment));
  return __bits < max_pointer_bits_available ? __bits : max_pointer_bits_available;
}

template <class _Tp>
inline constexpr unsigned __bits_available_v = std::pointer_bits_available(alignof(_Tp));

template <class _Up>
using __element_of_t _LIBCPP_NODEBUG = typename pointer_traits<_Up>::element_type;

template <class _Up, class _PtrT, unsigned _BitsRequested, size_t _Alignment = alignof(_Up)>
concept __tagging_compatible_pointee =
    convertible_to<_Up*, _PtrT> && std::pointer_bits_available(_Alignment) >= _BitsRequested &&
    (is_void_v<__element_of_t<_PtrT>> || is_scalar_v<__element_of_t<_PtrT>> || is_union_v<__element_of_t<_PtrT>> ||
     is_pointer_interconvertible_base_of_v<__element_of_t<_PtrT>, _Up>);

template <class _TagT>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr unsigned __tag_bit_width(_TagT __value) noexcept {
  if constexpr (is_enum_v<_TagT>)
    return static_cast<unsigned>(std::bit_width(std::__to_underlying(__value)));
  else
    return static_cast<unsigned>(std::bit_width(__value));
}

template <class _Tp, bool = is_enum_v<_Tp>>
struct __tag_underlying {
  using type _LIBCPP_NODEBUG = _Tp;
};
template <class _Tp>
struct __tag_underlying<_Tp, true> {
  using type _LIBCPP_NODEBUG = underlying_type_t<_Tp>;
};

template <class _Ptr, unsigned _BitsRequested = __bits_available_v<__element_of_t<_Ptr>>, class _TagT = unsigned>
class pointer_tag_pair {
  static_assert(is_same_v<remove_cvref_t<_Ptr>, _Ptr>, "Ptr must not be cv-qualified or a reference");
  static_assert(is_same_v<remove_cvref_t<_TagT>, _TagT>, "TagT must not be cv-qualified or a reference");
  static_assert(is_pointer_v<_Ptr> && !is_function_v<remove_pointer_t<_Ptr>>,
                "Ptr must be a pointer to an object or to void");
  static_assert(sizeof(_TagT) <= sizeof(void*), "TagT must not be larger than a pointer");
  static_assert(is_unsigned_v<typename __tag_underlying<_TagT>::type>, "TagT must be an unsigned integer or an enum of one");
  static_assert(_BitsRequested <= max_pointer_bits_available, "More tag bits requested than a pointer can hold");

public:
  using pointer_type        = _Ptr;
  using element_type        = typename pointer_traits<_Ptr>::element_type;
  using tagged_pointer_type = add_pointer_t<__copy_cv_t<element_type, void>>;
  using tag_type            = _TagT;
  static constexpr unsigned bits_requested = _BitsRequested;

  // [ptrtag.pair.cons]
  _LIBCPP_HIDE_FROM_ABI constexpr pointer_tag_pair() noexcept : __tagged_(nullptr) {}

  template <__tagging_compatible_pointee<pointer_type, bits_requested> _Up>
  _LIBCPP_HIDE_FROM_ABI constexpr pointer_tag_pair(_Up* __p, tag_type __t)
      : __tagged_(__make(static_cast<pointer_type>(__p), __t)) {}

  _LIBCPP_HIDE_FROM_ABI constexpr pointer_tag_pair(nullptr_t, tag_type __t)
      : __tagged_(__make(static_cast<pointer_type>(nullptr), __t)) {}

  // [ptrtag.pair.overalign]
  template <size_t _PromisedAlignment, __tagging_compatible_pointee<pointer_type, bits_requested, _PromisedAlignment> _Up>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr pointer_tag_pair from_overaligned(_Up* __p, tag_type __t) {
    if consteval {
      // Constant when the promise is provably kept: the object the pointer points into is aligned enough.
      if (__p != nullptr && !__builtin_is_aligned(__p, _PromisedAlignment))
        __builtin_unreachable();
    } else {
      _LIBCPP_ASSERT_UNCATEGORIZED(__p == nullptr || std::is_sufficiently_aligned<_PromisedAlignment>(__p),
                                   "from_overaligned requires a pointer aligned to the promised alignment");
    }
    pointer_tag_pair __result;
    __result.__tagged_ = __make(static_cast<pointer_type>(__p), __t);
    return __result;
  }

  // [ptrtag.pair.tagops]
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI tagged_pointer_type tagged_pointer() const noexcept { return __tagged_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static pointer_tag_pair from_tagged(tagged_pointer_type __p) noexcept {
    pointer_tag_pair __result;
    __result.__tagged_ = __p;
    return __result;
  }

  // [ptrtag.pair.accessors]
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr pointer_type pointer() const noexcept {
    return static_cast<pointer_type>(__builtin_pointer_tag_strip(__tagged_, bits_requested));
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr tag_type tag() const noexcept {
    return static_cast<tag_type>(__builtin_pointer_tag_get(__tagged_, bits_requested));
  }

  // [ptrtag.pair.swap]
  _LIBCPP_HIDE_FROM_ABI constexpr void swap(pointer_tag_pair& __o) noexcept {
    tagged_pointer_type __tmp = __tagged_;
    __tagged_                 = __o.__tagged_;
    __o.__tagged_             = __tmp;
  }

  // [ptrtag.pair.comp]
  _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(pointer_tag_pair __lhs, pointer_tag_pair __rhs) noexcept
    requires three_way_comparable<tag_type>
  {
    using _Result = common_comparison_category_t<strong_ordering, compare_three_way_result_t<tag_type>>;
    auto __ptr_cmp = __lhs.pointer() <=> __rhs.pointer();
    if (__ptr_cmp != 0)
      return static_cast<_Result>(__ptr_cmp);
    return static_cast<_Result>(__lhs.tag() <=> __rhs.tag());
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(pointer_tag_pair __lhs, pointer_tag_pair __rhs) noexcept
    requires equality_comparable<tag_type>
  {
    return __lhs.pointer() == __rhs.pointer() && __lhs.tag() == __rhs.tag();
  }

private:
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr tagged_pointer_type __make(pointer_type __p, tag_type __t) {
    _LIBCPP_ASSERT_UNCATEGORIZED(std::__tag_bit_width(__t) <= bits_requested, "the tag does not fit in the requested bits");
    size_t __tag = 0;
    if constexpr (is_enum_v<tag_type>)
      __tag = static_cast<size_t>(std::__to_underlying(__t));
    else
      __tag = static_cast<size_t>(__t);
    return static_cast<tagged_pointer_type>(
        __builtin_pointer_tag_insert(static_cast<tagged_pointer_type>(__p), __tag, bits_requested));
  }

  tagged_pointer_type __tagged_;
};

template <class _Ptr>
pointer_tag_pair(_Ptr*) -> pointer_tag_pair<_Ptr*>;

template <class _Ptr, class _TagT>
pointer_tag_pair(_Ptr*, _TagT) -> pointer_tag_pair<_Ptr*, __bits_available_v<_Ptr>, _TagT>;

template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_size<pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> : integral_constant<size_t, 2> {};

template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_element<0, pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> {
  using type _LIBCPP_NODEBUG = _Ptr;
};

template <class _Ptr, unsigned _BitsRequested, class _TagT>
struct tuple_element<1, pointer_tag_pair<_Ptr, _BitsRequested, _TagT>> {
  using type _LIBCPP_NODEBUG = _TagT;
};

template <size_t _Ip, class _Ptr, unsigned _BitsRequested, class _TagT>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr tuple_element_t<_Ip, pointer_tag_pair<_Ptr, _BitsRequested, _TagT>>
get(pointer_tag_pair<_Ptr, _BitsRequested, _TagT> __p) noexcept {
  static_assert(_Ip < 2, "the index of get on a pointer_tag_pair must be 0 or 1");
  if constexpr (_Ip == 0)
    return __p.pointer();
  else
    return __p.tag();
}

#endif // _LIBCPP_STD_VER >= 29

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MEMORY_POINTER_TAG_PAIR_H
