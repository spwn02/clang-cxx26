//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// M5 checklist row 2996-48 (P2996R13): a splice of a constructor reflection
// is ill-formed. (Destructor splices are already correctly rejected.)

#include <meta>

struct S {
  constexpr S() = default;
};

constexpr auto constructor = [] consteval {
  for (std::meta::info member : std::meta::members_of(
           ^^S, std::meta::access_context::unchecked()))
    if (std::meta::is_constructor(member))
      return member;
  return std::meta::info{};
}();

void bad() {
  [:constructor:] value;
  // expected-error@-1 {{reflection not usable in a splice type}}
}

void good() {
  [:^^S:] value;
  (void)value;
}
