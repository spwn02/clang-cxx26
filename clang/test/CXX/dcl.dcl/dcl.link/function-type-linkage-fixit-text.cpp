// RUN: not %clang_cc1 -std=c++26 -fsyntax-only -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s

// The note that says how to give a function C language linkage carries a fix-it that inserts
// extern "C" in front of the declaration (tools apply the fix-its of notes; -fixit does not).

extern "C" { typedef void (*CFnPtr)(int); }
void takes_c(CFnPtr);

void handler(int) {}
static void local(int) {}
namespace {
void hidden(int) {}
}

void use() {
  takes_c(handler);
  takes_c(&local);
  takes_c(hidden);
}

// CHECK: fix-it:"{{.*}}":{[[@LINE-12]]:1-[[@LINE-12]]:1}:"extern \"C\" "
// CHECK: fix-it:"{{.*}}":{[[@LINE-12]]:1-[[@LINE-12]]:1}:"extern \"C\" "
// CHECK: fix-it:"{{.*}}":{[[@LINE-11]]:1-[[@LINE-11]]:1}:"extern \"C\" "
