//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++26
// REQUIRES: clang

#include <bit>

using M = decltype(^^int);

consteval M test(M m) {
  return std::bit_cast<M>(m); // expected-error@*:* {{bit_cast requires types that are not consteval-only}}
}
