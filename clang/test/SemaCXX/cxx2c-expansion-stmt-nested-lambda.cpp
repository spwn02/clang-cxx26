// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -verify %s
// expected-no-diagnostics

// A lambda inside two nested expansion statements inside a generic lambda used
// to assert in the constant evaluator (value-dependent capture initializer).
int generic_lambda() {
  auto g = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) {
      template for (auto y : {x, x}) {
        auto add = [&] { n += y; };
        add();
      }
    }
    return n;
  };
  return g(1);
}

int non_generic() {
  int n = 0;
  template for (auto x : {1, 2}) {
    template for (auto y : {x, x}) {
      auto add = [&] { n += y; };
      add();
    }
  }
  return n;
}
