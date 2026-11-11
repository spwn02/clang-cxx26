//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXCEPTION_TERMINATE_H
#define _LIBCPP___EXCEPTION_TERMINATE_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_UNVERSIONED_NAMESPACE_STD
#if defined(__cpp_diverging_expressions)
// P3549: a function that cannot return returns the bottom type. (Same ABI as
// void: the definition in the runtime library is still 'void terminate()'.) The Microsoft ABI mangles the return type
// into the name, so there the declaration names the symbol of 'void terminate()' explicitly.
#  if defined(_LIBCPP_ABI_MICROSOFT)
_LIBCPP_EXPORTED_FROM_ABI decltype(throw 0) terminate() _NOEXCEPT __asm__("?terminate@std@@YAXXZ");
#  else
_LIBCPP_EXPORTED_FROM_ABI decltype(throw 0) terminate() _NOEXCEPT;
#  endif
#else
[[__noreturn__]] _LIBCPP_EXPORTED_FROM_ABI void terminate() _NOEXCEPT;
#endif
_LIBCPP_END_UNVERSIONED_NAMESPACE_STD

#endif // _LIBCPP___EXCEPTION_TERMINATE_H
