// RUN: %clang_cc1 -std=c++2c -emit-llvm -o - %s | FileCheck %s --check-prefix=CHECK
// RUN: %clang_cc1 -std=c++2c -O0 -S -o - %s | FileCheck %s --check-prefix=MACHINE
// RUN: %clang_cc1 -std=c++2c -O2 -S -o - %s | FileCheck %s --check-prefix=MACHINE
// RUN: %clang_cc1 -std=c++2c -O3 -S -o - %s | FileCheck %s --check-prefix=MACHINE
// RUN: %clang_cc1 -std=c++2c -O3 -debug-info-kind=standalone -fprofile-instrument=clang -emit-llvm -o - %s | FileCheck %s --check-prefix=PROFILEDEBUG
// RUN: %clang_cc1 -std=c++2c -emit-llvm -o - %s | FileCheck %s --check-prefix=IO
// RUN: %clang_cc1 -std=c++2c -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=IO
// RUN: %clang_cc1 -std=c++2c -O3 -emit-llvm -o - %s | FileCheck %s --check-prefix=IO
// RUN: %clang_cc1 -std=c++23 -emit-llvm -o - %s | FileCheck %s --check-prefix=IO23
//
// This file is distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

extern "C" int printf(const char *, ...);
extern "C" int putchar(int);
extern "C" int getchar();
extern "C" int wprintf(const wchar_t *, ...);

void checkpoint() {
  __builtin_observable_checkpoint();
}

extern "C" void checkpoint_c_linkage() {
  __builtin_observable_checkpoint();
}

// CHECK: call void @llvm.observable.checkpoint()

// MACHINE-LABEL: checkpoint_c_linkage:
// MACHINE-NOT: llvm.observable.checkpoint

// PROFILEDEBUG-LABEL: define{{.*}} @checkpoint_c_linkage
// PROFILEDEBUG: load i64, ptr @__profc_checkpoint_c_linkage
// PROFILEDEBUG: {{(call|tail call)}} void @llvm.observable.checkpoint(), !dbg

void io_checkpoint() {
  printf("observable output\n");
}

// IO-LABEL: define{{.*}} @{{.*}}io_checkpoint
// IO: {{(call|tail call) i32 .*@(printf|puts)}}
// IO-NEXT: {{(call|tail call)}} void @llvm.observable.checkpoint()

void builtin_io_checkpoint() {
  __builtin_printf("observable builtin output\n");
}

// IO-LABEL: define{{.*}} @{{.*}}builtin_io_checkpoint
// IO: {{(call|tail call) i32 .*@(printf|puts)}}
// IO-NEXT: {{(call|tail call)}} void @llvm.observable.checkpoint()

void fortified_builtin_io_checkpoint() {
  __builtin___printf_chk(1, "observable fortified output\n");
}

// IO-LABEL: define{{.*}} @{{.*}}fortified_builtin_io_checkpoint
// IO: {{(call|tail call) i32 .*@__printf_chk}}
// IO-NEXT: {{(call|tail call)}} void @llvm.observable.checkpoint()

// IO23-LABEL: define{{.*}} @{{.*}}io_checkpoint
// IO23: call i32 (ptr, ...) @printf
// IO23-NOT: call void @llvm.observable.checkpoint()

int stdio_characters() {
  putchar('x');
  int ch = getchar();
  wprintf(L"%lc", L'x');
  return ch;
}

// IO-LABEL: define{{.*}} @{{.*}}stdio_characters
// IO: call i32 @putchar
// IO-NEXT: {{(call|tail call)}} void @llvm.observable.checkpoint()
// IO23-LABEL: define{{.*}} @{{.*}}stdio_characters
// IO23-NOT: call void @llvm.observable.checkpoint()
// IO: call i32 @getchar
// IO-NEXT: {{(call|tail call)}} void @llvm.observable.checkpoint()
// IO: call i32 (ptr, ...) @wprintf
// IO-NEXT: {{(call|tail call)}} void @llvm.observable.checkpoint()
