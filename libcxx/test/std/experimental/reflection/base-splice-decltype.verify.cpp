// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [dcl.type.decltype]: decltype of an unparenthesized class member access is the
// type of the entity named by it and is ill-formed if there is no such entity. A
// direct base class relationship is not an entity ([basic.pre]) (#178).

#include <meta>

struct P { int x; };
struct Q : P {};
constexpr auto qb = std::meta::bases_of(^^Q, std::meta::access_context::unchecked())[0];
Q q;

using Bad1 = decltype(q.[:qb:]);   // expected-error {{names no entity}}
using Bad2 = decltype(Q{}.[:qb:]); // expected-error {{names no entity}}
using Ok1 = decltype((q.[:qb:]));
using Ok2 = decltype((Q{}.[:qb:]));
