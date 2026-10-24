//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stdckdint.h>

// [stdckdint]
// Mandates: Each of the types type1, type2, and type3 is a signed or unsigned integer type.

// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=error,note

#include <stdckdint.h>

void test() {
  double d;
  int i;
  (void)ckd_add(&d, 1, 2);
  // expected-error@*:* {{ckd_add requires signed or unsigned integer types}}
  (void)ckd_sub(&i, true, 2);
  // expected-error@*:* {{ckd_sub requires signed or unsigned integer types}}
  (void)ckd_mul(&i, 1, 'a');
  // expected-error@*:* {{ckd_mul requires signed or unsigned integer types}}
}
