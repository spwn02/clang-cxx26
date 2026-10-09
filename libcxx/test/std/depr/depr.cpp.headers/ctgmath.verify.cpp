//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <ctgmath>

// <ctgmath> was removed in C++20: including it is an error.

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: clang-modules-build

#include <ctgmath> // expected-error@*:* {{<ctgmath> was removed in C++20. Include <cmath> and <complex> instead.}}
