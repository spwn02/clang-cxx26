//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// A C library that declares atexit, qsort and bsearch, and at_quick_exit as selected by VARIANT.

#include <stddef.h>

extern "C" int atexit(void (*)(void));
extern "C" void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
extern "C" void* bsearch(const void*, const void*, size_t, size_t, int (*)(const void*, const void*));

#if VARIANT == 1
int at_quick_exit(void (*)(void)) noexcept; // the callback has C++ language linkage, as in glibc
#elif VARIANT == 2
extern "C" int at_quick_exit(void (*)(void)); // the callback has C language linkage, as in the BSDs
#endif                                        // VARIANT == 0: the C library has no at_quick_exit (macOS before 15)
