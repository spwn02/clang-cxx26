// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [expr.ref] member access naming a direct base class relationship (P3293R3,
// #178): diamond bases, private/protected bases, ->, this->, dependent splices,
// virtual bases, array elements, value category and decltype.

#include <meta>
#include <type_traits>
using namespace std::meta;
_Pragma("clang diagnostic ignored \"-Winaccessible-base\"")
struct B0{int x;}; struct C0:B0{}; struct D0:B0,C0{};
int f(D0& d){ return d.[: bases_of(^^D0, access_context::unchecked())[0] :].x; }
struct P{int x;}; struct Pr:private P{}; struct Pt:protected P{};
constexpr auto prb = bases_of(^^Pr, access_context::unchecked())[0];
constexpr auto ptb = bases_of(^^Pt, access_context::unchecked())[0];
int g1(Pr& p){ return p.[:prb:].x; } int g2(Pt& p){ return p.[:ptb:].x; }
struct Q:P{}; constexpr auto qb = bases_of(^^Q, access_context::unchecked())[0];
int h1(Q* p){ return p->[:qb:].x; } struct Q2:P{ int m(){ return this->[:bases_of(^^Q2, access_context::unchecked())[0]:].x; } };
template<class T> struct TD:P{ int m(){ return this->[:bases_of(^^TD, access_context::unchecked())[0]:].x; } }; int h2(){ return TD<int>{}.m(); }
struct V{int x;}; struct QV:virtual V{}; constexpr auto vb = bases_of(^^QV, access_context::unchecked())[0];
int v1(QV& q){ return q.[:vb:].x; }
int arr(Q* a){ return a[0].[:qb:].x; }
static_assert(std::is_same_v<decltype(Q{}.[:qb:]), P>);
static_assert(std::is_same_v<decltype((Q{}.[:qb:])), P&&>);

int main(int, char**) { return 0; }
