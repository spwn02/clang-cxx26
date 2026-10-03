// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [expr.ref]: for a direct base class relationship (D, B), the type of E1.E2 is
// cv B and E1.E2 is an lvalue if E1 is, an xvalue otherwise (#178). A direct base
// class relationship is not an entity, so an unparenthesized decltype of the access
// is ill-formed (see clang/test/SemaCXX/base-splice-decltype.cpp).

#include <meta>
#include <type_traits>

struct P { int x; };
struct Q : P {};
constexpr auto qb = std::meta::bases_of(^^Q, std::meta::access_context::unchecked())[0];
[[maybe_unused]] Q q;
[[maybe_unused]] const Q cq{};

static_assert(std::is_same_v<decltype((q.[:qb:])), P&>);
static_assert(std::is_same_v<decltype((Q{}.[:qb:])), P&&>);
static_assert(std::is_same_v<decltype((cq.[:qb:])), const P&>);

int main(int, char**) { return 0; }
