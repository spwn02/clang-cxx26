//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=note

// <memory>

// template<size_t Alignment, class T>
//   bool is_sufficiently_aligned(T* ptr);
//
// Mandates: Alignment is a power of two.

#include <memory>

void test(int* p) {
  (void)std::is_sufficiently_aligned<3>(p);
  // expected-error@*:* {{requires Alignment to be a power of two}}
  (void)std::is_sufficiently_aligned<0>(p);
  // expected-error@*:* {{requires Alignment to be a power of two}}
}
