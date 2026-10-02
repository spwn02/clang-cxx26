// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

struct S {
  int i;
  union { int u; float f; };
  struct { int x; int y; };
};
constexpr auto union_member = nonstatic_data_members_of(^^S, access_context::unchecked())[1];
constexpr auto struct_member = nonstatic_data_members_of(^^S, access_context::unchecked())[2];
static_assert(!has_identifier(union_member) && !has_identifier(struct_member));
constexpr auto union_name = display_string_of(union_member);
constexpr auto union_u8name = u8display_string_of(union_member);
constexpr auto struct_name = display_string_of(struct_member);
constexpr auto struct_u8name = u8display_string_of(struct_member);
static_assert(!union_name.empty() && !union_u8name.empty());
static_assert(!struct_name.empty() && !struct_u8name.empty());

int main() {}
