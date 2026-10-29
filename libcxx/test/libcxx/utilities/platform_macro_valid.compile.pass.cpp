//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// Some platform headers (the MSVC ones among them) define __valid as a macro. The names that the headers of the library
// use for locals and members must not be among the reserved names that such a header takes, so <charconv> and
// <execution> have to compile with __valid defined to something that cannot be an identifier's replacement.

#define __valid 0 0

#include <charconv>
#include <execution>
#include <memory>
