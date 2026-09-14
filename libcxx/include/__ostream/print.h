//===---------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//

#ifndef _LIBCPP___OSTREAM_PRINT_H
#define _LIBCPP___OSTREAM_PRINT_H

#include <__config>

#if _LIBCPP_HAS_LOCALIZATION

#  include <__fwd/ostream.h>
#  include <__iterator/ostreambuf_iterator.h>
#  include <__ostream/basic_ostream.h>
#  include <format>
#  include <ios>
#  include <print>
#  include <streambuf>

#  if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#    pragma GCC system_header
#  endif

_LIBCPP_BEGIN_NAMESPACE_STD

#  if _LIBCPP_STD_VER >= 23

template <class = void> // TODO PRINT template or availability markup fires too eagerly (http://llvm.org/PR61563).
_LIBCPP_HIDE_FROM_ABI inline void
__vprint_nonunicode(ostream& __os, string_view __fmt, format_args __args, bool __write_nl) {
  // [ostream.formatted.print]/3
  // Effects: Behaves as a formatted output function
  // ([ostream.formatted.reqmts]) of os, except that:
  // - failure to generate output is reported as specified below, and
  // - any exception thrown by the call to vformat is propagated without regard
  //   to the value of os.exceptions() and without turning on ios_base::badbit
  //   in the error state of os.
  // After constructing a sentry object, the function initializes an automatic
  // variable via
  //   string out = vformat(os.getloc(), fmt, args);

  ostream::sentry __s(__os);
  if (__s) {
    auto __flush = [&__os](const char* __data, size_t __size) {
      if (auto __rdbuf = __os.rdbuf();
          !__rdbuf || __rdbuf->sputn(__data, static_cast<streamsize>(__size)) != static_cast<streamsize>(__size))
        __os.setstate(ios_base::badbit | ios_base::failbit);
    };
    __format::__print_buffer<char, decltype(__flush)> __buffer{std::move(__flush)};

#    if _LIBCPP_HAS_EXCEPTIONS
    std::vformat_to(__buffer.__make_output_iterator(), __os.getloc(), __fmt, __args);
    try {
#    else
    std::vformat_to(__buffer.__make_output_iterator(), __os.getloc(), __fmt, __args);
#    endif // _LIBCPP_HAS_EXCEPTIONS
    if (__write_nl)
      __buffer.push_back('\n');
    __buffer.__flush();

#    if _LIBCPP_HAS_EXCEPTIONS
    } catch (...) {
      __os.__set_badbit_and_consider_rethrow();
    }
#    endif // _LIBCPP_HAS_EXCEPTIONS
  }
}

template <class = void>
_LIBCPP_HIDE_FROM_ABI inline void
__vprint_nonunicode_buffered(ostream& __os, string_view __fmt, format_args __args, bool __write_nl) {
  ostream::sentry __s(__os);
  if (__s) {
    string __out = std::vformat(__os.getloc(), __fmt, __args);
    if (__write_nl)
      __out.push_back('\n');

#    if _LIBCPP_HAS_EXCEPTIONS
    try {
#    endif
      if (auto __rdbuf = __os.rdbuf();
          !__rdbuf || __rdbuf->sputn(__out.data(), static_cast<streamsize>(__out.size())) != static_cast<streamsize>(__out.size()))
        __os.setstate(ios_base::badbit | ios_base::failbit);
#    if _LIBCPP_HAS_EXCEPTIONS
    } catch (...) {
      __os.__set_badbit_and_consider_rethrow();
    }
#    endif
  }
}

template <class = void> // TODO PRINT template or availability markup fires too eagerly (http://llvm.org/PR61563).
_LIBCPP_HIDE_FROM_ABI inline void vprint_nonunicode(ostream& __os, string_view __fmt, format_args __args) {
  std::__vprint_nonunicode_buffered(__os, __fmt, __args, false);
}

// Returns the FILE* associated with the __os.
// Returns a nullptr when no FILE* is associated with __os.
// This function is in the dylib since the type of the buffer associated
// with std::cout, std::cerr, and std::clog is only known in the dylib.
//
// This function implements part of the implementation-defined behavior
// of [ostream.formatted.print]/3
//   If the function is vprint_unicode and os is a stream that refers to
//   a terminal capable of displaying Unicode which is determined in an
//   implementation-defined manner, writes out to the terminal using the
//   native Unicode API;
// Whether the returned FILE* is "a terminal capable of displaying Unicode"
// is determined in the same way as the print(FILE*, ...) overloads.
_LIBCPP_EXPORTED_FROM_ABI FILE* __get_ostream_file(ostream& __os);

