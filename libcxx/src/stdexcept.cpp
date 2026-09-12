//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <__verbose_abort>
#include <new>
// Keep the historic out-of-line definitions in this TU.  C++26 clients see
// constexpr inline definitions in <stdexcept>; these symbols are retained for
// clients compiled against older headers.
#define _LIBCPP_STDEXCEPT_DEFINE_LEGACY_FUNCTIONS
#include <stdexcept>
#include <string>

#ifdef _LIBCPP_ABI_VCRUNTIME
#  include "support/runtime/stdexcept_vcruntime.ipp"
#else
#  include "support/runtime/stdexcept_default.ipp"
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

void __throw_runtime_error(const char* msg) {
#if _LIBCPP_HAS_EXCEPTIONS
  throw runtime_error(msg);
#else
  _LIBCPP_VERBOSE_ABORT("runtime_error was thrown in -fno-exceptions mode with message \"%s\"", msg);
#endif
}

_LIBCPP_END_NAMESPACE_STD
