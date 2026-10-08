//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// A static_assert(false) of a function template that is instantiated by substitute() from a consteval member function
// template (bloomberg/clang-p2996#180, #152) must be diagnosed.

#include <meta>

template <typename>
void test_impl() {
  static_assert(false); // expected-error {{static assertion failed}}
}

struct Foo {
  void (*test)() = nullptr;

  template <bool E>
  consteval void maybe_init() {
    if constexpr (E) {
      test = extract<void (*)()>(substitute(^^test_impl, {^^int}));
    }
  }

  consteval Foo(bool enabled) {
    (this->*extract<void (Foo::*)()>(substitute(^^maybe_init, {std::meta::reflect_constant(enabled)})))();
  }
};

int main(int, char**) {
  auto f = Foo(true);
  f.test();
  return 0;
}