#    if _LIBCPP_HAS_UNICODE
template <class = void> // TODO PRINT template or availability markup fires too eagerly (http://llvm.org/PR61563).
_LIBCPP_HIDE_FROM_ABI void __vprint_unicode(ostream& __os, string_view __fmt, format_args __args, bool __write_nl) {
#      if _LIBCPP_AVAILABILITY_HAS_PRINT == 0
  return std::__vprint_nonunicode(__os, __fmt, __args, __write_nl);
#      else
  FILE* __file = std::__get_ostream_file(__os);
  if (!__file || !__print::__is_terminal(__file))
    return std::__vprint_nonunicode(__os, __fmt, __args, __write_nl);

  // [ostream.formatted.print]/3
  //    If the function is vprint_unicode and os is a stream that refers to a
  //    terminal capable of displaying Unicode which is determined in an
  //    implementation-defined manner, writes out to the terminal using the
  //    native Unicode API; if out contains invalid code units, the behavior is
  //    undefined and implementations are encouraged to diagnose it. If the
  //    native Unicode API is used, the function flushes os before writing out.
  //
  // This is the path for the native API, start with flushing.
  __os.flush();

#        if _LIBCPP_HAS_EXCEPTIONS
  try {
#        endif // _LIBCPP_HAS_EXCEPTIONS
    ostream::sentry __s(__os);
    if (__s) {
#        ifndef _LIBCPP_WIN32API
      __print::__vprint_unicode_posix(__file, __fmt, __args, __write_nl, true);
#        elif _LIBCPP_HAS_WIDE_CHARACTERS
    __print::__vprint_unicode_windows(__file, __fmt, __args, __write_nl, true);
#        else
#          error "Windows builds with wchar_t disabled are not supported."
#        endif
    }

#        if _LIBCPP_HAS_EXCEPTIONS
  } catch (...) {
    __os.__set_badbit_and_consider_rethrow();
  }
#        endif // _LIBCPP_HAS_EXCEPTIONS
#      endif   // _LIBCPP_AVAILABILITY_HAS_PRINT
}

template <class = void>
_LIBCPP_HIDE_FROM_ABI inline void
__vprint_unicode_buffered(ostream&, string_view, format_args, bool);

template <class = void> // TODO PRINT template or availability markup fires too eagerly (http://llvm.org/PR61563).
_LIBCPP_HIDE_FROM_ABI inline void vprint_unicode(ostream& __os, string_view __fmt, format_args __args) {
  std::__vprint_unicode_buffered(__os, __fmt, __args, false);
}

template <class>
_LIBCPP_HIDE_FROM_ABI inline void
__vprint_unicode_buffered(ostream& __os, string_view __fmt, format_args __args, bool __write_nl) {
  string __out = std::vformat(__os.getloc(), __fmt, __args);
  if (__write_nl)
    __out.push_back('\n');
  std::__vprint_unicode(__os, "{}", std::make_format_args(__out), false);
}
#    endif // _LIBCPP_HAS_UNICODE

template <class... _Args>
_LIBCPP_HIDE_FROM_ABI void print(ostream& __os, format_string<_Args...> __fmt, _Args&&... __args) {
#    if _LIBCPP_HAS_UNICODE
  if constexpr (__print::__use_unicode_execution_charset)
    if constexpr ((enable_nonlocking_formatter_optimization<remove_cvref_t<_Args>> && ...))
      std::__vprint_unicode(__os, __fmt.get(), std::make_format_args(__args...), false);
    else
      std::__vprint_unicode_buffered(__os, __fmt.get(), std::make_format_args(__args...), false);
  else
    if constexpr ((enable_nonlocking_formatter_optimization<remove_cvref_t<_Args>> && ...))
      std::__vprint_nonunicode(__os, __fmt.get(), std::make_format_args(__args...), false);
    else
      std::__vprint_nonunicode_buffered(__os, __fmt.get(), std::make_format_args(__args...), false);
#    else  // _LIBCPP_HAS_UNICODE
  if constexpr ((enable_nonlocking_formatter_optimization<remove_cvref_t<_Args>> && ...))
    std::__vprint_nonunicode(__os, __fmt.get(), std::make_format_args(__args...), false);
  else
    std::__vprint_nonunicode_buffered(__os, __fmt.get(), std::make_format_args(__args...), false);
#    endif // _LIBCPP_HAS_UNICODE
}

template <class... _Args>
_LIBCPP_HIDE_FROM_ABI void println(ostream& __os, format_string<_Args...> __fmt, _Args&&... __args) {
#    if _LIBCPP_HAS_UNICODE
  // Note the wording in the Standard is inefficient. The output of
  // std::format is a std::string which is then copied. This solution
  // just appends a newline at the end of the output.
  if constexpr (__print::__use_unicode_execution_charset)
    if constexpr ((enable_nonlocking_formatter_optimization<remove_cvref_t<_Args>> && ...))
      std::__vprint_unicode(__os, __fmt.get(), std::make_format_args(__args...), true);
    else
      std::__vprint_unicode_buffered(__os, __fmt.get(), std::make_format_args(__args...), true);
  else
    if constexpr ((enable_nonlocking_formatter_optimization<remove_cvref_t<_Args>> && ...))
      std::__vprint_nonunicode(__os, __fmt.get(), std::make_format_args(__args...), true);
    else
      std::__vprint_nonunicode_buffered(__os, __fmt.get(), std::make_format_args(__args...), true);
#    else  // _LIBCPP_HAS_UNICODE
  if constexpr ((enable_nonlocking_formatter_optimization<remove_cvref_t<_Args>> && ...))
    std::__vprint_nonunicode(__os, __fmt.get(), std::make_format_args(__args...), true);
  else
    std::__vprint_nonunicode_buffered(__os, __fmt.get(), std::make_format_args(__args...), true);
#    endif // _LIBCPP_HAS_UNICODE
}

template <class = void> // TODO PRINT template or availability markup fires too eagerly (http://llvm.org/PR61563).
_LIBCPP_HIDE_FROM_ABI inline void println(ostream& __os) {
  std::print(__os, "\n");
}

#  endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_LOCALIZATION

#endif // _LIBCPP___OSTREAM_PRINT_H
