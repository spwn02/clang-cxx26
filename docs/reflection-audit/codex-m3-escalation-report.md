# M3 consteval self-reference escalation audit

Date: 2026-09-10

## Result

No safe fix was found. All temporary compiler and test edits were reverted. The
worktree contains only this report.

## Reproduction baseline and old escalation patch

The exact change from `6b5f636e6ba1` was reapplied manually: C++23 constexpr and
constinit variable initializers were changed from
`ImmediateFunctionContext` to `PotentiallyEvaluated`, and the two additional
diagnostic expectations in `clang/test/Reflection/consteval-only-types.cpp`
were added.

The current regression set is larger than the historical nine-test list.

Clang suites (`clang/test/Reflection`, `clang/test/SemaCXX`, `clang/test/AST`,
`clang/test/CodeGenCXX`, and `clang/test/SemaTemplate`, 3431 tests):

- Baseline: 3381 passed, 39 unsupported, 6 expectedly failed, 5 failed.
- Reapplied patch: 3382 passed, 39 unsupported, 6 expectedly failed, 4 failed.
- The patch fixed the two known self-reference failures in
  `SemaCXX/builtin-is-within-lifetime.cpp` and
  `SemaCXX/constant-expression-cxx11.cpp`.
- New failure: `Reflection/reflection-wording-examples.cpp`.
- Remaining failures were the baseline `SemaCXX/PR98671.cpp`,
  `SemaCXX/cxx2a-constexpr-dynalloc.cpp`, and
  `SemaCXX/cxx2b-consteval-propagate.cpp`.

The libc++ wrapper command covered 108 discovered tests across
`libcxx/test/std/experimental/reflection/` and the requested meta paths:

- Baseline: 100 passed, 1 unsupported, 7 failed.
- Reapplied patch: 75 passed, 1 unsupported, 32 failed.
- Exact newly failing regression set (25):

  - `base-splice.verify.cpp`
  - `define-aggregate.verify.cpp`
  - `description-of-template-kinds.verify.cpp`
  - `m5-p2996-batch1.verify.cpp`
  - `m5-p2996-batch10.verify.cpp`
  - `m5-p2996-batch11.verify.cpp`
  - `m5-p2996-batch2.verify.cpp`
  - `m5-p2996-batch3.verify.cpp`
  - `m5-p2996-batch7.verify.cpp`
  - `m5-p2996-batch8.verify.cpp`
  - `m5-p2996-batch9.verify.cpp`
  - `m5-p2996-p3491-p3560-p3795-batch16.verify.cpp`
  - `m5-p3795-batch14.verify.cpp`
  - `member-classification.pass.cpp`
  - `module-imports.sh.cpp`
  - `new-3-annotations-with-type.verify.cpp`
  - `new-4-closure-inaccessible.verify.cpp`
  - `p3096-fn-parameters.pass.cpp`
  - `p3394-annotations.pass.cpp`
  - `p3394-parameter-annotations.pass.cpp`
  - `reflect-invoke.pass.cpp`
  - `substitute-nested-dependent.pass.cpp`
  - `substitute.verify.cpp`
  - `template-arguments.pass.cpp`
  - `to-and-from-values.verify.cpp`

The seven baseline failures were exactly the documented warning/verify set:
`attributed-function-type-queries.pass.cpp`, `entity-proxies.pass.cpp`,
`entity-proxy-member-queries.pass.cpp`, `m5-p2996-batch13.verify.cpp`,
`m5-p2996-p3096-batch12.verify.cpp`, `m5-p3491-batch15.verify.cpp`, and
`namespace-reflection-equality-reopened.pass.cpp`.

## Initialization timing hypothesis

Hypothesis (a) was confirmed. `ParseDecl.cpp:2551-2563` constructs the
initializer RAII scope; its destructor calls
`ActOnCXXExitDeclInitializer` before the parser calls
`AddInitializerToDecl` at `ParseDecl.cpp:2704`. `SemaDecl.cpp:13842` and the
following initializer path perform `VDecl->setInit(Init)` only in
`AddInitializerToDecl`. Therefore, at
`SemaDeclCXX.cpp:19095-19116`/`19120`, when the expression context is popped,
the `VarDecl` has no attached initializer. `Decl.cpp:2552-2568` consequently
returns false from `isUsableInConstantExpressions()` when `getAnyInitializer`
returns null; `hasConstantInitialization()` cannot provide a reliable
replacement at this point either.

This explains why the existing `HandleImmediateInvocations` bailout at
`SemaExpr.cpp:18404-18421` does not recognize the ordinary constexpr reflection
idiom after the push is removed.

## Attempted candidate-level design

One temporary attempt changed `SemaExpr.cpp` as follows:

1. Permit immediate-invocation tracking in a constexpr/constinit variable
   initializer even while its context carries `ImmediateFunctionContext`.
2. Evaluate each `ImmediateInvocationCandidate` before the
   `ConstevalOnly` loop and remember candidates whose evaluation succeeded.
3. Diagnose failed candidates, but skip `err_expr_consteval_only_type` for a
   candidate that had already evaluated successfully.
4. Apply the manifestly-constant-evaluated bailout using the syntactic
   `VarDecl::isConstexpr()`/`ConstInitAttr` signal rather than attached-init
   queries.

This was implemented around `SemaExpr.cpp:18144-18224` and
`18400-18560`, then built successfully. It was rejected by focused lit:

- 24 discovered Clang tests: 19 passed, 5 failed test cases (the reflection
  test has two RUN lines and appeared twice).
- Failures: `Reflection/consteval-only-types.cpp`,
  `SemaCXX/builtin-is-within-lifetime.cpp`,
  `SemaCXX/constant-expression-cxx11.cpp`,
  `SemaCXX/cxx2a-constexpr-dynalloc.cpp`, and
  `SemaCXX/cxx2b-consteval-propagate.cpp`.
- The attempt produced spurious “expressions of consteval-only type” errors in
  ordinary non-consteval reflection declarations and altered expected warning
  behavior in the C++23 lifetime test. This is not a safe basis for refinement
  without a more carefully separated context state.

The failure demonstrates that a candidate-success bit alone is insufficient:
the context flag also controls other immediate/consteval-only diagnostics and
must not be globally relaxed for every variable-initializer record.

## Self-reference repros confirmed

- `clang/test/SemaCXX/builtin-is-within-lifetime.cpp` has C++23 and C++26 RUN
  lines. The self-reference diagnostics are in the `self` initializer around
  lines 187-192, plus the related NSDMI case around lines 232-242.
- `clang/test/SemaCXX/constant-expression-cxx11.cpp:2015-2017` contains
  `constexpr int &n = n;` and the C++23 expected consteval-call diagnostic.

The wholesale patch made these two diagnostics appear, but at the documented
cost of the 25 current libc++ regressions and one Clang reflection regression.

## Final state

No compiler or test expectation change from either experiment remains. The
current tree is restored to the pre-attempt baseline. A future fix needs a
distinct per-initializer mechanism that preserves ordinary immediate-context
and consteval-only diagnostics while exposing failed immediate-invocation
candidates for self-reference; changing the existing context push or bailout
alone is insufficient.
