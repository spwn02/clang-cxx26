//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cinttypes>

#include <cinttypes>

static_assert(std::imaxabs(-3) == 3);

constexpr auto quotient_and_remainder = std::imaxdiv(7, 2);
static_assert(quotient_and_remainder.quot == 3);
static_assert(quotient_and_remainder.rem == 1);

static_assert(std::imaxdiv(-7, 2).quot == -3);
static_assert(std::imaxdiv(-7, 2).rem == -1);

int main(int, char**) { }
