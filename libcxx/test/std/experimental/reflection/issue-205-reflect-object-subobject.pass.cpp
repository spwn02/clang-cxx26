// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// reflect_object of a subobject reached through a member and an array index
// (std::array element, nested arrays, base members) used to crash APValue::Lift.

#include <meta>
#include <array>
using namespace std::meta;
consteval int get(const int& v) { return v; }
struct In { int a; std::array<int,2> arr; };
struct Base { int b; };
struct Out : Base { In in; int m[2][2]; std::array<std::array<int,2>,2> aa; };
inline constexpr std::array<int,3> values{4,5,6};
inline constexpr Out o{{7},{8,{9,10}},{{11,12},{13,14}},{{{15,16},{17,18}}}};
consteval int viaObj(info r){ return extract<int>(reflect_invoke(^^get,{r})); }
static_assert(viaObj(reflect_object(values[2])) == 6);
static_assert(viaObj(reflect_object(o.in.arr[1])) == 10);
static_assert(viaObj(reflect_object(o.in.a)) == 8);
static_assert(viaObj(reflect_object(o.b)) == 7);
static_assert(viaObj(reflect_object(o.m[1][0])) == 13);
static_assert(viaObj(reflect_object(o.aa[1][1])) == 18);
static_assert(type_of(reflect_object(values[1])) == ^^const int);
static_assert(is_object(reflect_object(o.aa[1])));
static_assert(extract<const int&>(reflect_object(values[0])) == 4);

// Sugared array types and member-after-index paths.
using A3 = int[3];
using CA = const int[2];
inline constexpr A3 x3{1,2,3};
struct S2 { A3 a; CA c; };
inline constexpr S2 s{{1,2,3},{4,5}};
struct P2 { int x; int y; };
inline constexpr std::array<P2,2> ps{{{1,2},{3,4}}};
inline constexpr P2 cps[2]{{5,6},{7,8}};
inline constexpr std::array<A3,2> aa{{{1,2,3},{4,5,6}}};
static_assert(viaObj(reflect_object(x3[1]))==2);
static_assert(viaObj(reflect_object(s.a[2]))==3);
static_assert(viaObj(reflect_object(s.c[1]))==5);
static_assert(type_of(reflect_object(s.c[1]))==^^const int);
static_assert(viaObj(reflect_object(aa[1][2]))==6);
static_assert(viaObj(reflect_object(ps[1].y))==4);
static_assert(type_of(reflect_object(ps[1].x))==^^const int);
static_assert(viaObj(reflect_object(cps[1].x))==7);

consteval bool positive(const int& value) { return value > 0; }
inline constexpr std::array one{1};
static_assert(std::meta::extract<bool>(
    std::meta::reflect_invoke(^^positive, {std::meta::reflect_object(one[0])})));

int main(int, char**) { return 0; }
