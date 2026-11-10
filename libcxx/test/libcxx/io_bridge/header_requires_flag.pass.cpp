//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <io_bridge> is an empty, valid header in C++29 without -fexperimental-library: the networking library is experimental.

// REQUIRES: std-at-least-c++29

#include <io_bridge>
#include <version>

#include "test_macros.h"

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING
#  error "_LIBCPP_HAS_EXPERIMENTAL_NETWORKING must be 0 without -fexperimental-library"
#endif

#include <io_bridge>

namespace std::io {} // namespace std::io

int main(int, char**) { return 0; }
