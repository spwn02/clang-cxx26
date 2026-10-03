// RUN: %clang_cc1 -std=c++26 -x c++-header -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++26 -include-pch %t -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -include-pch %t -emit-llvm -o /dev/null %s
// expected-no-diagnostics
#ifndef HEADER
#define HEADER
inline constexpr int f(int n) {
  constexpr int a = 3;
  constexpr const int &r = a;
  constexpr const int &t = 42;
  static_assert(r == 3 && t == 42);
  int arr[2] = {n, 4};
  constexpr int *p = arr;
  auto l = [&] { return r + t + *p; };
  return l();
}
inline consteval int g() { constexpr const int &t = 42; return t; }
#else
static_assert(f(2) == 47 && g() == 42);
int runtime(int n) { return f(n); }
#endif
