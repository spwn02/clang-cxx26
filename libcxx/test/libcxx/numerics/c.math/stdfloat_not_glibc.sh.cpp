//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

// __math/stdfloat.h declares the glibc binary128 libm functions only with glibc 2.26 or newer. __GLIBC_PREREQ is a
// function-like macro that only glibc defines; the header must not mention it where there is none (Apple's libc, musl,
// Windows), or the preprocessor rejects it ("function-like macro '__GLIBC_PREREQ' is not defined").

// RUN: %{cxx} %s %{flags} %{compile_flags} -fsyntax-only -DCASE=0
// RUN: %{cxx} %s %{flags} %{compile_flags} -fsyntax-only -DCASE=1

#include <cstdlib> // any C library header: after it, glibc has defined __GLIBC__ and __GLIBC_PREREQ

#undef __GLIBC__
#undef __GLIBC_PREREQ
#if CASE == 1
// a glibc older than 2.26
#  define __GLIBC__ 2
#  define __GLIBC_PREREQ(maj, min) 0
#endif

#include <__math/stdfloat.h>
