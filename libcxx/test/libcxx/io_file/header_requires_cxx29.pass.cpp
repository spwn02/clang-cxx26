//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <io_file> is an empty, valid header before C++29 even with -fexperimental-library.

// UNSUPPORTED: c++29
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

#include <io_file>
#include <version>

#include "test_macros.h"

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING
#  error "_LIBCPP_HAS_EXPERIMENTAL_NETWORKING must be 0 before C++29"
#endif

int main(int, char**) { return 0; }
