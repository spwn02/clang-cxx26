// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FORMAT_FORMAT_PARSE_CONTEXT_H
#define _LIBCPP___FORMAT_FORMAT_PARSE_CONTEXT_H

#include <__config>
#include <__format/format_error.h>
#include <__type_traits/is_constant_evaluated.h>
#include <cstdint>
#include <string_view>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

namespace __format {
// Defined in format_arg.h, which is included after this header. An opaque
// enum declaration with a fixed underlying type is a complete type for the
// purposes of declaring a pointer member and comparing values, which is all
// basic_format_parse_context needs -- including format_arg.h here would be
// circular (format_arg.h includes this header).
enum class __arg_t : uint8_t;
} // namespace __format

template <class _CharT>
class basic_format_parse_context {
public:
  using char_type      = _CharT;
  using const_iterator = typename basic_string_view<_CharT>::const_iterator;
  using iterator       = const_iterator;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit basic_format_parse_context(
      basic_string_view<_CharT> __fmt, size_t __num_args = 0) noexcept
      : __begin_(__fmt.begin()),
        __end_(__fmt.end()),
        __indexing_(__unknown),
        __next_arg_id_(0),
        __num_args_(__num_args) {}

  basic_format_parse_context(const basic_format_parse_context&)            = delete;
  basic_format_parse_context& operator=(const basic_format_parse_context&) = delete;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_iterator begin() const noexcept { return __begin_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_iterator end() const noexcept { return __end_; }
  _LIBCPP_HIDE_FROM_ABI constexpr void advance_to(const_iterator __it) { __begin_ = __it; }

  _LIBCPP_HIDE_FROM_ABI constexpr size_t next_arg_id() {
    if (__indexing_ == __manual)
      std::__throw_format_error("Using automatic argument numbering in manual argument numbering mode");

    if (__indexing_ == __unknown)
      __indexing_ = __automatic;

    // Throws an exception to make the expression a non core constant
    // expression as required by:
    // [format.parse.ctx]/8
    //   Remarks: Let cur-arg-id be the value of next_arg_id_ prior to this
    //   call. Call expressions where cur-arg-id >= num_args_ is true are not
    //   core constant expressions (7.7 [expr.const]).
    // Note: the Throws clause [format.parse.ctx]/9 doesn't specify the
    // behavior when id >= num_args_.
    if (is_constant_evaluated() && __next_arg_id_ >= __num_args_)
      std::__throw_format_error("Argument index outside the valid range");

    return __next_arg_id_++;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr void check_arg_id(size_t __id) {
    if (__indexing_ == __automatic)
      std::__throw_format_error("Using manual argument numbering in automatic argument numbering mode");

    if (__indexing_ == __unknown)
      __indexing_ = __manual;

    // Throws an exception to make the expression a non core constant
    // expression as required by:
    // [format.parse.ctx]/11
    //   Remarks: Call expressions where id >= num_args_ are not core constant
    //   expressions ([expr.const]).
    // Note: the Throws clause [format.parse.ctx]/10 doesn't specify the
    // behavior when id >= num_args_.
    if (is_constant_evaluated() && __id >= __num_args_)
      std::__throw_format_error("Argument index outside the valid range");
  }

#  if _LIBCPP_STD_VER >= 26
  // P2757R3: lets a formatter's parse() validate the type of a dynamic
  // width/precision argument at compile time. __types_ is only non-null
  // when this context was built by the library's own compile-time format
  // string validation (see basic_format_string's consteval constructor in
  // format_functions.h); a directly user-constructed context has no type
  // information available, so these always fail to be a constant
  // expression in that case, same as when id >= num_args_.
  template <class... _Ts>
  _LIBCPP_HIDE_FROM_ABI constexpr void check_dynamic_spec(size_t __id) noexcept;

  _LIBCPP_HIDE_FROM_ABI constexpr void check_dynamic_spec_integral(size_t __id) noexcept {
    check_dynamic_spec<int, unsigned int, long long int, unsigned long long int>(__id);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void check_dynamic_spec_string(size_t __id) noexcept {
    check_dynamic_spec<const char_type*, basic_string_view<char_type>>(__id);
  }
#  endif // _LIBCPP_STD_VER >= 26

private:
  iterator __begin_;
  iterator __end_;
  enum _Indexing { __unknown, __manual, __automatic };
  _Indexing __indexing_;
  size_t __next_arg_id_;
  size_t __num_args_;
  const __format::__arg_t* __types_ = nullptr; // only set by the private constructor below

  _LIBCPP_HIDE_FROM_ABI constexpr explicit basic_format_parse_context(
      basic_string_view<_CharT> __fmt, size_t __num_args, const __format::__arg_t* __types) noexcept
      : __begin_(__fmt.begin()),
        __end_(__fmt.end()),
        __indexing_(__unknown),
        __next_arg_id_(0),
        __num_args_(__num_args),
        __types_(__types) {}

  template <class, class...>
  friend struct basic_format_string;
};
_LIBCPP_CTAD_SUPPORTED_FOR_TYPE(basic_format_parse_context);

using format_parse_context = basic_format_parse_context<char>;
#  if _LIBCPP_HAS_WIDE_CHARACTERS
using wformat_parse_context = basic_format_parse_context<wchar_t>;
#  endif

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___FORMAT_FORMAT_PARSE_CONTEXT_H
