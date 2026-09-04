# Contracts (P2900R14) Port Tracker

This document is the single source of truth for Epic B: porting C++ Contracts
(P2900R14) onto `cxx26`. Read it before `docs/CXX26_GAPS.md` or
`docs/REFLECTION.md` while any milestone below is `[~]`. Registered as a named,
time-boxed epic file per the `LLVM22_SYNC.md` precedent (see `docs/CXX26_GAPS.md`'s
"do not create parallel tracking files" note) — delete this file when the epic
closes, carrying forward any open items into `docs/CXX26_GAPS.md`.

Full rationale, cut list, and non-goals are in the plan this epic executes:
`/home/spawn/.claude/plans/plan-contracts-port-for-snug-pearl.md` (local to the
session that authored it — treat this file, not that path, as canonical once
work begins, and inline anything from the plan that must survive).

## Scope and Fixed Points

- Source fork: `https://github.com/efcs/llvm-project`
- Source branch: `contracts-nightly` @ `1634b387b76fe389fa65ee655d09ba978ac519bd` (2026-02-11)
- Source's own upstream base: `contracts-base` @ `5710e418c335c99f1d1ea619a4622837553b53d5`
- Contracts diff: `contracts-base...contracts-nightly` — 214 commits, 204 files, +13351/−298
- Our base: `llvmorg-22.1.8` (release branch point `e9f758a59`, 2026-01-13)
- Development branch: `integration/contracts-p2900` (not created yet)
- Target branch: `cxx26`
- Remote for the source fork: `contracts` (to be added:
  `git remote add contracts https://github.com/efcs/llvm-project.git`,
  fetched **without** `--filter=blob:none` — three-way apply needs pre-image blobs)
- Working fork remote: `origin` (`https://github.com/spwn02/clang-cxx26.git`)
- Upstream remote: `upstream` (`https://github.com/llvm/llvm-project.git`, push disabled)

