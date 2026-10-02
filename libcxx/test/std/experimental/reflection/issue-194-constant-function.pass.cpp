// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

void fn();
constexpr int value() { return 42; }
static_assert(constant_of(^^fn) == reflect_function(fn));
static_assert(constant_of(^^value) == reflect_function(value));
static_assert(is_function(constant_of(^^fn)));
static_assert(constant_of(constant_of(^^fn)) == constant_of(^^fn));

struct C { void member(); static void static_member(); };
void deleted_function() = delete;
consteval bool rejects_function(info r) {
  try { (void)constant_of(r); }
  catch (const exception&) { return true; }
  return false;
}
static_assert(rejects_function(^^C::member));
static_assert(rejects_function(^^deleted_function));
static_assert(constant_of(^^C::static_member) == reflect_function(C::static_member));

int main() {}
