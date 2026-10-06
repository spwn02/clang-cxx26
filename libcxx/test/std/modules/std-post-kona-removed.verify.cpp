//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build, gcc
// XFAIL: has-no-cxx-module-support
// MODULE_DEPENDENCIES: std

// P4052R0 renames the saturating functions: the old names are not exported any more.

import std;

void test() {
  (void)std::add_sat(1, 2);       // expected-error {{no member named 'add_sat' in namespace 'std'}}
  (void)std::saturate_cast<int>(1L); // expected-error {{no template named 'saturate_cast' in namespace 'std'}}
}
