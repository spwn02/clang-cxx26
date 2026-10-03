//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

// [meta.reflection.extract], extract-ref Throws: "If r represents a variable,
// then either that variable is usable in constant expressions or its lifetime
// began within the core constant expression currently under evaluation."
int global = 0;
// [expr.const] explicitly makes a reference bound to a static object
// constant-initialized, even when that object is mutable.
int& ordinary_reference = global;
int& runtime_reference() { return global; }
int& unusable_reference = runtime_reference();
constexpr int& usable_reference = global;

template <std::meta::info R>
consteval bool rejects() {
  try {
    (void)std::meta::extract<int&>(R);
  } catch (std::meta::exception& e) {
    return e.from() == ^^std::meta::extract<int&>;
  }
  return false;
}
static_assert(rejects<^^unusable_reference>());
static_assert(&std::meta::extract<int&>(^^usable_reference) == &global);
static_assert(&std::meta::extract<int&>(^^ordinary_reference) == &global);

consteval bool local_lifetimes() {
  int local = 1;
  std::meta::extract<int&>(^^local) = 2;
  return local == 2;
}
static_assert(local_lifetimes());

// The caller local is outside the separately evaluated immediate invocation.
constexpr bool outside_evaluation() {
  int caller_local = 0;
  return rejects<^^caller_local>();
}
static_assert(outside_evaluation());

int main(int, char**) { return 0; }
