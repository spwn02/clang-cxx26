// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___SYSTEM_ERROR_FORMATTER_H
#define _LIBCPP___SYSTEM_ERROR_FORMATTER_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__format/concepts.h>
#include <__format/format_error.h>
#include <__format/format_parse_context.h>
#include <__format/formatter.h>
#include <__format/formatter_output.h>
#include <__format/parser_std_format_spec.h>
#include <__format/unicode.h>
#include <__format/write_escaped.h>
#include <__system_error/error_category.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_same.h>
#include <string>
#include <string_view>

#  if _LIBCPP_HAS_WIDE_CHARACTERS
#    include <cwchar>
#  endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 29

_LIBCPP_BEGIN_NAMESPACE_STD

namespace __error_code_format {

// Appends the UTF-8 encoding of the code point __cp.
_LIBCPP_HIDE_FROM_ABI inline void __append_utf8(string& __res, char32_t __cp) {
  if (__cp < 0x80) {
    __res += static_cast<char>(__cp);
  } else if (__cp < 0x800) {
    __res += static_cast<char>(0xC0 | (__cp >> 6));
    __res += static_cast<char>(0x80 | (__cp & 0x3F));
  } else if (__cp < 0x10000) {
    __res += static_cast<char>(0xE0 | (__cp >> 12));
    __res += static_cast<char>(0x80 | ((__cp >> 6) & 0x3F));
    __res += static_cast<char>(0x80 | (__cp & 0x3F));
  } else {
    __res += static_cast<char>(0xF0 | (__cp >> 18));
    __res += static_cast<char>(0x80 | ((__cp >> 12) & 0x3F));
    __res += static_cast<char>(0x80 | ((__cp >> 6) & 0x3F));
    __res += static_cast<char>(0x80 | (__cp & 0x3F));
  }
}

#  if _LIBCPP_HAS_WIDE_CHARACTERS

// Is the multibyte encoding of the current C locale UTF-8?
_LIBCPP_HIDE_FROM_ABI inline bool __execution_encoding_is_utf8() {
  mbstate_t __state = mbstate_t();
  wchar_t __wc      = 0;
  return std::mbrtowc(&__wc, "\xC3\xA9", 2, &__state) == 2 && static_cast<char32_t>(__wc) == U'é';
}

// Converts one multibyte character of the C locale to a wide character. Returns the number of bytes consumed, 0 if
// the sequence is not a valid character of the current locale (the caller substitutes U+FFFD and skips one byte).
_LIBCPP_HIDE_FROM_ABI inline size_t __mb_to_wide(wchar_t& __wc, const char* __s, size_t __n) {
  mbstate_t __state = mbstate_t();
  size_t __r        = std::mbrtowc(&__wc, __s, __n, &__state);
  if (__r == static_cast<size_t>(-1) || __r == static_cast<size_t>(-2))
    return 0;
  return __r == 0 ? 1 : __r; // a NUL character consumes one byte
}

#  endif // _LIBCPP_HAS_WIDE_CHARACTERS

#  if _LIBCPP_HAS_UNICODE

// [syserr.fmt]: the message is transcoded from the execution character set to UTF-8, with maximal subparts of
// ill-formed subsequences substituted with U+FFFD.
_LIBCPP_HIDE_FROM_ABI inline string __message_to_utf8(const string& __msg) {
  string __res;
  __res.reserve(__msg.size());
#    if _LIBCPP_HAS_WIDE_CHARACTERS
  if (!__execution_encoding_is_utf8()) {
    for (size_t __i = 0; __i < __msg.size();) {
      wchar_t __wc;
      size_t __n = __mb_to_wide(__wc, __msg.data() + __i, __msg.size() - __i);
      if (__n == 0) {
        __append_utf8(__res, __unicode::__replacement_character);
        ++__i;
      } else {
        __append_utf8(__res, static_cast<char32_t>(__wc));
        __i += __n;
      }
    }
    return __res;
  }
#    endif
  // Unicode Standard, Chapter 3.9, "U+FFFD Substitution of Maximal Subparts": every maximal subpart of an ill-formed
  // sequence is replaced by one U+FFFD. (__unicode::__code_point_view replaces every offending code unit, which is
  // what the escaped output of [format.string.escaped] wants, not what is wanted here.)
  size_t __i = 0;
  while (__i < __msg.size()) {
    unsigned char __lead = static_cast<unsigned char>(__msg[__i]);
    if (__lead < 0x80) {
      __res += __msg[__i++];
      continue;
    }
    size_t __length;
    unsigned char __lo = 0x80, __hi = 0xBF; // the range of the first continuation byte
    if (__lead >= 0xC2 && __lead <= 0xDF) {
      __length = 2;
    } else if (__lead >= 0xE0 && __lead <= 0xEF) {
      __length = 3;
      if (__lead == 0xE0)
        __lo = 0xA0;
      else if (__lead == 0xED)
        __hi = 0x9F;
    } else if (__lead >= 0xF0 && __lead <= 0xF4) {
      __length = 4;
      if (__lead == 0xF0)
        __lo = 0x90;
      else if (__lead == 0xF4)
        __hi = 0x8F;
    } else {
      __append_utf8(__res, __unicode::__replacement_character);
      ++__i;
      continue;
    }
    size_t __j = __i + 1;
    bool __ok  = true;
    for (; __j < __i + __length; ++__j) {
      if (__j >= __msg.size()) {
        __ok = false;
        break;
      }
      unsigned char __c = static_cast<unsigned char>(__msg[__j]);
      if (__c < __lo || __c > __hi) {
        __ok = false;
        break;
      }
      __lo = 0x80;
      __hi = 0xBF;
    }
    if (__ok) {
      __res.append(__msg, __i, __length);
      __i += __length;
    } else {
      __append_utf8(__res, __unicode::__replacement_character);
      __i = __j; // resume at the byte that broke the sequence
    }
  }
  return __res;
}

#  endif // _LIBCPP_HAS_UNICODE

template <class _CharT>
_LIBCPP_HIDE_FROM_ABI basic_string<_CharT> __message(const error_code& __ec) {
  string __msg = __ec.message();
  if constexpr (is_same_v<_CharT, char>) {
#  if _LIBCPP_HAS_UNICODE
    return __message_to_utf8(__msg);
#  else
    return __msg;
#  endif
  } else {
    // An implementation-defined encoding: the wide characters of the C locale.
    basic_string<_CharT> __res;
#  if _LIBCPP_HAS_WIDE_CHARACTERS
    for (size_t __i = 0; __i < __msg.size();) {
      wchar_t __wc;
      size_t __n = __mb_to_wide(__wc, __msg.data() + __i, __msg.size() - __i);
      if (__n == 0) {
        __res += static_cast<_CharT>(0xFFFD);
        ++__i;
      } else {
        __res += static_cast<_CharT>(__wc);
        __i += __n;
      }
    }
#  endif
    return __res;
  }
}

// The category name is in the ordinary literal encoding.
template <class _CharT>
_LIBCPP_HIDE_FROM_ABI basic_string<_CharT> __default_text(const error_code& __ec) {
  basic_string<_CharT> __res;
  for (const char* __p = __ec.category().name(); *__p != '\0'; ++__p)
    __res += static_cast<_CharT>(static_cast<unsigned char>(*__p));
  __res += static_cast<_CharT>(':');
  for (char __c : std::to_string(__ec.value()))
    __res += static_cast<_CharT>(__c);
  return __res;
}

} // namespace __error_code_format

