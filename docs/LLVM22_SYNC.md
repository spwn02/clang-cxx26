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

Milestone 4's gate passed 2026-08-28: `clang/test/Reflection/` is 15/16,
the only remaining failure being the documented Milestone 1 baseline
(`splice-exprs.cpp` line 23). See the 2026-08-28 "Milestone 4 gate: both
remaining reflection failures fixed at their root cause" Session Log entry.
Milestone 5's gate passed 2026-08-28: `clang/test/AST/ByteCode/`,
`clang/test/Modules/`, `clang/test/PCH/`, `clang/test/Reflection/`, and
`clang/test/Import/` together (1339 tests) show only the documented
Milestone 1 baseline failure. See the 2026-08-28 "Milestone 5 gate: batched
evaluator/module/PCH/reflection run, two real serialization gaps fixed"
Session Log entry. Milestone 6's gate passed 2026-08-28: `ninja -C
build-libcxx libcxx-generate-files` and `ninja -C build-libcxx cxx` are both
clean, and every real libc++/libc++abi conflict against upstream is resolved
with both sides' independent changes preserved. See the 2026-08-28 "Milestone
6 gate: libc++ reconciliation, two silently-merge-deleted files, one real M4
gap discovered" Session Log entry. Milestone 7 (focused reflection/libc++
tests) is now the sole active milestone. Its starting point is already
known and is not a fresh investigation: the libc++ reflection suite
currently fails far more broadly than Milestone 1's 6-test baseline because
`__has_feature(reflection)` was false until this session (see the M6 log
entry) — the *previous* full-suite libc++ numbers were never actually valid
evidence, since `<meta>`'s entire body is gated on that feature check and
was silently compiling to nothing. Re-baseline `libcxx/test/std/experimental/
reflection/` now that the feature check is fixed, then triage from there;
the immediate next failure class is `std::vector<meta::info>`/
`std::__split_buffer<meta::info, ...>` hitting "call to immediate function
... is not a constant expression" / "expressions of consteval-only type are
only allowed in constant-evaluated contexts" — a `ConstevalOnly`
constant-evaluator interaction (Milestone 5's domain) that never ran before
today, not a libc++ source defect.

## Milestones

- [x] **1. Capture clean baseline builds and expected failures.** Gate passed 2026-08-24: both build trees succeeded; focused results and all failures are recorded below.
- [x] **2. Fetch exact LLVM tag and merge on integration branch.** Gate passed 2026-08-25 in merge `ea04e484b0b8` and its direct upstream-resolution correction: exact signed tag merged on `integration/llvm-22.1.8`; every conflict and resolution category is recorded.
- [x] **3. Restore base LLVM/Clang build.** Gate passed 2026-08-25: `ninja -C build-nyx clang` and then `ninja -C build-nyx` passed from the exact LLVM 22 `clang/` baseline.
- [x] **4. Reconcile reflection Parser, AST, Sema, templates, and flags.** Preserve CXX26 syntax, reflection contexts, metafunction evaluation, splice behavior, and all experimental flag plumbing. Gate passed 2026-08-28: `clang/test/Reflection/` is 15/16, with the only remaining failure being the documented Milestone 1 baseline (`splice-exprs.cpp` line 23). See the 2026-08-28 "Milestone 4 gate: both remaining reflection failures fixed at their root cause" Session Log entry.
- [x] **5. Reconcile constant evaluation, modules, and AST serialization.** Audit evaluator changes and module/PCH serialization boundaries. Gate passed 2026-08-28: `clang/test/AST/ByteCode/`, `clang/test/Modules/`, `clang/test/PCH/`, `clang/test/Reflection/`, `clang/test/Import/` (1339 tests) show only the Milestone 1 baseline failure. Two real serialization gaps found and fixed (`ReflectionSpliceType` PCH deserialization, `ASTImporter` splice-scoped `NestedNameSpecifier` import); the `CXXMetafunctionExpr` callback mechanism, previously assumed to be a limitation, was empirically verified to round-trip correctly through PCH. See the 2026-08-28 Session Log entry for the one remaining caveat (the `ASTImporter` fix is compile-verified but not runtime-verified — the tool needed to exercise it does not propagate `-freflection`).
- [x] **6. Reconcile libc++ and generated C++26 files without losing local conformance work.** Preserve post-upstream C++26 implementations and regenerate module/export artifacts with LLVM 22 tooling. Gate passed 2026-08-28: `ninja -C build-libcxx libcxx-generate-files` and `ninja -C build-libcxx cxx` (660/660) are both clean; every one of the 39 libc++/libc++abi paths where upstream's merge had discarded fork content is reconciled with both sides' independent changes preserved, plus two files silently deleted by the original merge (never conflicted, so never surfaced) restored. See the 2026-08-28 Session Log entry.
- [ ] **7. Pass focused reflection/libc++ tests.** Gate: complete Clang reflection directory and libc++ reflection suite pass, allowing only failures explicitly demonstrated in Milestone 1 and still justified here.
- [ ] **8. Pass full `check-clang` and `check-cxx`.** Gate: both full suites pass, allowing only explicitly recorded pre-existing failures with before/after evidence and exact test names.
- [ ] **9. Merge integration branch into `cxx26`, push, and release.** Recheck provenance and tracker state, merge without history rewriting, push `cxx26`, create the next free annotated `cxx26-YYYY.MM.DD[.N]` prerelease tag, push it explicitly, and verify remote resolution. Gate: clean worktree, remote branch/tag verification, and this epic marked complete.

## Blockers

None currently. Milestones 4, 5, and 6 closed 2026-08-28 (see Session Log).
Milestone 7 (focused reflection/libc++ tests) is active. It is not blocked,
but its starting point is not a clean slate: the libc++ reflection suite is
currently far below the Milestone 1 baseline pass rate because a real
`__has_feature(reflection)` gap (fixed in Milestone 6, but conceptually
Milestone 4/clang-flag-plumbing scope) meant `<meta>` silently compiled to
an empty namespace for the entire libc++ portion of this sync epic until
today — no libc++ reflection test run before today's actually exercised
`std::meta`. The next concrete failure class (`ConstevalOnly`
type/constant-evaluator interaction with `std::vector<meta::info>` and
`std::__split_buffer<meta::info, ...>`) is Milestone 5-adjacent and is
recorded in the Milestone 6 Session Log entry rather than re-described here.

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

### 2026-08-26 — Milestone 4 splice-specifier AST foundations

- Restored `SpliceSpecifier` and its dependence domain. The LLVM 22
  dependence conversions are retained and extended with
  `SpliceSpecifierDependence`; no pre-existing conversion was replaced.
- `ninja -C build-nyx -j22 clangAST` passed after adding
  `SpliceSpecifier.cpp` to the AST library.

### 2026-08-26 — Milestone 4 expression-family boundary

- The LLVM 22 integration audit found that `CXXSpliceExpr` alone crosses 28
  first-parent files: AST dependence, visitors, profiling, printing, codegen,
  parser, Sema, tree transforms, static analysis, and AST serialization.
  An isolated node attempt was reverted after its AST build exposed the missing
  visitor and profiler integrations. The validated splice-specifier foundation
  remains; restore the expression family only as its complete cross-subsystem
  bundle.

### 2026-08-26 — Milestone 4 APValue reflection foundation

- Separated `ReflectionKind` into `ReflectionValue.h` so `APValue` can carry
  an opaque reflection payload without reintroducing the `APValue`/`Type`
  include cycle. `Reflection.h` retains the AST-only descriptors.
- Added copy, profile, dumper, importer, linkage, constant-evaluation, and
  mangling boundary handling for the new `APValue::Reflection` kind. Full
  evaluator representation and reflection-expression semantics remain
  assigned to the integrated expression family and Milestone 5.
- `ninja -C build-nyx -j22 clangAST` passed cleanly.

### 2026-08-26 — Milestone 4 integrated expression-bundle port

- Began one integrated restoration of the reflection expression family: AST
  nodes and dependence, visitor/printer/profiler/importer hooks, parser,
  Sema/template transforms, code generation, static analysis, expansion
  statements, and the Clang reflection tests. The initial LLVM 22 build
  exposed the upstream `NestedNameSpecifier` storage redesign; its obsolete
  pre-22 implementation was removed before porting splice scopes onto the
  new value representation.
- `ninja -C build-nyx -j22 clangSema` is rebuilding the affected dependency
  closure in detached session `llvm22-m4-build`. No gate is recorded yet;
  do not commit or mark Milestone 4 complete until that build and focused
  reflection tests pass.

### 2026-08-26 — Milestone 4 Type.h/TypeBase.h reconciliation

- Resuming the build exposed a redefinition of the whole `Type` class
  hierarchy: the staged `clang/include/clang/AST/Type.h` was byte-identical
  to the pre-merge first-parent monolith (`6dd950bcd4ac`, 9181 lines), not
  upstream LLVM 22's 93-line `Type.h` shim (LLVM 22 split the class
  hierarchy into the new `TypeBase.h`, 9233 lines). Any TU including both
  headers redefined every `Type` subclass.
- Diffing the first-parent `Type.h` against its own merge-base
  (`b1774222c761a7`) isolated exactly the local CXX26 delta (40 hunks): a
  `ConstevalOnly` bit threaded through `Type`'s bitfields/constructor and
  every subclass constructor, `STK_Reflection`, `isReflectionType()` /
  `isConstevalOnly()`, and the `ReflectionSpliceType` /
  `DependentReflectionSpliceType` classes. Ported that delta into
  `TypeBase.h` (which did not yet contain it) and reset `Type.h` to the
  upstream shim (`git checkout llvmorg-22.1.8 -- clang/include/clang/AST/Type.h`).
  Two hunks were intentionally skipped: an additional
  `DependentTemplateSpecializationType` splice constructor/Profile
  overload, because upstream LLVM 22 no longer has that class standalone
  (merged into `TemplateSpecializationType`); revisit once the
  `TemplateName`/`NestedNameSpecifier` reconciliation below lands.
- `BUILTIN_TYPE(MetaInfo, MetaInfoTy)` was missing from
  `clang/include/clang/AST/BuiltinTypes.def` even though `BuiltinType::MetaInfo`
  is already referenced from five already-staged `.cpp` files; restored it
  (matching the pre-merge fork) immediately after `NullPtr`.
- `ReflectionSpliceType` was previously removed from
  `clang/include/clang/Basic/TypeNodes.td` in `1928ee669ebd3` for the
  Milestone 3 base-build baseline. Restored it
  (`TypeNode<Type>, NeverCanonicalUnlessDependent`), which is required for
  the already-staged bundle (`ASTContext.h/.cpp`, `Type.cpp`, `TypeLoc.h`,
  `ASTImporter.cpp`, Sema/TreeTransform) to compile at all.
- Restoring that node made `-gen-clang-type-reader`/`-gen-clang-type-writer`
  fail: every concrete type node requires a `TypeProperties.td` entry, and
  `ReflectionSpliceType` never had one (the pre-merge fork used the older,
  non-declarative PCH serialization scheme). Real serialization of
  `SpliceSpecifier` (and its `Expr*` operand/template-args) is AST-
  serialization work reserved for Milestone 5. Added a deferred-limitation
  stub entry (zero properties; `Creator` is
  `llvm_unreachable("ReflectionSpliceType PCH deserialization not yet
  implemented")`) so the type is usable everywhere except through a PCH/
  module, mirroring the already-documented `CXXMetafunctionExpr`
  non-serializable-callback precedent. Verified the generated
  `AbstractTypeReader.inc`/`AbstractTypeWriter.inc` stubs compile cleanly
  (no blanket `-Werror`, so the unused-context/no-return-statement shape is
  safe).
- `ninja -C build-nyx -j22 clangAST` then failed differently: a circular
  include through `TemplateName.h` (`M`, staged) → `NestedNameSpecifier.h`
  (`MM`, has unstaged edits atop staged ones) → `Decl.h` → `DeclBase.h`,
  which needs `TagTypeKind` before it is available. `NestedNameSpecifier.h`'s
  unstaged hunk shows a prior session already mid-migration from the old
  pointer-based `NestedNameSpecifier *`/`FoldingSetNode` design (which had
  dedicated `StoredSpliceSpecifier`/`StoredSpliceSpecifierWithTemplate`
  storage kinds) to upstream LLVM 22's by-value `NestedNameSpecifier`
  built on `NestedNameSpecifierBase.h`; that migration had not yet reached
  `TemplateName.h`, which still declares `QualifiedTemplateName` and
  `DependentTemplateStorage` in terms of the old `NestedNameSpecifier *`
  and still includes the heavy `NestedNameSpecifier.h` instead of
  `NestedNameSpecifierBase.h`.
- Do not design the `NestedNameSpecifier`/`TemplateName` splice-scope
  reconciliation from scratch: continue the in-progress migration visible
  in `NestedNameSpecifier.h`/`.cpp`'s unstaged hunks first.
- Fixed two more mechanical arity issues the original diff-based worklist
  could not have listed, both confirmed independent of the
  `NestedNameSpecifier` cascade below by reading clang's instantiation
  backtrace / `git diff --cached` before editing:
  - `PredefinedSugarType`'s `Type(...)` 3-arg call (a class upstream LLVM 22
    itself added, absent from the pre-merge fork): threaded
    `/*ConstevalOnly=*/false` through it.
  - `TypeWithKeyword`'s constructor forwarded only 4 args to its
    `KeywordWrapper<Type>` base (itself forwarding to `Type`'s now-5-arg
    ctor); this is also not in the reflection diff (`TypeWithKeyword`/
    `KeywordWrapper` are upstream LLVM 22 additions). Added a
    `bool ConstevalOnly = false` parameter and forwarded it.
- `clang/include/clang/AST/Expr.h:1066` (staged, part of this bundle) had
  `llvm::detail::ConstantLog2<alignof(Expr)>::value`, which does not exist
  in this LLVM 22 checkout's `MathExtras.h` (only the plain function
  template `llvm::ConstantLog2<kValue>()`). Confirmed via
  `git diff --cached` that this line *is* part of the staged fork changes,
  not upstream — corrected to `llvm::ConstantLog2<alignof(Expr)>()`.
- Probed the include swap (`TemplateName.h`: `NestedNameSpecifier.h` →
  `NestedNameSpecifierBase.h`). It resolves the `TagTypeKind` circular
  include, confirming the include itself was wrong (should follow
  upstream and use the lightweight base header), but this is not a small
  bounded fix: rebuilding `clangAST` afterward surfaces ~220 errors, all
  downstream of the same root cause and none newly introduced by this
  session's TypeBase.h work:
  - `NestedNameSpecifier` is now used as a value type in some contexts and
    still expected as `NestedNameSpecifier *` in others (`Expr.h`,
    `TypeLoc.h`) — the pointer-to-value migration described above is
    incomplete outside `NestedNameSpecifier.h`/`.cpp` itself.
  - `ElaboratedType` no longer exists as upstream removed it (folded into
    how qualified names/keywords attach via the new `NestedNameSpecifier`
    design); `TypeLoc.h` still references `ElaboratedType`,
    `ElaboratedTypeLoc::getQualifier/getNamedType/getKeyword`,
    `UsingType::getUnderlyingType/getFoundDecl`, and
    `DependentTemplateSpecializationTypeLoc::getDependentTemplateName/
    template_arguments` in their old forms.
  - `CXXRecordDecl::isInjectedClassName` and `Decl::ExpansionStmt` are
    referenced but no longer declared as such upstream.
  - This is a multi-file, cross-cutting architectural reconciliation (the
    qualified-name/keyword representation, not just a storage swap), on
    top of unstaged, partially-designed work from a prior session. It is
    not safe to improvise a fix for `TemplateName.h`'s
    `QualifiedTemplateName`/`DependentTemplateStorage` without first
    understanding how `NestedNameSpecifier.h`'s in-progress rewrite
    intends `ElaboratedType`'s replacement and splice-scope storage to
    work end to end.
- **Handoff**: keep `TemplateName.h`'s include fixed
  (`NestedNameSpecifierBase.h`) — it is correct and necessary regardless
  of approach. The next action is to read
  `NestedNameSpecifier.h`/`.cpp`'s complete unstaged diff (not just the
  excerpt reviewed here) end to end to recover the intended new design for
  qualified names and splice scopes, identify what replaces
  `ElaboratedType`/`ElaboratedTypeLoc` upstream, and only then reconcile
  `TemplateName.h`, `TypeLoc.h`, `Expr.h`, and `DeclCXX.h` against it as one
  coherent bundle. Do not attempt file-by-file mechanical patching here —
  the errors are symptoms of one representation change, not independent
  bugs.
- Final confirmation build after both arity fixes: 200 errors, all in the
  `NestedNameSpecifier`/`ElaboratedType`/`TypeLoc` family above (plus one
  more of the same kind, `use of undeclared identifier 'DependentNamespace'`).
  Zero remaining `Type(...)`/`ConstantLog2` errors — this session's
  `TypeBase.h`/`BuiltinTypes.def`/`TypeNodes.td`/`TypeProperties.td`/
  `Expr.h` work is confirmed complete and isolated from the remaining
  blocker.

### 2026-08-26 — Milestone 4 systemic finding: wholesale-restored bundle, not reconciled

- Classifying every file in the integrated expression-bundle port
  (`diff -q` against the pre-merge `cxx26` tip `6dd950bcd4ac`) found that
  the `Type.h` defect was not an isolated mistake: roughly 55 of the ~70
  touched Clang files are **byte-identical** to their pre-merge-cxx26
  version — i.e. wholesale `git checkout 6dd950bcd4ac -- <file>`, not a
  reconciliation onto the LLVM 22 baseline established in Milestone 3.
  Every one of upstream's independent changes to those files since the
  fork point (the `ElaboratedType` removal, `NestedNameSpecifier`
  pointer-to-value redesign, `CXXBaseSpecifier`'s `BaseOfClass`→`Derived`
  rework, `CXXRecordDecl::isInjectedClassName` rename, the C++23
  immediate-escalating-function bitfield, etc.) was silently discarded.
  This is the actual source of the `NestedNameSpecifier`/`ElaboratedType`
  cascade recorded above, not a narrow TemplateName.h problem.
