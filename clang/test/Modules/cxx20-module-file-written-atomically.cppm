// A module file must be replaced, not rewritten in place: a process that has the
// old file memory-mapped (clangd reading the build directory's module files while
// a build runs) would otherwise fault with SIGBUS when the file is truncated.
//
// REQUIRES: system-linux
//
// RUN: rm -rf %t && mkdir %t
// RUN: %clang_cc1 -std=c++20 -emit-module-interface -DV=1 %s -o %t/a.pcm
// RUN: ln %t/a.pcm %t/old.pcm
// RUN: %clang_cc1 -std=c++20 -emit-module-interface -DV=2 %s -o %t/a.pcm
// The hard link still names the first file: the second build created a new file.
// RUN: not cmp -s %t/old.pcm %t/a.pcm
// No temporary file is left behind.
// RUN: ls %t | FileCheck %s
// CHECK: a.pcm
// CHECK-NEXT: old.pcm
// CHECK-NOT: temp

export module a;
export int f() { return V; }
