//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <array>
#include <string_view>

// P3617-02 and P3617-04: string literals are represented without their source
// null character, and the reflected array has exactly one new terminator.
constexpr std::meta::info narrow = std::meta::reflect_constant_string("abc");
static_assert(std::meta::type_of(narrow) == ^^const char[4]);
static_assert(std::meta::extent(std::meta::type_of(narrow)) == 4);
static_assert(std::meta::extract<const char *>(narrow)[0] == 'a');
static_assert(std::meta::extract<const char *>(narrow)[3] == '\0');

constexpr std::meta::info utf8 =
    std::meta::reflect_constant_string(std::u8string_view(u8"xy"));
static_assert(std::meta::type_of(utf8) == ^^const char8_t[3]);
static_assert(std::meta::extent(std::meta::type_of(utf8)) == 3);
static_assert(std::meta::extract<const char8_t *>(utf8)[2] == u8'\0');

// P3617-03: a structural, copy-constructible element type can be lifted from
// a constant range, including the specified copy construction behavior.
struct Copyable {
  int value;
  constexpr Copyable(int v) : value(v) {}
};
constexpr auto values = std::define_static_array(std::array{Copyable{1}, Copyable{2}});
static_assert(values.size() == 2);
static_assert(values[0].value == 1 && values[1].value == 2);

// P3687-01: unparenthesized splice expressions are no longer template
// arguments.
template <auto> struct Holder {};
Holder<[:^^int:]> removed_splice_template_argument;
// expected-error@-1 {{unparenthesized splice expression cannot be used as a template argument}}

// P3687-02: reflecting a declaration introduced by a using-declarator is
// ill-formed.
struct Base { void f(); };
struct Derived : Base { using Base::f; };
constexpr auto using_decl = ^^Derived::f;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

// P3687-03: two using-declarators naming the same entity make the proxy
// reflection ambiguous.
struct ProxyBase { void g(); };
struct Left : ProxyBase { using ProxyBase::g; };
struct Right : ProxyBase { using ProxyBase::g; };
struct Ambiguous : Left, Right {};
constexpr auto ambiguous_proxy = ^^Ambiguous::g;
// expected-error@-1 {{member 'g' found in multiple base classes of different types}}

int main() {}
