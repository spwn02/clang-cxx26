//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection

// [meta.syn]: E.from() represents the function that threw and E.where() is where
// the call originated; this holds for the P4033R1/P3385R8 throw sites too
// (the macro-expanded call reports the line of the macro use).
#include <meta>
#include <string_view>
using namespace std::meta;
struct S {};
[[nodiscard]] int f();

#define CASE(NAME, EXPR, FN)                                                         \
  consteval bool NAME(unsigned line) {                                               \
    try { (void)(EXPR); } catch (const exception &e) {                              \
      return identifier_of(e.from()) == FN && e.where().line() == line &&             \
             std::string_view(e.where().file_name()) == __FILE__;                    \
    }                                                                                \
    return false;                                                                    \
  }
CASE(c1, enumerator_spec({.name = "class"}), "enumerator_spec")
CASE(c2, has_attribute(^^f, ^^int), "has_attribute")
CASE(c3, has_attribute(^^f, ^^int, attribute_comparison::ignore_argument), "has_attribute")
CASE(c4, enumerator_spec({.name = "A", .annotations = {^^int}}), "enumerator_spec")
CASE(c5, enumerator_spec({.name = "A", .attributes = {^^int}}), "enumerator_spec")
static_assert(c1(27));
static_assert(c2(28));
static_assert(c3(29));
static_assert(c4(30));
static_assert(c5(31));

int main(int, char**) {}
