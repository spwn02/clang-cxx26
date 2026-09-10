// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STACKTRACE_STACKTRACE_DECLS_H
#define _LIBCPP___STACKTRACE_STACKTRACE_DECLS_H

#include <__config>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

// This header is intentionally not gated on _LIBCPP_STD_VER (compare
// __debugging/is_debugger_present.h, which documents the same trick): the
// public <stacktrace> pieces (__stacktrace/stacktrace_entry.h,
// __stacktrace/basic_stacktrace.h) include it only under
// `#if _LIBCPP_STD_VER >= 26`, but libcxx/src/stacktrace.cpp -- which is
// compiled at a lower internal standard version, like every other file in
// libcxx/src -- includes it directly and unconditionally, to pick up these
// declarations' _LIBCPP_EXPORTED_FROM_ABI (default) visibility. Without a
// visible prior declaration carrying that attribute, stacktrace.cpp's own
// definitions would silently fall back to -fvisibility=hidden's default and
// never appear in libc++.so's dynamic symbol table, breaking every program
// that links against <stacktrace> at link time.

_LIBCPP_BEGIN_NAMESPACE_STD

// Raw capture: see __stacktrace/basic_stacktrace.h for the full contract.
_LIBCPP_EXPORTED_FROM_ABI vector<uintptr_t> __stacktrace_capture(size_t __skip, size_t __max_depth) noexcept;

// Lazy resolution: see __stacktrace/stacktrace_entry.h for the full contract.
_LIBCPP_EXPORTED_FROM_ABI string __stacktrace_description(uintptr_t __pc);
_LIBCPP_EXPORTED_FROM_ABI string __stacktrace_source_file(uintptr_t __pc);
_LIBCPP_EXPORTED_FROM_ABI uint_least32_t __stacktrace_source_line(uintptr_t __pc);

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___STACKTRACE_STACKTRACE_DECLS_H
