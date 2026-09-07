// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FORMAT_FORMAT_ARG_STORE_H
#define _LIBCPP___FORMAT_FORMAT_ARG_STORE_H

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__format/concepts.h>
#include <__format/format_arg.h>
#include <__type_traits/conditional.h>
#include <__type_traits/extent.h>
#include <__type_traits/integer_traits.h>
#include <__type_traits/remove_const.h>
#include <cstdint>
#include <string>
#include <string_view>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

namespace __format {

template <class _Arr, class _Elem>
inline constexpr bool __is_bounded_array_of = false;

template <class _Elem, size_t _Len>
inline constexpr bool __is_bounded_array_of<_Elem[_Len], _Elem> = true;

/// \returns The @c __arg_t based on the type of the formatting argument.
///
/// \pre \c __formattable<_Tp, typename _Context::char_type>
template <class _Context, class _Tp>
consteval __arg_t __determine_arg_t();

// Boolean
template <class, same_as<bool> _Tp>
consteval __arg_t __determine_arg_t() {
  return __arg_t::__boolean;
}

// Char
template <class _Context, same_as<typename _Context::char_type> _Tp>
consteval __arg_t __determine_arg_t() {
  return __arg_t::__char_type;
}
#  if _LIBCPP_HAS_WIDE_CHARACTERS
template <class _Context, class _CharT>
  requires(same_as<typename _Context::char_type, wchar_t> && same_as<_CharT, char>)
consteval __arg_t __determine_arg_t() {
  return __arg_t::__char_type;
}
#  endif

// Signed integers
template <class, __signed_integer _Tp>
consteval __arg_t __determine_arg_t() {
  if constexpr (sizeof(_Tp) <= sizeof(int))
    return __arg_t::__int;
  else if constexpr (sizeof(_Tp) <= sizeof(long long))
    return __arg_t::__long_long;
#  if _LIBCPP_HAS_INT128
  else if constexpr (sizeof(_Tp) == sizeof(__int128_t))
    return __arg_t::__i128;
#  endif
  else
    static_assert(sizeof(_Tp) == 0, "an unsupported signed integer was used");
}

// Unsigned integers
template <class, __unsigned_integer _Tp>
consteval __arg_t __determine_arg_t() {
  if constexpr (sizeof(_Tp) <= sizeof(unsigned))
    return __arg_t::__unsigned;
  else if constexpr (sizeof(_Tp) <= sizeof(unsigned long long))
    return __arg_t::__unsigned_long_long;
#  if _LIBCPP_HAS_INT128
  else if constexpr (sizeof(_Tp) == sizeof(__uint128_t))
    return __arg_t::__u128;
#  endif
  else
    static_assert(sizeof(_Tp) == 0, "an unsupported unsigned integer was used");
}

// Floating-point
template <class, same_as<float> _Tp>
consteval __arg_t __determine_arg_t() {
  return __arg_t::__float;
}
template <class, same_as<double> _Tp>
consteval __arg_t __determine_arg_t() {
  return __arg_t::__double;
}
template <class, same_as<long double> _Tp>
consteval __arg_t __determine_arg_t() {
  return __arg_t::__long_double;
}

// Char pointer
template <class _Context, class _Tp>
  requires(same_as<typename _Context::char_type*, _Tp> || same_as<const typename _Context::char_type*, _Tp>)
consteval __arg_t __determine_arg_t() {
  return __arg_t::__const_char_type_ptr;
}

// Char array
template <class _Context, class _Tp>
  requires __is_bounded_array_of<_Tp, typename _Context::char_type>
consteval __arg_t __determine_arg_t() {
  return __arg_t::__string_view;
}

// String view
template <class _Context, class _Tp>
  requires(same_as<typename _Context::char_type, typename _Tp::value_type> &&
           same_as<_Tp, basic_string_view<typename _Tp::value_type, typename _Tp::traits_type>>)
consteval __arg_t __determine_arg_t() {
  return __arg_t::__string_view;
}

// String
template <class _Context, class _Tp>
  requires(
      same_as<typename _Context::char_type, typename _Tp::value_type> &&
      same_as<_Tp, basic_string<typename _Tp::value_type, typename _Tp::traits_type, typename _Tp::allocator_type>>)
consteval __arg_t __determine_arg_t() {
  return __arg_t::__string_view;
}

// Pointers
template <class, class _Ptr>
  requires(same_as<_Ptr, void*> || same_as<_Ptr, const void*> || same_as<_Ptr, nullptr_t>)
consteval __arg_t __determine_arg_t() {
  return __arg_t::__ptr;
}

// Handle
//
// Note this version can't be constrained avoiding ambiguous overloads.
// That means it can be instantiated by disabled formatters. To solve this, a
// constrained version for not formattable formatters is added.
template <class _Context, class _Tp>
consteval __arg_t __determine_arg_t() {
  return __arg_t::__handle;
}

// The overload for not formattable types allows triggering the static
// assertion below.
template <class _Context, class _Tp>
  requires(!__formattable_with<_Tp, _Context>)
consteval __arg_t __determine_arg_t() {
  return __arg_t::__none;
}

// Pseudo constuctor for basic_format_arg
//
// Modeled after template<class T> explicit basic_format_arg(T& v) noexcept;
// [format.arg]/4-6
template <class _Context, class _Tp>
_LIBCPP_HIDE_FROM_ABI basic_format_arg<_Context> __create_format_arg(_Tp& __value) noexcept {
  using _Dp               = remove_const_t<_Tp>;
  constexpr __arg_t __arg = __format::__determine_arg_t<_Context, _Dp>();
  static_assert(__arg != __arg_t::__none, "the supplied type is not formattable");
  static_assert(__formattable_with<_Tp, _Context>);

  using __context_char_type = _Context::char_type;
  // Not all types can be used to directly initialize the
  // __basic_format_arg_value.  First handle all types needing adjustment, the
  // final else requires no adjustment.
  if constexpr (__arg == __arg_t::__char_type)

#  if _LIBCPP_HAS_WIDE_CHARACTERS
    if constexpr (same_as<__context_char_type, wchar_t> && same_as<_Dp, char>)
      return basic_format_arg<_Context>{__arg, static_cast<wchar_t>(static_cast<unsigned char>(__value))};
    else
#  endif
      return basic_format_arg<_Context>{__arg, __value};
  else if constexpr (__arg == __arg_t::__int)
    return basic_format_arg<_Context>{__arg, static_cast<int>(__value)};
  else if constexpr (__arg == __arg_t::__long_long)
    return basic_format_arg<_Context>{__arg, static_cast<long long>(__value)};
  else if constexpr (__arg == __arg_t::__unsigned)
    return basic_format_arg<_Context>{__arg, static_cast<unsigned>(__value)};
  else if constexpr (__arg == __arg_t::__unsigned_long_long)
    return basic_format_arg<_Context>{__arg, static_cast<unsigned long long>(__value)};
  else if constexpr (__arg == __arg_t::__string_view)
    // Using std::size on a character array will add the NUL-terminator to the size.
    if constexpr (__is_bounded_array_of<_Dp, __context_char_type>) {
      const __context_char_type* const __pbegin = std::begin(__value);
      const __context_char_type* const __pzero =
          char_traits<__context_char_type>::find(__pbegin, extent_v<_Dp>, __context_char_type{});
      _LIBCPP_ASSERT_VALID_INPUT_RANGE(__pzero != nullptr, "formatting a non-null-terminated array");
      return basic_format_arg<_Context>{
          __arg, basic_string_view<__context_char_type>{__pbegin, static_cast<size_t>(__pzero - __pbegin)}};
    } else
      // When the _Traits or _Allocator are different an implicit conversion will fail.
      return basic_format_arg<_Context>{__arg, basic_string_view<__context_char_type>{__value.data(), __value.size()}};
  else if constexpr (__arg == __arg_t::__ptr)
    return basic_format_arg<_Context>{__arg, static_cast<const void*>(__value)};
  else if constexpr (__arg == __arg_t::__handle)
    return basic_format_arg<_Context>{__arg, typename __basic_format_arg_value<_Context>::__handle{__value}};
  else
    return basic_format_arg<_Context>{__arg, __value};
}

template <class _Context, class... _Args>
_LIBCPP_HIDE_FROM_ABI void
__create_packed_storage(uint64_t& __types, __basic_format_arg_value<_Context>* __values, _Args&... __args) noexcept {
  int __shift = 0;
  (
      [&] {
        basic_format_arg<_Context> __arg = __format::__create_format_arg<_Context>(__args);
        if (__shift != 0)
          __types |= static_cast<uint64_t>(__arg.__type_) << __shift;
        else
          // Assigns the initial value.
          __types = static_cast<uint64_t>(__arg.__type_);
        __shift += __packed_arg_t_bits;
        *__values++ = __arg.__value_;
      }(),
      ...);
}

template <class _Context, class... _Args>
_LIBCPP_HIDE_FROM_ABI void __store_basic_format_arg(basic_format_arg<_Context>* __data, _Args&... __args) noexcept {
  ([&] { *__data++ = __format::__create_format_arg<_Context>(__args); }(), ...);
}

template <class _Context, size_t _Np>
struct __packed_format_arg_store {
  __basic_format_arg_value<_Context> __values_[_Np];
  uint64_t __types_ = 0;
};

template <class _Context>
struct __packed_format_arg_store<_Context, 0> {
  uint64_t __types_ = 0;
};

template <class _Context, size_t _Np>
struct __unpacked_format_arg_store {
  basic_format_arg<_Context> __args_[_Np];
};

#  if _LIBCPP_STD_VER >= 26
// P2757R3's check_dynamic_spec<Ts...> mandates that every type in Ts... is
// one of exactly these twelve types -- not the broader set of types
// __determine_arg_t accepts (e.g. it excludes plain char arrays and
// basic_string, both of which __determine_arg_t maps to __string_view
// alongside basic_string_view itself). This mapping is intentionally
// separate from __determine_arg_t: it both enforces that Mandates and
// gives the diagnostic when it's violated, via the else branch below.
template <class _CharT, class _Tp>
consteval __arg_t __dynamic_spec_arg_t() {
  if constexpr (same_as<_Tp, bool>)
    return __arg_t::__boolean;
  else if constexpr (same_as<_Tp, _CharT>)
    return __arg_t::__char_type;
  else if constexpr (same_as<_Tp, int>)
    return __arg_t::__int;
  else if constexpr (same_as<_Tp, unsigned int>)
    return __arg_t::__unsigned;
  else if constexpr (same_as<_Tp, long long int>)
    return __arg_t::__long_long;
  else if constexpr (same_as<_Tp, unsigned long long int>)
    return __arg_t::__unsigned_long_long;
  else if constexpr (same_as<_Tp, float>)
    return __arg_t::__float;
  else if constexpr (same_as<_Tp, double>)
    return __arg_t::__double;
  else if constexpr (same_as<_Tp, long double>)
    return __arg_t::__long_double;
  else if constexpr (same_as<_Tp, const _CharT*>)
    return __arg_t::__const_char_type_ptr;
  else if constexpr (same_as<_Tp, basic_string_view<_CharT>>)
    return __arg_t::__string_view;
  else if constexpr (same_as<_Tp, const void*>)
    return __arg_t::__ptr;
  else
    static_assert(sizeof(_Tp) == 0,
                  "check_dynamic_spec<Ts...>: each type in Ts... must be one of bool, char_type, int, unsigned int, "
                  "long long int, unsigned long long int, float, double, long double, const char_type*, "
                  "basic_string_view<char_type>, or const void*");
}

template <class...>
inline constexpr bool __dynamic_spec_types_are_unique = true;

template <class _Tp, class... _Rest>
inline constexpr bool __dynamic_spec_types_are_unique<_Tp, _Rest...> =
    (!same_as<_Tp, _Rest> && ...) && __format::__dynamic_spec_types_are_unique<_Rest...>;
#  endif // _LIBCPP_STD_VER >= 26

} // namespace __format