- Remedy: for each such file, a real three-way merge recovers the true
  local-only delta automatically:
  `git merge-file -p <(git cat-file -p 6dd950bcd4ac:<file>)
  <(git cat-file -p <merge-base>:<file>) <(git show HEAD:<file>)`
  — `git merge-file`'s real arg order is `<file1> <orig-file> <file2>`, so
  the common ancestor (`<merge-base>`) must be the **middle** argument, not
  the first; a `<merge-base> <local> <upstream>` ordering silently produces
  a no-op merge (verified 2026-08-27: it emitted upstream's content
  unchanged, discarding all 177 local-only lines of a test case). Also
  prefer real temp files over `<()` process substitution for both inputs —
  process substitution was observed to make `git merge-file` silently emit
  a truncated/empty result in this sandboxed shell even with correct arg
  order. (merge-base `b1774222c761a7912cdbe0d0004ca12dae95f721`; `HEAD:<file>`
  is already the clean upstream `llvmorg-22.1.8` content per Milestone 3's
  full `clang/` reset). This is exactly the Type.h methodology, applied
  mechanically instead of by hand.
- Ran this over the 53 wholesale-restored files with an existing upstream
  counterpart: 32 merged with **zero** conflicts, 21 produced 1–7 conflict
  hunks each (~40 total). Manually sampled the largest/most surprising
  zero-conflict diffs before trusting them (`DeclCXX.h`, `Stmt.h`,
  `Parser.h`, `DeclSpec.h`, `Lookup.h`, `SemaExprMember.cpp`,
  `SemaTemplateDeduction.cpp`, `SemaType.cpp`, the Parse/*.cpp removed-line
  cases) — every hunk was either a clean reflection addition or upstream's
  independent change correctly winning where local never touched those
  lines; the only relocated (not dropped) code was `SemaExprMember.cpp`'s
  `isRecordType`/`isPointerToRecordType` helpers, moved earlier in the
  file for the new splice-member-access overload.
- Applied all 32 zero-conflict merges directly:
  `ASTContext.h`, `ASTImporter.h`, `ComputeDependence.h`, `DeclCXX.h`,
  `IgnoreExpr.h`, `StmtCXX.h`, `Stmt.h`, `TypeLoc.h`, `StmtNodes.td`,
  `Parser.h`, `Sema/DeclSpec.h`, `Lookup.h`, `Ownership.h`, `ExprCXX.cpp`,
  `StmtPrinter.cpp`, `StmtProfile.cpp`, `CGExprAgg.cpp`, `CGExpr.cpp`,
  `CGExprScalar.cpp`, `Parse/CMakeLists.txt`, `ParseDecl.cpp`,
  `ParseDeclCXX.cpp`, `ParseExpr.cpp`, `Parser.cpp`, `Sema/DeclSpec.cpp`,
  `SemaExceptionSpec.cpp`, `SemaExprMember.cpp`, `SemaLookup.cpp`,
  `SemaTemplateDeduction.cpp`, `SemaTemplateVariadic.cpp`, `SemaType.cpp`,
  `ExprEngine.cpp`. Verified this session's own edits
  (`TypeBase.h`, `BuiltinTypes.def`, `Basic/TypeNodes.td`,
  `TypeProperties.td`, `TemplateName.h`'s include fix, `Expr.h`'s
  `ConstantLog2` fix) were untouched by the batch.
- This pass also surfaced two more restorations needed, same shape as
  `BuiltinTypes.def`/`TypeNodes.td`: a `Decl::ExpansionStmt` kind (used by
  `DeclCXX.h`'s merged `getDeclContext()` walk and referenced elsewhere)
  and `Decl::DependentNamespace` (used by the merged `DependentNamespaceDecl`
  in `DeclCXX.h`) are not yet registered — almost certainly missing entries
  in whatever replaced `DeclNodes.td`'s node list, exactly like the
  `ReflectionSpliceType`/`MetaInfo` omissions. Check there next.
- Remaining: 21 files with real conflicts to resolve by hand
  (`Type.cpp` 7, `SemaCXXScopeSpec.cpp` 4, `TreeTransform.h` 3,
  `SemaDeclCXX.cpp` 3, `ASTContext.cpp`/`SemaTemplateInstantiate.cpp` 2
  each, thirteen files with 1), plus the still-unreconciled
  `NestedNameSpecifier.h`/`.cpp` (in-progress from a prior session),
  `TemplateName.h` (`QualifiedTemplateName`/`DependentTemplateStorage`
  still pointer-based), `Expr.h`, and `Decl.h` (partially reconciled,
  not yet 3-way-merged). Not committed; `clangAST` has not built.
- Not yet committed; `clangAST` has not built successfully. Do not mark
  Milestone 4 progress beyond this note until it does.

### 2026-08-26 — Milestone 4: TemplateName.h/Expr.h/Decl.h were also wholesale-restored; DeclNodes.td and a second forgotten-files pass

- The three files above that looked "already reconciled" (different from
  the pre-merge fork) turned out to differ only by this session's own
  one-line edits; the same 3-way merge (base
  `b1774222c761a7912cdbe0d0004ca12dae95f721`, local `6dd950bcd4ac`,
  upstream `HEAD`) applied cleanly to all three with 0 conflicts and a
  tiny true delta each. This fully resolved `QualifiedTemplateName`'s and
  `DependentTemplateStorage`'s `NestedNameSpecifier *` vs value mismatch
  (upstream's redesign was never touched by the fork in this file) and
  subsumed the earlier manual `ConstantLog2` fix. Applied all three.
- Registered two more Decl nodes missing the same way as
  `ReflectionSplice`/`MetaInfo` (`clang/include/clang/Basic/DeclNodes.td`,
  currently untouched/clean): `def DependentNamespace : DeclNode<Namespace>;`
  (nested under `Namespace`, giving it membership in the
  `firstNamespace..lastNamespace` range that `DeclCXX.h`'s merged
  `NamespaceDecl::classofKind` now checks) and
  `def ConstevalBlock : DeclNode<Decl>;` /
  `def ExpansionStmt : DeclNode<Decl>, DeclContext;`. No Decl-serialization
  analog of `TypeProperties.td` exists (Decls still use the traditional
  ASTReader/ASTWriter switch, a separate library not required for
  `clangAST`/`clangSema`), so no further stub was needed there.
- Generalized the "forgotten files" check: diffed
  `b1774222c761a7912cdbe0d0004ca12dae95f721..6dd950bcd4ac` for the full
  list of files the fork ever touched (206), then filtered to those whose
  **current on-disk content** (not just `git status`) still equals
  upstream — this correctly excludes files already fixed in earlier
  commits (`Reflection.h/.cpp`, `SpliceSpecifier.h/.cpp`, etc.). 97 files
  are genuinely untouched; 5 more (`ExprConstantMeta.cpp`,
  `DiagnosticMetafn.h`, `DiagnosticMetafnKinds.td`, `Driver/Options.td`,
  `clang/test/SemaCXX/cxx2c-expansion-stmts.cpp`) don't exist on disk at
  all yet. Narrowed to the AST/Basic/Parse/Sema/Lex subset relevant to the
  `clangAST`/`clangSema` build targets (CodeGen/Serialization/Index/
  libclang/tests deferred — separate libraries, later milestones), ran
  the same 3-way merge over 45 files: 38 clean, 7 with 1-3 conflicts each.
  Applied all 38 clean ones directly, matching the same pattern as before
  (e.g. `TargetInfo.h`/`.cpp` gained `MetaInfoWidth`/`MetaInfoAlign`;
  `DeclTemplate.h` gained the full `ExpansionStmtDecl` class body its
  `Decl::ExpansionStmt` node needs).
- Resolved the 7 conflicts by hand (all genuine — real independent
  upstream changes landing on the same lines as local reflection work):
  - `DeclTemplate.cpp` (2): upstream renamed/refactored
    `getReplacedTemplateParameterList(const Decl*) -> TemplateParameterList*`
    into `getReplacedTemplateParameter(Decl*, unsigned) ->
    std::tuple<NamedDecl*, TemplateArgument>`. Kept local's
    `ExpansionStmtDecl` impl block, then added an `ExpansionStmt` case to
    the new tuple-returning function using `getTemplateParm()` (the single
    per-iteration parameter) instead of the old `getTemplateParameters()`.
  - `ODRHash.cpp` (1): local's NNS-kind switch still listed the fork's
    original (author-flagged `// TODO(CXX26): This is wrong.`) placeholder
    `NestedNameSpecifier::Splice` kind, which doesn't exist in the new
    by-value design yet. Took upstream's case list
    (`Null`/`Global`/`MicrosoftSuper`) for now; revisit once a real Splice
    kind is designed (see below).
  - `ParseStmt.cpp` (1): upstream added a `LabelDecl *PrecedingLabel`
    parameter to `ParseForStatement`; local added `template for`
    expansion-statement prefix parsing to the same function. Combined:
    upstream's new signature, local's prefix-parsing body (the later uses
    of `TemplateKWLoc`/`ExpansionStmtTemplateParm` at lines ~1972/1989/2249
    were untouched by either side and needed no changes).
  - `ParseTentative.cpp` (1): not reflection-related. Upstream's
    `Next.isNoneOf(tok::coloncolon, tok::less, tok::colon)` is a strict
    superset of local's `Tok.is(tok::identifier) && Next.isNot(...) &&
    Next.isNot(tok::less)` (the `Tok.is(identifier)` guard was already
    redundant in this context). Took upstream's version.
  - `SemaHLSL.cpp` (1): revealed that **local**, not upstream, redesigned
    `CXXBaseSpecifier`'s constructor from `(..., bool BaseOfClass, ...)` to
    `(..., CXXRecordDecl *Derived, ...)` (confirmed by re-reading the
    earlier `DeclCXX.h` batch-1 diff direction correctly this time — the
    fork added the `Derived`-pointer redesign, upstream kept the plain
    bool). Combined upstream's newer `AST.getCanonicalTagType(BaseDecl)`
    helper with local's `Derived`-pointer constructor shape. Grepped the
    whole tree afterward for other `CXXBaseSpecifier(...)` construction
    call sites — none remain unconverted.
  - `SemaDeclAttr.cpp` (3): one whitespace-only conflict (took upstream's
    indentation); `AL.isClangScope() || AL.isGNUScope()` (local, kept —
    additive, part of the annotation-attribute surface); and a switch
    where local added `AnnotationAttribute`/`AT_InstantiationDependent`
    cases while upstream independently added `AT_ModularFormat`/
    `AT_MSStruct`/`AT_GCCStruct` cases — concatenated both, with only the
    final case keeping the shared trailing `break;`.
  - `Lexer.cpp` (2 apparent conflicts): false positive — this file was
    already properly reconciled in an earlier committed session
    (`2a1aee668e5c8`, splice/`l_splice` token lexing), so it's no longer
    "forgotten"; the conflict was local's fork vs. that earlier commit's
    differently-worded comment on equivalent logic. Left untouched.
- Confirmed real, not yet designed: `NestedNameSpecifier::Kind` has no
  splice/scope variant. `NestedNameSpecifierBase.h`'s `StoredOrFlag` tags
  `StoredKind` in 2 bits (`FlagBits = 2`), and all 4 slots
  (`Type`/`NamespaceOrSuper`/`NamespaceWithGlobal`/`NamespaceWithNamespace`)
  are already used — adding a splice-scope kind needs either a bit-width
  bump (with an alignment/`NumLowBitsAvailable` audit of every pointee
  type and every `PointerUnion`/`PointerIntPair` user) or reusing an
  existing slot's pointee polymorphically. The fork's own pre-merge
  placeholder (`StoredSpliceSpecifier`/`StoredSpliceSpecifierWithTemplate`
  on the old pointer-based design, explicitly commented as wrong) is not a
  usable template for the new value-based design. This is the real design
  gap, not mechanical work — do not improvise it under build-error
  pressure; it needs its own focused pass once the rest of the mechanical
  reconciliation is done and the actual required call sites (splice as
  NNS scope, splice as template name) are enumerated from remaining build
  errors and `clang/test/Reflection/splice-namespaces.cpp`/
  `splice-templates.cpp`.
- Not committed; `clangAST` has not built. Rebuilding next.

### 2026-08-26 — Milestone 4: DiagnosticGroups.td, a whole missing diagnostic component, and Attr.td/ClangAttrEmitter reconciliation

- Rebuild after the above dropped to 4 errors, all
  `Variable not defined: 'ReflexingParse'` from `DiagnosticParseKinds.td`
  (already merged) referencing a `DiagGroup` never defined. 3-way merged
  `clang/include/clang/Basic/DiagnosticGroups.td` (0 conflicts, 1-line true
  delta: `def ReflexingParse : DiagGroup<"reflexing-parse">;`). Applied.
- Next rebuild: a whole diagnostic *component* is missing —
  `DIAG_START_METAFN`/`DIAG_SIZE_METAFN`/`NUM_BUILTIN_METAFN_DIAGNOSTICS`
  undefined. CXX26 metafunction diagnostics
  (`clang/include/clang/Basic/DiagnosticMetafnKinds.td`,
  `DiagnosticMetafn.h`) are genuinely new files (no upstream counterpart,
  not in git status because they were simply never created in this
  worktree) — copied both from the pre-merge fork verbatim (no merge
  needed, pure additions). Wired the new "Metafn" component into every
  place the existing 13 components (`AST`, `Analysis`, `CrossTU`, ...) are
  registered: `DIAG_SIZE_METAFN`/`DIAG_START_METAFN` inserted between
  `CrossTU` and `Sema` in `DiagnosticIDs.h`'s two enums;
  `include "DiagnosticMetafnKinds.td"` added to `Diagnostic.td`
  (alphabetical, between Lex and Parse); `clang_diag_gen(Metafn)` added to
  `clang/include/clang/Basic/CMakeLists.txt`; `#include
  "clang/Basic/DiagnosticMetafn.h"` added to `AllDiagnostics.h`. (Noted in
  passing: `clang/include/clang/Basic/AllDiagnosticKinds.inc` is an
  untracked stray build artifact sitting in the source tree, not a real
  file to reconcile — `git ls-files` confirms it isn't tracked.)
- Next rebuild: a new generated file, `AttrReflection.inc`
  (`-gen-clang-attr-reflection`, the P3385 attribute-reflection backend),
  needs every `Attr` subclass to have `extractSyntacticArguments()`, plus
  several attribute `Kind` enumerators were missing. This is the
  `ClangAttrEmitter.cpp` restoration the Milestone 3 log flagged and
  explicitly deferred ("required `EscapeReflection`, a field absent from
  upstream LLVM 22 `Attr.td`... their reflection extensions remain in the
  merge's first parent for Milestone 4 reconciliation"). 3-way merged
  `Attr.td` (1 conflict: local added `EscapeReflection`, upstream
  independently added an unrelated `IsTypeDependent` bit — kept both),
  `ClangAttrEmitter.cpp` (0 conflicts, 323-line delta: refactors
  `getParsedAttrList` to expose a per-record `getParsedAttrFromRecord`
  helper, and threads a new `StrictEnumParameters`-driven `isStrict` flag
  through `isIdentifierArgument`, both used by the reflection backend),
  `TableGen.cpp` and `TableGenBackends.h` (0 conflicts, wire up
  `-gen-clang-attr-reflection`). Applied all four.
- That rebuild reached 123 errors, cleanly isolated to two things: the
  `NestedNameSpecifier`/`ElaboratedType` splice-scope design gap (see
  above, still deferred), and a new discovery: `APValue`'s reflection API
  had regressed to a stale multi-accessor shape. See next entry.

### 2026-08-26 — Milestone 4: APValue reflection API restoration, a real circular-include bug, and 13 more reconciled files

- `getReflectedDecl`/`getReflectedType`/`getReflectedTemplate`/etc. were
  missing from `APValue`, breaking `ComputeDependence.cpp`,
  `SemaTemplateInstantiate.cpp`, `SemaTemplateVariadic.cpp`,
  `SemaReflect.cpp`, `TreeTransform.h`, `RecursiveASTVisitor.h`. Root
  cause: `clang/include/clang/AST/APValue.h`/`clang/lib/AST/APValue.cpp`
  were still a wholesale, un-reconciled copy of the pre-merge fork
  (`6dd950bcd4ac`), while an earlier commit this sync
  (`cf512ecceee84`, "Restore APValue reflection foundation") had
  independently landed a *simplified* replacement — a single
  `ReflectionData{ReflectionKind Kind; const void *Data;}` payload with
  generic `getReflectionKind()`/`getOpaqueReflectionData()` — that was
  correct as a foundation but dropped the fork's full typed-accessor
  layer and its `ReflectionDepth`/`UnderlyingTy` mechanism (the
  "reflection of a value/object is the underlying `APValue` with N
  layers of reflection over it" design, `APValue::Lift`/`Lower`). 3-way
  merged both files (base = pre-LLVM22-merge common ancestor, local =
  `6dd950bcd4ac`, upstream = current HEAD): 4 conflicts in `APValue.h`, 2
  in `APValue.cpp`, all resolved in favor of the fork's fuller design
  (the committed foundation's minimal accessors were a scaffold, not a
  design decision to strip the typed layer — confirmed by checking that
  the fork's own `ReflectionKind` enum already enumerates exactly the
  typed-accessor set). The stale call sites listed above needed no
  changes; they were already written against the correct (typed) API.
- This surfaced a real, independent circular-include bug (not a merge
  artifact): `clang/include/clang/AST/Reflection.h` included
  `clang/AST/Type.h`, but `Type.h` is deliberately the *heavy* shim
  ("this file defines some inline methods for clang::Type which depend
  on Decl.h, avoiding a circular dependency" — it `#include`s `Decl.h`
  and `DeclCXX.h` directly) as opposed to the lightweight `TypeBase.h`.
  Since `APValue.h` includes `Reflection.h`, and `Decl.h`/`Stmt.h`
  include `APValue.h`, this created a genuine cycle
  (`APValue.h` → `Reflection.h` → `Type.h` → `Decl.h` → `APValue.h`,
  guard-skipped, leaving `APValue` undefined) that broke `Decl.h` itself
  and cascaded into ~150 unrelated-looking errors (`getFirstDecl`
  ambiguity, covariant-return failures on unrelated `Decl` subclasses,
  bogus `static_cast`-not-related-by-inheritance errors) across any
  translation unit reaching `APValue.h` through `Reflection.h`. Fixed by
  changing `Reflection.h`'s include from `clang/AST/Type.h` to
  `clang/AST/TypeBase.h` (all it actually needs is `QualType`, defined in
  `TypeBase.h`); confirmed `MetaActions.h`/`Metafunction.h` (the other
  consumers of `Reflection.h`) already include `Type.h` themselves for
  the Decl-dependent inline methods, so this didn't regress them.
- While restoring the fork's design, found and fixed three genuine
  pre-existing bugs in the original fork's own `APValue.cpp` (not
  introduced by this sync, just never compiled before): `needsCleanup()`
  and `Profile()` and the linkage-computation switch in
  `getLVForValue`/`unwrapReflectedType`'s helper each had a duplicate
  `case Reflection:`/`case APValue::Reflection:` label — one correct, one
  a leftover from an earlier draft — plus a dead `ElaboratedType`-unwrap
  branch in `unwrapReflectedType` referencing a type node upstream
  removed entirely. All fixed by deleting the stale/dead branch, not by
  reinventing behavior. **Flag for the NNS design pass**: the
  `ElaboratedType` unwrap step deleted from `unwrapReflectedType` was
  removed on the assumption that no `QualType` can be elaborated-wrapped
  in the new design at all; verify this once the NNS/`TagType` prefix
  redesign is understood (elaborated-type qualification may have moved
  onto `TagType`'s own prefix per `TagTypeLoc::setQualifierLoc`/
  `TX->getPrefix()` seen in the `ASTContext.cpp`/`TreeTransform.h`
  conflicts) — if so this needs a *replacement* unwrap step, not a bare
  deletion, and `clang/test/Reflection/` dealias tests are where a wrong
  assumption here would surface.
- Fully 3-way merged (base = pre-LLVM22-merge ancestor, local =
  `6dd950bcd4ac`, upstream = pure `llvmorg-22.1.8` tag content — these
  were confirmed byte-identical to `6dd950bcd4ac` on disk, i.e. still
  wholesale copies) and applied 13 more files, all single-conflict:
  `ExprCXX.h`, `Sema.h`, `ComputeDependence.cpp`, `DeclCXX.cpp`,
  `Expr.cpp`, `ExprClassification.cpp`, `ParseExprCXX.cpp`,
  `ParseTemplate.cpp`, `SemaDeclCXX.cpp`, `SemaExpr.cpp`,
  `SemaLambda.cpp`, `SemaOverload.cpp`, `SemaTemplateInstantiateDecl.cpp`.
  Each conflict was inspected individually rather than mechanically
  resolved; notable ones: `Sema.h`/`SemaDeclCXX.cpp`'s
  `ActOnUsingEnumDeclaration`/`ActOnNamespaceAliasDef` keep the fork's
  extra splice-driven overloads layered on top of upstream's independent
  `CXXScopeSpec*`→`CXXScopeSpec&` and `RecordType*`→bool+
  `castAsCXXRecordDecl()` modernizations; `DeclCXX.cpp`'s `NamespaceDecl`
  constructor combines the fork's `Kind K` parameter (needed for the
  `DependentNamespace` decl kind) with upstream's new `NamespaceBaseDecl`
  base class; `SemaTemplateInstantiateDecl.cpp` drops the fork's manual
  immediate-function-context bookkeeping entirely in favor of upstream's
  new `EnterExpressionEvaluationContextForFunction` helper, which already
  subsumes it (confirmed by reading the helper's implementation).
- Also fixed, independent of any merge: `Stmt.h`'s
  `CXXConstructExprBitfields` was missing the `IsImmediateEscalating` bit
  that `ExprCXX.h`'s (upstream-added) `isImmediateEscalating()` accessor
  and `SemaExpr.cpp` (already pure upstream, untouched) both need —
  restored verbatim from the `llvmorg-22.1.8` tag. Removed a duplicate
  `llvm::DenseMapInfo<llvm::FoldingSetNodeID>` specialization from
  `ASTContext.cpp` (already present in the reconciled `ASTContext.h`).
  Replaced 2 calls to the now-deleted `TagDecl::getTypeForDecl()` in
  `APValue.cpp` with `TD->getASTContext().getCanonicalTagType(TD)`.
- Net effect: `ninja -C build-nyx -j22 clangAST` error count
  123 → 91, with the remaining 91 now cleanly isolated to (a) the
  `NestedNameSpecifier`/`ElaboratedType` splice-scope design gap (still
  correctly deferred, not attempted), and (b) the 4 still-unreconciled
  multi-conflict files from the original systemic-discovery batch:
  `clang/lib/AST/ASTContext.cpp` (2 conflicts — includes the
  `findPointerAuthContent`/`getMemberPointerType` mismatch against the
  already-merged `ASTContext.h`), `clang/lib/AST/Type.cpp` (7),
  `clang/lib/Sema/SemaCXXScopeSpec.cpp` (4), `clang/lib/Sema/
  TreeTransform.h` (3). `ASTImporter.cpp`, `ASTStructuralEquivalence.cpp`,
  and `SemaExprCXX.cpp`/`SemaTemplate.cpp`'s remaining single conflicts
  were confirmed to directly touch the deferred NNS/splice gap
  (`NestedNameSpecifier::Splice`/`SpliceWithTemplate`,
  `Builder.MakeSpliceScopeSpecifier`) and were deliberately left
  untouched rather than worked around.
- Not committed; `clangAST` has not built successfully. Do not mark
  Milestone 4 progress beyond this note until it does.

### 2026-08-27 — Milestone 4: Sema splice-scope reconciliation checkpoint

- Reconciled `SemaCXXScopeSpec.cpp`, `TreeTransform.h`,
  `SemaTemplateInstantiate.cpp`, `SemaDeclCXX.cpp`, `SemaExpr.cpp`,
  `SemaExprCXX.cpp`, `SemaExprMember.cpp`, and `SemaLambda.cpp` onto LLVM 22
  APIs while retaining reflection/splice behavior. Adjusted the namespace
  alias helper declaration in `Sema.h` to LLVM 22's `NamespaceBaseDecl`.
- Direct LLVM 22 syntax compilation passes for all listed implementation
  files. Remaining diagnostics are switch-coverage warnings for reflection
  extensions.
- Next action: three-way reconcile `SemaOverload.cpp` and `SemaTemplate.cpp`,
  then port the new local `SemaReflect.cpp` across LLVM 22 template, type, and
  nested-name-specifier APIs. Full build and focused reflection tests remain
  pending.

### 2026-08-26 — Milestone 4: by-value splice scopes and Type.cpp reconciliation

- Added splice and `template`-splice encodings to LLVM 22's by-value
  `NestedNameSpecifier`. Reused previously-unassigned odd low-bit tags, keeping
  the existing 8-byte pointer-alignment contract and one available low bit for
  `PointerLikeTypeTraits`; no alignment-width expansion was required.
- Added splice dependence, printing, source-location storage, canonical/value
  access, and builder support. Namespace prefixes remain restricted to LLVM
  22's valid namespace/global forms.
- Reconciled `Type.cpp` onto LLVM 22's `TypeWithKeyword`, `TagType`,
  `SubstPackType`, and by-value qualifier architecture while retaining the
  fork's `ConstevalOnly` propagation and splice-qualified member-pointer
  sugar behavior. Also reconciled three independent LLVM 22 API changes in
  `Expr.cpp` and the expanded constant-expression entry-point signature.
- Direct LLVM 22 compile commands pass for `Type.cpp`, `Expr.cpp`, and
  `ExprConstant.cpp`; a syntax-only pass over all modified AST sources reached
  `Type.cpp` as the sole substantive failure before that reconciliation.
  Full `clangAST` remains pending because the Ninja database repeatedly reports
  premature EOF and rebuilds its dependency closure from scratch.

### 2026-08-26 — M4 canonical rebase and first clean LLVM 22 compile pass

- Rebased all conflict-free M4 parser/AST/Sema/CodeGen files with correct
  three-way order (`cxx26`, merge base, `llvmorg-22.1.8`) instead of retaining
  pre-merge wholesale copies.
- Reconciled `ASTContext.cpp`, `ASTImporter.cpp`, `ExprCXX.h`,
  `RecursiveASTVisitor.h`, `Sema.h`, `ASTStructuralEquivalence.cpp`,
  `ComputeDependence.cpp`, and `DeclCXX.cpp` against LLVM 22 APIs.
- Direct LLVM 22 compile commands now pass for `ASTContext.cpp`,
  `ASTImporter.cpp`, `ExprCXX.cpp`, `ASTStructuralEquivalence.cpp`,
  `ComputeDependence.cpp`, and `DeclCXX.cpp`.
- Full `clang` build regenerated dependencies and reached Clang AST sources;
  fixed its first three failing translation units. Persistent Ninja database
  recovery restarts the 2,700-file dependency build, so use direct translation
  unit commands for continued reconciliation before the final canonical gate.
- Milestone remains active and uncommitted. Next action: continue direct AST
  translation-unit compilation, then implement LLVM 22 by-value
  `NestedNameSpecifier` splice storage required by namespace splice scopes.

### 2026-08-26 — Milestone 4: ASTContext.cpp reconciled; remaining errors now isolated entirely to the deferred NNS/splice gap

- 3-way merged (base = pre-LLVM22-merge ancestor, local = `6dd950bcd4ac`,
  upstream = pure `llvmorg-22.1.8` tag) and applied `clang/lib/AST/
  ASTContext.cpp`'s remaining 2 conflicts, both squarely inside the
  deferred `NestedNameSpecifier` redesign and resolved by taking
  upstream's side: `isSameQualifier`'s `Splice`/`SpliceWithTemplate`
  cases (no equivalent yet in the new value-based `NestedNameSpecifier`,
  same as every other NNS conflict resolved this session), and the
  entire pointer-based `ASTContext::getCanonicalNestedNameSpecifier`
  helper, which upstream removed outright (its callers, still using it,
  are already flagged under the same deferred gap — deleting the
  now-orphaned definition doesn't create a new failure mode). This
  incidentally fixed the `findPointerAuthContent`/`getMemberPointerType`
  "out-of-line definition does not match" errors, which were just fallout
  from this file still being a wholesale, un-reconciled copy — the rest
  of the file's content (deleted-file-vs-upstream diff) applied cleanly
  with no further conflicts once these two were resolved.
- Inspected `clang/lib/Sema/SemaCXXScopeSpec.cpp` (4 conflicts) and
  `clang/lib/Sema/TreeTransform.h` (3 conflicts) individually rather than
  applying either: confirmed all 4 of the former, and 2 of the latter 3,
  are pure `NestedNameSpecifier`/`ElaboratedType`/`DependentTemplateSpecializationType`
  design-gap territory (old `Kind::Identifier/TypeSpec/Super/Splice/
  SpliceWithTemplate` enumerators, `ElaboratedType`-based
  `TransformDependentTemplateSpecializationType` restructuring). Left
  both files untouched as still-wholesale copies rather than
  partially-apply — a partial merge would leave them broken either way,
  and mixing reconciled-vs-unreconciled hunks in one file makes the next
  session's job harder, not easier. **Flagged one thing worth carrying
  into the NNS design pass**: `TreeTransform.h`'s third conflict (around
  the `LDK_NeverDependent` computation for lambda contexts) is *not*
  NNS-related — it's an independent upstream behavioral change (generic
  lambda context handling via `dyn_cast_or_null<CXXRecordDecl>(DC->
  getParent())->isGenericLambda()` replacing local's `ManglingContextDecl`
  check) that should be merged normally (both changes may need to be
  combined, similar to the `Expr.cpp`/`SemaDeclCXX.cpp` "two independent
  improvements" conflicts resolved earlier this session) once the file's
  other two conflicts are addressed — do not let it get bulldozed by
  whichever side "wins" the NNS redesign.
- Net effect: `ninja -C build-nyx -j22 clangAST` error count
  91 → 75. Every remaining error is now one of: (a) a direct
  `NestedNameSpecifier`/`ElaboratedType` design-gap symptom, or (b)
  inside one of the four still-untouched files that need that same
  redesign first (`Type.cpp` — 7 conflicts, `SemaCXXScopeSpec.cpp` — 4,
  `TreeTransform.h` — 3, plus `ASTImporter.cpp`/
  `ASTStructuralEquivalence.cpp`/`SemaExprCXX.cpp`/`SemaTemplate.cpp`'s
  single conflicts each). There is no more "mechanical 3-way merge"
  low-hanging fruit left in Milestone 4 — the only path to `clangAST`
  building is the dedicated `NestedNameSpecifier` splice-scope design
  pass described earlier in this log (bit-width bump vs. slot reuse in
  `NestedNameSpecifierBase.h`'s `StoredKind`), which then unblocks
  `Type.cpp`, `SemaCXXScopeSpec.cpp`, `TreeTransform.h`, `ASTImporter.cpp`,
  `ASTStructuralEquivalence.cpp`, `SemaExprCXX.cpp`, and `SemaTemplate.cpp`
  together (they all fail on the same handful of missing/renamed
  `NestedNameSpecifier` members: `Kind::Identifier/TypeSpec/Super/Splice/
  SpliceWithTemplate`, `->getPrefix()`/`->getAsNamespace()`/
  `->getAsIdentifier()` as pointer-style calls, `ElaboratedType` itself).
- Not committed; `clangAST` has not built successfully. Do not mark
  Milestone 4 progress beyond this note until it does.

### 2026-08-27 — Milestone 4: SemaOverload.cpp/SemaTemplate.cpp reconciled; 15 more wholesale files discovered and fixed

- Resumed from the 2026-08-26 checkpoint's stated next action (three-way
  reconcile `SemaOverload.cpp`/`SemaTemplate.cpp`) but first re-verified
  against on-disk content rather than trusting the prior session's own log:
  both files were confirmed byte-identical to the pre-merge fork tip
  `6dd950bcd4ac`, i.e. genuinely still wholesale despite an earlier
  (2026-08-26, "APValue reflection API restoration") log entry listing
  `SemaOverload.cpp` among files already "applied" — that earlier entry was
  wrong for this file; trust `git diff --quiet 6dd950bcd4ac -- <file>`, not
  prior narrative.
- 3-way merged both (base `b1774222c761a7912cdbe0d0004ca12dae95f721`, local
  `6dd950bcd4ac`, upstream = current `HEAD`, confirmed equal to the
  `llvmorg-22.1.8` tag for both files). One conflict each:
  - `SemaOverload.cpp`: `AddTypesConvertedFrom`'s local reflection-type
    branch (`Ty->isReflectionType()`) combined with upstream's
    `RecordType*` → `bool TyIsRec` modernization.
  - `SemaTemplate.cpp`: `UnnamedLocalNoLinkageFinder::VisitNestedNameSpecifier`
    combined local's reflection intent with upstream's by-value
    `NestedNameSpecifier`/`Kind` redesign; added `Kind::Splice` and
    `Kind::SpliceWithTemplate` to the same no-recursion-needed bucket as
    `MicrosoftSuper` (`SpliceSpecifier` has no prefix chain to walk, mirroring
    the ODRHash.cpp precedent from the 2026-08-26 log, now revisited since a
    real Splice kind exists).
  - Verified reflection-content line counts (`grep -ci
    'splice\|reflect\|metafunction'`) before/after against the fork tip
    matched (9→9, 11→12, the +1 explained by the added switch case) — the
    "did the merge silently drop a local addition" check the systemic-finding
    entries above established but that this session had initially skipped.
- Both files then pass direct LLVM 22 `-fsyntax-only` compilation (only the
  already-documented `EnumeratorSpec` switch-coverage warning).
- Before continuing, swept the *entire* fork-touched file list (206 files
  changed between merge-base and `6dd950bcd4ac`) for any still byte-identical
  to the fork tip, since the `SemaOverload.cpp` discrepancy proved the
  session-log narrative alone isn't trustworthy. Found 15 more, all real
  (non-trivial) local deltas, contradicting their own prior "applied" log
  entries: `LocInfoType.h`, `StmtCXX.h`, `Parse/CMakeLists.txt` (both the
  `include` and `lib` ones), `RAIIObjectsForParser.h`, `Sema/Lookup.h`,
  `AttrImpl.cpp`, `ExprClassification.cpp`, `NSAPI.cpp`, `StmtCXX.cpp`,
  `OperatorPrecedence.cpp`, `ParseExprCXX.cpp`, `ParseTemplate.cpp`,
  `ScopeInfo.cpp`, `SemaTemplateInstantiateDecl.cpp`.
- 3-way merged all 15: 11 zero-conflict (of which 3 —  `StmtCXX.h`,
  `Sema/Lookup.h`, `Parse/CMakeLists.txt` — merged to content byte-identical
  to the fork tip, confirmed correct because upstream made zero independent
  changes to those files beyond the shared merge-base, not a dropped-merge
  artifact). 4 had 1 conflict each, resolved by hand:
  - `ExprClassification.cpp`: local's `CXXSpliceExprClass` value-classification
    case and upstream's independent `MatrixSingleSubscriptExprClass` case are
    unrelated additions in the same switch region; kept both.
  - `ParseExprCXX.cpp` (`ParseCXXCondition`): local changed the condition
    expression's evaluation context from `ConstantEvaluated` to
    `ImmediateFunctionContext` while upstream, independently, flattened the
    same region's helper lambda into plain statements (pure style, verified
    by diffing base vs. upstream directly — no semantic change upstream side).
    Combined: upstream's flattened structure, local's `ImmediateFunctionContext`.
  - `ParseTemplate.cpp`: comment-only conflict; took upstream's more
    descriptive wording.
  - `SemaTemplateInstantiateDecl.cpp`: local's manual immediate-function-context
    bookkeeping vs. upstream's new `EnterExpressionEvaluationContextForFunction`
    helper. Confirmed by reading `Sema::PushExpressionEvaluationContextForFunction`
    (`SemaExpr.cpp:17945`) that it already subsumes local's logic
    (`FD->isConsteval()` → `ImmediateFunctionContext`,
    `InImmediateEscalatingFunctionContext = FD->isImmediateEscalating()`) —
    this is the same resolution the 2026-08-26 log already documented for a
    different call site; applied the identical precedent here.
- Diagnosed and fixed a real shell-portability bug during verification, not a
  reconciliation bug: this environment's default shell is `zsh`, which does
  not word-split unquoted variable expansion by default. Storing the direct
  LLVM 22 compile command in a `FLAGS="..."` shell variable and interpolating
  `$FLAGS` unquoted silently collapsed every flag into one argument, so the
  first batch verification pass reported dozens of spurious "no member"/
  "unknown type" errors for `AttrImpl.cpp`, `ExprClassification.cpp`,
  `NSAPI.cpp`, `StmtCXX.cpp`, and `OperatorPrecedence.cpp` that were entirely
  artifacts of the broken include-path search list (confirmed via `clang++
  -v -fsyntax-only`, which showed only 2 of 8 intended `-I` flags actually
  reaching the compiler). Re-running the identical command through `bash -c`
  (which does word-split) showed all 5 files compile cleanly. **Always wrap
  multi-flag direct-compile verification in `bash -c '...'` or write flags
  literally on the command line in this environment; never trust a bare
  `$VAR`-interpolated multi-flag command's error output at face value.**
- After the shell-bug correction, `SemaTemplateInstantiateDecl.cpp` had two
  genuine remaining errors (independent of the merge conflict already
  resolved above), both instances of the by-value `NestedNameSpecifier` and
  `NamespaceBaseDecl` migrations already established elsewhere in this sync:
  `VisitNamespaceAliasDecl`'s `NestedNameSpecifier *NNS = ...` →
  `NestedNameSpecifier NNS = ...` (matching the `NNS && NNS.isDependent()`
  idiom already used in `SemaCXXScopeSpec.cpp`/`SemaDeclCXX.cpp`/`SemaExpr.cpp`/
  `SemaTemplate.cpp`), and `NamedDecl *NSDecl` → `NamespaceBaseDecl *NSDecl`
  (matching `NamespaceAliasDecl::Create`'s and `getAliasedNamespace()`'s
  already-reconciled `NamespaceBaseDecl *` signatures in `DeclCXX.h`).
- All 17 files touched this session (the original 2 plus the 15 newly
  discovered) now pass direct LLVM 22 `-fsyntax-only` compilation with no
  errors, only the already-documented reflection-switch-coverage warnings.
- **Carry forward**: the same "trust `git diff --quiet <file> <fork-tip>`
  over the session log" sweep should be repeated again before trusting *this*
  session's own "applied" claims in a future session — the failure mode
  (a file logged as reconciled that silently reverted to, or was never
  actually saved as, the wholesale fork copy) has now recurred twice.
- Next action: repeat the wholesale-file sweep restricted to Parse/Sema/AST
  files not yet checked this pass, then port the new local `SemaReflect.cpp`
  (no upstream counterpart — a manual port, not a merge) across LLVM 22
  template, type, and nested-name-specifier APIs, letting the compiler drive
  fixes batched by API per the enumerated failure classes in
  `HANDOFF_2026-08-26.md`.

### 2026-08-27 — Milestone 4: SemaReflect.cpp manually ported to LLVM 22, zero errors

- `SemaReflect.cpp` has no upstream counterpart, so this was a manual port
  driven by the compiler's error list (`-ferror-limit=0`), not a merge.
  Started at 23 direct errors (some duplicated across `SmallVector.h`
  instantiation sites), all falling into the API-mismatch classes the
  2026-08-26 handoff had already enumerated. Fixed in batches by API:
  - `TagDecl::getTypeForDecl()` removed upstream (6 call sites): replaced
    with `Decl->getASTContext().getCanonicalTagType(Decl)`, matching the
    precedent already established in `APValue.cpp` this sync.
  - `ASTContext::getRecordType(RecordDecl*)` removed upstream (1 call site,
    the `std::source_location` metafunction result type): same
    `getCanonicalTagType` replacement.
  - `CXXScopeSpec::Extend(Context, TypeLoc, Loc)` no longer exists — a
    type-qualified scope spec is always the *first* component now, so the
    correct call is `Make`, not `Extend` (3 call sites, matching the idiom
    already used throughout `SemaCXXScopeSpec.cpp`).
  - `Sema::CheckTemplateIdType`/`ASTContext::getDeducedTemplateSpecializationType`
    both gained a leading `ElaboratedTypeKeyword` parameter and trailing
    `Scope*`/`bool ForNestedNameSpecifier` parameters upstream (6 call
    sites); passed `ElaboratedTypeKeyword::None`, `/*Scope=*/nullptr`,
    `/*ForNestedNameSpecifier=*/false` throughout, matching every other
    reconciled call site of this API in the tree (`SemaCoroutine.cpp`,
    `SemaDeclCXX.cpp`, etc.) — these are inferred defaults, not verified
    against the fork's original intent, so a later reflection-test failure
    touching template-id splices should look here first.
  - `Sema::CheckVarTemplateId(VarTemplateDecl*, ...)` gained a trailing
    `bool SetWrittenArgs` (1 call site); passed `/*SetWrittenArgs=*/false`,
    matching the 6-arg overload's own internal call to the 5-arg one.
  - `ParsedTemplateArgument::getLocation()` renamed to `getNameLoc()` (2 call
    sites); its 4-arg `(TemplateKwLoc, SS, Template, NameLoc)` constructor
    gained the leading `TemplateKwLoc` (1 call site, passed `SourceLocation()`
    since no real template keyword exists in this synthetic-argument path).
  - `NestedNameSpecifierLoc::getNestedNameSpecifier()` now returns
    `NestedNameSpecifier` by value, not `NestedNameSpecifier *` (1 call site,
    `SS.getScopeRep()->isDependent()` → `.isDependent()`).
  - `UsingType::getFoundDecl()` renamed to `getDecl()` — verified via the
    fork tip (`6dd950bcd4ac`) that both always returned `UsingShadowDecl *`
    (the same stored field, just renamed), not a different entity, so this
    is a pure rename with no semantic change.
  - `TemplateSpecializationTypeLoc::setTemplateNameLoc()` no longer exists
    (only `DeducedTemplateSpecializationTypeLoc` kept the single-field
    setter); `ASTContext::getReflectionSpliceType` was confirmed (by reading
    its implementation) to *always* return a `ReflectionSpliceType*`
    regardless of the underlying reflected entity, which made the calling
    code's `isa<TemplateSpecializationType>`/`isa<DeducedTemplateSpecializationType>`
    branches in `BuildReflectionSpliceTypeLoc` dead on every path. Collapsed
    the three-way branch to a single `TLB.pushTrivial(Context, SpliceTy,
    Loc)`. Verified this is not a behavior change: `ReflectionSpliceTypeLoc`
    defines no `getInnerType()`, so `TypeLoc::getNextTypeLoc()` returns empty
    for it and `pushTrivial`'s desugar-chain walk produces exactly the one
    no-op `initializeLocal` call that the bare `TLB.push<ReflectionSpliceTypeLoc>`
    it replaced already did.
- `SemaReflect.cpp` now passes direct LLVM 22 `-fsyntax-only` compilation
  with zero errors; the only remaining diagnostics are the two
  already-documented `EnumeratorSpec`-not-handled `-Wswitch` warnings.
- Not yet run through the focused reflection test suite or a full
  `clangSema`/`clangAST` build — `NestedNameSpecifier`/`ElaboratedType`
  splice-scope-dependent files (`Type.cpp`, `SemaCXXScopeSpec.cpp`,
  `TreeTransform.h`, `ASTImporter.cpp`, `ASTStructuralEquivalence.cpp`,
  `SemaExprCXX.cpp`) are still pending from earlier in Milestone 4, so a
  full library build is not yet expected to succeed. Next action: continue
  clearing the remaining wholesale/conflicted files (see the 2026-08-26
  systemic-finding entries for the exact list) toward a first successful
  `clangAST`/`clangSema` build.

### 2026-08-27 — Milestone 4: first `clangAST`/`clangSema` build attempt, wholesale sweep

- Kicked off the first real `ninja -C build-nyx clangAST clangSema` build of
  the session (ninja's on-disk state had gone stale enough to warrant a full
  `-j$(nproc)` rebuild from LLVMSupport up; the "premature end of file"
  warning noted in earlier sessions is `ninja` recovering its own build log,
  not a source problem).
- First failure was unrelated to reflection: `clang/lib/AST/TextNodeDumper.cpp`
  still `#include`s `llvm/Frontend/HLSL/HLSLRootSignatureUtils.h`, the fork's
  pre-rename header name. Upstream renamed/split this to
  `llvm/Frontend/HLSL/HLSLRootSignature.h` before `llvmorg-22.1.8` (confirmed
  via `git show <upstream-tag>:clang/lib/AST/TextNodeDumper.cpp`, whose own
  `#include` already uses the new name, and via upstream commit
  `9ec5afea7737` "[NFC][RootSignature] Move RootSignature util functions").
  The M2 merge conflict on this line resolved to the fork's stale name
  instead of upstream's. Fixed by matching upstream's include; verified the
  only symbol the file uses from it, `llvm::hlsl::rootsig::dumpRootElements`,
  is declared in the new header. No other file in the tree referenced the
  old header name.
- Ran the carry-forward "reflection-content line count" sweep tree-wide this
  time (every `clang/` file present at fork tip `6dd950bcd4ac`, comparing
  `grep -ci 'splice|reflect|metafunction'` then vs. now), not just a
  hand-picked file list. Most deltas are false positives from legitimate
  upstream file splits/renames that a per-path count can't see across:
  `Type.h` (29→0) is now a 93-line upstream shim, with the content that
  matters moved into the new `TypeBase.h` (0→27, i.e. carried over
  correctly); `Reflection.h` (22→4) lost its `ReflectionKind` enum to the
  new `ReflectionValue.h` (0→4), which still has all 14 original
  enumerators, just reformatted onto fewer lines; `Driver/Options.td`
  (16→MISSING) moved wholesale to `Options/Options.td` (0→16, exact count
  match). Two `MISSING` test files (`acle_sve_splice-bfloat.c`,
  `debug-info-ivars-indirect.m`) are confirmed absent from the upstream tag
  too (the former merged into `acle_sve_splice.c`, which grew 66→73; the
  latter a plain upstream removal) — not local drops.
- One finding is real and unaddressed: `clang/lib/AST/ItaniumMangle.cpp`
  (51→4 matches) and `clang/lib/AST/MicrosoftMangle.cpp` (21→4) both lost
  their entire reflection-mangling support in the M2 wholesale-upstream
  resolution and it was never restored. Confirmed by diffing against
  `6dd950bcd4ac`: both lost a `mangleReflection(const APValue &)` method
  (switching on every `ReflectionKind` to mangle a reflected type/object/
  value/decl/template/namespace/entity-proxy/parameter/base-specifier/
  data-member-spec/annotation), `mangleType(const ReflectionSpliceType *)`,
  and (Itanium only) `NestedNameSpecifier::Splice`/`SpliceWithTemplate`
  handling in both the nested-name-specifier mangler and the general
  expression mangler (`CXXReflectExpr`, `CXXMetafunctionExpr`,
  `CXXSpliceExprClass`, `CXXDependentMemberSpliceExprClass`). What remains
  in both files is only the leftover `case APValue::Reflection:` arms,
  which now hit `llvm_unreachable` instead of calling the removed
  `mangleReflection`. This is a silent-miscompile risk, not a build error:
  neither switch is over an enum without a default/unreachable arm, so nothing
  currently fails to compile — but any template instantiated with a
  reflection-valued non-type template argument will hit the
  `llvm_unreachable` at mangle time. Not yet restored; carrying forward as
  the next concrete Milestone 4 restoration target after the current build's
  error surface is triaged, since Sema/AST must be stable before mangling
  can be meaningfully tested.
- Build is still in progress (LLVM support/analysis libraries, not yet at
  `clang/lib`); next action is to let it reach `clang/lib/AST`/`clang/lib/Sema`
  and triage whatever errors land there, batched by API per the established
  pattern, before returning to the two mangler files.
- The build reached `clang/lib` and surfaced two real failures, both fixed:
  - `clang/lib/AST/TextNodeDumper.cpp`: a duplicate `case APValue::Reflection:`
    arm (one stray copy reading "Reflection <opaque>" alongside the fork's
    original "Reflection <todo>"; removed the stray one).
    `dumpNestedNameSpecifier` still took `const NestedNameSpecifier *` and
    switched on the pre-LLVM-22 `Identifier`/`Namespace`/`NamespaceAlias`/
    `TypeSpec`/`Global`/`Super` enumerators with a manual `getPrefix()`
    recursion — the header (`TextNodeDumper.h:214`) already declared the
    by-value `NestedNameSpecifier NNS` signature, so this was a plain
    unported holdout. Rewrote to match the by-value `Kind::{Namespace,Type,
    Global,MicrosoftSuper,Splice,SpliceWithTemplate}` switch already
    established as this sync's precedent in `NestedNameSpecifier.cpp`,
    taking upstream's own reconciled `dumpNestedNameSpecifier` (confirmed via
    `git show <upstream-tag>:clang/lib/AST/TextNodeDumper.cpp`) as the base
    and adding back the two local `Splice`/`SpliceWithTemplate` arms.
    `VisitUsingType` called the removed `UsingType::getFoundDecl()`/
    `typeMatchesDecl()`; replaced with upstream's reconciled body (keyword +
    `dumpNestedNameSpecifier(qualifier)` + `dumpDeclRef` + `dumpType`, no
    local reflection-specific content to preserve there). Three
    `D->getQualifier()->print(...)` call sites (`VisitUsingDecl`,
    `VisitUnresolvedUsingTypenameDecl`, `VisitUnresolvedUsingValueDecl`)
    needed `->` → `.` for the same by-value migration. Also added the
    unrelated-to-reflection `RootSignatureVersion::V1_2` switch arm upstream
    added after this file's `HLSLRootSignatureUtils.h`-era baseline, to reach
    a fully warning-clean compile of the file.
  - `clang/lib/Sema/SemaTypeTraits.cpp` referenced `UTT_IsConstevalOnly` in
    two `switch` arms (both already correct and complete: one calls
    `RequireCompleteType`, the other calls the already-present
    `Type::isConstevalOnly()`/`RecordDecl::isConstevalOnly()`), but the
    identifier itself was undeclared. Root cause: the fork's
    `TokenKinds.def` had `TYPE_TRAIT_1(__is_consteval_only, IsConstevalOnly,
    KEYCXX)` (confirmed via `git show 6dd950bcd4ac:...TokenKinds.def`), and
    the M2 wholesale-upstream resolution of this file (in the "48 files"
    category) dropped it with no upstream equivalent to fall back on, unlike
    most of that category's drops. Restored the single macro line in its
    original alphabetical slot (between `__is_aggregate` and `__is_base_of`).
    This is a real, standalone data point for the "M2 wholesale resolution
    silently dropped fork-only content" failure mode this tracker has now
    recorded repeatedly — worth another full-tree sweep of `.def`/`.td`
    files specifically (as opposed to `.cpp`/`.h`) before Milestone 4 closes,
    since this one was invisible to the reflection-content grep sweep (the
    string "consteval" doesn't match `splice|reflect|metafunction`).
  - Two files, one real bug batch each; no other errors. Rebuilt clean:
    `ninja -C build-nyx clangAST clangSema` now reaches
    `[1655/1655] Linking CXX static library lib/libclangSema.a` with **zero
    errors**, only the pre-existing, already-tracked `EnumeratorSpec`
    `-Wswitch` warning (3 instantiation sites, all downstream of the same
    known-incomplete metafunction-kind switch in `SemaReflect.cpp`/
    `TreeTransform.h`). This is the first successful `clangAST`+`clangSema`
    build since the LLVM 22 merge began.
- Next action: build the full `clang` driver/frontend binary (needs
  CodeGen/Serialization/Frontend, which are Milestone 5 territory per the
  `ExprConstantMeta.cpp`/`ExprConstant.cpp` gap already on record) far enough
  to run `clang/test/Reflection/`; triage whatever new errors land there
  before deciding whether Milestone 4's gate can close on `clangAST`/
  `clangSema` alone or needs the mangler restoration
  (`ItaniumMangle.cpp`/`MicrosoftMangle.cpp`, still outstanding, see above)
  first.
- Built the full `clang` binary target. First pass surfaced 5 errors, all in
  `clang/lib/Parse/` (clangParse depends on nothing clangAST/clangSema's
  build already exercised), all fixed, commit `5f79df509148`:
  - `ParseDeclCXX.cpp`: the splice-as-namespace-alias path
    (`ActOnNamespaceAliasDef`'s 7-arg overload) passed
    `cast<NamedDecl>(DR.get())` but that overload takes `NamespaceBaseDecl
    *ND`. Verified safe: `BuildReflectionSpliceNamespace`
    (`SemaReflect.cpp:1897`) explicitly rejects a `TranslationUnitDecl`
    result before returning, so every successful result is a
    `NamespaceDecl`/`NamespaceAliasDecl`/`DependentNamespaceDecl`, all of
    which derive from `NamespaceBaseDecl`.
  - `ParseExpr.cpp`: the recursive `ParseCastExpression` call after
    splice-scope annotation forwarded the removed `isTypeCast` bool
    instead of the enclosing function's own `CorrectionBehavior`
    (`TypoCorrectionTypeBehavior`) parameter — a plain unrenamed
    identifier, not an API redesign.
  - `ParseReflect.cpp`: `CXXScopeSpec::getScopeRep()` returns
    `NestedNameSpecifier` by value now (`DeclSpec.h:98`); switched
    `->getKind() == NestedNameSpecifier::Global` to `.getKind() ==
    NestedNameSpecifier::Kind::Global`, matching every other by-value
    `NestedNameSpecifier` callsite already reconciled this sync.
  - `ParseStmt.cpp`: the expansion-statement `for` parse called the
    2-argument `ParseForStatement(TrailingElseLoc, PrecedingLabel)` with
    only 1 argument, unlike the ordinary `case tok::kw_for:` path three
    lines below it in the same switch, which already passes both.
  - **Systemic finding, high severity**: while diffing `ParseReflect.cpp`
    it turned out the file was entirely untracked by git (`git status`
    showed `??`, not `M`) — a prior session had recreated its content on
    disk but never `git add`ed it. Broadening the check
    (`git status --untracked-files=all`, filtered to non-build paths)
    found 22 such files total, all real, all load-bearing, all currently
    compiling as dependencies of `clangAST`/`clangSema`/`clangParse`:
    `AST/MetaActions.h`, `AST/Metafunction.h`,
    `Basic/DiagnosticMetafn.h`, `Basic/DiagnosticMetafnKinds.td`,
    `Parse/ParseReflect.cpp`, `Sema/SemaExpand.cpp`, and all 16 files of
    `clang/test/Reflection/`. Root cause: `llvmorg-22.1.8` has no
    corresponding paths, so the M2 merge's upstream-side deletion of them
    was never a real conflict, and some later session's reconciliation
    work silently recreated the content without restoring git tracking.
    This left a nontrivial fraction of this sync's actual work (including
    the entire reflection test suite) one `git clean -fd` or `checkout .`
    away from silent, total loss. Restored to tracking in commit
    `d2b4eef00cb7` (3671 lines, 22 files).
  - Ran the same check tree-wide and deliberately rather than by accident:
    `git ls-tree -r --name-only 6dd950bcd4ac -- clang/ llvm/` vs. `HEAD`
    via `comm -23`, cross-referenced against `git ls-tree` of the upstream
    tag to separate real drops from legitimate upstream renames/deletions
    (of which there are ~1493, essentially all unrelated target/test
    churn from a full LLVM version bump — spot-checked a random sample of
    15, all generic `CodeGen`/`MC`/debug-info tests with no reflection
    relevance). This method is strictly better than the reflection-content
    keyword sweep used earlier in Milestone 4: it caught
    `clang/test/SemaCXX/cxx2c-expansion-stmts.cpp`, which the keyword
    sweep's methodology could never have found (it was fully deleted from
    disk, not merely content-reduced at a surviving path) and which name
    hindsight makes obvious but the earlier sweep's "same path, content
    diff" design structurally could not see. It's `SemaExpand.cpp`'s test
    coverage, lost independently of the implementation file itself being
    restored earlier. Restored verbatim from the fork tip in commit
    `c6d163805cfb`. **Carry forward**: rerun this exact `comm`-based sweep
    (not the keyword-based one) at the start of future sessions touching
    this tree — it is the correct general tool, the keyword sweep is not.
  - Also confirms and supersedes the `__is_consteval_only` /
    `TokenKinds.def` finding from earlier this session: that was this same
    failure mode (M2 wholesale-upstream resolution silently dropping
    fork-only content with no upstream fallback) hitting a `.def` macro
    line instead of a whole file, which is why the reflection-content
    grep sweep missed it (the string "consteval" doesn't match
    `splice|reflect|metafunction`) but the `comm`-based path sweep
    structurally cannot miss it (`TokenKinds.def` survives at the same
    path in both trees, so the `comm` sweep doesn't catch *this specific*
    case either — a same-path, single-line drop needs the content-count
    method; only the whole-file-deletion case is what `comm` uniquely
    catches). The two methods are complementary, not redundant: `comm` for
    deleted paths, content-diff for shrunk-but-surviving paths. Neither
    alone is sufficient.
  - Note on attribution: `ParseReflect.cpp`'s one-line fix landed in commit
    `d2b4eef00cb7` (the tracking-restoration commit) rather than
    `5f79df509148` (the other three Parse-file fixes), because it was
    untracked at the time the restoration commit was staged — the fixed
    content and the tracking restoration are inseparable in that diff.
    `5f79df509148`'s message describes it as a fourth fix; the commit
    itself only shows three files changed. Not worth rewriting history
    for; noted here so a future `git log`/bisect isn't confused by the
    mismatch.
  - Note on prior-session hunks swept into these commits: `git add
    <file>` stages a file's *entire* working-tree diff, not just the lines
    touched this session. `SemaTypeTraits.cpp` (commit `970d57fb6caa`)
    carried 6 lines of already-correct, already-tested prior-session
    `UTT_IsConstevalOnly` handling that this session didn't write, only
    unblocked (by restoring the enum value it depended on). Commit
    `5f79df509148`'s three Parse files carried substantially more:
    `ParseDeclCXX.cpp` (205 lines), `ParseExpr.cpp` (126 lines),
    `ParseStmt.cpp` (74 lines) of pre-existing uncommitted M4
    reconciliation work from earlier sessions, on top of this session's
    single-line fix in each. This is expected and correct given the tree
    is intentionally kept dirty across sessions rather than committed
    file-by-file (see `HANDOFF_2026-08-26.md`), but it means these two
    commits' diffs are not fully described by their commit messages, which
    only narrate the specific bugs this session found and fixed.
  - Note on `clangd` diagnostics: throughout this session, the editor's
    live diagnostics for every file touched (`TextNodeDumper.cpp`,
    `ParseReflect.cpp`, `ParseExpr.cpp`, `ParseStmt.cpp`,
    `ParseDeclCXX.cpp`) reported large numbers of `no_member`/
    `unknown_typename`/`undeclared_var_use` errors — for symbols
    (`APValue::Reflection`, `NestedNameSpecifier::Kind::Splice`,
    `tok::l_splice`, `ParseSpliceSpecifier`, `ExpansionStmtDecl`, etc.)
    that demonstrably exist and compile correctly under direct
    `clang++ -fsyntax-only` invocation and under the real `ninja` build.
    `clangd`'s index/compile-database for this tree is stale or
    misconfigured and its diagnostics were wrong in every single instance
    checked this session, with no exceptions. Do not spend time
    investigating or trusting them; always verify against direct
    compilation or the real build instead.
