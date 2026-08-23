// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___DEBUGGING_IS_DEBUGGER_PRESENT_H
#define _LIBCPP___DEBUGGING_IS_DEBUGGER_PRESENT_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

// [debugging.utility]p5: "This function is replaceable ([dcl.fct.def.replace])." A replaceable function must be
// declared out-of-line, with default visibility, and defined in exactly one translation unit (libcxx/src/
// debugging.cpp normally provides that definition) so that a user-supplied definition in another translation
// unit wins at link time -- unlike an ordinary _LIBCPP_HIDE_FROM_ABI inline function, whose body is baked into
// every call site and can't be overridden. This header is intentionally not gated on _LIBCPP_STD_VER: <debugging>
// includes it only under `#if _LIBCPP_STD_VER >= 26`, but libcxx/src/debugging.cpp -- which is compiled at a
// lower internal standard version -- includes it directly to pick up the declaration's visibility attribute.
_LIBCPP_BEGIN_UNVERSIONED_NAMESPACE_STD
[[nodiscard]] _LIBCPP_OVERRIDABLE_FUNC_VIS bool is_debugger_present() noexcept;
_LIBCPP_END_UNVERSIONED_NAMESPACE_STD

#endif // _LIBCPP___DEBUGGING_IS_DEBUGGER_PRESENT_H
