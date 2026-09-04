# Contracts Hardening Tracker

This document is the single source of truth for the Contracts hardening epic:
fixing a real runtime bug found in production use of the completed Contracts
(P2900R14) port, closing the test-coverage gap that let it ship, and closing
the developer-tooling gaps (clangd, clang-tidy) discovered alongside it. Read
it before `docs/CXX26_GAPS.md` while any milestone below is `[~]` or `[!]`.

Full context, root causes, and the approved implementation plan live at
`/home/spawn/.claude/plans/atomic-beaming-whale.md` (this machine only — not
committed). This tracker mirrors its milestone structure for anyone resuming
the epic without access to that plan file.

## Scope and Fixed Points

- Prior epic: Contracts (P2900R14) port, closed 2026-09-04, merged to `cxx26`,
  pushed and tagged `cxx26-2026.09.04.1`. See `docs/CXX26_GAPS.md`'s Scope
  section for that epic's summary (its own tracker,
  `docs/CONTRACTS_PORT.md`, was deleted per policy on completion).
- Trigger: using the tagged toolchain in a downstream project (Miracle)
  surfaced a runtime bug in `post(r: ...)` result-name binding that the entire
  prior test gate (41/41 contracts tests, clean check-clang/check-cxx) missed.
- Development branch: `cxx26` directly (small enough milestones; revisit if
  scope grows).
- Baseline commit before this epic's changes: `543a397a185d`.
- Working fork remote: `origin` (`https://github.com/spwn02/clang-cxx26.git`).

## State Legend

Same convention as prior epics: `[ ]` not started, `[~]` active (exactly one
at a time), `[x]` complete with required test gate passed, `[!]` blocked with
an actionable unblock condition recorded.

## Milestones

