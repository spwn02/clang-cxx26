# LLVM 22 Synchronization Tracker

This document is the single source of truth for Epic A: synchronizing CXX26 Clang with LLVM 22. Read it before `docs/CXX26_GAPS.md` or `docs/REFLECTION.md` while any milestone below is `[~]` or `[!]`.

## Scope and Fixed Points

- Exact upstream target: `llvmorg-22.1.8`
- Development branch: `integration/llvm-22.1.8`
- Baseline commit: `4f1df39cf326d27e56f9e9ccc6a7f2124527749f`
- Baseline annotated tag: `cxx26-2026.08.24`
- Baseline branch: `cxx26`
- Upstream remote: `upstream` (`https://github.com/llvm/llvm-project.git`, push disabled)
- Historical implementation provenance is retained in Git history.
- Working fork remote: `origin` (`https://github.com/spwn02/clang-cxx26.git`)

Do not begin unrelated compiler implementation work until this epic completes. Preserve LLVM licensing, source headers, Git authorship, Bloomberg acknowledgments, and historical implementation provenance throughout the merge.

## State Legend

- `[ ]` not started
- `[~]` active; `Continue` resumes its Current action immediately
- `[x]` complete and required test gate passed
- `[!]` blocked; details and an actionable unblock condition are recorded below

Exactly one milestone may be `[~]`. A milestone changes to `[x]` only after its required test gate passes. Commit and push every completed major milestone; never push a knowingly broken milestone state. Record status changes in this file in the same commit as the relevant work.

## Baseline Environment

### Main LLVM/Clang build

- Tree: `build-nyx`
- Source: `llvm/`
- Generator/build type: Ninja / Release
- Projects: `clang;clang-tools-extra`
- Runtimes: none
- Targets: X86
- Bootstrap compilers: `/usr/bin/clang`, `/usr/bin/clang++`

### libc++ build

- Tree: `build-libcxx`
- Source: `runtimes/`
- Generator/build type: Ninja / Release
- Runtimes: `libcxx;libcxxabi;libunwind`
- Compilers: `build-nyx/bin/clang`, `build-nyx/bin/clang++`

The currently built compiler identifies itself as Clang 21.0.0git and may lag the tagged source. Milestone 1 must rebuild cleanly enough to establish reproducible source-baseline results before any merge.

### Known baseline failure

`clang/test/Reflection/splice-exprs.cpp` has one pre-existing failure: the expected diagnostic at line 23 is not seen. The prior reflection smoke run recorded 15/16 passing tests. Treat this as an allowed baseline failure only if Milestone 1 reproduces and records it without additional failures.

## Canonical Commands

```bash
# Inspect configurations
grep CMAKE_BUILD_TYPE build-nyx/CMakeCache.txt build-libcxx/CMakeCache.txt

# Build main tree, then regenerate and build libc++ with the new compiler
ninja -C build-nyx
ninja -C build-libcxx libcxx-generate-files
ninja -C build-libcxx cxx

# Focused reflection suites
./build-nyx/bin/llvm-lit clang/test/Reflection/ -v
libcxx/utils/libcxx-lit build-libcxx -sv libcxx/test/std/experimental/reflection/

# Single known-failure reproduction
./build-nyx/bin/llvm-lit clang/test/Reflection/splice-exprs.cpp -v

# Full gates
ninja -C build-nyx check-clang
ninja -C build-libcxx check-cxx
```

After upstream changes, always run `ninja -C build-libcxx libcxx-generate-files` before building or testing libc++. Use the libc++ wrapper for focused tests so staged headers cannot silently become stale.

## Current Action

Milestone 4 is active. On `Continue`:

1. Inventory the first-parent reflection parser, AST, Sema, template, and
   driver changes omitted from the LLVM 22 baseline.
2. Reintroduce one coherent subsystem at a time against LLVM 22 APIs.
3. Build the affected targets and run focused reflection tests after each
   subsystem; preserve the known baseline failure only where reproduced.

## Milestones

