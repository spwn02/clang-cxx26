// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

struct C {};
enum class E { x };
using CAlias = const C;
using EAlias = const E;
static_assert(has_identifier(^^C) && has_identifier(^^E));
static_assert(!has_identifier(^^const C));
static_assert(!has_identifier(^^volatile C));
static_assert(!has_identifier(^^const volatile E));
static_assert(!has_identifier(^^const E));
static_assert(has_identifier(^^CAlias) && identifier_of(^^CAlias) == "CAlias");
static_assert(has_identifier(^^EAlias) && u8identifier_of(^^EAlias) == u8"EAlias");
consteval bool throws_identifier(info r, bool utf8) {
  try {
    if (utf8) (void)u8identifier_of(r);
    else (void)identifier_of(r);
  } catch (const exception&) { return true; }
  return false;
}
static_assert(throws_identifier(^^const C, false));
static_assert(throws_identifier(^^volatile C, true));
static_assert(throws_identifier(^^const E, false));
static_assert(throws_identifier(^^const volatile E, true));

using CPlain = C;
[[maybe_unused]] const CPlain cv_object{};
static_assert(!has_identifier(^^const CPlain));
static_assert(throws_identifier(^^const CPlain, false));
static_assert(!has_identifier(dealias(^^CAlias)));
static_assert(!has_identifier(type_of(^^cv_object)));

int main() {}
