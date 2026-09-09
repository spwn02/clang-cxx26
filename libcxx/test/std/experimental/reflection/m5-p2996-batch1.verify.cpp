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

// <experimental/reflection>

#include <meta>

namespace p2996_batch1 {

struct NonCopyable {
  constexpr NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
};
constexpr NonCopyable noncopyable{};

// 2996-01: reflect_constant<T> requires T to be copy-constructible.
constexpr auto r1 = std::meta::reflect_constant(noncopyable);
// expected-error@-1 {{call to deleted constructor of 'p2996_batch1::NonCopyable'}}

// 2996-02: an explicitly supplied reference type is not permitted.
constexpr int value = 42;
constexpr auto r2 = std::meta::reflect_constant<int&>(value);
// expected-error@-1 {{no matching function for call to 'reflect_constant'}}

// 2996-03: a pointer value that cannot form the invented template argument
// object is not a constant subexpression.
constexpr auto r3 = std::meta::reflect_constant((const char*)"fails");
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{provided value cannot be represented}}

// 2996-04: reflect_object<T> requires T to be an object type.
void function();
constexpr auto r4 = std::meta::reflect_object(function);
// expected-error@-1 {{no matching function for call to 'reflect_object'}}

} // namespace p2996_batch1
