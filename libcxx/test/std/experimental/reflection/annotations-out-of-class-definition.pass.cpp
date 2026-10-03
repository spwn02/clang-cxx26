//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [dcl.attr.annotation]: an annotation on a qualified out-of-class definition is
// allowed (host scope == target scope; only block-scope extern/function
// declarations and non-defining friends are excluded) and annotates the entity.
#include <meta>
struct C { static int sm; void f(); void g(int); struct In; };
[[=1]] int C::sm = 0;
[[=2]] void C::f() {}
void C::g([[=3]] int) {}
struct [[=4]] C::In {};
namespace N { extern int v; void fn(); }
[[=5]] int N::v = 0;
void N::fn [[=6]] () {}
static_assert(std::meta::annotations_of(^^C::sm).size() == 1);
static_assert(std::meta::annotations_of(^^C::f).size() == 1);
static_assert(std::meta::annotations_of(std::meta::parameters_of(^^C::g)[0]).size() == 1);
static_assert(std::meta::annotations_of(^^C::In).size() == 1);
static_assert(std::meta::annotations_of(^^N::v).size() == 1);
static_assert(std::meta::annotations_of(^^N::fn).size() == 1);
static_assert(std::meta::extract<int>(std::meta::annotations_of(^^C::f)[0]) == 2);
int main(int, char**) {}
