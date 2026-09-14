// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STACKTRACE_FORMATTER_H
#define _LIBCPP___STACKTRACE_FORMATTER_H

#include <__config>
#include <__format/concepts.h>
#include <__format/format_parse_context.h>
#include <__format/formatter.h>
#include <__format/formatter_output.h>
#include <__format/parser_std_format_spec.h>
#include <__stacktrace/basic_stacktrace.h>
#include <__stacktrace/stacktrace_entry.h>
#include <string>
#include <string_view>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 23

_LIBCPP_BEGIN_NAMESPACE_STD

// [stacktrace.entry.fmt]/[stacktrace.basic.fmt]: narrow character only, matching
// to_string(const stacktrace_entry&)/to_string(const basic_stacktrace<Allocator>&)
// and their operator<< overloads, which are themselves narrow-only.

template <>
struct formatter<stacktrace_entry, char> {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr typename basic_format_parse_context<char>::iterator
  parse(basic_format_parse_context<char>& __ctx) {
    return __parser_.__parse(__ctx, __format_spec::__fields_fill_align_width);
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator format(const stacktrace_entry& __f, _FormatContext& __ctx) const {
    string __str                                          = std::to_string(__f);
    __format_spec::__parsed_specifications<char> __specs = __parser_.__get_parsed_std_specifications(__ctx);
    return __formatter::__write_string_no_precision(string_view(__str), __ctx.out(), __specs);
  }

  __format_spec::__parser<char> __parser_{.__alignment_ = __format_spec::__alignment::__left};
};

template <class _Allocator>
struct formatter<basic_stacktrace<_Allocator>, char> {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr typename basic_format_parse_context<char>::iterator
  parse(basic_format_parse_context<char>& __ctx) {
    return __parser_.__parse(__ctx, __format_spec::__fields_fill_align_width);
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator
  format(const basic_stacktrace<_Allocator>& __st, _FormatContext& __ctx) const {
    string __str                                          = std::to_string(__st);
    __format_spec::__parsed_specifications<char> __specs = __parser_.__get_parsed_std_specifications(__ctx);
    return __formatter::__write_string_no_precision(string_view(__str), __ctx.out(), __specs);
  }

  __format_spec::__parser<char> __parser_{.__alignment_ = __format_spec::__alignment::__left};
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 23

#endif // _LIBCPP___STACKTRACE_FORMATTER_H
