//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FUNCTIONAL_MOVE_ONLY_FUNCTION_H
#define _LIBCPP___FUNCTIONAL_MOVE_ONLY_FUNCTION_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 23 && !defined(_LIBCPP_COMPILER_GCC)

// Generate all cv/ref-qualified specializations. The noexcept variants are
// represented by the boolean noexcept template argument in each specialization.
#  define _LIBCPP_IN_MOVE_ONLY_FUNCTION_H

#  include <__functional/move_only_function_impl.h>

#  define _LIBCPP_MOVE_ONLY_FUNCTION_REF &
#  include <__functional/move_only_function_impl.h>

#  define _LIBCPP_MOVE_ONLY_FUNCTION_REF &&
#  include <__functional/move_only_function_impl.h>

#  define _LIBCPP_MOVE_ONLY_FUNCTION_CV const
#  include <__functional/move_only_function_impl.h>

#  define _LIBCPP_MOVE_ONLY_FUNCTION_CV const
#  define _LIBCPP_MOVE_ONLY_FUNCTION_REF &
#  include <__functional/move_only_function_impl.h>

#  define _LIBCPP_MOVE_ONLY_FUNCTION_CV const
#  define _LIBCPP_MOVE_ONLY_FUNCTION_REF &&
#  include <__functional/move_only_function_impl.h>

#  undef _LIBCPP_IN_MOVE_ONLY_FUNCTION_H

#endif // _LIBCPP_STD_VER >= 23 && !defined(_LIBCPP_COMPILER_GCC)

#endif // _LIBCPP___FUNCTIONAL_MOVE_ONLY_FUNCTION_H
