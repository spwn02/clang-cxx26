//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// object_of: \throws meta::exception unless r represents an object with static
// storage duration, or a variable that declares or refers to such an object and,
// if that variable is a reference R, either R is usable in constant expressions
// or the lifetime of R began within the evaluation.
// return_type_of: \throws meta::exception unless r represents a function and
// has-type(r) is true, or a function type (a function whose return type has not
// been deduced has no type yet).
#include <meta>
using namespace std::meta;
extern int &unknown_reference;
int gx = 1;
int &usable_ref = gx;
consteval bool unknown_throws() { try { (void)object_of(^^unknown_reference); } catch (const exception &) { return true; } return false; }
static_assert(unknown_throws());
static_assert(object_of(^^usable_ref) == object_of(^^gx));
consteval bool local_ref_static_object() { int &r = gx; return object_of(^^r) == object_of(^^gx); }
static_assert(local_ref_static_object());
auto pending();
consteval bool rt_throws() { try { (void)return_type_of(^^pending); } catch (const exception &) { return true; } return false; }
static_assert(rt_throws());
auto defined() { return 1; }
static_assert(return_type_of(^^defined) == ^^int);
int main(int, char**) {}
