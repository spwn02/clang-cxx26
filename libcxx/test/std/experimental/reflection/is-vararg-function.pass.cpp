//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;
void v(int, ...);
void c(...);
// Exercise the deprecated spelling without making this warning test-wide.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-missing-comma-variadic-parameter"
void spelled(int...);
#pragma clang diagnostic pop
void nv(int);
struct S { void m(int, ...) const volatile && noexcept; int data; };
template<class T> void f(T, ...);
template<class... T> void g(T...);
using V = void(int, ...);
using Q = void(int, ...) const volatile && noexcept;
constexpr auto lambda = [](int, ...) {};
constexpr auto pack = [](auto...) {};
int variable;
static_assert(is_vararg_function(^^v));
static_assert(is_vararg_function(type_of(^^v)));
static_assert(is_vararg_function(^^V));
static_assert(is_vararg_function(^^Q));
static_assert(is_vararg_function(^^c));
static_assert(is_vararg_function(^^spelled));
static_assert(is_vararg_function(^^S::m));
static_assert(is_vararg_function(type_of(^^S::m)));
static_assert(is_vararg_function(^^f<int>));
static_assert(is_vararg_function(^^decltype(lambda)::operator()));
static_assert(!is_vararg_function(^^decltype(pack)::operator()<int>));
static_assert(!is_vararg_function(^^g<int>));
static_assert(!is_vararg_function(^^g));
static_assert(!is_vararg_function(^^f));
static_assert(!is_vararg_function(^^nv));
static_assert(!is_vararg_function(^^int));
static_assert(!is_vararg_function(^^S));
static_assert(!is_vararg_function(^^::));
static_assert(!is_vararg_function(info{}));
static_assert(!is_vararg_function(^^variable));
static_assert(!is_vararg_function(reflect_object(variable)));
static_assert(!is_vararg_function(^^S::data));
static_assert(!is_vararg_function(reflect_constant(42)));
#if __has_feature(parameter_reflection)
static_assert(!is_vararg_function(parameters_of(^^v)[0]));
#endif
int main() {}
