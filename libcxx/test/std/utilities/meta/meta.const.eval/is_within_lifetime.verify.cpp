//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <type_traits>

// template<class T>
//   consteval bool is_within_lifetime(const T* p) noexcept; // C++26
//
// Constraints: is_function_v<T> is false.

#include <type_traits>

void f();

void test() {
  // Explicit template argument forces T = void() rather than letting
  // deduction against `const T*` fail first, matching how
  // clang/test/SemaCXX/builtin-is-within-lifetime.cpp exercises the same
  // constraint. The rejection notes ("constraints not satisfied ...
  // because '!is_function_v<void ()>' evaluated to false") live inside
  // <type_traits> itself, not this file, so only the top-level diagnostic
  // is checked here to avoid coupling this test to the header's exact
  // line numbers.
  // expected-error@+1 {{no matching function for call to 'is_within_lifetime'}}
  std::is_within_lifetime<void()>(&f);
}
