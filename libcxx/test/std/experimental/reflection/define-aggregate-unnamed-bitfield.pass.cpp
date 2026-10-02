// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <algorithm>
#include <meta>
using namespace std::meta;
struct A;
consteval {
  define_aggregate(^^A, {data_member_spec(^^int, {.bit_width=3}),
                        data_member_spec(^^int, {.name="x"})});
}
constexpr auto ctx = access_context::unchecked();
static_assert(std::ranges::count_if(members_of(^^A, ctx), [](info r) {
  return is_bit_field(r) || is_nonstatic_data_member(r);
}) == 2);
static_assert(!has_identifier(members_of(^^A, ctx)[0]));
static_assert(is_bit_field(members_of(^^A, ctx)[0]));
static_assert(bit_size_of(members_of(^^A, ctx)[0]) == 3);
static_assert(nonstatic_data_members_of(^^A, ctx).size() == 1);
constexpr A a{1};
static_assert(a.x == 1);
struct Expected { int : 3; int x; };
static_assert(sizeof(A) == sizeof(Expected));
static_assert(alignof(A) == alignof(Expected));
int main() {}