- Rebuilt after the Parse fixes: `clangAST`, `clangSema`, and `clangParse`
  all now compile with zero errors. The full `clang` binary build reaches
  `[2779/2793]` before stopping on exactly 4 remaining errors, all in
  `clang/lib/Serialization/`: `ASTReader.cpp` (`CXXBaseSpecifier`
  constructor mismatch), `ASTReaderStmt.cpp`
  (`DeclRefExprBitfields::IsImmediateEscalating` missing),
  `AttrPCHRead.inc`/`AttrPCHWrite.inc` (generated `readCXX26AnnotationAttr`/
  `AddCXX26AnnotationAttr` missing). These are the leading edge of the
  already-recorded Milestone 5 bundle, not four isolated fixes: the sweep
  earlier this session already found `ASTReader.cpp` (11→0),
  `ASTReaderStmt.cpp` (16→0), `ASTWriter.cpp` (14→0), `ASTWriterStmt.cpp`
  (11→0), `AbstractBasicReader.h`/`AbstractBasicWriter.h` (3→0, 2→0),
  `PropertiesBase.td` (59→0), `ASTBitCodes.h` (5→0), `TypeBitCodes.def`
  (1→0) all zeroed out by the same M2 wholesale-upstream resolution: the
  reader/writer property (de)serialization plumbing these four errors sit
  on top of is entirely absent, not partially broken. Per this tracker's
  explicit scope boundary ("Do not begin unrelated compiler implementation
  work until this epic completes" plus Milestone 5's own separate scope
  statement), this session stops here rather than starting the M5 bundle.
