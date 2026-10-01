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
namespace NS {}
int x;
int& ref = x;
struct B {};
struct D : B { static int s; int m; };
struct Abstract : virtual B { virtual void f() = 0; };
struct Concrete : virtual B {};

consteval bool bases_namespace() {
  try { (void)(std::meta::bases_of(^^NS, std::meta::access_context::unchecked())); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(bases_namespace());

consteval bool static_namespace() {
  try { (void)(std::meta::static_data_members_of(^^NS, std::meta::access_context::unchecked())); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(static_namespace());

consteval bool nonstatic_namespace() {
  try { (void)(std::meta::nonstatic_data_members_of(^^NS, std::meta::access_context::unchecked())); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(nonstatic_namespace());

consteval bool symbol_invalid() {
  try { (void)(std::meta::symbol_of((std::meta::operators)9999)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(symbol_invalid());

consteval bool u8symbol_invalid() {
  try { (void)(std::meta::u8symbol_of((std::meta::operators)9999)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(u8symbol_invalid());

consteval bool alignment_ref() {
  try { (void)(std::meta::alignment_of(^^ref)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(alignment_ref());


static_assert(std::meta::bases_of(^^D, std::meta::access_context::unchecked()).size() == 1);
static_assert(std::meta::static_data_members_of(^^D, std::meta::access_context::unchecked()).size() == 1);
static_assert(std::meta::nonstatic_data_members_of(^^D, std::meta::access_context::unchecked()).size() == 1);
static_assert(std::meta::symbol_of(std::meta::op_plus) == "+");
static_assert(std::meta::u8symbol_of(std::meta::op_comma) == u8",");
static_assert(std::meta::alignment_of(^^x) == alignof(int));
static_assert(std::meta::alignment_of(^^int&) == alignof(int*));

consteval bool abstract_virtual_base() {
  try {
    (void)std::meta::offset_of(std::meta::bases_of(
        ^^Abstract, std::meta::access_context::unchecked())[0]);
  } catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(abstract_virtual_base());
static_assert(std::meta::offset_of(std::meta::bases_of(
    ^^D, std::meta::access_context::unchecked())[0]).bytes == 0);
static_assert(std::meta::offset_of(std::meta::bases_of(
    ^^Concrete, std::meta::access_context::unchecked())[0]).bytes >= 0);

consteval bool constant_local_pointer() {
  int local = 1;
  try { (void)std::meta::reflect_constant(&local); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(constant_local_pointer());
consteval bool array_local_pointer() {
  int local = 1;
  int* pointers[] = {&local};
  try { (void)std::meta::reflect_constant_array(pointers); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(array_local_pointer());
consteval bool static_object_local_pointer() {
  int local = 1;
  try { (void)std::define_static_object(&local); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(static_object_local_pointer());
static_assert(std::meta::extract<int*>(std::meta::reflect_constant(&x)) == &x);
static_assert(*std::define_static_object(&x) == &x);

int main() {}
