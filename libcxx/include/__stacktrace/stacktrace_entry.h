// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STACKTRACE_STACKTRACE_ENTRY_H
#define _LIBCPP___STACKTRACE_STACKTRACE_ENTRY_H

#include <__compare/ordering.h>
#include <__config>
#include <__functional/hash.h>
#include <__stacktrace/stacktrace_decls.h>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

// Resolution (symbol name / source file / source line) is expensive -- it may
// require opening and parsing an ELF module's DWARF debug info the first time
// a given program counter in that module is resolved. It is therefore done
// lazily, on demand, by __stacktrace_description/__stacktrace_source_file/
// __stacktrace_source_line (declared in __stacktrace/stacktrace_decls.h,
// defined out-of-line in stacktrace.cpp so the ELF/DWARF machinery they
// depend on stays out of this header and isn't duplicated into every
// translation unit that includes <stacktrace>), rather than eagerly when a
// stacktrace_entry is constructed. A process-wide cache inside
// stacktrace.cpp, keyed by the owning module, means repeated resolutions of
// addresses in the same module are cheap after the first.

// [stacktrace.entry], class stacktrace_entry
class stacktrace_entry {
public:
  using native_handle_type = uintptr_t;

  _LIBCPP_HIDE_FROM_ABI constexpr stacktrace_entry() noexcept : __pc_(0) {}
  _LIBCPP_HIDE_FROM_ABI constexpr stacktrace_entry(const stacktrace_entry&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI constexpr stacktrace_entry& operator=(const stacktrace_entry&) noexcept = default;
  _LIBCPP_HIDE_FROM_ABI ~stacktrace_entry()                                                     = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr native_handle_type native_handle() const noexcept { return __pc_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr explicit operator bool() const noexcept { return __pc_ != 0; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string description() const {
    return __pc_ == 0 ? string() : std::__stacktrace_description(__pc_);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string source_file() const {
    return __pc_ == 0 ? string() : std::__stacktrace_source_file(__pc_);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI uint_least32_t source_line() const {
    return __pc_ == 0 ? 0 : std::__stacktrace_source_line(__pc_);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const stacktrace_entry& __x, const stacktrace_entry& __y) noexcept {
    return __x.__pc_ == __y.__pc_;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr strong_ordering
  operator<=>(const stacktrace_entry& __x, const stacktrace_entry& __y) noexcept {
    return __x.__pc_ <=> __y.__pc_;
  }

private:
  native_handle_type __pc_;

  // Only basic_stacktrace::current() constructs a non-empty entry: it is the
  // sole holder of a captured raw program counter.
  _LIBCPP_HIDE_FROM_ABI constexpr explicit stacktrace_entry(native_handle_type __pc) noexcept : __pc_(__pc) {}

  template <class _Allocator>
  friend class basic_stacktrace;
};

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline string to_string(const stacktrace_entry& __f) {
  if (!__f)
    return string("0x0");
  string __desc = __f.description();
  string __file = __f.source_file();
  string __result;
  __result.reserve(__desc.size() + __file.size() + 16);
  __result += __desc.empty() ? string("<unknown>") : __desc;
  if (!__file.empty()) {
    __result += " at ";
    __result += __file;
    uint_least32_t __line = __f.source_line();
    if (__line != 0) {
      __result += ':';
      __result += std::to_string(__line);
    }
  }
  return __result;
}

template <class _CharT, class _Traits>
_LIBCPP_HIDE_FROM_ABI basic_ostream<_CharT, _Traits>&
operator<<(basic_ostream<_CharT, _Traits>& __os, const stacktrace_entry& __f) {
  // [stacktrace.entry.observers]: "the streamed result is the same as
  // (the corresponding call to) std::to_string(f)".
  return __os << std::to_string(__f);
}

template <>
struct hash<stacktrace_entry> {
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const stacktrace_entry& __f) const noexcept {
    return hash<stacktrace_entry::native_handle_type>()(__f.native_handle());
  }
};

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___STACKTRACE_STACKTRACE_ENTRY_H
