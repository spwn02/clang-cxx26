// RUN: %clang_cc1 -std=c++26 -freflection -triple x86_64-unknown-linux-gnu -verify -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -freflection -triple x86_64-unknown-linux-gnu -verify -emit-obj -o %t.o %s
// RUN: %clang_cc1 -std=c++26 -freflection -triple x86_64-unknown-linux-gnu -verify -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++26 -freflection -triple x86_64-unknown-linux-gnu -verify -include-pch %t.pch -emit-llvm -o - %s | FileCheck %s
// expected-no-diagnostics

#ifndef NULL_REFLECTION_H
#define NULL_REFLECTION_H

using info = decltype(^^int);
extern constexpr info scalar = {};
struct S { info value; int tag; };
extern constexpr S aggregate = {{}, 42};
extern constexpr info array[2] = {};

// CHECK: @scalar = constant i128 0, align 16
// CHECK: @aggregate = constant %struct.S { i128 0, i32 42 }, align 16
// CHECK: @array = constant [2 x i128] zeroinitializer, align 16

#endif
