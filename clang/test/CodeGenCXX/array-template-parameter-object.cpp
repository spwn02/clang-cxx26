// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// RUN: %clang_cc1 -std=c++26 -freflection -triple x86_64-windows-msvc -emit-llvm -o - %s | FileCheck %s --check-prefix=MSVC
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// RUN: %clang_cc1 -std=c++26 -freflection -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s
#include "Inputs/array-template-parameter-object.h"
// CHECK: @_ZTA{{.*}} = linkonce_odr constant [3 x i32] [i32 1, i32 2, i32 3], comdat
const int* address() { return pointer; }
int read(int index) { return reference[index]; }
int specialization() { return Specialization::read(); }
// CHECK: define linkonce_odr {{.*}} @_ZN1WIL_ZTA{{.*}}4readEv
// MSVC: linkonce_odr dso_local constant [3 x i32] [i32 1, i32 2, i32 3], comdat
