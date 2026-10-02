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

struct NonStructural { private: [[maybe_unused]] int value = 0; };
struct NonCopyable {
  constexpr NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
};
void function() {}
int object;

// Mandates do not remove these functions from overload resolution.
static_assert(requires(NonStructural s) { std::meta::reflect_constant(s); });
static_assert(requires { std::meta::reflect_object(function); });
static_assert(requires { std::meta::reflect_function(object); });

void test() {
  (void)std::meta::reflect_constant(NonStructural{});
  // expected-error@meta:* {{reflect_constant requires a cv-unqualified structural non-reference type}}
  (void)std::meta::reflect_constant(NonCopyable{});
  // expected-error@meta:* {{reflect_constant requires a copy-constructible type}}
  (void)std::meta::reflect_constant<const int>(1);
  // expected-error@meta:* {{reflect_constant requires a cv-unqualified structural non-reference type}}
  (void)std::meta::reflect_constant<int&>(object);
  // expected-error@meta:* {{reflect_constant requires a cv-unqualified structural non-reference type}}
  (void)std::meta::reflect_object(function);
  // expected-error@meta:* {{reflect_object requires an object type}}
  (void)std::meta::reflect_function(object);
  // expected-error@meta:* {{reflect_function requires a function type}}
}
