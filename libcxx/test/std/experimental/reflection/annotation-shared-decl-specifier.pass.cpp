// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS:
// ADDITIONAL_COMPILE_FLAGS:

// [meta.reflection.annotation] example: an annotation of a decl-specifier-seq is
// one annotation for every declarator, each written annotation is a new one.

#include <meta>
using namespace std::meta;

[[=1]] int x, y;
static_assert(annotations_of(^^x).size() == 1 && annotations_of(^^y).size() == 1);
static_assert(annotations_of(^^x)[0] == annotations_of(^^y)[0]);

struct S {
  [[=2]] int a, b;
  [[=3]] int c;
  [[=3]] int d;
};
static_assert(annotations_of(^^S::a)[0] == annotations_of(^^S::b)[0]);
static_assert(annotations_of(^^S::c)[0] != annotations_of(^^S::d)[0]);

int main(int, char**) { return 0; }
