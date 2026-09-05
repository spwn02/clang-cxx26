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
- [x] **M1 — Build-tree hardening and baseline.** `build-nyx` reconfigured
  with `LLVM_ENABLE_ASSERTIONS=ON` + `LLVM_ENABLE_RUNTIMES=compiler-rt`
  (`cxx26/dev/configure-build-trees.sh` updated to match); `clang`/`clangd`/
  `clang-tidy` rebuilt clean; `-fsanitize=address`/`memory`/`undefined` all
  verified linking against the new compiler-rt. `check-clang` baseline
  captured (44628/49837 pass, 7 fail -- see M2). A second `check-cxx` run
  (to fold in the M2/M3 fixes below) crashed the whole desktop: the
  assertions build's crash storm plus this suite's module-precompilation
  cache churn filled the disk to 100%. Disk recovered (freed 27G stale
  module-cache + 4G coredumps; 32G free, repo/build integrity confirmed
  intact). Hardened in two layers so this cannot recur: (1)
  `/etc/systemd/coredump.conf.d/99-disk-safety.conf` caps coredump storage
  system-wide (`MaxUse=2G KeepFree=10G`), independent of this repo or this
  script; (2) `cxx26/dev/testrun.sh` (`c0e970b1a0ad`, `5a5659d614ba`) disables
  core dumps for test runs, refuses to start under 10GB free, and wraps every
  long-running command in a live watchdog that hard-kills it if free space
  drops below 3GB mid-run -- the start-of-run check alone wouldn't have
  caught this incident, since the module cache grew *during* a single run
  against an already-adequate-looking margin. Watchdog kill/non-kill paths
  verified in isolation before trusting it on a real run.

  `check-cxx` baseline re-captured cleanly with both guards active (disk
  held at 23G free throughout): 10617/11766 pass, 52 fail -- down from 56
  in the pre-fix run, exactly the 4 contracts tests M3 below fixes. Of the
  remaining 52: ~40 already match the documented pre-existing
  `std::execution`/atomics/gdb-pretty-printer/etc. buckets in this file's
  Scope section; the other ~11 are new reflection/`optional`-iterator
  findings, recorded in `docs/CXX26_GAPS.md` (one trivial one fixed on the
  spot) rather than chased here -- explicitly out of this milestone's scope,
  same discipline as the two crashes M2 already deferred. Archived:
  `check-cxx-20260904T234114Z-50283cf26005-hardening-m1-baseline-v3.json`.
