# M4 Batch 6 Reflection Closure Report

Date: 2026-09-09

## Fixed

- **#169** — Reproduced the assertion in
  `CheckIfAnyEnclosingLambdasMustCaptureAnyPotentialCaptures` with the exact NBTV repro. An
  expansion statement can leave the current lambda scope on `FunctionScopes` while `CurContext`
  refers to the enclosing expansion context. The potential-capture walk now returns for this
  unsynchronized post-parameter-list state. Added `clang/test/Reflection/issue-169.cpp`.
  Rebuilt-compiler direct probe passes.
- **#346** — Reproduced the false `-Wreflexing-parse` warning for
  `^^decltype(std::move(1))`. The parser now checks that the actual raw trailing token is `&&`
  before warning. The reported case is clean; direct `^^int&& != ^^int` still warns. Added
  `clang/test/Reflection/issue-346.cpp`.

No commit IDs are available: this environment exposes `.git` read-only, and Git cannot create
`.git/index.lock`; consequently commit and push could not be performed.

## Confirmed genuinely open and deferred

- **#180** — Exact repro still compiles and instantiates `test_impl<int>` while ignoring its
  dependent `static_assert(false)`. This is deferred-instantiation behavior and needs the broader
  expansion-body deferral design associated with PR #261, not a safe local diagnostic tweak.
- **#181** — Exact repro still attempts to copy `const tuple<unique_ptr<...>>` before iterating,
  producing the deleted-copy-constructor diagnostic. Correct range-based expansion lowering needs
  to preserve the non-copyable range/reference path; this is coupled to the #180/#261 expansion
  work.
- **#188** — Exact `ranges::max_element` sequence still rejects `display_string_of(dealias(...))`
  cases as non-constant expressions in the reflection pretty-printer/metafunction path. The
  failure affects template-heavy iterator types and requires evaluator/printer investigation;
  no narrow safe fix was established.
- **#220** — Exact `display_string_of(type_of(undeduced))` repro still exceeds a 20-second timeout.
  This remains a likely infinite-recursion path around undeduced return types, plausibly involving
  `return_type_of` handling in `ExprConstantMeta.cpp`; it needs a dedicated traced evaluator fix.
- **#237** — Exact closure-type alias splice still diagnoses `'auto' not allowed in type alias`.
  Reconstructing an invented closure type through `typename[:cr:]` requires deeper alias/type
  reconstruction work; no safe local patch was identified.

## Confirmed not actually a reflection bug

- **#221** — Exact repro fails in libc++ `expected` constraint normalization while copying two
  `std::expected<bool, int>` values into a `std::vector`. The diagnostics recurse through
  `expected`/reverse-iterator `operator==` constraints; the source contains no reflection and
  the failure is independent of the reflection implementation. Leave outside the reflection M4
  backlog.

## Backlog

The final M4 Confirmed-Open reflection backlog is **6**: #150, #180, #181, #188, #220, and #237.
#169 and #346 are fixed pending commit/push once Git metadata is writable. #221 is removed as
not-a-reflection-bug. #239 and #334 were already fixed before this batch.

The requested full `check-clang` gate was not completed in this environment. The focused direct
rebuilt-compiler probes for #169 and #346 passed; `llvm-lit` could not start because the sandbox
forbids Python's `forkserver` operation.