- [x] **1. Capture clean baseline builds and expected failures.** Gate passed 2026-08-24: both build trees succeeded; focused results and all failures are recorded below.
- [x] **2. Fetch exact LLVM tag and merge on integration branch.** Gate passed 2026-08-25 in merge `ea04e484b0b8` and its direct upstream-resolution correction: exact signed tag merged on `integration/llvm-22.1.8`; every conflict and resolution category is recorded.
- [x] **3. Restore base LLVM/Clang build.** Gate passed 2026-08-25: `ninja -C build-nyx clang` and then `ninja -C build-nyx` passed from the exact LLVM 22 `clang/` baseline.
- [~] **4. Reconcile reflection Parser, AST, Sema, templates, and flags.** Preserve CXX26 syntax, reflection contexts, metafunction evaluation, splice behavior, and all experimental flag plumbing. Gate: relevant unit/build targets and focused Clang reflection tests pass except explicitly retained baseline failures.
- [ ] **5. Reconcile constant evaluation, modules, and AST serialization.** Audit evaluator changes and module/PCH serialization boundaries, including the known non-serializable `CXXMetafunctionExpr` callback limitation. Gate: focused evaluator, module, PCH, and reflection tests pass; any intentionally deferred limitation is documented with a reproducer.
- [ ] **6. Reconcile libc++ and generated C++26 files without losing local conformance work.** Preserve post-upstream C++26 implementations and regenerate module/export artifacts with LLVM 22 tooling. Gate: libc++ builds and generated-file checks are clean; local conformance commits remain reachable and represented.
- [ ] **7. Pass focused reflection/libc++ tests.** Gate: complete Clang reflection directory and libc++ reflection suite pass, allowing only failures explicitly demonstrated in Milestone 1 and still justified here.
- [ ] **8. Pass full `check-clang` and `check-cxx`.** Gate: both full suites pass, allowing only explicitly recorded pre-existing failures with before/after evidence and exact test names.
- [ ] **9. Merge integration branch into `cxx26`, push, and release.** Recheck provenance and tracker state, merge without history rewriting, push `cxx26`, create the next free annotated `cxx26-YYYY.MM.DD[.N]` prerelease tag, push it explicitly, and verify remote resolution. Gate: clean worktree, remote branch/tag verification, and this epic marked complete.

## Blockers

None currently recorded.

When blocked, record the failing command, essential diagnostic, affected milestone, attempted remedies, and exact condition needed to resume. Use `[!]` only for a genuine external or technical impasse, not for ordinary incomplete work.

## Decisions

- Merge the exact release tag `llvmorg-22.1.8`; do not track a moving LLVM branch.
- Preserve history with a merge on `integration/llvm-22.1.8`; do not rebase or squash the historical CXX26 implementation.
- Establish and commit baseline evidence before fetching/merging LLVM 22 so regressions remain attributable.
- Separate base LLVM/Clang build repair from reflection reconciliation to keep commits reviewable and failures diagnosable.
- Preserve local libc++ C++26 conformance work even when upstream LLVM 22 contains overlapping implementations; resolve case by case rather than preferring either side wholesale.
- No failure becomes an allowed exception without an exact baseline or independently verified pre-existing reproducer recorded here.

## Conflict Notes

Milestone 2 merge began 2026-08-25. All 86 content conflicts were resolved
to upstream LLVM 22 as the stable synchronization baseline; the complete
local CXX26 implementation remains preserved in the merge's first parent for
the dedicated reconciliation milestones. Resolution categories:

- **Ancillary upstream components (3):**
  `lldb/source/Host/common/JSONTransport.cpp`,
  `lldb/source/Host/windows/MainLoopWindows.cpp`, and
  `llvm/include/llvm/ADT/SmallVector.h` use upstream LLVM 22 unchanged; they
  have no local CXX26 intent. Follow-up: Milestone 3 base build.
- **Clang Parser/AST/Sema/serialization and test overlap (48):** use upstream
  LLVM 22 declarations, evaluator, parser, Sema, serialization, driver, and
  test contents to establish its API baseline. Local reflection changes are
  intentionally deferred, not discarded: reconcile them from the merge's
  first parent in Milestones 4 and 5. Follow-up: `ninja -C build-nyx clang`.
- **libc++ headers, generated artifacts, tests, and status data (35):** use
  upstream LLVM 22 contents, including deletion of obsolete
  `libcxx/test/libcxx/clang_modules_include.gen.py`. Local C++26 library work
  remains in the first parent for Milestone 6's case-by-case reconciliation.
  Follow-up: `ninja -C build-libcxx libcxx-generate-files`.

For Milestone 2 and later, append notes by subsystem and include:

- paths and upstream/local intent;
- chosen resolution and why;
- focused test covering the resolution;
- follow-up debt or known limitation.

Do not paste voluminous conflict listings or build logs into this file; keep durable summaries and exact commands, with temporary logs outside the repository.

## Session Log

