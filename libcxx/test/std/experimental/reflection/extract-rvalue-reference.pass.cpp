//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [meta.reflection.extract]: extract<T> for a reference type T returns
// extract-ref<T>(r), a reference to the object; this includes rvalue references.
#include <meta>
#include <type_traits>

using namespace std::meta;

constexpr int g = 5;
int ng = 6;
struct S { int m = 7; static constexpr int sm = 8; };
constexpr S s;

static_assert(std::is_same_v<decltype(extract<const int&&>(^^g)), const int&&>);
static_assert(extract<const int&&>(^^g) == 5);
static_assert(&static_cast<const int&>(extract<const int&&>(^^g)) == &g);
static_assert(&extract<const int&>(^^g) == &g);

// Objects and variables of static storage; qualification conversion only.
static_assert(extract<const int&&>(^^S::sm) == 8);
static_assert(extract<const S&&>(^^s).m == 7);

// A non-const variable not usable in constant expressions: lifetime must have
// begun in the evaluation, which it did not.
consteval bool throws_for_ng() {
  try { (void)extract<int&&>(^^ng); } catch (const exception&) { return true; }
  return false;
}
static_assert(throws_for_ng());

// A value reflection is not an object: extracting a reference throws.
consteval bool throws_for_value() {
  try { (void)extract<const int&&>(reflect_constant(1)); } catch (const exception&) { return true; }
  return false;
}
static_assert(throws_for_value());

// Dropping a qualifier is not a qualification conversion.
consteval bool throws_for_dropped_const() {
  try { (void)extract<int&&>(^^g); } catch (const exception&) { return true; }
  return false;
}
static_assert(throws_for_dropped_const());

// Mismatched type.
consteval bool throws_for_wrong_type() {
  try { (void)extract<const long&&>(^^g); } catch (const exception&) { return true; }
  return false;
}
static_assert(throws_for_wrong_type());

// Lifetime began within the evaluation: a mutable local can be extracted.
consteval int local_object() {
  int x = 3;
  int&& r = extract<int&&>(^^x);
  return r;
}
static_assert(local_object() == 3);

// A reference variable: the object referred to by the variable is extracted.
constexpr const int &rg = g;
static_assert(&static_cast<const int&>(extract<const int&&>(^^rg)) == &g);

// Adding const is a qualification conversion.
consteval int local_adds_const() {
  int x = 4;
  const int&& r = extract<const int&&>(^^x);
  return r;
}
static_assert(local_adds_const() == 4);

// A reference variable: only a qualification conversion from the referenced
// type is allowed, for lvalue and rvalue reference targets alike.
static_assert(&extract<const int&>(^^rg) == &g);
static_assert(extract<const int&&>(^^rg) == 5);
consteval bool reference_variable_throws() {
  try { (void)extract<int&>(^^rg); return false; } catch (const exception&) {}
  try { (void)extract<const long&&>(^^rg); return false; } catch (const exception&) {}
  return true;
}
static_assert(reference_variable_throws());

// A local reference variable of a live frame: the object it is bound to is
// extracted (lifetime began within the evaluation); in a dead frame it throws.
consteval int local_reference_variable() {
  int x = 3;
  int& r = x;
  const int& c = extract<const int&>(^^r);
  const int&& rv = extract<const int&&>(^^r);
  x = 4;
  return c + rv;
}
static_assert(local_reference_variable() == 8);

consteval int reference_parameter(int& p) { return extract<int&>(^^p); }
consteval int call_reference_parameter() { int z = 9; return reference_parameter(z); }
static_assert(call_reference_parameter() == 9);

consteval info dead_reference() {
  int y = 1;
  int& q = y;
  return ^^q;
}
consteval bool dead_reference_throws() {
  try { (void)extract<int&>(dead_reference()); } catch (const exception&) { return true; }
  return false;
}
static_assert(dead_reference_throws());

int main(int, char**) {}
