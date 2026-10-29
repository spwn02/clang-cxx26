// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <exception>
#include <stdio.h>
#include <stdlib.h>

_LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrCreate(void*);
_LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrDestroy(void*);
_LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrCopy(void*, const void*);
_LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrAssign(void*, const void*);
_LIBCPP_CRT_FUNC bool __cdecl __ExceptionPtrCompare(const void*, const void*);
_LIBCPP_CRT_FUNC bool __cdecl __ExceptionPtrToBool(const void*);
_LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrSwap(void*, void*);
_LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrCurrentException(void*);
[[noreturn]] _LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrRethrow(const void*);
_LIBCPP_CRT_FUNC void __cdecl __ExceptionPtrCopyException(void*, const void*, const void*);

namespace std {

// The members of exception_ptr, operator==, swap, current_exception and rethrow_exception are inline in <exception>
// (they call the __exception_ptr_*_runtime functions of exception.cpp), so that every translation unit has the one
// definition of them whatever its language mode.

exception_ptr __copy_exception_ptr(void* __except, const void* __ptr) {
  exception_ptr __ret = nullptr;
  if (__ptr)
    __ExceptionPtrCopyException(&__ret, __except, __ptr);
  return __ret;
}

nested_exception::nested_exception() noexcept : __ptr_(current_exception()) {}

nested_exception::~nested_exception() noexcept {}

[[noreturn]] void nested_exception::rethrow_nested() const {
  if (__ptr_ == nullptr)
    terminate();
  rethrow_exception(__ptr_);
}

} // namespace std