#  if _LIBCPP_STD_VER >= 26
template <class _CharT>
template <class... _Ts>
_LIBCPP_HIDE_FROM_ABI constexpr void basic_format_parse_context<_CharT>::check_dynamic_spec(size_t __id) noexcept {
  static_assert(sizeof...(_Ts) > 0, "check_dynamic_spec<Ts...> requires at least one type");
  static_assert(__format::__dynamic_spec_types_are_unique<_Ts...>, "the types in Ts... must be unique");

  // [format.parse.ctx]: Call expressions where id >= num_args_ or the type
  // of the corresponding format argument is not one of the types in Ts...
  // are not core constant expressions. At runtime this performs no check
  // at all (matches next_arg_id/check_arg_id's existing is_constant_evaluated
  // gating just above).
  if (is_constant_evaluated()) {
    if (__id >= __num_args_ || __types_ == nullptr ||
        ((__types_[__id] != __format::__dynamic_spec_arg_t<_CharT, _Ts>()) && ...))
      std::__throw_format_error("Dynamic spec argument index outside the valid range or of an unexpected type");
  }
}
#  endif // _LIBCPP_STD_VER >= 26

template <class _Context, class... _Args>
struct __format_arg_store {
  _LIBCPP_HIDE_FROM_ABI __format_arg_store(_Args&... __args) noexcept {
    if constexpr (sizeof...(_Args) != 0) {
      if constexpr (__format::__use_packed_format_arg_store(sizeof...(_Args)))
        __format::__create_packed_storage(__storage.__types_, __storage.__values_, __args...);
      else
        __format::__store_basic_format_arg<_Context>(__storage.__args_, __args...);
    }
  }

  using _Storage _LIBCPP_NODEBUG =
      conditional_t<__format::__use_packed_format_arg_store(sizeof...(_Args)),
                    __format::__packed_format_arg_store<_Context, sizeof...(_Args)>,
                    __format::__unpacked_format_arg_store<_Context, sizeof...(_Args)>>;

  _Storage __storage;
};

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___FORMAT_FORMAT_ARG_STORE_H
