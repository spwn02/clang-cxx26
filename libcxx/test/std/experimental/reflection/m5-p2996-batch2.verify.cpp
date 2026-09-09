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

#include <meta>

namespace p2996_batch2 {

// 2996-05: the object expression must be suitable as a constant template
// argument for its reference type.
constexpr auto bad_object = [] {
  int local = 0;
  return std::meta::reflect_object(local);
}();
// expected-error@20 {{constexpr variable 'bad_object' must be initialized by a constant expression}}
// expected-note@22 {{provided object cannot be represented by a reflection}}

// 2996-06: reflect_function requires a function type.
int object = 0;
constexpr auto bad_function_type = std::meta::reflect_function(object);
// expected-error@-1 {{no matching function for call to 'reflect_function'}}

void target() {}

// 2996-07: the function expression must be suitable as a constant template
// argument for its reference type.
void (*function_pointer)() = &target;
constexpr auto bad_function_value = std::meta::reflect_function(*function_pointer);
// expected-error@-1 {{must be initialized by a constant expression}}

// 2996-08: identifier queries require a declaration with an identifier.
constexpr auto no_identifier = std::meta::identifier_of(^^int);
// expected-error@-1 {{constexpr variable 'no_identifier' must be initialized by a constant expression}}
// expected-note@-2 {{reflected a type is anonymous and has no associated identifier}}
constexpr auto no_u8identifier = std::meta::u8identifier_of(^^int);
// expected-error@-1 {{constexpr variable 'no_u8identifier' must be initialized by a constant expression}}
// expected-note@-2 {{reflected a type is anonymous and has no associated identifier}}

} // namespace p2996_batch2
