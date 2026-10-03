// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -emit-module-interface %t/M.cppm -o %t/M.pcm
// RUN: %clang_cc1 -std=c++26 -fmodule-file=M=%t/M.pcm -fsyntax-only -verify %t/use.cpp
// RUN: %clang_cc1 -std=c++26 -fmodule-file=M=%t/M.pcm -emit-llvm -o /dev/null %t/use.cpp

//--- M.cppm
export module M;
export inline constexpr int f(int n) {
  constexpr int a = 3;
  constexpr const int &r = a;
  constexpr const int &t = 42;
  static_assert(r == 3 && t == 42);
  int arr[2] = {n, 4};
  constexpr int *p = arr;
  auto l = [&] { return r + t + *p; };
  return l();
}
export inline consteval int g() { constexpr const int &t = 42; return t; }
//--- use.cpp
// expected-no-diagnostics
import M;
static_assert(f(2) == 47 && g() == 42);
int runtime(int n) { return f(n); }
