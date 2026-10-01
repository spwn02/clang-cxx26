//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
struct B {};
union U {};
struct D : B {};
namespace NS {}
template<class> struct TT {};
template<class> void FT();
int x;
thread_local int tls;
enum E { first, second };
constexpr int constant = 42;

consteval bool enumerators_class() {
  try { (void)(std::meta::enumerators_of(^^B)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(enumerators_class());

consteval bool enumerators_union() {
  try { (void)(std::meta::enumerators_of(^^U)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(enumerators_union());

consteval bool parameters_function_template() {
  try { (void)(std::meta::parameters_of(^^FT)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(parameters_function_template());

consteval bool parameters_class_template() {
  try { (void)(std::meta::parameters_of(^^TT)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(parameters_class_template());

consteval bool object_thread_local() {
  try { (void)(std::meta::object_of(^^tls)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(object_thread_local());

consteval bool can_namespace() {
  try { (void)(std::meta::can_substitute(^^TT, {^^NS})); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(can_namespace());

consteval bool can_null() {
  try { (void)(std::meta::can_substitute(^^TT, {std::meta::info{}})); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(can_null());

consteval bool can_base() {
  try { (void)(std::meta::can_substitute(^^TT, {std::meta::bases_of(^^D, std::meta::access_context::unchecked())[0]})); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(can_base());


static_assert(std::meta::enumerators_of(^^E).size() == 2);
static_assert(std::meta::can_substitute(^^TT, {^^int}));
static_assert(!std::meta::can_substitute(^^TT, {std::meta::reflect_constant(1)}));
static_assert(std::meta::extract<const int&>(^^constant) == 42);
static_assert(std::meta::is_object(std::meta::object_of(^^x)));
consteval bool local_reference() {
  int local = 7;
  return &std::meta::extract<int&>(^^local) == &local;
}
static_assert(local_reference());

int main() {}
