// RUN: %clang_cc1 -std=c++26 -freflection -fsyntax-only -verify %s

constexpr struct {
  decltype(^^::) _;
  static auto f(int &) {}
} a{};

void test() {
  int i;
  a.f(i); // expected-no-diagnostics
}