### 2026-08-24 — Repository transformation and synchronization setup

- Added canonical root `AGENTS.md`; retained `.claude/CLAUDE.md` as a pointer.
- Published `cxx26` through context commit `4f1df39cf326d27e56f9e9ccc6a7f2124527749f`.
- Configured `origin`, push-disabled `upstream`, and push-disabled `bloomberg` remotes.
- Archived six obsolete branch tips under annotated `archive/pre-llvm22/*` tags, verified their peeled remote targets, deleted the branches, and made `cxx26` the sole active origin branch and GitHub default.
- Published annotated baseline tag `cxx26-2026.08.24`, peeled to `4f1df39cf326d27e56f9e9ccc6a7f2124527749f`.
- Created this tracker. Milestone 1 remains active; no compiler build or test was required for the documentation/metadata transformation.

### 2026-08-24 — Milestone 1 baseline evidence

- `cxx26` was clean at `c3329f6578f5`; baseline tag
  `cxx26-2026.08.24` peels to `4f1df39cf326d27e56f9e9ccc6a7f2124527749f`.
- Host: Linux 7.1.8-arch1-3 x86_64; `/usr/bin/clang` 22.1.8; CMake 4.4.2;
  Ninja 1.13.2. Both build trees are Release.
- `ninja -C build-nyx` passed. `ninja -C build-libcxx libcxx-generate-files`
  and `ninja -C build-libcxx cxx` passed.
- Clang reflection suite: 15 passed, 1 failed. The only failure is
  `clang/test/Reflection/splice-exprs.cpp`; its expected error at line 23
  ("not derived from") was not emitted. The isolated reproduction failed
  identically.
- libc++ reflection suite: 54 passed, 6 failed:
  `annotation-module-serialization.sh.cpp`, `miscellaneous.pass.cpp`,
  `namespace-reflection-equality-reopened.pass.cpp`,
  `p3096-fn-parameters.pass.cpp`, `parameter-reflection-kind-preserved.pass.cpp`,
  and `to-and-from-values.pass.cpp`. These are baseline failures, not merge
  regressions, and are carried forward for Milestone 7 comparison.

### 2026-08-24 — Milestone 2 blocked before fetch

- `git fetch upstream refs/tags/llvmorg-22.1.8:refs/tags/llvmorg-22.1.8`
  failed with `cannot open '.git/FETCH_HEAD': Read-only file system`.
- Non-mutating remote verification (`git ls-remote --tags upstream
  'llvmorg-22.1.8^{}' 'llvmorg-22.1.8'`) could not reach upstream because
  `github.com` DNS resolution failed.
- No local ref, branch, merge, or commit was created. Resume when writable
  Git metadata and upstream network/DNS access are available.

### 2026-08-25 — Milestone 2 resumed and merge started

- Upstream publication verification returned annotated tag
  `e013073558445169e8732e25fa86e9913bfdd24e` and peeled commit
  `ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`. The tag object is PGP-signed
  by Douglas Yung (`douglas.yung@sony.com`); the signature key's local trust
  status has not yet been independently established.
- The upstream object transfer completed after an interactive fetch runner
  was interrupted before ref installation. Object and tag identity were
  verified locally, then the exact verified tag ref was installed. Its full
  history has merge-base `b1774222c761a7912cdbe0d0004ca12dae95f721` with
  `cxx26`.
- Created `integration/llvm-22.1.8` at the tracker-bearing `cxx26` tip and
  began a no-fast-forward merge of `llvmorg-22.1.8` without rewriting history.
- Initial merge reported 86 content conflicts: 48 Clang (including reflection
  AST/Sema/serialization and parser code), 35 libc++ (headers, generated
  files, tests, and status data), two LLDB, and one LLVM ADT. The two LLDB and
  one LLVM ADT conflicts were resolved to upstream; 83 remain.
- Resolved remaining 83 conflicts to upstream LLVM 22 baseline. This makes
  Milestone 3's base-build repair attributable to upstream API/build-system
  changes, while retaining every local CXX26 change in the merge's first
  parent for the explicitly sequenced reflection and libc++ reconciliation
  milestones. No conflict markers remain.
- Committed the resolved no-fast-forward merge as `cefe063754c59` with parents
  `6dd950bcd4ac` (`cxx26`) and `ca7933e47d3a` (`llvmorg-22.1.8`). Milestone 2
  gate passed; Milestone 3 is now active.
