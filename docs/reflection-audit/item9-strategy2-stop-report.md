# Item 9 — P3560R2 strategy 2 stop report

Date: 2026-09-11

## Result

Stopped without a partial implementation.  Item 9 still needs an evaluator design
that can construct a `std::meta::exception` APValue for compiler-originated
metafunction failures.  Converting only some `DiagFn` sites would violate the
epic completion bar.

## Investigation reproduced in this session

Read in full: `codex-strategy2-design.md`, `codex-strategy2-pilot-report.md`,
the item-9/Next-Up/session-log material in `REFLECTION_CLOSEUP.md`, and
`AGENTS.md`'s baseline section.

Current source independently confirms both blockers:

* `CXXMetafunctionExpr::ImplFn` accepts only `EvaluateFn` and `DiagnoseFn`
  (`clang/include/clang/AST/ExprCXX.h:5605-5612`); the evaluator invocation at
  `ExprConstant.cpp:9755` has no API that accepts a `StringRef` plus reflection
  `APValue` and installs a pending exception.
* `PendingExceptionInfo` stores only `ThrowExpr`, and the reference-catch path
  unconditionally casts it to `CXXThrowExpr` (`ExprConstant.cpp:6670`).  Thus a
  compiler-originated failure cannot use the ordinary reference catch path.

I also inspected the retained implementation-attempt report personally.  Its
focused pilot reached the real assertion in `extractSubobject` through repeated
`HandleConstructorCall` / `VisitCXXInheritedCtorInitExpr` frames when it tried
to evaluate a synthesized `CXXConstructExpr`; this is a distinct, documented
evaluator abort, not a diagnostic failure.  The only source-level exception
test was checked with the project harness (`libcxx/.../exception.pass.cpp`);
my standalone AST-dump invocation was invalid because it omitted staged
`__config_site`, and is deliberately not treated as reproduction evidence.

## Different construction angles considered

An evaluator-owned synthetic AST constructor reuses exactly the aborting path.
Hand-populating `optional<string>`, `u8string`, `info`, and `source_location`
would make compiler code depend on libc++ private APValue/layout details and is
not a valid full implementation.  Altering `meta::exception` to a trivial
layout would be an ABI/semantic redesign and still requires stable string
object identity for `what()` and reference catches.

Required next implementation is therefore a real evaluator primitive that
constructs the object without re-entering synthesized inherited constructors,
plus a separate stable object key in `PendingExceptionInfo`.  It must be proven
on `members_of(^^int, access_context::unchecked())` before converting all 24
backend targets.

## Commands run

* `sed`/`rg` source audits listed above: confirmed both interface gaps.
* `./build-nyx/bin/clang++ ... -I libcxx/include -nostdinc++ -Xclang -ast-dump`:
  invalid harness invocation; failed for missing `__config_site`, then hit an
  unrelated value-dependent evaluator assertion.  Excluded from conclusions.
* `./build-nyx/bin/llvm-lit libcxx/test/.../exception.pass.cpp`: rejected by
  libc++ configuration (must use `libcxx/utils/libcxx-lit`); no test result
  claimed.

No compiler/library source changed; no full regression gate is applicable.
