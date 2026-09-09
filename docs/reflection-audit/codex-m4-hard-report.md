# Reflection M4 hard-issue audit

Date: 2026-09-09  
Fix commit: `706251663c1e` (`reflection: close M4 static-call and label gaps`)

Validation baseline used for this audit: 23 known failures — five established
consteval-escalation failures and 18 failures from the confirmed pre-existing
ASTUnit/libclang PCH-loading bug. Neither cluster was attributed to this work.

| Issue | Result | Effort |
|---|---|---:|
| #150 | Deferred after a fresh parser/Sema trace | ~25 min |
| #200 | Already fixed in current checkout; tracker corrected | ~20 min |
| #334 | Fixed | ~50 min |
| #146 | Fixed | ~35 min |

## #150 — spliced explicit destructor call

The `value.~[:^^test:]()` path reaches `ParseUnqualifiedId` as an ordinary
`IK_DestructorName`. `ParseReflect.cpp`'s existing splice path handles a
spliced member expression, but there is no destructor-name form that can carry
the splice, perform the required type/name lookup, and form the destructor call.
This is a parser/Sema/AST design change, not a safe local extension. It remains
deferred with the original issue, now with the missing representation identified
precisely.

## #200 — `parent_of` and class-template aliases

The stale deferral was incorrect for this checkout. A direct probe of
`static_assert(parent_of(^^T::A) == ^^T)` passes. `findTypeDecl` preserves the
top-level `UsingType` alias before the template-specialization fallback, so the
alias declaration layer is retained. No code change was needed.

## #334 — static member call and consteval-only object type

The diagnostic did not originate in the constant evaluator's lvalue check. It
came from Sema's `ConstevalOnly` bookkeeping: `MarkMemberReferenced` allowed the
discarded object expression of a non-arrow static member call to poison the
surrounding potentially-evaluated context. The fix removes a direct base
`DeclRefExpr` from that set for static member calls. Side effects in the base
remain normally analyzed. `static-member-consteval-only.cpp` covers the issue
reproducer.

## #146 — case/default labels in expansion statements

Added a dedicated diagnostic and parser checks in `ParseCaseStatement` and
`ParseDefaultStatement` while the current context is an expansion statement.
The existing Sema visitor was deliberately not used: it produced duplicate
diagnostics for `case`, and parser rejection prevents the raw-label CodeGen path
from being reached. `expansion-case-labels.cpp` covers both labels.

## Verification

Focused lit run passed 4/4, including both new tests and the existing expansion
and splice regressions. `ninja -C build-nyx -j2 clang` passed with
`CCACHE_DISABLE=1`; stale `c-index-test`, `clang-extdef-mapping`,
`clang-scan-deps`, and `clang-import-test` consumers were rebuilt serially.

A clean full `clang/test` run was started with `-j4` after removing generated
`Output` directories. It was stopped after the first 50 tests because the suite
hit the known `PR98671` baseline abort and nested lit invocations failed the
sandbox forkserver bind (`PermissionError`), making the run unsuitable as a
complete 23-failure comparison. No failure from either new reflection test
appeared before stopping.
