//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// LWG4245: the operators of counted_iterator that take default_sentinel_t are noexcept.

#include <iterator>

std::counted_iterator<int*> it(nullptr, 3);
static_assert(noexcept(it == std::default_sentinel));
static_assert(noexcept(it - std::default_sentinel));
static_assert(noexcept(std::default_sentinel - it));