- **Milestone 4 status**: `clangAST`/`clangSema`/`clangParse` are code-complete
  and build with zero errors (only the pre-existing, already-tracked
  `EnumeratorSpec` `-Wswitch` warning remains). The milestone stays `[~]`,
  not `[x]`: its gate requires focused Clang reflection tests to pass, and
  those need a working `clang` binary, which needs Milestone 5. Checked
  whether the still-outstanding `ItaniumMangle.cpp`/`MicrosoftMangle.cpp`
  reflection-mangling gap (recorded earlier this session) actually blocks
  that gate: `clang/include/clang/Frontend/FrontendOptions.h:454` defaults
  `ProgramAction` to `ParseSyntaxOnly`, and all 16 `clang/test/Reflection/`
  RUN lines invoke bare `%clang_cc1 %s <flags>` with no `-emit-llvm`/`-S`/
  other action flag — so none of them trigger CodeGen, hence none trigger
  real Itanium/MS mangling of a reflection-valued template instantiation.
  The mangler gap is confirmed **not** a Milestone 4 blocker; it remains a
  correctness carry-forward for whenever Milestone 5/8 exercises CodeGen.
- Next action: implement the Milestone 5 bundle (constant evaluation,
  serialization, `ExprConstantMeta.cpp`/`ExprConstant.cpp`,
  `ASTReader*.cpp`/`ASTWriter*.cpp`, `AbstractBasicReader/Writer.h`,
  `PropertiesBase.td`, `ASTBitCodes.h`) as its own scoped effort, per the
  systemic-finding evidence above — not as a side effect of chasing the 4
  visible `clang` build errors. Once a working `clang` binary exists, run
  `clang/test/Reflection/` (now that its 16 test files are git-tracked
  again) to get Milestone 4's actual gate result before marking it `[x]`.
  Before trusting any future session's "applied"/"restored" narrative in
  this file, rerun both sweeps from this session (the `comm`-based deleted-
  path sweep and the content-count shrunk-path sweep) rather than the
  session log alone — this failure mode has now recurred at every session
  boundary checked so far.