Upstream LLVM has **not** landed contracts (issue #127613 open, no
`SemaContract.cpp` on `main` as of this epic's start). This fork is the only
viable source.

**"Merge" means integration-branch → `cxx26` at the end** (Epic A's
`ca44e7b01b09` shape), never `git merge contracts-nightly` — that branch is 3721
commits of LLVM 23-dev ahead of our base.

## State Legend

- `[ ]` not started
- `[~]` active; `Continue` resumes its Current action immediately
- `[x]` complete and required test gate passed
- `[!]` blocked; details and an actionable unblock condition are recorded below

Exactly one milestone may be `[~]`. A milestone changes to `[x]` only after its
required test gate passes. Commit every coherent step; push only completed
milestones; never push a knowingly broken state.

## Baseline Environment

### Main LLVM/Clang build

- Tree: `build-nyx`
- Source: `llvm/`
- Generator/build type: Ninja / Release
- Projects: `clang;clang-tools-extra`
- Targets: X86
- Assertions: OFF
- `LLVM_INCLUDE_TESTS=ON`, `CLANG_INCLUDE_TESTS=ON` (required for `check-clang`)
- Compiler launcher: `ccache` (via `CMAKE_{C,CXX}_COMPILER_LAUNCHER`), max 8G,
  `compiler_check=content` — added this epic specifically so switching between
  `cxx26` and `integration/contracts-p2900` doesn't cost a full clang rebuild.
- Bootstrap compilers: `/usr/bin/clang`, `/usr/bin/clang++`

### libc++ build

- Tree: `build-libcxx`
- Source: `runtimes/`
- Generator/build type: Ninja / Release
- Runtimes: `libcxx;libcxxabi;libunwind`
- Compilers: `build-nyx/bin/clang`, `build-nyx/bin/clang++`
- Compiler launcher: `ccache`, same config as above

Configure either tree with `cxx26/dev/configure-build-trees.sh {nyx|libcxx|all}`
(idempotent, never deletes an existing tree — unlike
`cxx26/toolchain/build-linux-x86_64.sh`, which `rm -rf`s its arguments).

**This config must exactly match Epic A's** (Release, X86, assertions OFF) — a
different config makes the archived 199/5-failure reconciliations non-comparable.

## Test-Result Archive

Persistent, outside the work tree (survives `git clean -xdf` and branch churn):
`~/.local/share/cxx26-contracts/{results,lit-times}/`.

```bash
# Run a suite, archive its stamped JSON result:
cxx26/dev/testrun.sh <suite>
# suites: check-clang, check-cxx, contracts, contracts-lib, reflection,
#         reflection-lib, semacxx, serialization, regression-clusters

# Diff two archived results, refusing to compare across mismatched configs:
cxx26/dev/testdiff.py <baseline.json> <candidate.json>
```

Every archived result is stamped with the `cxx26` SHA, a `CMakeCache.txt` config
fingerprint, and `clang --version` (sibling `<name>.meta.json`); `testdiff.py`
refuses to diff across mismatched fingerprints unless `--allow-config-mismatch`
is passed. **No "skip previously-passing tests" cache** — a stale pass is not
evidence after a rebuild. Cheapness comes from persistent build trees (ninja
incrementality), a focused inner loop for fixes, and archived baselines so a
full suite runs once per milestone gate rather than to rediscover what was
already failing.

Before any full `check-cxx`: `rm -rf` the `clang_modules_include.gen.py` output
dir first — it reached ~9G in one Epic A run (`testrun.sh check-cxx` does this
automatically).

## Canonical Commands

```bash
# Build
ninja -C build-nyx -j$(nproc)
ninja -C build-libcxx -j$(nproc) libcxx-generate-files
ninja -C build-libcxx -j$(nproc) cxx
# Forced-clean libc++ rebuild (required before trusting libc++ results after any
# clang/lib/Sema or clang/lib/AST change):
ninja -C build-libcxx -t clean cxx && ninja -C build-libcxx -j$(nproc) cxx

# Archived test runs (see Test-Result Archive above)
cxx26/dev/testrun.sh contracts
cxx26/dev/testrun.sh check-clang
cxx26/dev/testrun.sh check-cxx

# Fast re-check after a fix (lit's own failed-first ordering)
./build-nyx/bin/llvm-lit --filter-failed clang/test/Contracts
```

## Current Action

M3 is blocked on one item: the constification subsystem (see Known Bugs/TODOs)
is incomplete across three mechanisms (out-of-line declarations, template
instantiation, lambda captures), accounting for all 5 remaining test failures.
This needs its own dedicated investigation session rather than being rushed —
next actionable step is reading `Sema::getContractConstification`
(`SemaContract.cpp:1232`) and the dead `#if 0` block above it (~1191-1226,
an abandoned earlier implementation) to understand why
`CSR->ContextAtPush->Encloses(VD->getDeclContext())` behaves asymmetrically
for in-class vs. out-of-line declarators. Meanwhile M4 (no reflection/Sema
regressions vs. the M1 baseline) is runnable now and independent of this —
do that next while constification is parked.

## Milestones

- [x] **M1 — Environment, infrastructure, baseline.** Both trees built clean
  from the new `configure-build-trees.sh` (`build-nyx`: 5014/5014 targets;
  `build-libcxx`: 1956/1956, both fresh — this doubled as the "forced-clean"
  libc++ build the plan asked for). Archive scripts done
  (`cxx26/dev/{configure-build-trees.sh,testrun.sh,testdiff.py}`), two bugs
  found and fixed in `testrun.sh` during the run — see Known Bugs / TODOs.
  Baseline captured at `97ea0acaee51` (2026-09-04, pre-contracts): `check-clang`
  44513 pass / 5 fail, `check-cxx` 10605 pass / 50 fail, `reflection-lib` (60
  libc++ reflection tests) 54 pass / 6 fail, `regression-clusters` (6 named
  files) 5 fail (clang-side) + 1 pass (libcxx-side). All archived under
  `~/.local/share/cxx26-contracts/results/` with `-latest` symlinks.
  *Gate:* both trees build ✓; baseline archived and stamped ✓; deviations from
  Epic A's exception lists recorded below ✓.
- [x] **M2 — Mechanical port, build-green.** Fetched `contracts-base`/
  `contracts-nightly` from `efcs/llvm-project` (full blobs, no filter); branched
  `integration/contracts-p2900` off `cxx26`; `git apply -3` of the
  `contracts-base...contracts-nightly` diff restricted to `clang/` +
  `llvm/include/llvm/Support/TrailingObjects.h` (libcxx/ deferred to M5;
  scratch/CMake-debug-opt/doc files cut). 155 files touched (57 new, 98
  modified), only 19 real conflicts (well under the plan's 77-file worst
  case) — resolved by hand, mostly additive merges plus a handful of
  same-anchor-point insertions where diff3 silently swallowed one side's
  shared closing brace (fixed by re-adding the dropped `}`); one real rename
  collision (`FoundImmediateEscalatingConstruct` vs upstream's
  `FoundImmediateEscalatingExpression` — kept our fork's name, no contracts
  file referenced the other spelling) and one real API split
  (`lookupStdSourceLocationImpl`: kept our fork's caching member function,
  used already by `SemaReflect.cpp`).
  `ninja -C build-nyx` built clean on the first attempt after conflict
  resolution — zero compile errors. Then found and fixed two real bugs via
  the PCH/module round-trip checks below (both in the ported code, not
  merge artifacts): (1) `ContractSpecifierDecl`'s deserialization ctor
  unconditionally called `DC->isDependentContext()` with `DC == nullptr`
  (`CreateDeserialized` always passes null; fixed with a null guard,
  `clang/include/clang/AST/DeclCXX.h`); (2) `ASTDeclWriter`/`ASTDeclReader`
  disagreed on field order for `ContractSpecifierDecl` — writer emitted
  `NumContracts` *after* the base `VisitDecl` fields, reader read it
  *before* (needed up front to size the trailing-objects allocation),
  desyncing the record cursor for everything after — fixed by moving the
  writer's `Record.push_back(CSD->NumContracts)` before `VisitDecl(CSD)`,
  matching the working `DecompositionDecl` precedent
  (`ASTWriterDecl.cpp`/`ASTReaderDecl.cpp`). Confirmed fixed via both a PCH
  round-trip and a C++20 module round-trip (`--precompile` + `import`) over
  a `pre`/`post`/`contract_assert`-carrying function — full AST, correctly
  deserialized on both paths.
  *Gate:* `ninja -C build-nyx` succeeds ✓; `-fcontracts -std=c++26` smoke
  file compiles (verified AST + rejects `pre(...)` without the flag) ✓; PCH
  round-trip ✓; module round-trip ✓. (Linking the smoke file fails on
  `__handle_contract_violation_v3` — expected, that's libc++'s runtime hook,
  M5's job, not a gate requirement here.) Note: both round-trips verify
  *deserialization of the AST* only, not codegen from a deserialized
  contract-carrying decl — M3 found a real codegen bug (postconditions on
  void functions, see Known Bugs/TODOs) that a round-trip followed by
  execution would also have caught. Not a gate failure (M2's gate says
  "compiles"), noted so the claim isn't read as broader than it is.
- [!] **M3 — `clang/test/Contracts` green.** 43 tests + 2 in `clang/test/Parser`.
  Suite widened to also cover `clang/test/Modules/contracts.cppm` and
  `clang/test/SemaCXX/ericwf-crash.cpp` (previously not run by
  `testrun.sh contracts` at all — see Known Bugs/TODOs). 36/41 passing.
  Three stale test-expectation fixes (`contracts.cpp`, `contract-group-attr.cpp`,
  `repro.cpp` — commits `7aa52c0c1c0d`, `5655604488c2`, `413311678584`) plus
  two real compiler bugs (#3, #4 below) closed 6 of the original 8 failures.
  **Blocked** on the remaining 5 (`constification.cpp`, `friendship.cpp`,
  `lambda.cpp`, `template-test2.cpp`, `templates.cpp`) — all one root cause,
  see the consolidated Known Bugs/TODOs entry. Unblock condition: implement/
  repair contract constification as its own dedicated session. The gate's
  "or each failure is documented with a root cause and a decision" clause is
  satisfied (root cause: identified; decision: defer, not accept) — marked
  `[!]` rather than `[x]` because the decision is explicitly *not* to accept
  these 5 as a permanent exception list.
- [ ] **M4 — No reflection/Sema regressions.** `clang/test/{Reflection,SemaCXX,
  AST/ByteCode,Modules,PCH,Import}` vs the M1 archive; zero new failures.
- [ ] **M5 — Library side.** `<contracts>`, `src/contracts.cpp`, module wiring,
  `libcxx/test/std/contracts/` + 5 support headers, minimal `features.py`
  `contracts` lit feature (default off; run via `--param use-contracts=True`),
  header modernized, P3819R0 `evaluation_exception` removal.
- [ ] **M6 — Full-suite gate.** Full `check-clang` + `check-cxx` vs M1 archive,
  every new failure fixed or named.
- [ ] **M7 — Merge and tag.** `--no-ff` into `cxx26`, push, tag
  `cxx26-YYYY.MM.DD`; update `Cxx2cPapers.csv`, `CXX26_GAPS.md`, `AGENTS.md`,
  `REFLECTION.md` if needed.
- [ ] **M8 — Downstream verification.** Package the reference toolchain; two
  passes over Nyx + Miracle + Switch (no-regression, then `-fcontracts` +
  capability probes + contract tests, uncommitted in those repos).

## Known Bugs / TODOs

- **M1 baseline deviations from Epic A's recorded exception lists** (recorded,
  not investigated — not this epic's job per M1's gate):
  - `check-clang`: 5 failures, same count as Epic A but **not the same set** —
    `clang/test/Reflection/splice-exprs.cpp` is a new failure not in Epic A's
    list, alongside the 4 previously-known `SemaCXX`/`AST` regressions. Likely
    one of the three post-Epic-A escalation-subsystem commits
    (`55872c0f`/`079b2078`/`33df47d5`) named in the plan.
  - `check-cxx`: 50 failures, down from Epic A's recorded 199. Not
    investigated — could be prior fixes landing, could be a config difference.
    Revisit if M6's `check-cxx` diff looks confusing relative to this number.
- `testrun.sh` bugs found and fixed during M1 (both already fixed in the
  committed script, noted here for context if the fix needs revisiting):
  - `set -e` was aborting the script before the `.meta.json`/`-latest` symlink
    step whenever the suite under test had *expected* failures (i.e. always,
    for `check-clang`/`check-cxx`). Fixed by running the suite under `set +e`,
    capturing its exit code as `suite_rc`, then `set -e` again before the
    archiving steps; the script now `exit`s with `suite_rc` at the very end.
  - `regression-clusters`' file list had two path bugs: `builtin-is-within-
    lifetime.cpp` is under `clang/test/SemaCXX/`, not `clang/test/AST/
    ByteCode/` (lit silently discovered only 4 of 5 intended files with no
    error); and the 5th plan-named file
    (`reflection-ex-parsing-command-line-options-2.sh.cpp`) is a libcxx test
    needing `libcxx-lit`, not `llvm-lit` — it's now a second invocation in the
    same case, writing to `<out>.lib.json`, with `suite_rc` reflecting either
    invocation's failure.
- **M2/M3 compiler bugs found and fixed in the ported code** (real bugs in
  `contracts-nightly`, not merge artifacts — see the M2 milestone entry and
  commit messages for the two serialization ones):
  1. `ContractSpecifierDecl`'s deserialization ctor dereferenced a null
     `DeclContext*` unconditionally (`DeclCXX.h`, fixed in `bae93f99bfde`).
  2. `ASTDeclWriter`/`ASTDeclReader` disagreed on field order for
     `ContractSpecifierDecl`, desyncing the record cursor (`ASTWriterDecl.cpp`
     / `ASTReaderDecl.cpp`, fixed in `bae93f99bfde`).
  3. `ParseContractSpecifierSequence` parsed `pre`/`post` conditions after an
     out-of-line declarator's qualified scope had already closed, so access
     checks (friend/private-member) inside the conditions spuriously failed
     for out-of-line member/friend function definitions. Fixed in `8801be677844`
     by re-entering the scope the same way `ParseTrailingRequiresClause`
     already does — `clang/lib/Parse/ParseContracts.cpp`. (Getting the new
     RAII scope-object's declaration order wrong relative to the existing
     `ParserScope`/`ThisScope` first produced a spurious "extra qualification
     on member" diagnostic — the fix must declare it *before* them so it's
     destroyed *after*, keeping the parser's scope stack correctly nested.)
  4. `EmitFunctionEpilog`'s pre-existing "no result" fast path for
     void-returning functions (`!ReturnValue.isValid()`) returns before
     reaching the contracts patch's `EmitPostContracts(RV)` call further down
     — so postconditions on **any void-returning function** never evaluate at
     all, under every evaluation semantic. Fixed in `1ec85941d1f8` —
     `clang/lib/CodeGen/CGCall.cpp`. Caught by
     `clang/test/Contracts/Runnable/breathing-test.cpp`'s own runtime
     self-check (also had an unrelated test-file bug: `%t 1>&2` uses a
     redirect direction lit's internal shell doesn't support, fixed in the
     same commit).
- **Constification is systematically incomplete — one root cause behind all 5
  remaining M3 failures** (`constification.cpp`, `friendship.cpp`, `lambda.cpp`,
  `template-test2.cpp`, `templates.cpp`). [basic.contract.general] requires
  id-expressions referring to automatic-storage variables to be treated as
  `const` inside a contract predicate; the ported implementation
  (`Sema::getContractConstification`, `clang/lib/Sema/SemaContract.cpp:1232`,
  gated by `LangOpts.ContractConstification`) applies this inconsistently
  across three distinct mechanisms:
  1. **In-class vs. out-of-line declarations produce different AST shapes for
     the same source.** Repro:
     `/home/spawn/.claude/jobs/15c946bc/tmp/m2smoke/member_contract_check.cpp`
     (a friend-access `a.g()` predicate declared in-class, redefined
     out-of-line). AST dump (`-Xclang -ast-dump -Xclang -ast-dump-filter=u`)
     shows: in-class declaration constifies via a wrapping
     `ImplicitCastExpr 'const A' lvalue <NoOp>` around a plain
     `DeclRefExpr 'A'`; the out-of-line definition instead bakes `const`
     directly into the `DeclRefExpr`'s own type and marks it
     `in-contract` (no wrapping cast). Same source, same predicate, two
     different node shapes — `Context.hasSameExpr()` (used by
     `Sema::CheckEquivalentContractSequence` to compare a redeclaration's
     contracts against the canonical/first declaration's, per
     [basic.contract.func]) is shape-sensitive and reports "non-equivalent"
     for the textually-identical `a.g()`, which is `friendship.cpp`'s
     failure. **Verified this predates and is independent of the parser
     access-scope fix (#3 below, `8801be677844`)**: checked out
     `ParseContracts.cpp` at `8801be677844^` (pre-fix), rebuilt, re-dumped —
     identical asymmetric AST shapes appear before that commit too. That
     commit's message (which describes it as purely an access-checking fix)
     is accurate; it did not introduce or change this bug.
  2. **Template instantiation doesn't re-apply constification.**
     `templates.cpp` expects `'const NoBool' is not contextually convertible
     to 'bool'` on a contract-guarded template but gets the un-constified
     `'NoBool'` — the `TreeTransform`-rebuilt predicate at instantiation time
     loses the constification the original (non-template) parse would have
     applied.
  3. **Lambda captures interact wrongly** — `lambda.cpp` and
     `template-test2.cpp` show both "implicit capture...not allowed" errors
     and constification over/under-application on captured-by-reference
     variables inside contract predicates (see `Cap.isCapturedAcrossContract()`
     in `clang/lib/Sema/SemaLambda.cpp` around line 1999).
  `constification.cpp` exercises a broad mix of these and additionally shows
  an unrelated `decltype`/`AssertSame` template mismatch at line 122 —
  `decltype` inside a contract expression doesn't reflect the constified type
  either.
  The likely entry point for a future fix is
  `Sema::getContractConstification` itself, specifically the early-return
  condition `CSR->ContextAtPush->Encloses(VD->getDeclContext())` at
  `SemaContract.cpp:1241` and its interaction with
  `getLastEnclosingContractScopeForContext(CurContext)` — this is where the
  in-class/out-of-line asymmetry most plausibly originates (different
  `CurContext` at the time the predicate is parsed for the two declarator
  forms). Also worth knowing before diving in: there's a **dead, abandoned
  earlier implementation** of the same enclosing-scope check sitting in an
  `#if 0`/`#endif` block directly above `getContractConstification`
  (`SemaContract.cpp:1191-1226`) — read it for context, don't resurrect it
  without understanding why it was disabled.
  Removed one piece of noise found along the way: `lambda.cpp` was printing
  an unconditional `Setting Is Constified capture!\n` to stderr on every
  compile via a bare `llvm::errs()` in `SemaLambda.cpp` (not gated behind
  `EricWFDebug`/`ERICWF_DEBUG` like the rest of the fork's debug output) —
  removed, since it would have polluted stderr in every downstream build
  (M8) and any test capturing stderr.
  Deferred rather than fixed in this session — see Current Action and the M3
  milestone entry. Not this epic's smallest fix; treat as its own session.
- `cxx26/dev/testrun.sh`'s `contracts` suite only ran `clang/test/Contracts`,
  missing 4 other contracts-relevant files the port added
  (`clang/test/Parser/{cxx-contracts,contract-inline-methods}.cpp`,
  `clang/test/Modules/contracts.cppm` — the regression guard for bugs #1/#2
  above — and `clang/test/SemaCXX/ericwf-crash.cpp`). Fixed in `045e92d9f32a`;
  all 5 pass. Also moved `clang/test/ctest.cpp` (a real test, misplaced) into
  `clang/test/Contracts/` and deleted `clang/test/Contracts/summary.txt` (not
  a test at all — an unrelated AI-chat summary, evidently carried over by
  mistake from the source fork).

## Session Log

Append a dated entry each session — what moved, what's blocked, what's next.
Do not remove old entries.

### 2026-09-04 — M1 complete: infra, both trees, pre-port baseline

Reclaimed ~9G disk (deleted the superseded `~/.local/opt/clang-p2996` install
and Nyx's regenerable `vcpkg/buildtrees`), installed and wired `ccache` into
both trees. Wrote `cxx26/dev/{configure-build-trees.sh,testrun.sh,testdiff.py}`
and this tracker doc. Built `build-nyx` (5014/5014) and `build-libcxx`
(1956/1956) from scratch. Captured and archived the pre-port baseline at
`97ea0acaee51`: `check-clang` (44513/5), `check-cxx` (10605/50),
`reflection-lib` (54/6 of 60), `regression-clusters` (5 clang-side fail + 1
libcxx-side pass, all 6 plan-named files). Found and fixed two bugs in
`testrun.sh` along the way (see Known Bugs / TODOs) — both fixed before the
final archived baseline runs, so the archived JSON/meta files are from the
corrected script. M1 gate passed; M2 is next: add the `contracts` remote,
fetch `contracts-base`+`contracts-nightly` without a blob filter, branch
`integration/contracts-p2900`, three-way apply.

### 2026-09-04 — M2 complete, M3 in progress: 33/41 passing, 4 real bugs found

M2: fetched and applied the contracts diff (155 files, 19 real conflicts,
resolved by hand — see the M2 milestone entry for details). Build green on
the first attempt. Found and fixed two serialization bugs via the PCH/module
round-trip gate (null-DC deref, writer/reader field-order desync) — both
caught same-session instead of bisecting at M6, exactly as the M2 gate was
designed to do. Committed as `bae93f99bfde`.

M3: widened `testrun.sh`'s `contracts` suite to catch a coverage gap (missed
4 relevant test files, see Known Bugs/TODOs), moved/deleted two misplaced
non-Contracts files. Found and fixed two more real compiler bugs while
triaging failures: an out-of-line access-checking scope bug in the parser
(`8801be677844`) and a postconditions-never-fire-on-void-functions codegen
bug (`1ec85941d1f8`) — the latter found by reading source after an advisor
consult rather than continuing to add debug instrumentation, which was the
faster path. 33/41 tests passing now (started at 27/36 before any fixes).
8 failures remain: `constification.cpp`, `contract-group-attr.cpp`,
`contracts.cpp`, `friendship.cpp` (down to a narrower equivalence-checking
bug, not access-checking anymore), `lambda.cpp`, `repro.cpp`,
`template-test2.cpp`, `templates.cpp` — not yet triaged individually. Next:
continue through those one at a time the same way (read `llvm-lit -v`
output, isolate a minimal repro, find root cause in source before guessing,
fix, rebuild, recheck, commit each fix separately).

### 2026-09-04 — M3 at 36/41: three stale-test fixes, constification root-caused and deferred

Triaged and fixed 3 of the remaining 8 M3 failures as stale `-verify`
expectations, not compiler bugs (pre-existing typo-correction diagnostic in
`contracts.cpp`, a qualified-vs-bare attribute-name string in
`contract-group-attr.cpp`, and a wrong-declaration-cited note in
`repro.cpp` — P2900 always compares against the *canonical*/first
declaration, not the nearest prior one). Commits `7aa52c0c1c0d`,
`5655604488c2`, `413311678584`.

The remaining 5 (`constification.cpp`, `friendship.cpp`, `lambda.cpp`,
`template-test2.cpp`, `templates.cpp`) all trace to one root cause:
constification ([basic.contract.general]'s const-ification of automatic-
storage id-expressions inside contract predicates) is incomplete across
three mechanisms — in-class/out-of-line AST-shape asymmetry, template
instantiation not re-applying it, and lambda-capture interaction. Verified
by AST dump that the in-class/out-of-line asymmetry predates and is
independent of the M3 access-scope fix (`8801be677844`) — checked out that
commit's parent, rebuilt, re-dumped, saw the identical asymmetric shapes.
Full technical writeup in the consolidated Known Bugs/TODOs entry, including
the specific `Sema::getContractConstification` code path and a dead `#if 0`
abandoned-implementation block sitting just above it. Also removed a stray
unconditional `llvm::errs()` debug print in `SemaLambda.cpp` that was
polluting `lambda.cpp`'s stderr (not gated behind `EricWFDebug` like the
rest of the fork's debug output).

M3 marked `[!]` blocked rather than `[x]` accepted — the decision on these
5 is explicitly to defer to a dedicated session, not to carry them as a
permanent exception list. M4 (no reflection/Sema regressions vs. the M1
baseline) is independent of constification and is the more valuable next
step: it validates whether the ~13k lines of ported Sema/CodeGen changes
damaged reflection or general Sema, which the M1 baseline can catch cheaply
right now. Recommended order for the next session: M4 first, then
constification as its own dedicated push.
