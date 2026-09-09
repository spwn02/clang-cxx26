//===----------------------------------------------------------------------===//
//
// Copyright 2026 CXX26 Clang contributors.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

struct Base {
  int value;
};

struct Derived : Base {
  int own;
};

struct VirtualDerived : virtual Base {};

constexpr auto ctx = std::meta::access_context::unchecked();
constexpr auto base = std::meta::bases_of(^^Derived, ctx)[0];
constexpr auto virtual_base = std::meta::bases_of(^^VirtualDerived, ctx)[0];
constexpr auto not_a_base = ^^int;

int read(Derived& d) {
  return d.[:base:].value;
}

void write(Derived& d) {
  d.[:base:].value = 42;
}

void rejects_virtual(VirtualDerived& d) {
  (void)d.[:virtual_base:]; // expected-error {{virtual base class subobject}}
}

void rejects_array_element(Derived (&d)[1]) {
  (void)d[0].[:base:]; // expected-error {{array element}}
}

void rejects_non_base(Derived& d) {
  (void)d.[:not_a_base:]; // expected-error {{reflection not usable in a splice expression}}
}

int main() {
  Derived d{{7}, 11};
  if (read(d) != 7)
    return 1;
  write(d);
  return d.Base::value != 42;
}
