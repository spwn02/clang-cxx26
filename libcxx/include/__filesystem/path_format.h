// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FILESYSTEM_PATH_FORMAT_H
#define _LIBCPP___FILESYSTEM_PATH_FORMAT_H

#include <__config>
#include <__filesystem/path.h>
#include <__format/concepts.h>
#include <__format/format_error.h>
#include <__format/format_parse_context.h>
#include <__format/formatter.h>
#include <__format/formatter_output.h>
#include <__format/parser_std_format_spec.h>
#include <__format/write_escaped.h>
#include <string>
#include <string_view>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD

// [fs.path.fmtr]
//   path-format-spec:
//     fill-and-align_opt width_opt ?_opt g_opt
template <__fmt_char_type _CharT>
struct formatter<filesystem::path, _CharT> {
public:
  template <class _ParseContext>
  _LIBCPP_HIDE_FROM_ABI constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    typename _ParseContext::iterator __it = __parser_.__parse(__ctx, __format_spec::__fields_fill_align_width);
    typename _ParseContext::iterator __end = __ctx.end();

    if (__it != __end && *__it == _CharT('?')) {
      __parser_.__type_ = __format_spec::__type::__debug;
      ++__it;
    }
    if (__it != __end && *__it == _CharT('g')) {
      __generic_ = true;
      ++__it;
    }
    if (__it != __end && *__it != _CharT('}'))
      std::__throw_format_error("The format specifier for a path does not accept this option");

    return __it;
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator
  format(const filesystem::path& __path, _FormatContext& __ctx) const {
    basic_string<_CharT> __str =
        __generic_ ? __path.template generic_string<_CharT>() : __path.template string<_CharT>();

    if (__parser_.__type_ == __format_spec::__type::__debug)
      return __formatter::__format_escaped_string(
          basic_string_view<_CharT>(__str), __ctx.out(), __parser_.__get_parsed_std_specifications(__ctx));

    return __formatter::__write_string(
        basic_string_view<_CharT>(__str), __ctx.out(), __parser_.__get_parsed_std_specifications(__ctx));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_debug_format() { __parser_.__type_ = __format_spec::__type::__debug; }

  __format_spec::__parser<_CharT> __parser_{.__alignment_ = __format_spec::__alignment::__left};
  bool __generic_ = false;
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP___FILESYSTEM_PATH_FORMAT_H
