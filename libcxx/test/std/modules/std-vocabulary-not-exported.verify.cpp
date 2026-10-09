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

// `import std;` provides the names of the C++ standard library only: the scope guards of the Library
// Fundamentals TS (<experimental/scope>) and the names that were removed because the draft does not
// have them (libcxx/utils/vocabulary_audit.py) must not be visible.

import std;

std::scope_exit<int> g;                // expected-error {{no template named 'scope_exit' in namespace 'std'}}
using std::atomic_store_fmaximum;      // expected-error {{no member named 'atomic_store_fmaximum' in namespace 'std'}}

void test() {
  std::experimental::scope_exit h([] {}); // expected-error {{no member named 'experimental' in namespace 'std'}}
}
