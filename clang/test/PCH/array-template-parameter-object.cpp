// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// RUN: %clang_cc1 -std=c++26 -freflection -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -verify %s
// expected-no-diagnostics
#ifndef ARRAY_TPO_PCH
#define ARRAY_TPO_PCH
#include "../CodeGenCXX/Inputs/array-template-parameter-object.h"
#else
constexpr int again[] = {1, 2, 3};
constexpr auto restored = array_object(dims, 1, again, 3);
static_assert(restored == r);
static_assert(&extract<const int(&)[3]>(restored) == &reference);
static_assert(extract<const int*>(restored)[1] == 2);
using After = W<extract<const int(&)[3]>(restored)>;
static_assert(__is_same(After, Specialization));
constexpr int flat_again[] = {1, 2, 3, 4, 5, 6};
constexpr Leaf leaves_again[] = {{1}, {2}, {3}};
static_assert(array_object(ndims, 2, flat_again, 6) == nested);
static_assert(array_object(dims, 1, leaves_again, 3) == classes);
static_assert(extract<const int(*)[3]>(nested)[1][2] == 6);
static_assert(extract<const Leaf*>(classes)[2].value == 3);
const int* after_pch() { return extract<const int*>(restored); }
#endif
