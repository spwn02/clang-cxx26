//===----------------------------------------------------------------------===//
//
// Copyright 2026
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

struct no_range {
  constexpr int begin() const { return 0; }
  constexpr int end() const { return 1; }
};
struct one_member { int value; };

constexpr int values[] = {1, 2};

int runtime_value();

void nonconstant_range() {
  // P1306R5 [stmt.expand]/5.2: the range must be a constant expression.
  template for (constexpr auto value : runtime_value()) {}
  // expected-error@-1 {{constexpr variable '__range' must be initialized by a constant expression}}
}

void invalid_range() {
  // The non-iterable, non-destructurable fallback is ill-formed.
  template for (auto value : no_range{}) {}
  // expected-error@-1 {{indirection requires pointer operand ('__size_t' (aka 'unsigned long') invalid)}}
}

void invalid_for_range_declaration() {
  // P1306R5 [stmt.pre]/8: only type specifiers and constexpr are permitted.
  template for (static auto value : values) {}
  // expected-error@-1 {{loop variable 'value' may not be declared 'static'}}
}

void escaping_label() {
  template for (auto value : values) {
    target: // expected-error {{identifier labels are not allowed in expansion statements}}
    ;
  }
}

void invalid_destructuring() {
  // A destructuring expansion uses the structured-binding rules.
  template for (auto [first, second] : one_member{}) {}
  // expected-error@-1 {{cannot bind non-class, non-array type 'int'}}
}