- [x] **M0 — Root-cause and scope.** Bug root-caused to
  `clang/lib/CodeGen/CGCall.cpp` (`EmitFunctionEpilog`/`EmitPostContracts`)
  and `CGExpr.cpp` (`EmitDeclRefLValue`'s `ResultNameDecl` handling): the
  store into the `ReturnValue` alloca is erased by a legitimate scalar-return
  optimization before `EmitPostContracts` runs, and `r`'s lvalue
  unconditionally reads that now-dead alloca. Root cause of the *test* gap:
  the only executing tests exercising `r`'s value went through the constant
  evaluator, not CodeGen; the only executing CodeGen-path test checked `r`'s
  address, not its value, on an empty class (`Direct` ABI, same broken path);
  the file literally named for this feature
  (`clang/test/Contracts/Runnable/contract-result-name.cpp`) was
  `-fsyntax-only` with an empty `main()`. Two adjacent tooling gaps
  root-caused alongside it: `clang-tools-extra/clang-tidy` false-positives on
  `template for`'s synthesized `__range`/`__N` decls (missing `setImplicit()`
  in `SemaExpand.cpp`, asymmetric with range-for's `BuildForRangeVarDecl`),
  and clangd never completing `pre`/`post`/`contract_assert` (zero mentions
  in `SemaCodeComplete.cpp`).
- [~] **M1 — Build-tree hardening and baseline.** `build-nyx` reconfigured
  with `LLVM_ENABLE_ASSERTIONS=ON` + `LLVM_ENABLE_RUNTIMES=compiler-rt`
  (`cxx26/dev/configure-build-trees.sh` updated to match); `clang`/`clangd`/
  `clang-tidy` rebuilt clean. **Current action: verify `-fsanitize=address`
  links against compiler-rt, then re-baseline `check-clang`/`check-cxx`/
  contracts/reflection via `cxx26/dev/testrun.sh` before any further source
  change.**
- [~] **M2 — Assertions triage.** First assertions-on unit-test run crashed
  25034-test `AllClangUnitTests` outright (`SIGABRT`) on a bogus assertion in
  `Sema::getContractConstification` (`SemaContract.cpp:1266-1268`):
  `assert(!VD->getDeclContext()->Equals(CSR->ContextAtPush))` guarded by
  `Encloses(...)`, but `DeclContext::Encloses` is reflexive, so the assertion
  is unconditionally false whenever a variable's own DeclContext *is* the
  contract scope's push context -- the ordinary case of a parameter
  referenced in its own function's postcondition. Dead debug scaffolding from
  the mechanical port, invisible with assertions off. Removed; full
  `AllClangUnitTests` now passes 25034/25034. **Current action: continue
  triage against `check-clang`/`check-cxx` once the M1 baseline run
  completes.**
- [ ] **M3 — Fix the result-name bug.** Write failing T1 (IR/FileCheck) and
  T2 (libc++ execution) regression tests first; confirm red; fix
  `EmitPostContracts` to re-store `RV` into `ReturnValue`; remove the dead
  `OVEStore`/`OVEBind` scaffolding; confirm green. Cover scalar, small-POD
  (≤16 byte, still `Direct` ABI — the case the prior test suite mistook for
  safe), large-aggregate (sret), reference, `void`, template, and lambda
  return cases.
- [ ] **M4 — Coverage-gate tooling + tier buildout.** Add the anti-regression
  checker script (flags "looks executable but isn't" and "value coverage only
  in the constant evaluator" patterns) to `cxx26/dev/`, wired into the test
  gate. Build out T1/T2 tiers for contracts, then reflection, then
  `template for`, in that priority order. Budget for bugs the new tiers
  uncover.
- [ ] **M5 — `template for` implicit decls.** Add `setImplicit()` to the
  synthesized `__range` VarDecl and `__N` NTTP in `SemaExpand.cpp`, matching
  range-for's treatment. Acceptance test: Miracle's `NOLINT` wrappers around
  `template for` can be removed cleanly.
- [ ] **M6 — clangd contract-keyword completion.** Add `contract_assert` to
  `SemaCodeComplete.cpp`'s statement-completion results, gated on
  `LangOpts.Contracts`. Attempt `pre`/`post` via a new completion hook in
  `ParseContractSpecifierSequence`; fall back to `contract_assert`-only if
  that proves disproportionate.
- [ ] **M7 — Sanitizer tiers.** `build-libcxx-asan` (cheap, native libc++
  support) first; `build-libcxx-msan` (needs an instrumented libc++,
  time-boxed) second. Re-run T2 tests under both.
- [ ] **M8 — Toolchain default `-fcontracts`.** Add to
  `cxx26/toolchain/toolchain.cmake.in`'s append-if-absent flag loop; add a
  per-config evaluation-semantic block (Debug/RelWithDebInfo=`enforce`,
  Release/MinSizeRel=`ignore`). Verify libc++'s `std.cppm` synthetic module
  target still builds with the flag injected.
- [ ] **M9 — Full-suite gate.** `check-clang` + `check-cxx` vs. the M1
  baseline via `testrun.sh`/`testdiff.py`. Remember the `build-libcxx`
  stale-artifact gotcha after any Sema/CodeGen change.
- [ ] **M10 — Package and tag.** ~2 hour packaging build; smoke tests; verify
  `git ls-remote --tags origin` before minting a same-day tag.
- [ ] **M11 — Downstream verification.** Nyx/Miracle/Switch, contracts on and
  off, per the same methodology as the prior epic's M8 (verify
  `CMAKE_CXX_COMPILER:` isn't stale in each `CMakeCache.txt`; never pass
  `-DCMAKE_CXX_FLAGS` alone). No commits/pushes to those repos. Push + tag
  `clang-p2996` only after this passes clean.

## Explicitly out of scope

Two pre-existing open fork regressions recorded in `docs/CXX26_GAPS.md`
(`HandleImmediateInvocations`/`Rec.ConstevalOnly` consteval-escalation
clusters) are **not** part of this epic — they predate it, are not contract
bugs, and either could consume the epic on its own. `clang/test/Reflection/
splice-exprs.cpp`'s known regression is in scope (cheap, sits in an area this
epic is already touching).

## Session Log

- **2026-09-04 (continued session)**: Epic opened. Root-caused the `r`
  result-name bug, the clang-tidy `template for` false-positives, and the
  clangd completion gap, all confirmed by direct source reading (not agent
  claims taken on faith — two agent claims were checked and one corrected:
  `Runnable/` does contain executing tests, contrary to an earlier
  assumption; the real gap is narrower — value-level coverage only in the
  constant evaluator). Plan approved. Delete this file when the epic closes,
  per the same policy as `docs/CONTRACTS_PORT.md` and `docs/LLVM22_SYNC.md`
  before it, carrying forward any open items into `docs/CXX26_GAPS.md`.