### 2026-08-27 — Milestone 5 started: serialization vocabulary, NNS splice plumbing, EvaluationMode restoration

- Reran both carry-forward sweeps, scoped to `clang/lib/Serialization`,
  `clang/include/clang/Serialization`, `clang/include/clang/AST`, and
  `clang/lib/AST`, with the content-count sweep's keyword pattern widened to
  include `consteval`/`expansionstmt` per the earlier session's own finding
  that the 3-term pattern missed same-path single-line drops. `comm` found
  exactly one deleted path in this scope, `clang/lib/AST/ExprConstantMeta.cpp`
  (confirmed absent from both the upstream tag and the current worktree, a
  genuine fork-only file with no upstream counterpart to merge against).
  Content-count confirmed the already-catalogued Serialization bundle
  (`ASTReader.cpp`, `ASTReaderStmt.cpp`, `ASTWriter.cpp`, `ASTWriterStmt.cpp`,
  `ASTCommon.cpp`, `ASTReaderDecl.cpp`, `PropertiesBase.td`, `ASTBitCodes.h`,
  `TypeBitCodes.def`) fully zeroed (same as upstream); also found
  `AbstractBasicReader.h`/`AbstractBasicWriter.h` live at
  `clang/include/clang/AST/`, not `clang/include/clang/Serialization/` as an
  initial guess assumed — corrected before merging.
- 3-way merged (base `b1774222c761a7912cdbe0d0004ca12dae95f721`, local
  `6dd950bcd4ac`, upstream = current on-disk worktree content) and applied:
  `PropertiesBase.td` (0 conflicts, 177-line pure local addition),
  `AbstractBasicReader.h`/`AbstractBasicWriter.h`/`TypeBitCodes.def` (1
  conflict each), `ASTBitCodes.h`/`ASTCommon.cpp`/`ASTReaderDecl.cpp`/
  `ASTWriterDecl.cpp` (0 conflicts), `ASTReader.cpp`/`ASTWriter.cpp` (1
  conflict each), `ASTReaderStmt.cpp`/`ASTWriterStmt.cpp` (0 conflicts, the
  full reflection-expression and expansion-statement (de)serialization
  visitors). `TypeBitCodes.def`'s conflict was two independent additions on
  the same free bit-code slot (upstream's `PredefinedSugar`/
  `SubstBuiltinTemplatePack`, local's `ReflectionSplice`) — kept both,
  renumbered `ReflectionSplice` to the next free slot (63).
  `ASTBitCodes.h` gained `PREDEF_TYPE_META_INFO_ID = 75`, which overflowed
  `NUM_PREDEF_TYPE_IDS` (514, exactly at the old ceiling); bumped to 515.
- **Process note, corrects the recipe documented earlier in this file**:
  `git merge-file`'s real argument order is `<file1> <orig-file> <file2>`,
  not `<merge-base> <local> <upstream>` as written above — the common
  ancestor must be the *middle* argument. Using it as the first argument
  (matching the literal order this file previously documented) silently
  produces a no-op merge; verified empirically on `PropertiesBase.td`, where
  it emitted upstream's content completely unchanged, discarding all 177
  local-only lines. Also found `<()` process substitution unreliable for
  `git merge-file` in this sandboxed shell (silently truncated output even
  with correct argument order); switched to real temp files for every merge
  this session. The prior sessions' own file batches are not suspected,
  since this session's re-check of a sample (`DeclCXX.h`, `ASTImporter.h`)
  shows their content already landed correctly — only the written recipe
  was wrong, not necessarily every application of it. Corrected the recipe
  in this file's Milestone-4 log section rather than leaving it to mislead
  a future session.
- The NNS-splice conflicts in `AbstractBasicReader.h`/`Writer.h` and
  `ASTReader.cpp`/`ASTWriter.cpp` all showed the same shape: local's
  pre-merge placeholder (`NestedNameSpecifier::Splice` unscoped enumerator,
  a `readBool()`-encoded "has template keyword" flag) against upstream's
  already-real `Kind::Splice`/`Kind::SpliceWithTemplate` design (confirmed
  live in `NestedNameSpecifierBase.h`, landed by an earlier M4 session's
  "reconcile LLVM 22 splice scope types" commits, not by this one). The
  with-template-or-not distinction is already carried by which of the two
  Kind enumerators was serialized, making the placeholder's explicit bool
  field redundant; resolved every site to construct via
  `NestedNameSpecifier(SpliceSpecifier*, bool WithTemplate = false)` /
  `.getAsSplice()`, matching the by-value API precedent already established
  throughout M4. `ASTWriter.cpp`'s conflict also carried a dead
  `case NestedNameSpecifier::Super:` arm (the old unscoped name for what
  `MicrosoftSuper` already handled above it) — dropped as a duplicate, not
  new content.
- `ExprConstant.cpp` 3-way merged with 8 real conflicts (not the whole
  4254-line fork/upstream delta — the file diverged narrowly). First
  merge attempt via `<()` process substitution silently produced a
  1143-line file (should have been ~22000); caught via a line-count sanity
  check before it ever reached the repo, confirmed the real repo file was
  untouched, and regenerated cleanly from temp files. 4 of the 8 conflicts
  were local's stale unscoped `EM_ConstantFold`-style `EvaluationMode`
  enumerators against upstream's already-scoped `EvaluationMode::ConstantFold`
  (the enum moved to the new shared `clang/lib/AST/ByteCode/State.h` —
  `Interp/` was renamed `ByteCode/` upstream — with `CheckingPotentialConstantExpression`/
  `CheckingForUndefinedBehavior`/`checkingPotentialConstantExpression()`/
  `checkingForUndefinedBehavior()` also already living on the `interp::State`
  base class); took upstream's naming throughout. One conflict was local's
  now-dead `IsImmediateEscalating` `EvalInfo` field/constructor logic —
  confirmed unused anywhere in current upstream (grep found zero read sites),
  consistent with the M4 log's independent finding that
  `SemaTemplateInstantiateDecl.cpp` already dropped the fork's manual
  immediate-escalating bookkeeping in favor of upstream's
  `EnterExpressionEvaluationContextForFunction`; dropped it, not restored.
  The remaining conflicts were the real reflection-specific piece: local's
  `Kind::PlainlyConstantEvaluated`-driven `AllowInjection` gate for
  metafunction-driven declaration injection (`std::meta::define_class` and
  friends) needs an `EvaluationMode` value upstream never added (upstream's
  own `ConstantExprKind::PlainlyConstantEvaluated`/`ContainingDecl`
  parameter already exist in `Expr.h`, but the `.cpp` body was a stub —
  `(void)ContainingDecl;`, always `EvaluationMode::ConstantExpression` —
  confirming upstream's own P2564 injected-declarations feature is
  incomplete in this exact tag, not something this merge broke). Added
  `ConstantExpressionPlainlyConstantEvaluated` to `State.h`'s
  `EvaluationMode` enum as a local-only extension (verified its only two
  other consumers, `ExprConstant.cpp` and `ByteCode/Interp.h`, don't
  exhaustively switch over it elsewhere), restored the
  `EvaluateAsConstantExpr` Kind-to-mode selection logic, and added the new
  enumerator to all 5 switch sites that group it (always alongside
  `ConstantExpression`/`ConstantExpressionUnevaluated`, per every one of the
  5 sites in the fork's own source — confirmed this grouping rather than
  assumed it).
- Filled two generic serialization primitives the fork's `PropertiesBase.td`
  additions need but upstream's `DataStreamBasicReader`/`Writer` don't
  provide (`def Char : CountPropertyType<"char">;` needs `readChar`/
  `writeChar`, the same shape as the already-upstream `readBool`/
  `writeBool`): added both to `ASTRecordReader.h`/`ASTRecordWriter.h` from
  the fork tip verbatim.
- Restored 3 declaration/wrapper sites the clean-merged `.cpp` bodies were
  already calling but had no header declaration for (their `.cpp`
  implementations were already present from the merges above — this was a
  declaration-only gap, not a missing-implementation one):
  `ASTRecordReader::readSpliceSpecifierRef()`/`readCXX26AnnotationAttr()`/
  `getMetafunctionCb()` in `ASTRecordReader.h`;
  `ASTRecordWriter::AddSpliceSpecifier()`/`writeSpliceSpecifierRef()`/
  `AddCXX26AnnotationAttr()` in `ASTRecordWriter.h`.
- `ASTImporter.cpp` and `ASTStructuralEquivalence.cpp` were already mostly
  reconciled by an earlier session (unclear which — not attributed in this
  file); `ASTImporter.cpp`'s remaining delta against fork tip is
  overwhelmingly upstream's own independent Concepts/Requires-expression
  importer support the fork predates, not a reflection gap. Found and fixed
  the two real reflection gaps by direct comparison against fork tip rather
  than the diff-size sweep (which was too noisy here to trust blindly):
  `ASTStructuralEquivalence.cpp` was missing the
  `NestedNameSpecifier::Kind::Splice`/`SpliceWithTemplate` case in its NNS
  equivalence switch (added, delegating to the already-present
  `SpliceSpecifier*` overload). `ASTImporter.cpp` was missing the same case
  in both its plain-`NestedNameSpecifier` and `NestedNameSpecifierLoc`
  import functions; added both, faithfully preserving a latent gap already
  present in the fork's own pre-merge code rather than inventing a fix: the
  plain-`NestedNameSpecifier` importer's `Splice`/`SpliceWithTemplate` cases
  are `llvm_unreachable("unimplemented")` in the fork tip too (never
  implemented pre-merge), yet the `NestedNameSpecifierLoc` importer's loop
  calls into that same unimplemented path via `importInto(Spec, ...)` before
  reaching its own (real) Splice-handling case — so a Splice-kind NNS
  crossing the `ASTImporter` (module cross-TU import) is a real,
  pre-existing, not-this-session's-doing gap. Documented here rather than
  silently fixed under build pressure, since resolving it is a design task
  (extending `Import(NestedNameSpecifier)` to actually import the
  `SpliceSpecifier` payload), not a mechanical reconciliation.
- Not yet built to a link-clean `clang` binary. A background
  `ninja -C build-nyx clang` was in flight validating this session's changes
  when this entry was written; `clang/lib/AST/ExprConstantMeta.cpp` (fully
  absent, ~7409 lines in the fork, no upstream counterpart) is the next and
  largest remaining piece — restore verbatim, `git add` immediately, wire
  into `clang/lib/AST/CMakeLists.txt`, then manually port with the direct
  `-fsyntax-only -ferror-limit=0` compile loop the same way `SemaReflect.cpp`
  was ported in Milestone 4, since it will very likely define the
  metafunction table `Sema::getMetafunctionCb` (already restored, declared
  `clang/include/clang/Sema/Sema.h:15892`) reads from — its absence should
  surface as undefined-symbol errors at the final `clang` link, not as a
  `clangAST`/`clangSerialization` compile failure, since static archives
  don't resolve symbols at archive-build time.

### 2026-08-27 — Milestone 5: first linking `clang`, three crashes root-caused, reflection suite triage

Continuing directly from the entry above in the same day's session (user
instruction: "it's only logical to begin M5 before M4, so go on"). Restored
and ported `ExprConstantMeta.cpp` to zero compile errors, then re-derived
`Expr.cpp`/`TextNodeDumper.cpp` via fresh 3-way merges after discovering an
unattributed prior session had silently lost real upstream content in
those files (`CallExpr::getCalleeAllocSizeAttr()`,
`ArraySectionExpr::getElementType()`,
`CompoundLiteralExpr::getOrCreateStaticValue()`, and others) — caught via
diff against a clean re-merge, not by inspection. Also restored
`Lexer::validateIdentifier` and both manglers' `ReflectionSpliceType`
overloads (`ItaniumMangle.cpp`, `MicrosoftMangle.cpp`); the mangler gap
turned out to be a hard link-time requirement (referenced unconditionally
from generated type-switch dispatchers), not merely CodeGen-only
correctness debt as Milestone 4 had assumed. This produced the first
successful `clang` binary link since the LLVM 22 merge began.

