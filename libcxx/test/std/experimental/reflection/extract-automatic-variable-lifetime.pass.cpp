//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [meta.reflection.extract], extract-ref: a variable whose lifetime began within
// the evaluation can be extracted, whereas the variable of a frame that is no
// longer live cannot.
#include <meta>
using namespace std::meta;
consteval int in_frame() { int x = 3; int &r = extract<int &>(^^x); r = 9; return x; }
static_assert(in_frame() == 9);
consteval info leak() { int x = 1; return ^^x; }          // reflection of a dead frame's variable
consteval bool dead_throws() {
  try { (void)extract<int &>(leak()); } catch (const exception &) { return true; }
  return false;
}
static_assert(dead_throws());
consteval bool dead_throws_rv() {
  try { (void)extract<int &&>(leak()); } catch (const exception &) { return true; }
  return false;
}
static_assert(dead_throws_rv());
int main(int, char**) {}
