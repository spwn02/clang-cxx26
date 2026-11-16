//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___META_TOKEN_LOCATIONS_H
#define _LIBCPP___META_TOKEN_LOCATIONS_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 29

#  include <meta>
#  include <source_location>
#  include <string_view>

_LIBCPP_BEGIN_NAMESPACE_REFLECTION_V2

namespace __x {

// A fix-it suggestion attached to a located expression-macro diagnostic.
// Replacement and removal ranges include both endpoint tokens.
struct __diagnostic_fixit {
  enum class __kind : unsigned char { replace, insert_after, remove };
  __kind __operation;
  token __first;
  token __last; // ignored for insert_after
  string_view __replacement; // empty except replace/insert_after
};

consteval auto __replace(token __begin, token __end, string_view __text)
    -> __diagnostic_fixit {
  return {__diagnostic_fixit::__kind::replace, __begin, __end, __text};
}

consteval auto __insert_after(token __where, string_view __text)
    -> __diagnostic_fixit {
  return {__diagnostic_fixit::__kind::insert_after, __where, token{}, __text};
}

consteval auto __remove(token __begin, token __end) -> __diagnostic_fixit {
  return {__diagnostic_fixit::__kind::remove, __begin, __end, {}};
}

consteval bool __error(token __where, string_view __message,
                       __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_error, __where,
                        __message, __fix);
}
consteval bool __error(token __begin, token __end, string_view __message,
                       __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_error, __begin, __end,
                        __message, __fix);
}
consteval bool __warning(token __where, string_view __message,
                         __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_warning, __where,
                        __message, __fix);
}
consteval bool __warning(token __begin, token __end, string_view __message,
                         __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_warning, __begin,
                        __end, __message, __fix);
}
consteval bool __note(token __where, string_view __message,
                      __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_note, __where,
                        __message, __fix);
}
consteval bool __note(token __begin, token __end, string_view __message,
                      __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_note, __begin, __end,
                        __message, __fix);
}

// Fork extension: query the source coordinates carried by an individual
// P4380 token. The reserved __x namespace and __-prefixed names keep this
// extension out of the standard API. The end location is exclusive. Annotation
// and genuinely zero-length tokens end where they begin. Non-literal token
// lengths are zero after PCH/module serialization, so recoverable spellings
// are used to compute their ends. A missing source location (as with
// synthesized tokens) produces a default source_location.
consteval auto __token_begin(token __tok) -> source_location {
  auto __ptr = __metafunction(__detail::__metafn_token_begin_location, __tok);
  return source_location::current(__ptr);
}

consteval auto __token_end(token __tok) -> source_location {
  auto __ptr = __metafunction(__detail::__metafn_token_end_location, __tok);
  return source_location::current(__ptr);
}

// Convenience queries for a token sequence or contiguous token subrange.
// The empty range has no location and returns a default source_location.
template <class _Range>
  requires ranges::contiguous_range<_Range> && ranges::sized_range<_Range> &&
           same_as<remove_cv_t<ranges::range_value_t<_Range>>, token>
consteval auto __token_sequence_begin(_Range const& __tokens)
    -> source_location {
  return __tokens.empty() ? source_location{} : __token_begin(__tokens.front());
}

template <class _Range>
  requires ranges::contiguous_range<_Range> && ranges::sized_range<_Range> &&
           same_as<remove_cv_t<ranges::range_value_t<_Range>>, token>
consteval auto __token_sequence_end(_Range const& __tokens)
    -> source_location {
  return __tokens.empty() ? source_location{} : __token_end(__tokens.back());
}

} // namespace __x

_LIBCPP_END_NAMESPACE_REFLECTION_V2

#endif // _LIBCPP_STD_VER >= 29

#endif // _LIBCPP___META_TOKEN_LOCATIONS_H
