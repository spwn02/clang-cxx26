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

// Entry points the non-legacy, header-inline <stdexcept> constructors (used
// by ordinary C++26 client TUs, e.g. libcxx/src/string.cpp's format_error,
// system_error.cpp, future.cpp, regex.cpp, ...) call into. Defined exactly
// once, only here -- not in libcxx/src/include/refstring.h, which is also
// included by libcxxabi/src/stdlib_stdexcept.cpp and would duplicate-define
// these ordinary (non-inline) symbols when both TUs' objects end up
// statically linked into the same archive (e.g. compiler-rt's hermetic
// fuzzer runtime). libcxxabi's copy never calls these itself.
_LIBCPP_BEGIN_NAMESPACE_STD

_LIBCPP_EXPORTED_FROM_ABI void __libcpp_refstring_init(__libcpp_refstring& __s, const char* __msg) {
  size_t __len     = strlen(__msg);
  auto* __rep      = static_cast<__refstring_imp::_Rep_base*>(::operator new(sizeof(__refstring_imp::_Rep_base) + __len + 1));
  __rep->len       = __len;
  __rep->cap       = __len;
  __rep->count     = 0;
  char* __data     = __refstring_imp::data_from_rep(__rep);
  memcpy(__data, __msg, __len + 1);
  __s.__imp_ = __data;
}

_LIBCPP_EXPORTED_FROM_ABI void
__libcpp_refstring_copy(__libcpp_refstring& __dst, const __libcpp_refstring& __src) noexcept {
  __dst.__imp_ = __src.__imp_;
  if (__dst.__uses_refcount())
    __libcpp_atomic_add(&__refstring_imp::rep_from_data(__dst.__imp_)->count, 1);
}

_LIBCPP_EXPORTED_FROM_ABI void
__libcpp_refstring_assign(__libcpp_refstring& __dst, const __libcpp_refstring& __src) noexcept {
  bool __adjust_old_count           = __dst.__uses_refcount();
  __refstring_imp::_Rep_base* __old_rep = __refstring_imp::rep_from_data(__dst.__imp_);
  __dst.__imp_                      = __src.__imp_;
  if (__dst.__uses_refcount())
    __libcpp_atomic_add(&__refstring_imp::rep_from_data(__dst.__imp_)->count, 1);
  if (__adjust_old_count && __libcpp_atomic_add(&__old_rep->count, __refstring_imp::count_t(-1)) < 0)
    ::operator delete(__old_rep);
}

_LIBCPP_EXPORTED_FROM_ABI void __libcpp_refstring_destroy(__libcpp_refstring& __s) noexcept {
  if (__s.__uses_refcount()) {
    __refstring_imp::_Rep_base* __rep = __refstring_imp::rep_from_data(__s.__imp_);
    if (__libcpp_atomic_add(&__rep->count, __refstring_imp::count_t(-1)) < 0)
      ::operator delete(__rep);
  }
}

void __throw_runtime_error(const char* msg) {
#if _LIBCPP_HAS_EXCEPTIONS
  throw runtime_error(msg);
#else
  _LIBCPP_VERBOSE_ABORT("runtime_error was thrown in -fno-exceptions mode with message \"%s\"", msg);
#endif
}

_LIBCPP_END_NAMESPACE_STD