First `clang/test/Reflection/` run: 9/16, later 7/16 across iterations, with
one hard crash. Asked the user whether to keep debugging now or hand off;
answered "keep debugging now." Root-caused and fixed three real bugs via
`gdb -batch` backtraces and hypothesis testing:

1. **Diagnostic ID/table desync** (bad_alloc crash in
   `EscapeStringForDiagnostic`): `AllDiagnosticKinds.inc` was missing
   `#include "clang/Basic/DiagnosticMetafnKinds.inc"` between the CrossTU
   and Sema includes, silently misaligning every Sema diagnostic ID against
   its lookup table. Fixed by adding the include in the position matching
   `DiagnosticIDs.h`'s enum ordering. 7/16 → 10/16.
2. **`RecursiveASTVisitor.h` missing Splice NNS traversal** (segfault in
   `TraverseUsingType`/`MarkDeclarationsReferencedInType`): both
   `TraverseNestedNameSpecifier` and `TraverseNestedNameSpecifierLoc`
   switches were missing `Kind::Splice`/`Kind::SpliceWithTemplate` cases,
   so `DynamicRecursiveASTVisitorBase` (new in LLVM 22) silently skipped a
   splice operand reached through a `UsingType`'s qualifier. Fixed by
   delegating both to the existing `TraverseSpliceSpecifier` helper.
   10/16 → 11/16.
3. **`Sema::ActOnCXXReflectExpr` crash on splice-scoped dependent template
   names** (segfault in `Decl::isTemplateParameterPack()` via
   `CollectUnexpandedParameterPacksVisitor::VisitCXXReflectExpr`,
   reproduced by `splice-templates.cpp`'s
   `^^[:R:]::template tmemfn`): for `TNK_Dependent_template_name`, the
   function unconditionally called
   `BuildCXXReflectExpr(OpLoc, TemplateKWLoc, Template.get())`, building a
   reflection of an incomplete/dependent `TemplateName` that later crashed.
   First attempt special-cased *all* `TNK_Dependent_template_name` results
   to route through `BuildDependentDeclRefExpr` instead — this stopped the
   crash but silently changed behavior for the *non-splice* dependent case
   too (plain `T::template Member` through a normal type-dependent scope),
   newly breaking `lift-operator.cpp` (`DepScope<T>`'s
   `T::template NestedTemplateStruct` / `T::template template_var`), which
   had been passing. Consulted `advisor`, which correctly identified the
   two failing tests differ in scope *kind* (splice vs. plain dependent
   type) and suggested gating the new branch on
   `SS.getScopeRep().getKind()` being `Splice`/`SpliceWithTemplate`.
   Applied that narrower condition
   (`clang/lib/Sema/SemaReflect.cpp:1001`, commit `b47063279fee`):
   confirmed by rebuild that `lift-operator.cpp` returns to passing (takes
   the untouched original path) and `splice-templates.cpp` stays
   crash-free. Net: still 11/16, but the crash is eliminated with no
   regression, which is a strictly better position than the pre-fix 11/16
   (one of those was a live crash).

`splice-templates.cpp` still fails on content after the crash fix, and this
is a real, separate, unresolved semantic gap, not a mechanical bug:
reflecting a bare (no explicit template arguments) dependent template-name
through a splice scope (`^^[:R:]::template tmemfn`) only carries its
"this names a template, not a value" intent through
`Sema::ActOnCXXReflectExpr` at the point of first parsing. Once
`TreeTransform` re-instantiates the resulting `DependentScopeDeclRefExpr`
(built via `BuildDependentDeclRefExpr`) with a concrete `R`, the
substituted expression becomes a plain `UnresolvedLookupExpr` naming the
(still-templated) member — indistinguishable, by the time
`BuildCXXReflectExpr(Expr*)` sees it, from `^^someOverloadedFunction`. It
therefore falls into `BuildCXXReflectExpr(UnresolvedLookupExpr*)`, which
uses `auto`-deduction + `ResolveAddressOfOverloadedFunction` to pick a
*function* overload — machinery built for reflecting a call target, not a
template entity — and fails with "cannot take the reflection of an
overload set". A correct fix needs to preserve the "reflect the template
itself" intent through instantiation (e.g. a distinct AST representation,
or having `ActOnCXXReflectExpr`'s template-name path re-fire after
substitution) rather than downgrading to a value expression up front. Not
attempted this session; flagged in `Current Action`.

Ran the full `clang/test/Reflection/` suite and triaged every remaining
failure with direct `-cc1 ... -verify` invocations (not just `llvm-lit`
pass/fail) to get exact diagnostics:

- `splice-exprs.cpp`: **only** the documented Milestone 1 baseline failure
  (line 23, "not derived from") remains — no other regressions in this
  file. (An earlier, uncommitted summary of this session had mentioned
  additional "using-declarator" errors here; they do not reproduce against
  this build and were not investigated further.)
- `splice-expr-errors.cpp`: exactly one missing pair of diagnostics, line
  66, `void fn([:^^int:]);` — a splice-typed function parameter no longer
  triggers the expected parameter-declaration ambiguity errors
  (`variable has incomplete type` / `not usable in a splice expression`);
  it is silently accepted instead. Everything else in the file passes.
  Likely cause: LLVM 22 reworked tentative-parsing disambiguation
  (`Parser::ParseParameterDeclarationClause` /
  `isCXXDeclarationSpecifier`) in a way that no longer flags a splice type
  in this position. Not investigated further.
- `splice-namespaces.cpp` (no `-verify`/`FileCheck` — must compile with
  zero errors to pass): fails with "no member named 'x' in namespace ''"
  for `namespace ReAlias = [:R:]; ... ReAlias::x`. The empty namespace name
  is the signal: `Sema::ActOnNamespaceAliasDef` (or whatever resolves a
  splice-kind `CXXScopeSpec` into a `NamespaceAliasDecl` target) is
  dropping the resolved namespace rather than binding it to the reflected
  one. Not investigated further.
- `reflection-wording-examples.cpp`: still crashes (SIGSEGV, exit -11).
  `gdb`-less backtrace from `llvm-lit -v` shows the fault inside
  `CollectUnexpandedParameterPacksVisitor::TraverseTypeLoc` →
  `TraverseNestedNameSpecifierLoc` (`temp_dep_splice` namespace, line ~164:
  `static_assert([:NS:]::template TCls<1>::v == a::v)`). Reasoned, but did
  not empirically confirm with `git stash`, that this crash is unrelated to
  this session's `SemaReflect.cpp` fix: `TCls<1>` supplies explicit
  template arguments, so the fix's `TemplateKWLoc.isValid() && !TArgs`
  guard should never fire on this code path. Recorded as unverified,
  pre-existing, not yet root-caused.

Result: `clang/test/Reflection/` is 11/16. Did not attempt Milestone 5's
evaluator/module/PCH focused tests this session — recorded as not
attempted, not as passed.

### 2026-08-27 — Milestone 4: three mechanical fixes land, 11/16 → 13/16

Resumed from the "reflection suite triage" entry above. Ran both
carry-forward sweeps first (per that entry's own advice): the `comm`-based
deleted-path sweep found no reflection-relevant real drops in `clang/`/
`llvm/`; a working-tree-vs-fork-tip byte-identity check over all 139
fork-touched Parser/Sema/AST/Basic files found zero still-wholesale copies.
Tree was clean; proceeded directly to the four items.

Fixed three real bugs, all confirmed with direct `-cc1` reproducers before
and after, not just `llvm-lit` pass/fail:

1. **`splice-expr-errors.cpp` line 66** — root cause was a self-inflicted
   merge-resolution error, not an LLVM 22 tentative-parsing redesign as
   earlier session log entries speculated. The 2026-08-26 "systemic
   finding" entry's own `ParseTentative.cpp` conflict note says upstream's
   `Next.isNoneOf(tok::coloncolon, tok::less, tok::colon)` is "a strict
   superset of local's `Tok.is(tok::identifier) && Next.isNot(...) &&
   Next.isNot(tok::less)`... the `Tok.is(identifier)` guard was already
   redundant in this context" — true only because upstream's `case
   tok::identifier:` block never sees a non-identifier `Tok`. Once local's
   `case tok::annot_splice:` shares that same block (added earlier in
   Milestone 4), the guard is exactly what routes a splice-typed token to
   `isCXXDeclarationSpecifier`'s `TryAnnotateTypeOrScopeToken`-then-`TPResult::False`
   path (the fork tip's original behavior) instead of `TryAnnotateName`
   (which asserts `Tok.is(tok::identifier) || Tok.is(tok::annot_cxxscope)`
   and, past the assert in a Release/NDEBUG build, returns `Unresolved` for
   a splice, letting parameter-declaration ambiguity resolution silently
   accept the malformed splice-typed parameter instead of diagnosing it).
   Restored `Tok.is(tok::identifier) &&` in front of the `Next.isNoneOf(...)`
   condition (`clang/lib/Parse/ParseTentative.cpp`); trivially true for the
   `identifier` case, so ordinary (non-reflection) disambiguation is
   provably unaffected. Confirmed via direct `-cc1 -verify` against the
   test file: only the documented baseline failure remains.
2. **`splice-namespaces.cpp`** — root cause is `NestedNameSpecifier::
   getDependence()`'s `Kind::Namespace` case unconditionally returning
   `None`. Correct in vanilla C++ (a namespace-alias's target is always a
   concrete, already-resolved namespace — there is no way to make one
   dependent), but wrong once a splice-based alias's target can itself be
   an unresolved `DependentNamespaceDecl` (transitively, through a chain of
   aliases). This made `Sema::computeDeclContext` treat a still-dependent
   splice-alias scope as already-resolved, eagerly binding member lookups
   (`Alias::x`) against the empty placeholder `DependentNamespaceDecl`/
   `NamespaceDecl` created for the not-yet-substituted splice, instead of
   deferring to instantiation via a `DependentScopeDeclRefExpr`. Explains
   both observed symptoms from the earlier "reflection suite triage" entry
   — the empty-named-namespace error and (isolated separately this
   session, same underlying cause) a same-shape failure where a re-aliased
   splice-alias's target reports a real but still-wrong placeholder name
   (`"no member named 'z' in namespace 'inner'"` for a chain like
   `namespace InnerAlias = [:R:]::inner; namespace ReAliasInner =
   InnerAlias;`). Fix: check whether the stored `NamespaceBaseDecl` is a
   `NamespaceAliasDecl` and, if so, delegate to its already-correct
   `NamespaceAliasDecl::isDependent()` (which already recurses through
   alias chains and already checks both `getQualifier().isDependent()` and
   `isa<DependentNamespaceDecl>` — this method existed and was correct, it
   was simply never consulted from `NestedNameSpecifier::getDependence()`).
   (`clang/lib/AST/NestedNameSpecifier.cpp`.) Confirmed via three direct
   `-cc1` reproducers (single-level splice-alias, re-aliased splice-alias,
   the two-chain `Alias`/`ReAlias`/`InnerAlias`/`ReAliasInner` case) and the
   test file itself, all clean.
3. **Latent `Sema::FindInstantiatedDecl` type confusion** (found while
   investigating item 2, kept as independent hardening even though it
   turned out not to be item 2's root cause) — its "local decl not found in
   `CurrentInstantiationScope`, not a template-param/enum/local-class/
   local-typedef" fallback unconditionally `assert(isa<LabelDecl>(D))`s
   (a no-op in Release) and then `cast<LabelDecl>(SubstDecl(D, ...))`s,
   which is genuine undefined behavior if `D` is actually one of
   reflection's new local-decl kinds. Confirmed this code is byte-identical
   to the pre-merge fork tip (`6dd950bcd4ac`) — i.e. a real, pre-existing
   gap exposed by reflection's dependent-namespace-alias feature, not a
   merge regression, since standard C++ has no way to make a namespace
   alias's target dependent and therefore never needed this path. Added a
   dedicated `isa<NamespaceAliasDecl>(D) || isa<DependentNamespaceDecl>(D)`
   branch before the label-only fallback, mirroring the existing local-
   class/local-enum precedent immediately above it in the same function
   (`clang/lib/Sema/SemaTemplateInstantiateDecl.cpp`).
- **Process note on debugging without a debug build**: this build tree has
  no usable DWARF (`addr2line` on backtrace addresses returns `??`), so
  `gdb -batch` backtraces only ever gave function-level frames, never
  source lines or local-variable values — confirmed by testing
  `print <var>` inside a live breakpoint, which fails with "No symbol table
  is loaded" even mid-session. Register/disassembly inspection at the
  fault instruction (`x/Ni $pc-N`, `info registers`) was still usable to
  distinguish "null pointer" from "garbage pointer" crashes. Where that
  wasn't enough (item 2's actual root cause, and the item-4 crash below),
  a temporary `llvm::errs()` print plus one fast incremental
  `ninja -C build-nyx clang` (single changed `.cpp` + relink, not a full
  rebuild) gave a definitive answer in each case; both prints were removed
  before the final validation build. Do not trust hypotheses built purely
  from reading call chains in this codebase without one of these two
  empirical checks — this session's own first two hypotheses for item 2
  (a `TreeTransform`/`FindInstantiatedDecl` instantiation-scope-lookup
  failure) were internally consistent, plausible, and wrong; the actual bug
  was at parse time, before any instantiation occurred at all.
- **Full suite result after all three fixes and one clean rebuild (no
  debug instrumentation)**: `clang/test/Reflection/` is 13/16 — up from
  11/16, no regressions. Remaining 3: the documented Milestone 1 baseline
  (`splice-exprs.cpp`), and the two `Current Action` items above
  (`splice-templates.cpp`, `reflection-wording-examples.cpp`), both
  investigated this session but **not fixed** — see Current Action for the
  precise, now much-better-localized root cause of the
  `reflection-wording-examples.cpp` crash. Both remaining failures are in
  the same "dependent template name through a splice scope" family but
  have distinct root causes; do not conflate them into one fix.
- Explicitly deferred `reflection-wording-examples.cpp` and
  `splice-templates.cpp` rather than attempting a fix under time pressure:
  both need a real design decision about how a splice-scope-qualified
  dependent template name's AST representation should survive
  `TreeTransform` re-instantiation, which the advisor and this tracker's
  own Decisions section both caution against improvising. Milestone 4
  gate remains `[!]`, not closed.

### 2026-08-28 — Milestone 4 gate: both remaining reflection failures fixed at their root cause

Resumed from the "three mechanical fixes" entry above (user instruction:
"Finish M4 in one go" — batch both fixes, one rebuild, one test pass,
rather than alternating single-fix/rebuild cycles). Root-caused both
failures with `gdb -batch` (breakpoints on mangled symbol names plus raw
register/memory inspection — this build still has no usable DWARF) before
writing any fix, per the "three mechanical fixes" entry's own warning that
plausible-looking hypotheses here have previously been wrong.

**`reflection-wording-examples.cpp` crash — genuine encoding bug, not a
"garbage `NestedNameSpecifier` gets constructed somewhere" symptom as
previously described.** Root cause: `NestedNameSpecifier`'s `StoredKind`
tag occupies **3** low bits at offset 0 (`Type=0, Splice=1,
NamespaceOrSuper=2, SpliceWithTemplate=3, NamespaceWithGlobal=4,
NamespaceWithNamespace=6` — 6 enumerators, needed for the fork's
Splice/SpliceWithTemplate additions), but
`NestedNameSpecifier::NumLowBitsAvailable` (consumed by
`llvm::PointerLikeTypeTraits`) was left at `FlagOffset` (`1`), copied
unchanged from upstream LLVM 22. Diffing against
`git show llvmorg-22.1.8:clang/include/clang/AST/NestedNameSpecifierBase.h`
found *why* upstream's `1` is correct there and wrong here: upstream's
`StoredKind` only has 4 enumerators and is shifted left by `FlagOffset`
(occupying bits **1-2**, leaving bit 0 always free); this fork's
`NestedNameSpecifier(PtrKind)` constructor ORs `PK.SK` in **unshifted**
(occupying bits **0-2**, bit 0 no longer free) to fit the 2 extra
enumerators, but nobody updated `NumLowBitsAvailable` to match. The only
two consumers of that promise
(`grep -rn "PointerIntPair<NestedNameSpecifier"`) are
`QualifiedTemplateName::Qualifier` and
`DependentTemplateStorage::Qualifier` in `TemplateName.h`, both
byte-identical to upstream (confirmed via the same 3-way-merge-base diff
this sync has used throughout) — so the bug is real and entirely local to
`NestedNameSpecifierBase.h`'s tag redesign, not a mis-merge of
`TemplateName.h` itself. `llvm::PointerIntPairInfo::updateInt` XORs the
packed bool unconditionally into bit 0
(`(OrigValue & ~ShiftedIntMask) | Int << IntShift` with `IntShift =
NumLowBitsAvailable - IntBits = 0`), silently reinterpreting the 3-bit tag
whenever the packed bool's value differs from the tag's own bit 0 — e.g.
`StoredKind::NamespaceWithNamespace` (`110`, bit 0 = 0) packed with
`HasTemplateKeyword=true` (bit 0 forced to 1) becomes `111`, an
unassigned/unhandled `StoredKind`. Confirmed empirically, not just by
static analysis: breaking on `ASTContext::getDependentTemplateName` and
dumping `*(long*)$rsi` (the raw pre-packing `DependentTemplateStorage`
bytes) at each call for `template <auto T, auto NS> void fn() {
static_assert([:NS:]::template TCls<1>::v == a::v); }` caught the first
call's `Qualifier` at `0x…09` (tag `1` = `Splice`, valid, pointing at the
real `SpliceSpecifier` for `NS` at `0x…08`) and the second call's at
`0x5900000047` (`& 0x7 == 7`, unassigned tag); the crash itself is
`getAsNamespaceAndPrefix()` dereferencing the resulting misinterpreted
pointer (`r14 = 0x5900000040`, confirmed via `x/20i $pc-24` plus
`info registers` at the fault). **Correction (post-advisor review, same
session): an earlier draft of this entry reverse-derived the second
value as "can only come from an original `NamespaceWithNamespace`
qualifier with the keyword bit forced to 1" — that is wrong; the actual
chain (which the advisor caught by re-checking the numbers) starts on the
*read* side, one step earlier than the write-side formula above.**
`ASTContext::getCanonicalTemplateName`'s `DependentTemplate` case
(`ASTContext.cpp:7287-7296`) calls `DTN->getQualifier()` on the *first*
(valid, `Splice`-tagged, `0x…09`) `DependentTemplateStorage` — but
`getQualifier()` goes through `llvm::PointerIntPairInfo::getPointer()`,
which unconditionally masks off bit 0 on every *read*, not just on
write: `0x…09 & ~1 = 0x…08`, decoding as `StoredKind::Type` (`0`) with
`Ptr = 0x…08` — the real `SpliceSpecifier`'s own address, now
misinterpreted as a `Type*`. `NestedNameSpecifier::getCanonical()`'s
`Kind::Type` case then calls
`getAsType()->getCanonicalTypeInternal().getTypePtr()` on those bytes —
undefined behavior, since they are a `SpliceSpecifier`, not a `Type` —
which is what actually produces the arbitrary `0x5900000046`-shaped
result (whatever `Type`-shaped method landed on at those reinterpreted
field offsets). That becomes `CanonQualifier`; since `Qualifier !=
CanonQualifier`, the code proceeds to
`getDependentTemplateName({CanonQualifier, ..., /*HasTemplateKeyword=*/
true})`, and packing `HasTemplateKeyword=true` onto `CanonQualifier`'s
already-odd-looking low bits via the *write*-side formula above is what
lands exactly on the observed `0x5900000047`. So the write-side
corruption described above is real and still the reason the fix is
correct and necessary, but it is the *second* link in the chain here, not
the first — the first link is this read-side `getPointer()` masking,
which corrupts *any* `Splice`/`SpliceWithTemplate`-tagged qualifier the
moment `getQualifier()` is called on it, independently of what the
packed bool's value is. The fix below (removing the packing entirely)
eliminates both the read-side and write-side corruption in one change,
so no code change follows from this correction — only the recorded
root-cause narrative needed fixing, so a future session does not reason
from the wrong link in the chain. Fix: declared
`NestedNameSpecifier::NumLowBitsAvailable = 0` (`NestedNameSpecifierBase.h`,
with a comment explaining why, since this now permanently diverges from
upstream's value) and stopped packing `HasTemplateKeyword`/hasTemplateKeyword
into the `NestedNameSpecifier` pointer's bits in both `TemplateName.h`
classes — each now stores it as a separate `unsigned ... : 1` bitfield
instead of `llvm::PointerIntPair<NestedNameSpecifier, 1, bool>`; updated
`DependentTemplateStorage`'s constructor in `TemplateName.cpp` to match.
No other type packs extra bits into `NestedNameSpecifier` (confirmed by
grep), so this is a complete, contained fix, not a partial mitigation.

**`splice-templates.cpp` semantic gap — the SemaReflect.cpp special case
from the "reflection suite triage" entry was routing around a *different*,
independently fixable bug, not a real design limitation.** Traced why
`^^[:R:]::template tmemfn` (a bare, non-templated dependent template-name
through a splice scope) needed to preserve its "this is a template, not a
value" identity through `TreeTransform`: `TreeTransform::
TransformCXXReflectExpr`'s `ReflectionKind::Template` case
(`TreeTransform.h:9073-9113`) *already* does this correctly — it
transforms the `NestedNameSpecifierLoc` (which already handles
`Kind::Splice`/`SpliceWithTemplate` via `TransformSpliceSpecifier`,
confirmed by reading `TreeTransform.h:4784-4802`) and rebuilds the
`TemplateName` via `TransformTemplateName`'s `DependentTemplateName`
branch (`TreeTransform.h:4931-4952`), which re-resolves the name against
the now-substituted scope. The 2026-08-27 special case in
`Sema::ActOnCXXReflectExpr` (`SemaReflect.cpp`, around line 1001) bypassed
this correct path for splice-scoped dependent template names specifically
because routing through it crashed — but the crash was in a third,
unrelated place:
`CollectUnexpandedParameterPacksVisitor::VisitCXXReflectExpr`
(`SemaTemplateVariadic.cpp:149-170`)'s `isReflectedTemplate()` branch
called `addUnexpanded(TName.getAsTemplateDecl())` whenever
`TName.containsUnexpandedParameterPack()` is true — but
`TemplateName::getAsTemplateDecl()` unconditionally returns `nullptr` for
a `DependentTemplateName` (confirmed by reading `TemplateName.cpp:199-212`:
its `Storage` union alternative for that kind is never a `Decl*`), and
`addUnexpanded(NamedDecl *ND, ...)` dereferences `ND` immediately
(`dyn_cast<VarDecl>(ND)` / `ND->isTemplateParameterPack()`) — a
null-pointer deref matching the previously-recorded "segfault in
`Decl::isTemplateParameterPack()`" symptom exactly. This same visitor
class already has the *correct* general-purpose logic for this
(`TraverseTemplateName`, `SemaTemplateVariadic.cpp:173-186`): it
null-safely checks `dyn_cast_or_null<TemplateTemplateParmDecl>` for the
`TemplateName::Template` case and, for `DependentTemplateName`/
`QualifiedTemplateName`, delegates to the base
`DynamicRecursiveASTVisitor::TraverseTemplateName`, which recurses into
the qualifier (confirmed against `RecursiveASTVisitor.h:892-903`) and so
would correctly visit a splice operand referencing an actual pack via the
ordinary `VisitDeclRefExpr` path. Fix, two parts: (1)
`SemaTemplateVariadic.cpp`'s `VisitCXXReflectExpr` now calls
`TraverseTemplateName(TName)` instead of the unsafe
`addUnexpanded(TName.getAsTemplateDecl())`. (2) With that crash fixed at
its root, the `SemaReflect.cpp` special case routing splice-scoped
dependent template names through `BuildDependentDeclRefExpr` (a
value/decl-ref shape that loses "this is a template" identity once
substituted, which is what caused the original content failure — the
substituted expression becomes indistinguishable from
`^^someOverloadedFunction` and falls into the function-overload-resolution
path) was removed; splice-scoped dependent template names now take the
exact same `BuildCXXReflectExpr(OpLoc, TemplateKWLoc, Template.get())`
path as every other dependent template name, matching
`TransformCXXReflectExpr`'s existing `ReflectionKind::Template` handling.

Applied both fixes together, then ran exactly one `ninja -C build-nyx
clang` and one full `clang/test/Reflection/` pass (per the user's request
to batch fixes rather than rebuild after each one): **15/16**, up from
13/16 — both `splice-templates.cpp` and `reflection-wording-examples.cpp`
now pass, with zero regressions in the other 14 tests. The only remaining
failure is the documented Milestone 1 baseline (`splice-exprs.cpp` line
23). Also ran `clang/test/Lexer/cxx26-reflection-tokens.cpp` and
`clang/test/SemaCXX/cxx2c-expansion-stmts.cpp` (both pass) as an adjacent
regression check. Files changed:
`clang/include/clang/AST/NestedNameSpecifierBase.h`,
`clang/include/clang/AST/TemplateName.h`, `clang/lib/AST/TemplateName.cpp`,
`clang/lib/Sema/SemaReflect.cpp`, `clang/lib/Sema/SemaTemplateVariadic.cpp`.
Committed as `41fa327e7d63`.

**Post-commit wider regression check (advisor-prompted):** the initial
verification above was reflection-local, but `NumLowBitsAvailable` and
the `QualifiedTemplateName`/`DependentTemplateStorage` layout are the
universal qualified/dependent-template-name representation — every
`T::template foo<...>` in any TU goes through `getQualifier()`/
`hasTemplateKeyword()` on these two classes, so the change's real blast
radius is far wider than the reflection suite. Two follow-up checks:
1. Tree-wide grep (`grep -rn "PointerIntPair<.*NestedNameSpecifier\|
   PointerUnion<.*NestedNameSpecifier" .`, not just `clang/include`+
   `clang/lib`) confirms `QualifiedTemplateName`/`DependentTemplateStorage`
   are still the *only* two consumers of `NumLowBitsAvailable` anywhere in
   the tree; no `static_assert` in some other target (e.g.
   clang-tools-extra) can newly fail from this change.
