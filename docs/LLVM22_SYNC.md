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
2. Reintroduce the reflection expression family as one integrated AST/Sema/
   parser/visitor/profiler/dumper/importer/serialization bundle; do not land
   isolated generated `StmtNode` classes.
3. Build the affected targets and run focused reflection tests after each
   integrated bundle; preserve the known baseline failure only where reproduced.

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
  `git merge-file -p <(git cat-file -p <merge-base>:<file>)
  <(git cat-file -p 6dd950bcd4ac:<file>) <(git show HEAD:<file>)`
  (merge-base `b1774222c761a7912cdbe0d0004ca12dae95f721`; `HEAD:<file>`
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
