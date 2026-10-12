// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -verify=expected,ref %s
// RUN: %clang_cc1 -std=c++26 -verify=expected,classic %s

void reads() {
  constexpr int a = 3; // expected-note 4 {{address of non-static constexpr variable 'a' may differ on each invocation}}
  struct Matrix { const int *p[2][2]; };
  constexpr Matrix matrix{{{nullptr, &a}, {nullptr, nullptr}}};
  struct Cell { const int *p; };
  struct Cells { Cell values[2]; };
  constexpr Cells cells{{{nullptr}, {&a}}};
  union Union { const int *p; int n; };
  constexpr Union pointer_union{&a};
  constexpr Union numeric_union{.n = 7};
  struct Base { const int *p; };
  struct Derived : Base { int n; }; // ref-note {{pointer to 'a' is not a constant expression}} ref-note {{in call to 'Base(derived)'}}
  constexpr Derived derived{{&a}, 7};
  auto lambda = [&] {
    constexpr Matrix matrix_copy = matrix; // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'a' is not a constant expression}} expected-note {{in call to 'Matrix(matrix)'}}
    constexpr Cells cells_copy = cells; // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'a' is not a constant expression}} expected-note {{in call to 'Cells(cells)'}} ref-note {{in call to 'Cell(cells.values[1])'}}
    constexpr Union union_copy = pointer_union; // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'a' is not a constant expression}} classic-note {{in call to 'Union(pointer_union)'}}
    constexpr Union numeric_copy = numeric_union;
    static_assert(numeric_copy.n == 7);
    constexpr Derived derived_copy = derived; // expected-error {{must be initialized by a constant expression}} classic-note {{pointer to 'a' is not a constant expression}} expected-note {{in call to 'Derived(derived)'}}
  };
}
