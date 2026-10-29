//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, gcc

// std::bsearch has overloads for a callback with C and with C++ language linkage ([alg.c.library]). The mangling of the
// MSVC ABI does not tell the function types apart, so the two overloads must not both be non-templates: they would have
// the same mangled name and the compiler rejects the second definition when it emits both (-femit-all-decls does).
// This compiles <__cstdlib/callbacks.h> for that target with a stand-in for the C library of Windows.

// RUN: %{cxx} %s -I %S/Inputs/msvc_libc %{flags} %{compile_flags} --target=x86_64-pc-windows-msvc -fms-compatibility -femit-all-decls -S -emit-llvm -o -

#include <__config>
#include <__cstdlib/callbacks.h>

int cxx_compare(const void*, const void*) { return 0; }
extern "C" int c_compare(const void*, const void*) { return 0; }

void use(void* base, const void* const_base) {
  (void)std::bsearch(base, base, 1, 1, cxx_compare);
  (void)std::bsearch(base, base, 1, 1, c_compare);
  (void)std::bsearch(const_base, const_base, 1, 1, cxx_compare);
  (void)std::bsearch(const_base, const_base, 1, 1, c_compare);
}
