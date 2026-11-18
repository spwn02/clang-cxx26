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

// C++29-only clangd highlighting roles for expression-macro input tokens.
// These values are private transport ABI: clangd maps them explicitly to its
// semantic-token kinds and does not rely on enum ordinals.
enum class __semantic_token_kind : unsigned char {
  variable = 0,
  local_variable = 1,
  parameter = 2,
  function = 3,
  method = 4,
  static_method = 5,
  field = 6,
  static_field = 7,
  class_ = 8,
  interface = 9,
  enum_ = 10,
  enum_constant = 11,
  typedef_ = 12,
  type = 13,
  namespace_ = 14,
  template_parameter = 15,
  concept_ = 16,
  primitive = 17,
  macro = 18,
  modifier = 19,
  operator_ = 20,
  bracket = 21,
  label = 22,
};

enum class __semantic_token_modifier : unsigned {
  declaration = 1u << 0,
  definition = 1u << 1,
  deprecated = 1u << 2,
  deduced = 1u << 3,
  readonly = 1u << 4,
  static_ = 1u << 5,
  abstract = 1u << 6,
  virtual_ = 1u << 7,
  dependent_name = 1u << 8,
  default_library = 1u << 9,
  used_as_mutable_reference = 1u << 10,
  used_as_mutable_pointer = 1u << 11,
  constructor_or_destructor = 1u << 12,
  user_defined = 1u << 13,
  function_scope = 1u << 14,
  class_scope = 1u << 15,
  file_scope = 1u << 16,
  global_scope = 1u << 17,
};

enum class __completion_kind : unsigned char {
  text,
  method,
  property,
  class_,
  field,
  variable,
  keyword,
  value
};

consteval bool __is_completion(token __where) {
  return __metafunction(__detail::__metafn_is_completion, __where);
}

// Macro-authored completion candidates are considered only while completing
// the token sequence containing the cursor. The editor replaces the
// identifier prefix at the cursor, so for data-f^, insert_text should be
// "foo", not "data-foo". Completion is unavailable inside string literals
// and comments because lexing stops before the raw argument closes. Candidates
// are macro-authored and bounded: 128 per expansion, 512 per request, 64 KiB
// total text, 256-byte labels, 4096-byte insertions, 1024-byte details and
// 4096-byte documentation.
consteval bool __complete(
    token __where,
    string_view __label,
    string_view __insert_text,
    __completion_kind __kind    = __completion_kind::text,
    string_view __detail        = {},
    string_view __documentation = {}) {
  return __metafunction(
      __detail::__metafn_macro_complete, __where, __label.size(),
      __label.data(), __insert_text.size(), __insert_text.data(), __kind,
      __detail.size(), __detail.data(), __documentation.size(),
      __documentation.data());
}

// Roles are transported as private diagnostics and intercepted by clangd.
// Consequently clangd can report roles only for evaluations it observes while
// parsing the current translation unit; after a fatal error, diagnostic
// suppression prevents roles from being reported.
consteval bool __role(token __where, __semantic_token_kind __kind,
                      unsigned __modifiers = 0) {
  return __metafunction(__detail::__metafn_macro_token_role, __where, __kind,
                        __modifiers);
}

consteval bool __role(token __first, token __last,
                      __semantic_token_kind __kind,
                      unsigned __modifiers = 0) {
  return __metafunction(__detail::__metafn_macro_token_role, __first, __last,
                        __kind, __modifiers);
}

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
                        __message.size(), __message.data(),
                        static_cast<unsigned>(__fix.__operation), __fix.__first,
                        __fix.__last, __fix.__replacement.size(),
                        __fix.__replacement.data());
}
consteval bool __error(token __begin, token __end, string_view __message,
                       __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_error, __begin, __end,
                        __message.size(), __message.data(),
                        static_cast<unsigned>(__fix.__operation), __fix.__first,
                        __fix.__last, __fix.__replacement.size(),
                        __fix.__replacement.data());
}
consteval bool __warning(token __where, string_view __message,
                         __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_warning, __where,
                        __message.size(), __message.data(),
                        static_cast<unsigned>(__fix.__operation), __fix.__first,
                        __fix.__last, __fix.__replacement.size(),
                        __fix.__replacement.data());
}
consteval bool __warning(token __begin, token __end, string_view __message,
                         __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_warning, __begin,
                        __end, __message.size(), __message.data(),
                        static_cast<unsigned>(__fix.__operation), __fix.__first,
                        __fix.__last, __fix.__replacement.size(),
                        __fix.__replacement.data());
}
consteval bool __note(token __where, string_view __message,
                      __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_note, __where,
                        __message.size(), __message.data(),
                        static_cast<unsigned>(__fix.__operation), __fix.__first,
                        __fix.__last, __fix.__replacement.size(),
                        __fix.__replacement.data());
}
consteval bool __note(token __begin, token __end, string_view __message,
                      __diagnostic_fixit __fix = {}) {
  return __metafunction(__detail::__metafn_macro_located_note, __begin, __end,
                        __message.size(), __message.data(),
                        static_cast<unsigned>(__fix.__operation), __fix.__first,
                        __fix.__last, __fix.__replacement.size(),
                        __fix.__replacement.data());
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