// [syserr.fmt]
//   error-code-format-spec:
//     fill-and-align_opt width_opt ?_opt s_opt
template <__fmt_char_type _CharT>
struct formatter<error_code, _CharT> {
public:
  template <class _ParseContext>
  _LIBCPP_HIDE_FROM_ABI constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    typename _ParseContext::iterator __it  = __parser_.__parse(__ctx, __format_spec::__fields_fill_align_width);
    typename _ParseContext::iterator __end = __ctx.end();

    if (__it != __end && *__it == _CharT('?')) {
      __parser_.__type_ = __format_spec::__type::__debug;
      ++__it;
    }
    if (__it != __end && *__it == _CharT('s')) {
      __message_ = true;
      ++__it;
    }
    if (__it != __end && *__it != _CharT('}'))
      std::__throw_format_error("The format specifier for an error_code does not accept this option");

    return __it;
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator format(const error_code& __ec, _FormatContext& __ctx) const {
    basic_string<_CharT> __str = __message_ ? __error_code_format::__message<_CharT>(__ec)
                                            : __error_code_format::__default_text<_CharT>(__ec);

    if (__parser_.__type_ == __format_spec::__type::__debug)
      return __formatter::__format_escaped_string(
          basic_string_view<_CharT>(__str), __ctx.out(), __parser_.__get_parsed_std_specifications(__ctx));

    return __formatter::__write_string(
        basic_string_view<_CharT>(__str), __ctx.out(), __parser_.__get_parsed_std_specifications(__ctx));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_debug_format() { __parser_.__type_ = __format_spec::__type::__debug; }

  __format_spec::__parser<_CharT> __parser_{.__alignment_ = __format_spec::__alignment::__left};
  bool __message_ = false;
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 29

#endif // _LIBCPP___SYSTEM_ERROR_FORMATTER_H
