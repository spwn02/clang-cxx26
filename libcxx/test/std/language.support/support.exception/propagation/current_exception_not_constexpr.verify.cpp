//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-exceptions

// <exception>

// P3842R2: current_exception() and uncaught_exceptions() are not constexpr (only make_exception_ptr, rethrow_exception and
// the exception_ptr operations are).

#include <exception>

constexpr bool f() {
  auto p = std::current_exception(); // expected-note {{non-constexpr function 'current_exception' cannot be used in a constant expression}}
  return !p;
}
static_assert(f()); // expected-error {{static assertion expression is not an integral constant expression}}

constexpr int g() {
  return std::uncaught_exceptions(); // expected-note {{non-constexpr function 'uncaught_exceptions' cannot be used in a constant expression}}
}
static_assert(g() == 0); // expected-error {{static assertion expression is not an integral constant expression}}

constexpr bool h() {
  auto p = std::make_exception_ptr(1); // still constexpr
  return static_cast<bool>(p);
}
static_assert(h());
