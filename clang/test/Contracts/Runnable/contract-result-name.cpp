// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe -o %t
// RUN: %t

#include "contracts.h"
#include "contracts-runtime.h"
#include "my_assert.h"

// Regression test for a real bug found by the Contracts Hardening epic
// (docs/CONTRACTS_HARDENING.md M3, 2026-09-05): in `post(r: ...)`, the
// result-name `r` read uninitialized stack memory instead of the actual
// return value, for any scalar (Direct/Extend ABI) return type -- which
// includes small (<=16 byte) class/struct types, not just int/double. Root
// cause: CodeGenFunction::EmitFunctionEpilog's scalar-return path
// (CGCall.cpp) finds the store into the `ReturnValue` alloca and erases it
// as a legitimate optimization (turning the memory round-trip into a pure
// SSA value), but EmitPostContracts ran *after* that erasure and
// EmitDeclRefLValue's ResultNameDecl case unconditionally read the
// now-dead `ReturnValue` alloca.
//
// This file is the one originally named for this exact feature; before the
// fix it was `-fsyntax-only` with an empty `main()` (a "looks executable,
// isn't" test -- see this epic's coverage-gate discussion) and so never
// caught the bug it was named for.

int g_calls = 0;

int scalar_int(const int x) post(r : r == x) {
  ++g_calls;
  return x;
}

double scalar_double(const double x) post(r : r == x) {
  ++g_calls;
  return x;
}

struct SmallPod { int a; int b; }; // 8 bytes: still Direct ABI, not sret.

SmallPod small_pod(const int a, const int b) post(r : r.a == a && r.b == b) {
  ++g_calls;
  return SmallPod{a, b};
}

struct LargeAggregate { long a, b, c, d; }; // > 16 bytes: sret ABI.

LargeAggregate large_aggregate(const long v) post(r : r.a == v && r.d == v) {
  ++g_calls;
  return LargeAggregate{v, v, v, v};
}

void handle_contract_violation(const std::contracts::contract_violation &violation) {
  fprintf(stderr, "unexpected contract violation: %s\n", violation.comment());
  __builtin_abort();
}

int main() {
  assert(scalar_int(42) == 42);
  assert(scalar_double(2.5) == 2.5);

  SmallPod sp = small_pod(3, 4);
  assert(sp.a == 3 && sp.b == 4);

  LargeAggregate la = large_aggregate(7);
  assert(la.a == 7 && la.d == 7);

  assert(g_calls == 4);
  return 0;
}
