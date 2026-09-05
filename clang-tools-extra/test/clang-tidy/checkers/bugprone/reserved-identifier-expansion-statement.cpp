// RUN: %check_clang_tidy %s bugprone-reserved-identifier %t -std=c++26 -- \
// RUN:   -fexpansion-statements

// Regression test for the Contracts Hardening epic's M5
// (docs/CONTRACTS_HARDENING.md): `template for`'s synthesized `__range`
// range-variable and `__N` non-type template parameter used to leak into
// this check because SemaExpand.cpp created them without marking them
// implicit, unlike range-based for's analogous synthesized decls. No
// warning should fire here -- this file should produce zero diagnostics.

constexpr int values[] = {1, 2, 3};

constexpr auto sum_values() -> int {
  int total = 0;
  template for (constexpr int v : values) {
    total += v;
  }
  return total;
}

static_assert(sum_values() == 6);
