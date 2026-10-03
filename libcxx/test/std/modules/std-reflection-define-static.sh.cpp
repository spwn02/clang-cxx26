//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// define_static_array keeps a static extent for fixed-size ranges (P3491R3),
// reflect_constant takes its argument by value, and define_static_object works
// for class and union types, all through `import std;`.

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-reflection-define-static.sh.cpp.tsk
// RUN: %{exec} %t/std-reflection-define-static.sh.cpp.tsk

import std;

union value_union {
  int integer;
  float real;
};

constexpr int raw[3]{1, 2, 3};
constexpr std::array<int, 3> fixed{4, 5, 6};
constexpr value_union a_union{.integer = 7};

static_assert(std::define_static_array(raw).extent == 3);
static_assert(std::define_static_array(fixed).extent == 3);
static_assert(std::define_static_array(std::vector<int>{1, 2}).extent == std::dynamic_extent);
static_assert(std::meta::is_value(std::meta::reflect_constant(5)));
static_assert(std::meta::type_of(std::meta::reflect_constant(raw)) == ^^const int*);
static_assert(std::define_static_object(a_union)->integer == 7);

static_assert(std::meta::is_object(std::meta::reflect_constant_array(raw)));
static_assert(std::meta::type_of(std::meta::reflect_constant_array(raw)) == ^^const int[3]);
static_assert((*std::define_static_object(raw))[2] == 3);
static_assert((*std::define_static_object("hi"))[0] == 'h');
static_assert(std::meta::is_object(std::meta::constant_of(^^raw)));

struct PublicOnly { int a; };
static_assert(!std::meta::has_inaccessible_subobjects(^^PublicOnly, std::meta::access_context::unprivileged()));
constexpr auto an_access_context = std::meta::access_context::unprivileged();
static_assert(std::meta::is_structural_type(std::meta::type_of(std::meta::reflect_object(an_access_context))));

// P1317R2 traits are exported by <tuple> in C++26.
struct Callable { void operator()(int) const {} };
static_assert(std::is_applicable_v<Callable, std::tuple<int>>);
static_assert(!std::is_applicable_v<Callable, int>);
static_assert(std::is_same_v<std::apply_result_t<int (&)(int), std::tuple<int>>, int>);
static_assert(std::is_nothrow_applicable_v<int (&)(int) noexcept, std::tuple<int>>);

int main(int, char**) { return 0; }
