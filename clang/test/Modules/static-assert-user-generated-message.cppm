// RUN: rm -rf %t && mkdir %t && split-file %s %t
// RUN: %clang_cc1 -std=c++26 %t/m.cppm -emit-module-interface -o %t/m.pcm
// RUN: %clang_cc1 -std=c++26 %t/use.cpp -fmodule-file=m=%t/m.pcm -fsyntax-only -verify

// C++26 allows the static_assert message to be a constant expression with
// size() and data(); the AST reader must not assume a string literal.

//--- m.cppm
export module m;
struct Msg {
  constexpr unsigned long size() const { return 2; }
  constexpr const char *data() const { return "ok"; }
};
export constexpr int f() {
  static_assert(true, Msg{});
  return 1;
}
export template <class T> constexpr int g() {
  static_assert(sizeof(T) > 0, Msg{});
  return sizeof(T);
}
static_assert(true, Msg{});

//--- use.cpp
// expected-no-diagnostics
import m;
static_assert(f() == 1 && g<int>() == 4);
