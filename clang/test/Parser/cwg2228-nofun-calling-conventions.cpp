// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -triple x86_64-unknown-linux-gnu -fms-extensions -Wno-ignored-attributes -Wno-cast-calling-convention %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -triple i686-pc-windows-msvc -fms-extensions -Wno-ignored-attributes -Wno-cast-calling-convention %s
// expected-no-diagnostics

// CWG2228: calling conventions, attributes and qualifiers in the parentheses
// of a nested declarator do not make the operand begin with a function
// declarator, so these remain type-ids.

struct A { int m(); };
void f(int);

auto b1 = (void (__stdcall *)(int))f;
auto b2 = sizeof(void (__cdecl *)());
auto b3 = sizeof(int (__fastcall A::*)());
auto b4 = (int (__vectorcall *)(int))nullptr;
auto b5 = (int (__thiscall A::*)())nullptr;
auto b6 = (void (__attribute__((stdcall)) *)(int))f;
auto b7 = (void (__attribute__((noreturn)) *)(int))f;
