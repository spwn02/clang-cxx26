//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <cstdlib>

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <cstdlib>

static_assert(std::abs(-5) == 5);
static_assert(std::abs(-5L) == 5L);
static_assert(std::abs(-5LL) == 5LL);

int main(int, char**) { return 0; }
