//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=error,warning,note

// [utility.intcmp] Mandates: each of T and U is a signed or unsigned integer type
// (so bool, the character types, enumerations and class types are rejected).

#include <utility>

enum Color { red, green };
struct Empty {};

void test() {
  (void)std::cmp_equal(true, 1);      // expected-error@*:* {{cmp_equal requires signed or unsigned integer types}}
  (void)std::cmp_equal('a', 1);       // expected-error@*:* {{cmp_equal requires signed or unsigned integer types}}
  (void)std::cmp_less(1, red);        // expected-error@*:* {{cmp_less requires signed or unsigned integer types}}
  (void)std::cmp_less(Empty(), 1);    // expected-error@*:* {{cmp_less requires signed or unsigned integer types}}
  (void)std::in_range<bool>(1);       // expected-error@*:* {{in_range requires signed or unsigned integer types}}
  (void)std::in_range<int>(Color());  // expected-error@*:* {{in_range requires signed or unsigned integer types}}

  (void)std::cmp_equal(1.0, 1);  // expected-error@*:* {{cmp_equal requires signed or unsigned integer types}}
  (void)std::cmp_less(1.0, 1);   // expected-error@*:* {{cmp_less requires signed or unsigned integer types}}
  (void)std::in_range<int>(1.0); // expected-error@*:* {{in_range requires signed or unsigned integer types}}
}
