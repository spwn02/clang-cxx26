// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;
struct A;
consteval {
  define_aggregate(^^A, {data_member_spec(^^int, {.name="_"}),
                        data_member_spec(^^int, {.name="_"})});
}
constexpr auto ctx = access_context::unchecked();
consteval auto members() { return nonstatic_data_members_of(^^A, ctx); }
static_assert(members().size() == 2);
static_assert(identifier_of(members()[0]) == "_");
static_assert(identifier_of(members()[1]) == "_");
constexpr A a{1, 2};
static_assert(a.[:members()[0]:] == 1);
static_assert(a.[:members()[1]:] == 2);
int main() {}
