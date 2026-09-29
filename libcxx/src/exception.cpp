//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#define _LIBCPP_ENABLE_CXX20_REMOVED_UNCAUGHT_EXCEPTION
#define _LIBCPP_DISABLE_DEPRECATION_WARNINGS

#include <__config>

#if defined(_LIBCPP_ABI_MICROSOFT)
#  include "support/runtime/exception_msvc.ipp"
#  include "support/runtime/exception_pointer_msvc.ipp"
#elif defined(LIBCXX_BUILDING_LIBCXXABI)
#  include "support/runtime/exception_libcxxabi.ipp"
#  include "support/runtime/exception_pointer_cxxabi.ipp"
#elif defined(LIBCXXRT)
#  include "support/runtime/exception_libcxxrt.ipp"
#  include "support/runtime/exception_pointer_cxxabi.ipp"
#elif defined(__GLIBCXX__)
#  include "support/runtime/exception_glibcxx.ipp"
#  include "support/runtime/exception_pointer_glibcxx.ipp"
#else
#  include "include/atomic_support.h"
#  include "support/runtime/exception_fallback.ipp"
#  include "support/runtime/exception_pointer_unimplemented.ipp"
#endif

// Runtime fallbacks for the C++26 constexpr-facing exception_ptr wrappers.
// These deliberately delegate to the established ABI entry points above.
#include <exception>
#include <new>
_LIBCPP_BEGIN_UNVERSIONED_NAMESPACE_STD
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_copy_runtime(
    exception_ptr* __to, const exception_ptr& __from) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  __ExceptionPtrCopy(__to, &__from);
#else
  *__to = __from;
#endif
}
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_assign_runtime(
    exception_ptr* __to, const exception_ptr& __from) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  __ExceptionPtrAssign(__to, &__from);
#else
  *__to = __from;
#endif
}
_LIBCPP_EXPORTED_FROM_ABI void
__exception_ptr_destroy_runtime(exception_ptr* __ptr) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  __ExceptionPtrDestroy(__ptr);
#else
  exception_ptr __tmp;
  swap(__tmp, *__ptr);
#endif
}
_LIBCPP_EXPORTED_FROM_ABI void
__exception_ptr_default_runtime(exception_ptr* __ptr) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  __ExceptionPtrCreate(__ptr);
#else
  ::new (static_cast<void*>(__ptr)) exception_ptr();
#endif
}
_LIBCPP_EXPORTED_FROM_ABI void
__exception_ptr_assign_null_runtime(exception_ptr* __ptr) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  exception_ptr __null;
  __ExceptionPtrCreate(&__null);
  __ExceptionPtrAssign(__ptr, &__null);
  __ExceptionPtrDestroy(&__null);
#else
  *__ptr = nullptr;
#endif
}
_LIBCPP_EXPORTED_FROM_ABI bool
__exception_ptr_to_bool_runtime(const exception_ptr* __ptr) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  return __ExceptionPtrToBool(__ptr);
#else
  return static_cast<bool>(*__ptr);
#endif
}
_LIBCPP_EXPORTED_FROM_ABI bool __exception_ptr_equal_runtime(
    const exception_ptr* __lhs, const exception_ptr* __rhs) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  return __ExceptionPtrCompare(__lhs, __rhs);
#else
  return *__lhs == *__rhs;
#endif
}
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_swap_runtime(
    exception_ptr* __lhs, exception_ptr* __rhs) noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  __ExceptionPtrSwap(__lhs, __rhs);
#else
  swap(*__lhs, *__rhs);
#endif
}
_LIBCPP_EXPORTED_FROM_ABI exception_ptr __current_exception_runtime() noexcept {
#if defined(_LIBCPP_ABI_MICROSOFT)
  exception_ptr __result;
  __ExceptionPtrCurrentException(&__result);
  return __result;
#else
  return std::current_exception();
#endif
}
[[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void
__rethrow_exception_runtime(exception_ptr __ptr) {
#if defined(_LIBCPP_ABI_MICROSOFT)
  __ExceptionPtrRethrow(&__ptr);
#else
  std::rethrow_exception(std::move(__ptr));
#endif
}
_LIBCPP_END_UNVERSIONED_NAMESPACE_STD