- The first Milestone 3 build exposed conflict markers that had remained in
  staged Clang files after a failed bulk checkout. All 85 non-deleted conflict
  paths were restored directly from merge parent 2 (`llvmorg-22.1.8`), no
  markers remain, and the correction is committed immediately after this
  tracker update. The upstream deletion of
  `libcxx/test/libcxx/clang_modules_include.gen.py` remains intact.

### 2026-08-25 — Milestone 3 first base-build repair

- After the marker correction, `ninja -C build-nyx clang` reached generation
  of `Attrs.inc` and failed because the retained local reflection
  `ClangAttrEmitter.cpp` required `EscapeReflection`, a field absent from
  upstream LLVM 22 `Attr.td`.
- Restored `clang/utils/TableGen/ClangAttrEmitter.cpp`, `TableGen.cpp`, and
  `TableGenBackends.h` from the LLVM 22 merge parent. This is base-build work;
  their reflection extensions remain in the merge's first parent for
  Milestone 4 reconciliation. Rebuild in progress.
- The next rebuild reached AST serialization TableGen and found the retained
  local `ReflectionSpliceType` in `TypeNodes.td` without corresponding LLVM
  22 `TypeProperties.td` serialization metadata. Removed that local node for
  the base-build baseline; recover and reconcile it in Milestone 5.
- To prevent serial base-build failures from unrelated retained reflection
  edits, reset the complete `clang/` subtree to exact `llvmorg-22.1.8`
  contents and removed 29 local-only reflection sources/tests. Verified no
  staged difference remains between `clang/` and the release tag. This is the
  required Milestone 3 base baseline; every removed CXX26 file remains in the
  merge's first parent for Milestones 4 and 5.

### 2026-08-25 — Milestone 3 gate

- `ninja -C build-nyx clang` passed from commit `488d104cecfe6` (the initial
  tool-attached invocation was interrupted by its execution wrapper, so the
  completed build was captured in `/tmp/llvm22-m3-build.log`).
- Follow-on `ninja -C build-nyx -j4` passed with no remaining work. The exact
  LLVM 22 Clang baseline is now buildable; reflection reconciliation begins in
  Milestone 4.

### 2026-08-25 — Milestone 4 reflection driver plumbing

- Restored the reflection language options, cc1/driver option definitions,
  driver forwarding, `-freflection-latest` expansion, and dependent-option
  diagnostics in `d2ae0e59597a7`. LLVM 22 moved the option-table source from
  `clang/include/clang/Driver/Options.td` to `clang/include/clang/Options/Options.td`;
  the restored definitions use that new location.
- A Clang rebuild was started, but this workspace's attached execution wrapper
  repeatedly terminated it while rebuilding the checkout-reset tree. Resume
  its focused build before proceeding to parser/AST reconciliation.

### 2026-08-26 — Milestone 4 reflection driver validation

- The persistent `ninja -C build-nyx -j4 clang` build completed successfully:
  all 2,782 steps passed and `build-nyx/bin/clang` was relinked.
- Focused driver validation passed. `-freflection-latest` forwards
  `-freflection`, `-fparameter-reflection`, `-fattribute-reflection`,
  `-fannotation-attributes`, and `-fexpansion-statements` to cc1. Direct cc1
  use correctly rejects `-fparameter-reflection` without `-freflection` and
  accepts the dependent flag when reflection is enabled.

### 2026-08-26 — Milestone 4 lexical reflection tokens

- Restored the lexer support for `^^`, `[:`, and `:]`, plus
  `__metafunction` keyword classification and the disabled-feature warning.
  LLVM 22's `_Defer` keyword uses the previously final token-key bit, so the
  reconciliation adds `KEYREFLECT` at the next available bit instead of
  dropping `_Defer`.
- `ninja -C build-nyx -j22 clangLex` and the follow-on full
  `ninja -C build-nyx -j22 clang` relink passed. Added
  `clang/test/Lexer/cxx26-reflection-tokens.cpp`; its focused lit run passed.

### 2026-08-26 — Milestone 4 reflection-value AST foundations

- Restored `ReflectionKind`, `TagDataMemberSpec`, and `EnumeratorSpec` in
  `clang/AST/Reflection.h`, with their implementation in `Reflection.cpp`.
  This supplies the shared reflection-value vocabulary without prematurely
  restoring evaluator or serialization machinery assigned to Milestone 5.
- `ninja -C build-nyx -j22 clangAST` passed after adding `Reflection.cpp` to
  the LLVM 22 AST library.
