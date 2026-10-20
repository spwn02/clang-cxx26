//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: clang
// UNSUPPORTED: no-threads, c++03, c++11, c++14, c++17

// RUN: %{cxx} -fsyntax-only -Xclang -ast-dump -Xclang -ast-dump-filter=barrier %s %{flags} %{compile_flags} > %t.ast
// RUN: grep -E "CXXConstructorDecl.*constexpr barrier 'void \(ptrdiff_t," %t.ast

#include <barrier>

static_assert(__is_constructible(std::barrier<>, std::ptrdiff_t));
