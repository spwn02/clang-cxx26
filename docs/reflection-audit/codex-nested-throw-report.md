# Nested consteval throw audit

Date: 2026-09-09

## Result

The reported nested-call failure is not present at the current source and tool state. No
compiler fix was made. The apparent failure was caused by a stale staged libc++ header, not by
`EvalInfo::PendingException` propagation.

The first manual test used `build-libcxx/include/c++/v1/meta` before refreshing the staged
headers. It diagnosed `std::meta::annotations_of_with_type` as missing, although both the source
header and current staged header define it. Running the libc++ test wrapper refreshed the staged
headers. Re-running the same kind of test then passed.

## Reproduction matrix

Using `clang++ -nostdinc++ -I build-libcxx/include/c++/v1 -I libcxx/test/support -std=c++26
-freflection -fsyntax-only`, the matrix passed with all of these assertions:

* direct throw/catch inside one consteval function;
* one-level `A -> B`, where `B` throws and `A` catches;
* two-level `caller -> A -> B`, caught by the outer caller;
* ordinary `constexpr` code calling the consteval catcher;
* `std::meta::has_inaccessible_bases` throwing `std::meta::exception` from its wrapper;
* that wrapper throw propagating through two consteval calls and being caught by a typed
  `std::meta::exception` handler;
* the same wrapper path caught by `catch (...)`;
* both `static_assert` and ordinary constant-evaluation contexts.

## Evaluator analysis

The relevant implementation is coherent for the tested paths:

* `EvalInfo::PendingException` is a single optional state in the shared `EvalInfo`; user function
  calls do not create a second `EvalInfo`.
* `VisitCXXThrowExpr` evaluates the operand, captures call-stack notes, stores the exception, and
  returns failure without diagnosing it.
* `HandleFunctionCall` returns that failure while native `CallStackFrame` and `ScopeRAII` objects
  unwind. `ScopeRAII` runs destructors when `PendingException` is set.
* `EvaluateStmt`'s `CXXTryStmtClass` case consumes the pending state, applies `HandlerCanCatch`,
  evaluates the matching handler, and restores the state if no handler matches.
* `VisitCXXMetafunctionExpr` and the `DiagFn` callback do not interfere with a pending exception
  on the tested library-wrapper path.

There is no evidence of a propagation, RAII, or handler-scoping bug in the current classic
evaluator. The initially observed symptom is explained by the documented staged-header
staleness gotcha. This audit does not validate the still-open compiler-side `DiagFn` to
`meta::exception` conversion work; it validates propagation once a library wrapper actually
throws a C++ exception object.

## Follow-up

Refresh staged libc++ headers before testing P3560R2 wrappers. No `ExprConstant.cpp` change or
full check-clang gate was warranted by this audit.
