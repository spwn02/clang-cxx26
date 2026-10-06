// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___THREAD_SUPPORT_EXTERNAL_H
#define _LIBCPP___THREAD_SUPPORT_EXTERNAL_H

#include <__config>

#ifndef _LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER
#  pragma GCC system_header
#endif

#include <__external_threading>

_LIBCPP_BEGIN_NAMESPACE_STD

inline _LIBCPP_HIDE_FROM_ABI int
__libcpp_thread_create_with_stack_size(__libcpp_thread_t* __t, void* (*__func)(void*), void* __arg, size_t) {
  return __libcpp_thread_create(__t, __func, __arg); // the stack size is a hint (not supported by this thread API)
}

inline _LIBCPP_HIDE_FROM_ABI void __libcpp_thread_set_current_name(const char*, size_t) {} // a hint, not supported

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___THREAD_SUPPORT_EXTERNAL_H