2. Ran the wider template/name-lookup/serialization corpus:
   `llvm-lit -q clang/test/SemaTemplate/ clang/test/CXX/ clang/test/SemaCXX/
   clang/test/Parser/ clang/test/Modules/ clang/test/PCH/` (4181 tests).
   9 failed (0.22%): `SemaTemplate/instantiate-static-var.cpp`,
   `SemaTemplate/concepts-lambda.cpp`,
   `CXX/temp/temp.constr/temp.constr.constr/non-function-templates.cpp`,
   `CXX/dcl.dcl/dcl.attr/dcl.attr.grammar/p2-1z.cpp`,
   `SemaCXX/builtin-is-within-lifetime.cpp`,
   `SemaCXX/cxx2b-consteval-propagate.cpp`,
   `SemaCXX/cxx2a-constexpr-dynalloc.cpp`,
   `SemaCXX/constant-expression-cxx11.cpp`, `Parser/cxx-casting.cpp`.
   Inspected each with `-v` (the actual mismatched `-verify` diagnostics,
   not just pass/fail): none involve a qualified or dependent template
   name, `NestedNameSpecifier`, or a splice scope at all. They're two
   unrelated pre-existing gap families instead: (a) this fork's C++26
   reflection lexer claiming `[:`/`:]` token sequences that pre-date
   reflection and appear in older-standard-mode tests
   (`dcl.attr.grammar/p2-1z.cpp`, `Parser/cxx-casting.cpp` — both fail with
   "not parsing token '[:'/'​:]'; use '-freflection'" warnings on unrelated
   syntax); (b) general C++20/23 consteval/immediate-function-propagation
   and constant-evaluator diagnostic-completeness gaps
   (`concepts-lambda.cpp`, `cxx2b-consteval-propagate.cpp`,
   `cxx2a-constexpr-dynalloc.cpp`, `builtin-is-within-lifetime.cpp`,
   `constant-expression-cxx11.cpp`, `instantiate-static-var.cpp`,
   `non-function-templates.cpp` — all mismatched-diagnostic failures in
   `-verify` expectations for immediate/consteval/constexpr behavior,
   nothing to do with name qualification). Not rebuilt at `HEAD~1` for a
   literal before/after baseline (time-boxed; the per-failure diagnostic
   inspection was conclusive enough that the discriminator the advisor
   proposed — "does it involve a qualified/dependent template name at
   all" — cleanly resolved every one of the 9 as unrelated). Milestone 4's
   gate is met; marked `[x]`.

Also corrected this same entry's root-cause narrative for the
`reflection-wording-examples.cpp` crash after advisor review caught that
an earlier draft mis-derived the causal chain (see the "Correction
(post-advisor review...)" paragraph inline above) — the fix itself was
unaffected, only the recorded provenance.

### 2026-08-28 — Milestone 5 gate: batched evaluator/module/PCH/reflection run, two real serialization gaps fixed

Resumed Milestone 5 (user instruction: "finish as much of M5 as you can in
one go", batch fixes and defer recompiles rather than alternating
single-fix/rebuild cycles, per the same pattern the M4-closing session used).
The M5 log entries from 2026-08-27 ("serialization vocabulary...", "first
linking clang, three crashes root-caused...") had already restored the
serialization bundle and reached a link-clean `clang`, but the milestone's
own gate (focused evaluator/module/PCH/reflection tests) had never actually
been run, and the M4 corpus that later closed Milestone 4 never covered
`clang/test/AST/ByteCode/` (the evaluator directory, `Interp/` renamed
upstream) at all.

**Batch 1 — confirm the binary matches the tree, then run the untested
suites.** Checked no tracked modified file was newer than
`build-nyx/bin/clang-21`'s mtime (01:20:40, from the M4-closing session's
build) before testing, to avoid a stale-binary false pass. Ran
`llvm-lit -q clang/test/AST/ByteCode/ clang/test/Modules/ clang/test/PCH/
clang/test/Reflection/` in one invocation (1281 tests): only the documented
Milestone 1 baseline failure (`splice-exprs.cpp` line 23). Zero cost, since
the existing binary was already valid for this — confirms the M5
restoration work from 2026-08-27 was already sound for everything these
directories cover.

**Milestone 5's own gate text calls out "the known non-serializable
`CXXMetafunctionExpr` callback limitation" — empirically false, corrected
rather than left standing.** `Sema::getMetafunctionCb` rebuilds the
`ImplFn` callback from just `MetaFnID` (a static-table index) and the
*reading* `Sema` (via `Reader->getSema()->getMetafunctionCb(ID)` in
`ASTRecordReader::getMetafunctionCb`, `ASTRecordReader.h:376`), not by
literally serializing a function pointer — a comment at that call site
already calls it a "CXX26 hack". Rather than trust the milestone text's
characterization, built two direct `-cc1 -emit-pch`/`-include-pch`
reproducers in the scratchpad (not committed — throwaway probes): (1) a
`constexpr` variable whose initializer is a `__metafunction(...)` call,
evaluated at PCH-write time; (2) the same metafunction call inside a
template function, only instantiated (and thus only *re*-evaluated via the
deserialized `CXXMetafunctionExpr`) in the second TU after `-include-pch`.
Both passed cleanly — no crash, correct results. The mechanism is fragile
by design (any future metafunction whose closure captures more than
`this`+`Metafn` would break it silently) but is not, today, a limitation.
Corrected the Milestone 5 line in the Milestones table to say so instead of
repeating the unverified claim.

**Found and fixed a real, verified serialization gap: `ReflectionSpliceType`
PCH deserialization was a bare `llvm_unreachable`.**
`clang/include/clang/AST/TypeProperties.td`'s `ReflectionSpliceType` entry
(added during the 2026-08-26 Type.h/TypeBase.h reconciliation, deliberately
deferred to "the AST-serialization reconciliation milestone" per its own
comment) had a `Creator` that unconditionally called
`llvm_unreachable("ReflectionSpliceType PCH deserialization not yet
implemented")`, with zero declared properties. Confirmed this is live and
reachable, not theoretical: a `-cc1 -emit-pch` on a header containing
`using SplicedInt = [:^^int:];` succeeded (the writer path doesn't
round-trip through the reader), but `-include-pch` on a second TU using
that alias crashed with a plain `SIGSEGV` inside `ASTReader::GetType`
(Release build, so `llvm_unreachable` compiles to `__builtin_unreachable`
and jumps into garbage rather than aborting with a message — no diagnostic
at all, just a raw stack trace landing in
`AbstractTypeReader<ASTRecordReader>::readTypedefType()`). Fixed by
declaring the three real properties the type already exposes
(`getTypenameKWLoc()`, `getSplice()`, `getUnderlyingType()`) and a `Creator`
that calls `ctx.getReflectionSpliceType(typenameKWLoc, splice,
underlyingType)` — the same construction path `Sema::BuildReflectionSpliceType`
already uses for both the dependent case (passing `Context.DependentTy`)
and the non-dependent case. The `splice` property uses the already-existing
`SpliceSpecifierRef` declarative property type (`PropertiesBase.td:144`),
whose `readSpliceSpecifierRef`/`writeSpliceSpecifierRef` primitives on
`ASTRecordReader`/`ASTRecordWriter` were already implemented by the
2026-08-27 serialization-vocabulary session for `NestedNameSpecifier::Splice`
— no new primitive needed, just a missing consumer. `ReflectionSpliceTypeLoc`
needed no changes (`ReflectionSpliceTypeLocInfo` is an empty struct, so its
`TypeLoc` serialization is fully generic once the `Type` itself is
serializable). Verified with direct `-cc1` round trips for both the
non-dependent case (`using SplicedInt = [:^^int:];`, a real value read back
across the PCH boundary) and the dependent case (`template <info I> using
SplicedFromParam = typename [:I:];`, instantiated only in the second TU,
exercising the `getReflectionSpliceType`-with-`DependentTy` construction
path and its later re-resolution) — both pass. Added
`clang/test/PCH/cxx26-reflection-splice-type.cpp` covering both cases as a
permanent regression test (previously zero PCH coverage existed for
reflection at all).

**Found and implemented a second real gap:
`ASTImporter::Import(SpliceSpecifier *)` was also a bare
`llvm_unreachable`, corroborating the 2026-08-27 log's finding.** That
entry had already traced, by direct code reading (not empirical
reproduction), that a `Splice`/`SpliceWithTemplate`-kind `NestedNameSpecifier`
crossing `ASTImporter` (cross-TU import, e.g. CTU analysis or
`clang-import-test`) hits `llvm_unreachable("unimplemented")` in the plain
`Import(NestedNameSpecifier)` overload (`ASTImporter.cpp`, was line 10182)
before ever reaching the `Import(NestedNameSpecifierLoc)` overload's own
already-correct `Splice`/`SpliceWithTemplate` cases (`Builder.MakeSpliceScopeSpecifier`)
— because that loop calls `importInto(Spec, NNS.getNestedNameSpecifier())`
first, which routes through the unreachable plain-NNS importer. Implemented
`ASTNodeImporter::ImportSpliceSpecifier` (imports `LSpliceLoc`, `Operand`
(the reflection expression) via the ordinary generic `Expr` importer,
`RSpliceLoc`, and the optional `ASTTemplateArgumentListInfo` template-args
list via the already-existing `ImportTemplateArgumentListInfo` helper —
mirroring the `import<ConceptReference *>` specialization's pattern at
`ASTImporter.cpp:1043` exactly, since `ConceptReference` has the same
"required locs/operand plus optional template-arg list" shape), wired
`ASTImporter::Import(SpliceSpecifier *)` to call it, and replaced the plain
`Import(NestedNameSpecifier)` overload's `Splice`/`SpliceWithTemplate` case
with a real implementation
(`NestedNameSpecifier(*SSOrErr, Kind == SpliceWithTemplate)`) instead of
`llvm_unreachable`. This makes the already-correct
`Import(NestedNameSpecifierLoc)` case's `Spec.getAsSplice()` call
meaningful for the first time (`Spec` now really is the imported specifier,
not garbage from a call that could never have returned).
**Caveat, stated plainly rather than glossed over: this is
compile-verified only, not runtime-verified.** Attempted a live reproducer
via `clang-import-test -import ... -expression ...` with a header using
`[:^^N:]::y` (a `Splice`-kind qualified name) and `-Xcc -std=c++23 -Xcc
-freflection`; the `[:` was parsed as an ordinary lambda-capture `[`
regardless, meaning `-freflection` never reached the tool's `LangOpts` for
either the imported-from or the expression TU. Ran a control to rule out
"my splice syntax is wrong" instead: passed `-Xcc
-Wfoo-bar-baz-nonexistent` (a guaranteed-invalid flag) and got no
diagnostic at all — confirms `-Xcc` args are not reliably reaching
`CompilerInvocation::CreateFromArgs`'s effect on the parsed TU in this
tool, a pre-existing `clang-import-test` limitation unrelated to this
fork's reflection work, not something to route around under build
pressure. `clang/test/Import/` (1339-test batch below) passing is not
evidence either way, since none of those tests enable `-freflection` so
the new code path never executes in that corpus. Did not attempt a
`clang/unittests/AST/ASTImporterTest.cpp` case either: that fixture's
`getCommandLineArgsForLanguage(TestLanguage)` has no extension point for
extra flags like `-freflection` without editing
`clang/include/clang/Testing/CommandLineArgs.h`, and building the
`ASTTests` unittest binary was not already available — both are reasonably
sized follow-up work, not a batch-fits-now fix. **Net effect of landing
this anyway: replaced a guaranteed Release-mode `SIGSEGV` (verified
first-hand on the sibling `ReflectionSpliceType` bug above — the same
`llvm_unreachable`-compiles-to-jump-into-garbage failure mode) with
untested-but-directly-analogous code that mirrors an already-proven-correct
import pattern in the same file.** Not a safety regression either way, but
runtime correctness of this one path is owed, not proven. Follow-up: wire
`-freflection` into `clang-import-test` or `CommandLineArgs.h`, then add
a splice-scoped-qualified-name cross-TU test.

**Batch 2 — validate both fixes together plus a wider corpus, one
rebuild.** One `ninja -C build-nyx clang` (2794/2794, ~25 min). Re-ran the
original 1281-test batch (still only the M1 baseline failure) plus
`clang/test/Import/` (the `ASTImporter` lit corpus, 58 more tests, all
pass — confirms no regression in ordinary, non-reflection import paths from
the `Import(NestedNameSpecifier)` signature change) for 1339 total. Checked
for a dedicated `clang/test/Serialization/` lit directory per Milestone 5's
own "AST serialization" wording (advisor-prompted, since `TypeProperties.td`
regenerates the core type (de)serialization codegen and the original batch
didn't target it by name) — does not exist in this tree or upstream LLVM
22; AST/type serialization is exercised through `Modules/` and `PCH/`
instead, both already in the batch. `ASTImporter.cpp` alone was also
checked with the project's standard direct `-fsyntax-only` translation-unit
compile (per `HANDOFF_2026-08-26.md`'s documented recipe) before the full
rebuild, to catch a syntax error without waiting ~25 minutes; it was clean.

Files changed: `clang/include/clang/AST/TypeProperties.td`,
`clang/lib/AST/ASTImporter.cpp`,
`clang/test/PCH/cxx26-reflection-splice-type.cpp` (new). Milestone 5's gate
(focused evaluator, module, PCH, and reflection tests pass; any
intentionally deferred limitation documented with a reproducer) is met: all
five directories pass except the Milestone 1 baseline, and the one
incompletely-verified piece (the `ASTImporter` splice-import runtime
behavior) is documented above with its exact blocker and reproducer
attempt. Marked `[x]`. Milestone 6 (libc++/generated C++26 files) is next;
nothing attempted for it yet.

### 2026-08-28 — Milestone 6 gate: libc++ reconciliation, two silently-merge-deleted files, one real M4 gap discovered

Resumed Milestone 6 (user instruction: "finish as much of M6 as you can in
one go", same batch-then-verify discipline as the M4/M5-closing sessions).
First committed 84 files of forgotten-to-stage Milestone 4 work found
uncommitted in the working tree since 2026-08-26 (confirmed build- and
gate-verified via `find clang -newer build-nyx/bin/clang-21` returning
nothing but the already-committed M5 test file) as its own commit
(`a2d5edff3f95`), so history matches the binary that already passed the M4
and M5 gates before starting Milestone 6's own work.

**Enumeration.** `git diff --name-status <merge-base> <pre-merge-cxx26-tip>
-- libcxx/ libcxxabi/ runtimes/` listed 527 paths the fork ever touched.
Classified each by comparing blob hashes across the fork tip, HEAD, and the
upstream `llvmorg-22.1.8` merge parent: 436 already matched the fork
untouched (pure local additions that never conflicted during the original
merge — `<meta>`, the reflection test directory, etc. — already fine, no
action), 51 differed from both fork and upstream (git's automatic
non-conflicting three-way merge had already correctly blended both sides'
independent changes — spot-checked the highest-risk ones, including
`__config`/`string`/`deque`/`list`/`forward_list`, for any dropped
reflection/annotation content via keyword grep before trusting this bucket;
all clean, all false-positive keyword hits from unrelated upstream
`_LIBCPP_CONSTEXPR_SINCE_CXX26`/ASan-annotation naming), 1 was the
already-documented deliberate upstream deletion
(`clang_modules_include.gen.py`), and 39 were genuine conflicts where the
original merge had resolved to pure upstream content, discarding fork's
side entirely — this 39 is the tracker's "35 libc++" conflict count plus a
few more in the generated-feature-test-macro family.

**Generated vs. hand-written split.** Of the 39, 10 are outputs of
`generate_feature_test_macro_components.py` (`libcxx/include/version`,
`libcxx/docs/FeatureTestMacroTable.rst`, and 8
`*.version.compile.pass.cpp` tests) — per the advisor's guidance, these are
never hand-merged; only the generator script itself is reconciled, then
`ninja -C build-libcxx libcxx-generate-files` regenerates the rest. The
remaining 29 (2 hand-maintained paper-status CSVs plus one more
`Cxx2cPapers.csv`, `include/CMakeLists.txt`, 21 headers, 3 tests, and the
generator script itself) needed real reconciliation.

**Conflict resolution.** Ran the same `git merge-file <fork> <base> <head>`
three-way merge used for the M4 clang batch (real temp files, base in the
middle) over all 29: **all 29 conflicted** (unlike the clang M4 batch's
32-clean/21-conflict split) — expected, since this 29-file set was
specifically selected as the paths where the *original* merge itself had a
real content conflict, so an independent re-merge finding conflicts on the
same lines is corroborating, not surprising. Delegated the mechanical
resolution to a forked subagent (same "keep both independent changes,
don't invent content" instruction as the M4 conflict-resolution log
entries); it made good progress but exhausted its session budget mid-task
(`libcxx/include/complex`, the largest at 25 conflict hunks, was left with
all markers still in place; a stray untracked `is_replaceable.h` also
surfaced from its intermediate work). Finished by hand/script:
- `libcxx/utils/generate_feature_test_macro_components.py` (3 conflicts):
  two were the same paper's value written twice with a typo/revision-number
  difference (`__cpp_lib_atomic_ref`'s C++26 value 202603 vs. 202411 with a
  stray space in the comment — took upstream's, the exact-tag-shipped
  value, since this isn't fork-specific conformance work and the fork's
  comment typo suggested it was the less-careful draft; `__cpp_lib_optional`
  P2988R11 vs. R12 comment — took upstream's newer revision number, values
  already matched). The third was purely additive (upstream independently
  added `test_suite_guard`/`libcxx_guard` gating for
  `__cpp_lib_optional_range_support` on `_LIBCPP_HAS_EXPERIMENTAL_OPTIONAL_ITERATOR`)
  — confirmed that guard macro is real and already used by the merged
  `optional` header before keeping it.
- `libcxx/include/complex` (25 conflicts, all one mechanical pattern):
  fork added `_LIBCPP_CONSTEXPR_SINCE_CXX26` plus constexpr-capable
  `__complex_dispatch_*`/`__constexpr_*` implementations for every
  transcendental function (`abs`, `arg`, `log`, `sqrt`, `sinh`, `asin`,
  etc.); upstream independently added `[[__nodiscard__]]` to the same
  declarations with the old runtime-only bodies. Wrote a small script
  (not committed) that verified this pattern held for all 25 hunks, then
  mechanically prepended `[[__nodiscard__]]` to fork's constexpr-capable
  declaration and dropped upstream's runtime-only duplicate — combining
  both independent changes exactly as the M4 precedent does by hand.

**Verification, not just trust.** Rather than accept the fork subagent's
own (truncated, budget-exhausted) self-report, independently re-verified
all 29 files: confirmed zero conflict markers remain
(`grep -c '^<<<<<<< \|^=======$\|^>>>>>>> '` across the repo), then
directly `-fsyntax-only` compiled every merged header against
`build-nyx/bin/clang++` with real `-isystem` paths (not the background
LSP, which gave false "file not found"/cascading parse-error diagnostics
for `atomic.h`, `stop_token.h`, etc. purely because it lacks libc++'s own
include path — confirmed false by direct compilation, all real headers
exist on disk and compile clean). This caught what the subagent's
self-check missed: `libcxx/include/complex` still had all 25 conflict
markers in place.

**Two files silently deleted by the original merge, invisible to the
527-path enumeration above.** `ninja -C build-libcxx cxx` failed on a
missing `libcxx/include/stdbool.h` (still listed in the reconciled
`include/CMakeLists.txt`, which is fork content). Root cause, generalized:
a file present in the fork tip *and* the merge-base but absent from the
upstream tag is invisible to a `<base>..<fork>` diff (no change recorded
between those two points) yet still gets silently deleted by git's
automatic three-way merge, since only the upstream side touched it (by not
having it) — this is a real gap in the M4/M6 diff-based enumeration
methodology, not just a one-off. Computed the full set with
`comm -23 <(ls-tree fork) <(ls-tree HEAD)` (248 paths), classified by
whether fork had modified them since the merge-base (only
`clang_modules_include.gen.py`, already documented) vs. left them
untouched (247 — mostly genuine upstream renames/consolidations, e.g.
`__type_traits/add_lvalue_reference.h` + `add_rvalue_reference.h` merged
into upstream's new `add_reference.h`, confirmed by checking the upstream
tag's actual tree rather than assuming), then grepped the current tree's
plain (non-`__cxx03`) `CMakeLists.txt`/`module.modulemap.in`/headers for
real references to each candidate, filtering out `__cxx03`'s own
self-contained legacy mirror (which still uses the pre-consolidation
names on both sides, correctly). Only two were still genuinely referenced:
`stdbool.h` (restored from the fork tip) and `is_replaceable.h` (the one
the subagent had already restored but left untracked — `git add`ed it;
confirmed `optional`'s merged `#include <__type_traits/is_replaceable.h>`
and `__is_replaceable_v` usage are real, needed fork content, not
upstream, since `is_replaceable.h`'s addition commit is not an ancestor of
the `llvmorg-22.1.8` tag).

**A stale, untracked build-directory artifact, not a source bug.**
`ninja -C build-libcxx cxx` then succeeded (660/660), but the libc++
reflection lit suite's `google-benchmark` dependency build failed two
different ways in sequence, neither a source defect:
1. `_LIBCPP_STD_VER >= 11` C++11 build hit `use of undeclared identifier
   'is_floating_point_v'` in `__atomic/support/c11.h`'s
   `__cxx_atomic_consteval_maximum_num`/`minimum_num` — a pre-existing fork
   bug (this file is untouched-since-fork, `ALREADY_FORK` in the
   classification above) already found, fixed, and validated on a
   *different* branch/worktree per `HANDOFF_2026-08-26.md`
   (`83232ca0995a0`, "libc++: keep C++26 atomic helpers out of C++11", never
   ported to `integration/llvm-22.1.8`). Ported the identical two-line
   `#if _LIBCPP_STD_VER >= 26` / `#endif` guard here; confirmed zero call
   sites of either function anywhere else in `libcxx/include/`, so gating
   their definition cannot regress anything.
2. `_LIBCPP_ASSERTION_SEMANTIC_DEFAULT is not defined` from
   `build-libcxx/include/c++/v1/__config_site` (the *non*-triple-specific
   copy, dated 2026-08-15 — three weeks before this sync epic started,
   with zero references anywhere in `build-libcxx/build.ninja`, i.e. a
   dead build-directory leftover from before this project's multi-target
   CMake setup, not a real build output). The triple-specific
   `x86_64-unknown-linux-gnu/c++/v1/__config_site` (real ninja output, up
   to date) already had the macro; `google-benchmark`'s C++11 subbuild's
   `-isystem` order just hit the stale generic one first. Overwrote it
   with the current triple-specific content (build-directory hygiene, not
   a source change, nothing to commit).

**A real Milestone 4-scope gap, discovered only because it finally got
exercised.** With both of the above fixed, `google-benchmark` built, but
59 of libc++'s 60 reflection tests still failed with "no member named
'meta' in namespace 'std'" — `<meta>`'s entire body is gated on `#if
__has_feature(reflection)`. Direct test
(`clang++ -std=c++26 -freflection -fsyntax-only` on a `__has_feature`
probe) showed it evaluates false despite `-freflection` being on: the
`FEATURE(reflection, LangOpts.Reflection)` family (`reflection`,
`parameter_reflection`, `attribute_reflection`, `expansion_statements`,
`annotation_attributes`, `entity_proxy_reflection`, `reflection_latest`)
present in the fork's `clang/include/clang/Basic/Features.def` was never
reconciled into the current tree at all — this file is untouched-since-M3
pure upstream content (confirmed via diff against the fork tip: dozens of
real independent upstream additions — sanitizer features, ptrauth, CFI —
with zero trace of the reflection block). This slipped through Milestone
4's own gate because `clang/test/Reflection/` apparently never exercises
`__has_feature(reflection)` directly, only Sema's internal `LangOpts`
checks. Three-way merged (0 conflicts, pure addition), rebuilt
`ninja -C build-nyx clang` (2794/2794), confirmed the probe now reports
true, then re-ran the full Milestone 5 gate corpus (same 1339 tests) to
check for wider regression from a file this central: still only the one
documented Milestone 1 baseline failure. Rebuilt `ninja -C build-libcxx
cxx` against the new `clang` (660/660, unaffected — no libc++ source
depends on `__has_feature(reflection)` for its own build) and re-ran the
reflection lit suite.

**Where that leaves Milestone 7, stated plainly.** `std::meta` now
resolves, but the suite still shows 59/60 failing, with a new failure
class: `std::vector<meta::info>`/`std::__split_buffer<meta::info, ...>`
hitting "call to immediate function ... is not a constant expression" /
"expressions of consteval-only type are only allowed in constant-evaluated
contexts". This is not a regression introduced this session and not a
libc++ source defect to chase under Milestone 6 — since
`__has_feature(reflection)` was false for the entire span of this sync
epic until today, **no libc++ reflection test has actually exercised
`std::meta` since Milestone 1's pre-merge baseline was recorded**; today is
the first real data point. The failure shape (`ConstevalOnly`
type/constant-evaluator interaction) points at Milestone 5's evaluator
domain, not libc++ source. Did not chase it further: Milestone 6's own
gate (library builds clean, generated-file checks clean, local work
represented) is met and doesn't require test-suite green — that is
Milestone 7's explicit job, and it needs its own from-scratch baseline
run, not a continuation of Milestone 1's now-invalid 54/6 split.

Files changed: `clang/include/clang/Basic/Features.def`;
`libcxx/utils/generate_feature_test_macro_components.py`; regenerated
`libcxx/include/version`, `libcxx/docs/FeatureTestMacroTable.rst`, and 9
`*.version.compile.pass.cpp` tests; `libcxx/docs/Status/{Cxx17,Cxx23,Cxx2c}Papers.csv`;
`libcxx/include/CMakeLists.txt`; `libcxx/include/{complex,map,optional,set,
type_traits,unordered_map,unordered_set,module.modulemap.in,version}`;
`libcxx/include/__algorithm/{count,fill_n}.h`;
`libcxx/include/__atomic/{atomic,atomic_ref,support/c11}.h`;
`libcxx/include/__chrono/{leap_second,zoned_time}.h`;
`libcxx/include/__configuration/availability.h`;
`libcxx/include/__format/range_default_formatter.h`;
`libcxx/include/__locale_dir/locale_base_api.h`;
`libcxx/include/__mdspan/mdspan.h`; `libcxx/include/__ranges/iota_view.h`;
`libcxx/include/__stop_token/{stop_token,inplace_stop_callback,inplace_stop_source}.h`;
`libcxx/include/__type_traits/is_within_lifetime.h`;
`libcxx/include/__type_traits/is_replaceable.h` (restored, new);
`libcxx/include/stdbool.h` (restored, new);
`libcxx/test/std/atomics/atomics.ref/address.pass.cpp`;
`libcxx/test/std/utilities/optional/optional.object/{optional.object.ctor/ctor,../optional_requires_destructible_object}.verify.cpp`.
**Caveat on what the 660/660 `cxx` build actually proves (advisor-prompted):**
since `<meta>`'s whole body is `#if __has_feature(reflection)`-gated and
that feature was false for both `cxx` builds this session (the pre-fix one
and, separately, the library itself doesn't reference `__has_feature
(reflection)` at all so the post-fix rebuild wasn't proving anything new
about it either), the 660/660 count says the *non*-reflection 99% of
libc++ compiles, not that the 29 reconciled files interact correctly with
`std::meta`. Ruled out that the Milestone 7 `ConstevalOnly`/
`std::vector<meta::info>` failure is actually undiscovered M6 merge
damage before closing this out: `vector.h`, `__split_buffer`, and
`__utility/exception_guard.h` (the three files the failing stack traces
point into) all show byte-identical content between the merge-base and
the fork tip (`git diff --stat <merge-base> 6dd950bcd4ac -- <each>` is
empty) — the fork never touched any of them, so there is no fork-side
`_LIBCPP_CONSTEXPR_SINCE_CXX26` or similar annotation for a three-way
merge to have dropped. The failure is a real interaction between
`meta::info`'s `ConstevalOnly` AST property and pure-upstream generic
container code exercised by `std::vector<meta::info>` for the first time
today, not lost libc++ content — confirming the Milestone 7 assignment
above rather than reopening Milestone 6.

Milestone 6's gate (libc++ builds and generated-file checks are clean;
local conformance commits remain reachable and represented) is met.
Marked `[x]`. Milestone 7 (focused reflection/libc++ tests) is next, with
its concrete non-baseline starting point recorded above and in Current
Action/Blockers.
