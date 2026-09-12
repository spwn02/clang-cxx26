//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// #include <memory>

// template<class T> T* start_lifetime_as(void* p) noexcept;

// Mandates: T is an implicit-lifetime type.

#include <memory>
#include <string>
#include <type_traits>

// std::string is not an implicit-lifetime type: it's not an aggregate, and
// none of its constructors are trivial (nor is its destructor).
static_assert(!std::is_implicit_lifetime_v<std::string>, "");

void f(void* p) {
  std::start_lifetime_as<std::string>(p);
  // expected-error@*:* {{static assertion failed due to requirement 'is_implicit_lifetime_v<std::string>'}}
  // expected-note@-2 {{in instantiation of function template specialization}}
}
