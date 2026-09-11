//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <string_view>

using namespace std::literals;

namespace p3491 {

static_assert(__cpp_lib_define_static == 202506L);

constexpr char ordinary[] = "ordinary";

static_assert(std::is_string_literal("ordinary"));
static_assert(std::is_string_literal(&"ordinary"[3]));
static_assert(std::is_string_literal(L"wide"));
static_assert(std::is_string_literal(&L"wide"[2]));
static_assert(std::is_string_literal(u8"utf8"));
static_assert(std::is_string_literal(u"utf16"));
static_assert(std::is_string_literal(U"utf32"));
static_assert(!std::is_string_literal(ordinary));
static_assert(!std::is_string_literal(ordinary + 2));

// Literal ranges already contain their terminator, so the result stays a
// single-terminated array. Non-literal ranges receive one terminator here.
constexpr auto literal = std::meta::reflect_constant_string("abc");
static_assert(std::meta::extent(std::meta::type_of(literal)) == 4);
static_assert(std::meta::extract<const char *>(literal)[3] == '\0');

constexpr auto nonliteral = std::meta::reflect_constant_string("abc"sv);
static_assert(std::meta::extent(std::meta::type_of(nonliteral)) == 4);
static_assert(std::meta::extract<const char *>(nonliteral)[3] == '\0');

constexpr auto wide = std::meta::reflect_constant_string(L"wide");
constexpr auto utf16 = std::meta::reflect_constant_string(u"utf16");
constexpr auto utf32 = std::meta::reflect_constant_string(U"utf32");
static_assert(std::meta::extract<const wchar_t *>(wide)[4] == L'\0');
static_assert(std::meta::extract<const char16_t *>(utf16)[5] == u'\0');
static_assert(std::meta::extract<const char32_t *>(utf32)[5] == U'\0');

static_assert(std::define_static_string(L"wide")[4] == L'\0');
static_assert(std::define_static_string(u8"utf8")[4] == u8'\0');
static_assert(std::define_static_string(u"utf16")[5] == u'\0');
static_assert(std::define_static_string(U"utf32")[5] == U'\0');

struct point {
  int x;
  int y;
  friend consteval bool operator==(point, point) = default;
};

constexpr auto scalar = std::define_static_object(42);
constexpr auto object = std::define_static_object(point{1, 2});
static_assert(*scalar == 42);
static_assert(object->x == 1 && object->y == 2);

} // namespace p3491

int main(int, char**) { return 0; }
