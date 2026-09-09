# Reflection M4 batch 5

Date: 2026-09-09

## Completed

- PR #166: corrected `is_reflection_type` to compare de-aliased reflections and added an alias regression test.
- PR #340: added `std::meta::has_c_language_linkage`, including compiler dispatch, declaration/variable/function-typedef handling, and focused tests.
- PR #244: preserved a unique function template when invented-`auto` deduction fails, fixing issue #239's closure `operator()` false-overload diagnostic.

Commit: `6bbb1c0cfbea` (`reflection: close linkage and closure reflection gaps`). Pushed to `origin/cxx26`; `git log --oneline origin/cxx26..HEAD` is empty.

## Verification

- Focused libc++ reflection tests: 2/2 passed.
- Clang Reflection suite: 20/20 passed.
- Full Clang gate: 44,596 passed, 25 expected failures, 26 failures. The 23 documented baseline failures remain unchanged (five consteval-escalation tests and 18 ASTUnit/libclang PCH failures). Three additional helper-test failures are Python 3.14 forkserver restrictions in this sandbox, not compiler failures.

## Remaining open work

PR #261 and #168 were not attempted in this batch. #261 remains a broad AST/Sema/TreeTransform change for #180/#181; #168 requires a wording-correct P3491R3 implementation rather than porting its known-incomplete upstream draft. No fix was attempted for the no-ready-made-PR issues #169, #180, #181, #188, #220, #221, #237, or #346. #150 remains deferred with its existing parser/Sema/AST rationale. #345 remains skipped because #184 still lacks a buildable reproducer. P3560R2 strategy 2 was not touched.

Confirmed-Open backlog is now 9 items: #150, #169, #180, #181, #188, #220, #221, #237, and #346. Issue #239 is closed by the PR #244 port.
