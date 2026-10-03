//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [meta.reflection.result]: reflect_constant throws meta::exception unless the
// template-id TCls<V> would be valid. A pointer to a local object is not a
// valid template argument, whether it is the value itself or a member of a
// class value or an array element, so each of these must be catchable.
#include <meta>
#include <array>
using namespace std::meta;

struct K { const int *p; };
consteval bool plain_ptr_local() {
  int x = 1;
  try { (void)reflect_constant(&x); } catch (const exception &) { return true; }
  return false;
}
static_assert(plain_ptr_local());
consteval bool class_leaf_local() {
  int x = 1;
  try { (void)reflect_constant(K{&x}); } catch (const exception &) { return true; }
  return false;
}
static_assert(class_leaf_local());
consteval bool array_leaf_local() {
  int x = 1;
  try { (void)reflect_constant_array(std::array<K, 1>{K{&x}}); } catch (const exception &) { return true; }
  return false;
}
static_assert(array_leaf_local());

int main(int, char**) {}
