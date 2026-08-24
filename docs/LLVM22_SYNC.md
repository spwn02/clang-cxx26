# LLVM 22 Synchronization Tracker

This document is the single source of truth for Epic A: synchronizing Clang/P2996 with LLVM 22. Read it before `docs/CXX26_GAPS.md` or `P2996.md` while any milestone below is `[~]` or `[!]`.

## Scope and Fixed Points

- Exact upstream target: `llvmorg-22.1.8`
- Development branch: `integration/llvm-22.1.8`
- Baseline commit: `4f1df39cf326d27e56f9e9ccc6a7f2124527749f`
- Baseline annotated tag: `p2996-2026.08.24`
- Baseline branch: `p2996`
- Upstream remote: `upstream` (`https://github.com/llvm/llvm-project.git`, push disabled)
- Historical implementation remote: `bloomberg` (`https://github.com/bloomberg/clang-p2996.git`, push disabled)
- Working fork remote: `origin` (`https://github.com/spwn02/clang-p2996.git`)

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

Milestone 1 is active. On `Continue`:

1. Confirm `p2996` is clean and resolves to the published baseline plus this tracker commit.
2. Capture exact CMake configuration and host/tool versions in the session log.
3. Rebuild `build-nyx`, regenerate libc++ files, and rebuild `build-libcxx` in that order.
4. Run both focused reflection suites and the single known-failure reproduction.
5. Record exact pass/fail counts and any new failures here. If results match expectations, mark Milestone 1 `[x]`, Milestone 2 `[~]`, commit, and push.

## Milestones

- [~] **1. Capture clean baseline builds and expected failures.** Gate: both build trees succeed; focused reflection results and the known failure are recorded precisely.
- [ ] **2. Fetch exact LLVM tag and merge on integration branch.** Fetch `refs/tags/llvmorg-22.1.8`, verify the signed/published tag object as available, create `integration/llvm-22.1.8` from the tracker-bearing `p2996` tip, and merge the exact peeled tag without rewriting history. Gate: merge commit exists; every conflict and resolution category is recorded.
- [ ] **3. Restore base LLVM/Clang build.** Resolve API/build-system drift unrelated to reflection first. Gate: `ninja -C build-nyx clang` succeeds, followed by the full main-tree build required by later milestones.
- [ ] **4. Reconcile reflection Parser, AST, Sema, templates, and flags.** Preserve P2996 syntax, reflection contexts, metafunction evaluation, splice behavior, and all experimental flag plumbing. Gate: relevant unit/build targets and focused Clang reflection tests pass except explicitly retained baseline failures.
- [ ] **5. Reconcile constant evaluation, modules, and AST serialization.** Audit evaluator changes and module/PCH serialization boundaries, including the known non-serializable `CXXMetafunctionExpr` callback limitation. Gate: focused evaluator, module, PCH, and reflection tests pass; any intentionally deferred limitation is documented with a reproducer.
- [ ] **6. Reconcile libc++ and generated C++26 files without losing local conformance work.** Preserve post-upstream C++26 implementations and regenerate module/export artifacts with LLVM 22 tooling. Gate: libc++ builds and generated-file checks are clean; local conformance commits remain reachable and represented.
- [ ] **7. Pass focused reflection/libc++ tests.** Gate: complete Clang reflection directory and libc++ reflection suite pass, allowing only failures explicitly demonstrated in Milestone 1 and still justified here.
- [ ] **8. Pass full `check-clang` and `check-cxx`.** Gate: both full suites pass, allowing only explicitly recorded pre-existing failures with before/after evidence and exact test names.
- [ ] **9. Merge integration branch into `p2996`, push, and release.** Recheck provenance and tracker state, merge without history rewriting, push `p2996`, create the next free annotated `p2996-YYYY.MM.DD[.N]` prerelease tag, push it explicitly, and verify remote resolution. Gate: clean worktree, remote branch/tag verification, and this epic marked complete.

## Blockers

None currently recorded.

When blocked, record the failing command, essential diagnostic, affected milestone, attempted remedies, and exact condition needed to resume. Use `[!]` only for a genuine external or technical impasse, not for ordinary incomplete work.

## Decisions

- Merge the exact release tag `llvmorg-22.1.8`; do not track a moving LLVM branch.
- Preserve history with a merge on `integration/llvm-22.1.8`; do not rebase or squash the historical P2996 implementation.
- Establish and commit baseline evidence before fetching/merging LLVM 22 so regressions remain attributable.
- Separate base LLVM/Clang build repair from reflection reconciliation to keep commits reviewable and failures diagnosable.
- Preserve local libc++ C++26 conformance work even when upstream LLVM 22 contains overlapping implementations; resolve case by case rather than preferring either side wholesale.
- No failure becomes an allowed exception without an exact baseline or independently verified pre-existing reproducer recorded here.

## Conflict Notes

No LLVM 22 merge has been attempted. For Milestone 2 and later, append notes by subsystem and include:

- paths and upstream/local intent;
- chosen resolution and why;
- focused test covering the resolution;
- follow-up debt or known limitation.

Do not paste voluminous conflict listings or build logs into this file; keep durable summaries and exact commands, with temporary logs outside the repository.

## Session Log

### 2026-08-24 — Repository transformation and synchronization setup

- Added canonical root `AGENTS.md`; retained `.claude/CLAUDE.md` as a pointer.
- Published `p2996` through context commit `4f1df39cf326d27e56f9e9ccc6a7f2124527749f`.
- Configured `origin`, push-disabled `upstream`, and push-disabled `bloomberg` remotes.
- Archived six obsolete branch tips under annotated `archive/pre-llvm22/*` tags, verified their peeled remote targets, deleted the branches, and made `p2996` the sole active origin branch and GitHub default.
- Published annotated baseline tag `p2996-2026.08.24`, peeled to `4f1df39cf326d27e56f9e9ccc6a7f2124527749f`.
- Created this tracker. Milestone 1 remains active; no compiler build or test was required for the documentation/metadata transformation.
