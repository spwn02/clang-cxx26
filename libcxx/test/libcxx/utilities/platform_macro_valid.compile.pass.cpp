//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// Some platform headers (the MSVC ones among them) define __valid, __nothrow and __out as macros. The names that the
// headers of the library use for locals and members must not be among the reserved names that such a header takes, so
// the headers have to compile with these defined to something that cannot replace an identifier.

#define __valid 0 0
#define __nothrow 0 0
#define __out 0 0

#include <charconv>
#include <execution>
#include <memory>
#include <simd>
