// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// Regression test for a crash found by the Contracts Hardening epic
// (docs/CONTRACTS_HARDENING.md M1/M2, 2026-09-05): with -fcontracts enabled,
// Sema::CheckCompleteVariableDeclaration unconditionally calls
// VarDecl::recheckForConstantInitialization to satisfy [intro.compliance]p2's
// requirement to diagnose a contract violation during constant
// initialization. VarDecl::checkForConstantInitialization's early return for
// a described variable-template pattern (`if (getDescribedVarTemplate())
// return true;`) never populates the EvaluatedStmt's WasEvaluated /
// HasConstantInitialization fields, but the caller still treats that `true`
// as license to call recheckForConstantInitialization, whose assertion
// expects those fields to already be populated -- crashing on essentially
// any variable-template pattern with an initializer. This is a Sema-only
// crash regression (bug is at parse/Sema time, not at runtime), so this test
// is intentionally -fsyntax-only with no main() -- see
// docs/CONTRACTS_HARDENING.md's anti-regression rule for what does and does
// not require an executing test.
//
// libc++'s <utility> hits this via __type_traits/is_referenceable.h's
// `template <class _Tp, class = void> inline const bool __is_referenceable_v
// = false;`, so before the fix this crashed on nearly any -fcontracts
// translation unit that includes so much as <utility>.

// The crash fires on parsing the pattern's own declaration -- no
// instantiation is required to reproduce it.
template <class T, class = void>
inline const bool is_referenceable_v = false;
// expected-no-diagnostics
