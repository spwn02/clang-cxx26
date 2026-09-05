// RUN: clang-tidy %s -checks='-*,readability-identifier-naming' \
// RUN:   -config="{CheckOptions: {readability-identifier-naming.VariableCase: camelBack}}" \
// RUN:   -- -std=c++26 -fexpansion-statements 2>&1 | count 0

// Regression test for the Contracts Hardening epic's M5
// (docs/CONTRACTS_HARDENING.md): `template for`'s synthesized `__range`
// range-variable used to leak into readability-identifier-naming (it isn't
// camelBack) because SemaExpand.cpp created it without marking it implicit,
// unlike range-based for's analogous synthesized decls. No warning should
// fire here -- this file should produce zero diagnostics.

constexpr int values[] = {1, 2, 3};

constexpr auto sum_values() -> int {
  int total = 0;
  template for (constexpr int v : values) {
    total += v;
  }
  return total;
}

static_assert(sum_values() == 6);