- [x] **M2 — Assertions triage.** First assertions-on unit-test run crashed
  25034-test `AllClangUnitTests` outright (`SIGABRT`) on a bogus assertion in
  `Sema::getContractConstification` (`SemaContract.cpp:1266-1268`):
  `assert(!VD->getDeclContext()->Equals(CSR->ContextAtPush))` guarded by
  `Encloses(...)`, but `DeclContext::Encloses` is reflexive, so the assertion
  is unconditionally false whenever a variable's own DeclContext *is* the
  contract scope's push context -- the ordinary case of a parameter
  referenced in its own function's postcondition. Dead debug scaffolding from
  the mechanical port, invisible with assertions off. Removed; full
  `AllClangUnitTests` now passes 25034/25034.

  `check-clang` M1 baseline (44628/49837 pass, 5171 unsupported, 25 xfail, 7
  fail): 5 failures match already-documented pre-existing regressions
  (`docs/CXX26_GAPS.md`'s consteval-escalation clusters + `splice-exprs.cpp`,
  none newly caused by this epic). 2 are new, both `llvm_unreachable`/`assert`
  crashes invisible before assertions were on, both out of scope for this
  epic (recorded in `docs/CXX26_GAPS.md`): a reflection namespace-splicing
  AST-representation gap (`Reflection/splice-namespaces.cpp`) and a pure
  vanilla concepts partial-ordering assertion unrelated to any fork feature
  (`SemaCXX/PR98671.cpp`, predates this fork's work). Archived:
  `check-clang-20260904T210524Z-a53ad30ef186-hardening-m1-baseline.json`.
  **Current action: capture the `check-cxx` M1 baseline, then close M1.**
- [x] **M3 — Fix the result-name bug.** Fixed differently than originally
  planned, and more surgically: rather than re-storing `RV` into
  `ReturnValue` inside `EmitPostContracts`, `EmitFunctionEpilog`'s
  store-erasure optimization (`CGCall.cpp`) is now skipped whenever the
  function has a postcondition result name, so `ReturnValue`'s natural-type
  memory simply never goes dead in the first place — one guard covers
  scalar, small-POD, and any other `Direct`/`Extend`-ABI shape uniformly,
  without needing per-ABI coercion logic. Removed the dead `OVEStore`/
  `OVEBind` scaffolding in `EmitPostContracts` (confirmed never consulted).
  `clang/test/Contracts/Runnable/contract-result-name.cpp` — the file
  literally named for this feature, previously `-fsyntax-only` with an
  empty `main()` — rewritten into a real executing regression test (int,
  double, 8-byte POD, 32-byte sret aggregate); confirmed red before the fix,
  green after. Full contracts suite 42/42, libcxx contracts suite 4/4,
  `AllClangUnitTests` 25034/25034 — zero regressions. This was the actual
  headline bug the epic opened on.
- [ ] **M4 — Coverage-gate tooling + tier buildout.** Add the anti-regression
  checker script (flags "looks executable but isn't" and "value coverage only
  in the constant evaluator" patterns) to `cxx26/dev/`, wired into the test
  gate. Build out T1/T2 tiers for contracts, then reflection, then
  `template for`, in that priority order. Budget for bugs the new tiers
  uncover.
- [x] **M5 — `template for` implicit decls.** `setImplicit()` added to the
  synthesized `__range` VarDecl (`tryMakeCXXIterableExpansionSelectExpr`) and
  the `__N` NTTP (`BuildExpansionStmtDeclaration`, the chokepoint shared with
  template instantiation) in `SemaExpand.cpp`, matching range-for's
  treatment. Two new regression tests confirmed red (exact match to the
  user's screenshots) before the fix and green after:
  `clang-tools-extra/test/clang-tidy/checkers/bugprone/reserved-identifier-expansion-statement.cpp`
  and `.../readability/identifier-naming-expansion-statement.cpp`.
  Acceptance test done via a structurally-equivalent standalone repro
  (member-function template wrapping `template for`, matching Miracle's
  actual `list_members` shape) rather than a full Miracle rebuild — clean,
  zero `bugprone-reserved-identifier`/`readability-identifier-naming`
  findings. `cxx2c-expansion-stmts.cpp`, `clang/test/Reflection/` (55/57,
  same 2 pre-existing regressions), `clang/test/Contracts/` (42/42), and
  `AllClangUnitTests` (25034/25034) all still pass.
- [x] **M6 — clangd contract-keyword completion.** `contract_assert` added
  to `SemaCodeComplete.cpp`'s statement-completion results
  (`AddOrdinaryNameResults`' `PCC_Statement` branch), gated on
  `LangOpts.Contracts`, mirroring the existing `static_assert`/`co_return`
  pattern. `pre`/`post` deliberately not attempted this round — they're
  parsed post-declarator inside `ParseContractSpecifierSequence`, a
  position the existing `CodeCompleteFunctionQualifiers` hook doesn't
  reach; a new completion hook would be needed, carried forward as an open
  item in `docs/CXX26_GAPS.md` rather than built here (the fallback the
  plan anticipated). Two new regression tests
  (`clang/test/CodeCompletion/contract-assert{,-disabled}.cpp`) confirmed
  red before the fix (no completion offered with `-fcontracts`) and green
  after, plus the negative case (correctly absent without `-fcontracts`).
  Full `CodeCompletion` suite 96/96, `AllClangUnitTests` 25034/25034 —
  zero regressions.
- [ ] **M7 — Sanitizer tiers.** `build-libcxx-asan` (cheap, native libc++
  support) first; `build-libcxx-msan` (needs an instrumented libc++,
  time-boxed) second. Re-run T2 tests under both.
- [x] **M8 — Toolchain default `-fcontracts`.** Added to
  `cxx26/toolchain/toolchain.cmake.in`'s append-if-absent flag loop, plus a
  per-config evaluation-semantic block (Debug/RelWithDebInfo=`enforce`,
  Release/MinSizeRel=`ignore`) via the same idiom. Verified against a real
  merged install stage (`cmake --install` of both `build-nyx` and
  `build-libcxx` into one prefix, matching the real packaging pipeline) --
  not just the generated flag strings: configured and built the relocation
  smoke test under both Debug and Release, confirmed the resolved
  `CMakeCache.txt` values, and ran both resulting binaries (exit 0). The
  smoke test (`cxx26/toolchain/smoke/main.cxx`) now also exercises a
  postcondition matching M3's fix, so it validates a real consumer gets a
  correct contracts result, not just that `import std`/reflection work.
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
- **2026-09-05**: M1/M2 assertions build done; found and fixed two more real
  bugs beyond the original `r` finding, both via direct empirical
  bisection/gdb-adjacent print-debugging rather than guesswork: a bogus
  assert in `getContractConstification` (dead scaffolding, reflexive
  `DeclContext::Encloses`), and a crash on nearly any `-fcontracts`
  translation unit including `<utility>` (a variable-template pattern's
  `checkForConstantInitialization` early-return never populates the state
  `recheckForConstantInitialization`'s assert depends on) — the latter was
  blocking all 4 `libcxx/test/std/contracts/*.pass.cpp` tests outright. Both
  have red-then-green regression tests. Mid-way through re-baselining
  `check-cxx`, the disk filled to 100% and crashed the desktop (assertions
  crash-dump storm + `extensions/clang`'s module-cache churn, against an
  already-thin 16GB margin). Recovered (32G free now); `testrun.sh` hardened
  against recurrence (`ulimit -c 0`, refuses to start under 10GB free).
  Recorded here in full rather than glossed over, per this epic's own point.
