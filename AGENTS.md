# Codex Repository Context

## Project Overview

**CXX26 Clang** is an experimental LLVM fork implementing C++26 and earlier
C++ standards, including static reflection (metafunctions, reflection
operators, splice expressions — see `clang/lib/AST/ExprConstantMeta.cpp`,
`clang/include/clang/AST/Metafunction.h`) and contracts (P2900R14). As of
2026-09-30, every C++26 paper/library facility has either landed or has a
GitHub issue tracking its remaining gap; the one open issue is **#116**,
the fork-wide final-review conformance audit that precedes the first
release. See the Trackers section below.

This repository preserves the Apache-2.0 WITH LLVM-exception license, upstream
source headers and Git authorship, and the history of the Bloomberg-originated
reflection implementation. Do not remove or rewrite that provenance, and do
not imply Bloomberg endorses this fork.

## Command Dispatch

- `Continue`: GitHub Issues (`gh issue list --repo spwn02/clang-cxx26 --state open`) is the living tracker — as of 2026-09-30 the only open issue is **#116** (fork-wide final-review conformance audit) plus whatever it has since spawned under its "Final Review Audit" milestone (`gh issue list --repo spwn02/clang-cxx26 --milestone "Final Review Audit"`). Resume that work; do not create a new parallel tracking doc. (All prior epic-specific tracking docs under `docs/` — `LLVM22_SYNC.md`, `CONTRACTS_PORT.md`, `REFLECTION.md`/`REFLECTION_GAPS.md`/`REFLECTION_CLOSEUP.md`, and finally `CXX26_GAPS.md` itself — were deleted at their epics' close-outs once GitHub Issues fully absorbed their content; see `git log -- docs/` for that history if needed.)
- `Begin PXXXX`: search `gh issue list --repo spwn02/clang-cxx26 --state all --search "P<number>"` for the paper; if an issue exists, resume it. If none exists, this is new-paper triage: fetch the adopted wording from wg21.link/eel.is, research its requirements, implement autonomously, run focused tests, and commit — filing a fresh issue only if the work spans more than one session or needs to be handed off. Begin work without introductory narration.

## Build Architecture

This repository uses a **two-build-tree architecture**:

1. **`build-nyx/`**: Main LLVM/Clang build (configured from `llvm/` with projects: `clang;clang-tools-extra`). Generates `clang`, `clang-tools-extra`, and `llvm-*` tools.
2. **`build-libcxx/`**: libc++ build (configured from `runtimes/` with `LLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind`). **Bootstraps from `build-nyx/bin/clang`** — front-end changes require rebuilding both trees in order. **`ninja -C build-libcxx cxx` has no dependency edge on the external `build-nyx/bin/clang` binary and can silently no-op after a fresh clang rebuild**, leaving a stale `libc++.so`/`.a` in place (the `libcxx-lit` wrapper's `cxx-test-depends` rebuild only runs `cmake --install` steps, which doesn't force a relink either). After any `clang/lib/Sema` or `clang/lib/AST` change, before trusting libc++ test results run `ninja -C build-libcxx -t clean cxx && ninja -C build-libcxx -j$(nproc) cxx` explicitly.

Both use Ninja and Release builds. Check actual configurations with:

```bash
grep CMAKE_BUILD_TYPE build-nyx/CMakeCache.txt build-libcxx/CMakeCache.txt
```

**Two configure-time traps:**
- `-DLLVM_INCLUDE_TESTS=ON` alone is not enough to get a `check-clang`
  target — `CLANG_INCLUDE_TESTS` is a separate cached CMake option that
  only defaults from `LLVM_INCLUDE_TESTS` on a *fresh* configure; once
  cached `OFF` it stays `OFF` regardless of `LLVM_INCLUDE_TESTS`. Pass both
  explicitly: `cmake -S llvm -B build-nyx -DLLVM_INCLUDE_TESTS=ON -DCLANG_INCLUDE_TESTS=ON`.
- A bare `-DCMAKE_CXX_FLAGS=...` on an already-configured cache **replaces**
  a toolchain file's `_INIT` flags rather than appending to them (e.g.
  re-injecting `-fcontracts` alone silently drops the toolchain's
  `-std=c++26 -stdlib=libc++ -freflection-latest`) — replicate the full
  flag set explicitly, don't assume append semantics.

## Building

Always compile with all available host CPUs: pass `-j$(nproc)` to every
`ninja` invocation (currently `-j22`). Do not use Ninja's implicit default
parallelism or a fixed lower job count unless the user explicitly requests it.

```bash
# Full rebuild (after clang changes, rebuild both)
ninja -C build-nyx -j$(nproc)
ninja -C build-libcxx -j$(nproc) libcxx-generate-files
ninja -C build-libcxx -j$(nproc) cxx

# Incremental clang-only
ninja -C build-nyx -j$(nproc) clang
```

Generated C++26 module files must be refreshed after upstream changes:

```bash
ninja -C build-libcxx -j$(nproc) libcxx-generate-files
```

## Testing

Reflection tests live in two locations with different lit configurations:

```bash
# libc++ metafunction tests. Use the wrapper: it rebuilds cxx-test-depends
# and avoids silently testing stale staged headers.
libcxx/utils/libcxx-lit build-libcxx -sv libcxx/test/std/experimental/reflection/entity-classification.pass.cpp

# Full libc++ suite
ninja -C build-libcxx check-cxx

# Clang reflection tests
./build-nyx/bin/llvm-lit clang/test/Reflection/ -v

# Single Clang reflection test
./build-nyx/bin/llvm-lit clang/test/Reflection/splice-types.cpp -v

# Full Clang suite
ninja -C build-nyx check-clang
```

### Known pre-existing baseline failures (not regressions — check here before re-investigating)

**This section is stale by construction and due for a fresh pass** —
Phase 1 of issue #116's audit plan (milestone "Final Review Audit")
establishes a current baseline against `HEAD` on a clean Release build;
until that lands, don't trust a frozen list here, re-run the suite and
isolate via `git stash` against an unmodified checkout (see below) before
attributing a failure to your own change.

What's confirmed live as of 2026-09-30, individually tracked (not a
frozen baseline list, but real open issues — check their current state):
- `clang/test/SemaCXX/cxx2b-consteval-propagate.cpp`'s `escalating<int>`
  heap-allocation-diagnostic sub-case — issue #158.
- Two libc++ tests carried as "known/expected" across many sessions with
  no prior root-cause: `numerics/numarray/nodiscard.verify.cpp` and
  `input.output/iostream.format/print.fun/bounded_writes.pass.cpp` — issue
  #159.
- 132 `check-cxx` failures under `benchmarks/`, caused by the vendored
  `libbenchmark.a` going ABI-stale against a freshly rebuilt libc++ (not a
  real regression) — exclude via `--filter-out 'benchmarks/'` for a pure
  correctness gate, matching upstream LLVM's own `check-cxx` scope — issue
  #162 tracks actually fixing the vendored copy.

Isolation method that's still correct and worth keeping: to confirm a
failure predates your change, `git stash` your diff, rebuild the minimum
needed target, and re-run just the failing test against the unmodified
tree before attributing it to your own work.

### Archived test runs (`cxx26/dev/`)

Built for the (now-complete) Contracts epic but generically useful for any
future gate that needs to prove "zero new failures vs. a known-good
baseline" rather than eyeballing a failure list — exactly the kind of gate
issue #116's audit phases need:

```bash
# Run a suite, archive its stamped JSON result under
# ~/.local/share/cxx26-contracts/{results,lit-times}/ (persists across
# git clean -xdf and branch switches):
cxx26/dev/testrun.sh <suite>
# suites: check-clang, check-cxx, contracts, contracts-lib, reflection,
#         reflection-lib, semacxx, serialization, regression-clusters

# Diff two archived results (refuses to compare across mismatched configs
# unless --allow-config-mismatch is passed):
cxx26/dev/testdiff.py <baseline.json> <candidate.json>

# Idempotent build-tree setup (never deletes an existing tree, unlike
# cxx26/toolchain/build-linux-x86_64.sh, which rm -rfs its arguments):
cxx26/dev/configure-build-trees.sh {nyx|libcxx|all}
```

Each archived result is stamped with the git SHA, a `CMakeCache.txt` config
fingerprint, and `clang --version`. Before any full `check-cxx`,
`testrun.sh` automatically clears
`build-libcxx/libcxx/test/extensions/clang/clang_modules_include.gen.py` —
a lit-output directory that has been observed to grow past 19G over
repeated runs.

## Experimental reflection flags

Enable reflection with `-std=c++26 -freflection`. Extended features require additional flags:

- `-freflection-latest`: all experimental features (recommended for testing); the driver expands it to `-freflection -fparameter-reflection -fattribute-reflection -fannotation-attributes -fexpansion-statements`
- `-fparameter-reflection`: P3096 parameter reflection in metafunctions (gates only the libc++ surface)
- `-fexpansion-statements`: P1306 expansion statements (`template for`: enumerating, iterable and destructuring forms)
- `-fannotation-attributes`: P3394 annotations (`[[=expr]]`)
- `-fattribute-reflection`: P3385 attributes reflection (targets C++29, not in the C++26 draft)
- P4033 `define_enum` has no flag of its own: it is exposed under plain `-freflection` (targets C++29)

## Code Organization

- Reflection core (`clang/lib/AST/`): `ExprConstantMeta.cpp`, `Metafunction.h`; parsing via `CXXReflectExpr`, `CXXSpliceSpecifierExpr`, and `CXXMetafunctionExpr`.
- Metafunction table: `clang/include/clang/AST/Metafunction.h` (60+ functions, each with a `Metafunction::evaluate` implementation).
- Splice desugaring: reflection contexts (`Sema::isReflectionContext()`) handle special parsing rules for `^E` and `[:R:]`.
- Known limitation: the evaluation callback in `CXXMetafunctionExpr` is non-serializable, breaking precompiled headers and C++20 modules.

## Trackers

**GitHub Issues (`spwn02/clang-cxx26`) is the single source of truth.**
`docs/CXX26_GAPS.md` (the prior living C++26 conformance tracker) and the
entire `docs/` tracking apparatus — every per-epic tracker
(`REFLECTION.md`/`REFLECTION_GAPS.md`/`REFLECTION_CLOSEUP.md`,
`LLVM22_SYNC.md`, `CONTRACTS_PORT.md`, `WAVES56.md`/`WAVES56_HANDOFF.md`,
`CLAUDE_HANDOFF.md`), every per-paper design/report doc, and the
`reflection-audit/` archive — were deleted 2026-09-30 once a full audit
(issue #116's Phase 0) confirmed every still-relevant fact in them was
either already reflected in a GitHub issue or freshly filed as one (issues
#152-164, milestone "Final Review Audit"). Do not recreate a parallel
`docs/`-based tracker; file or update a GitHub issue instead.

As of 2026-09-30, the reflection escalation cluster that both the
Reflection Closure Epic (2026-09-08–10) and its Reflection Closeup
follow-on (2026-09-10–11) left open is **fully resolved or tracked**:
- The consteval self-reference escalation cluster → issue #1, closed
  2026-09-12 (`7cce8e55d08d`, after 7 documented attempts — see
  `clang/lib/Sema/SemaExpr.cpp`'s `HandleImmediateInvocations` for the full
  multi-attempt history, still the right place to look for that specific
  bug's technical detail).
- P3560R2 strategy 2's ~20 remaining `Throws`-bearing metafunctions →
  issue #88, closed 2026-09-14 across 5 commits.
- Upstream `bloomberg/clang-p2996#180` (`static_assert(false)` silently
  ignored in a consteval function template) → now tracked as this fork's
  own issue #152, still open, confirmed still reproducing 2026-09-30.
- Upstream `bloomberg/clang-p2996#275` (clangd crash, two independent
  causes) → now tracked as issue #153, still open; one cause may already
  be resolved by #121, needs re-verification.
- Two more reflection gaps surfaced by the same audit, previously
  undocumented anywhere except session archives: upstream
  `bloomberg/clang-p2996#120` (Windows/MSVC mangling `llvm_unreachable` for
  reflection NTTPs) → issue #154; `reflect_constant_string`'s missing
  long-string chunking path (`bloomberg/clang-p2996#254`) → issue #155;
  `std::meta::data_member_spec(^^void, {})` wrongly accepted → issue #156.

`std::execution` (P2300R10) and Contracts (P2900R14) are both complete —
consult closed issues #10-13 and the `git log` `contracts:`-prefixed
commits respectively for history if needed; neither needs a live sub-plan
going forward.

**Status CSV mechanics:** `libcxx/docs/Status/Cxx2cPapers.csv` and
`Cxx2cIssues.csv` are hand-edited directly by this fork's commits (a
`synchronize_csv_status_files.py` GitHub-sync script exists but isn't the
normal path — ignore it unless specifically reconciling with the upstream
GitHub project board). Status vocabulary: empty string (not started),
`|In Progress|`, `|Partial|`, `|Complete|`, `|Nothing To Do|`. Update the
CSV row in the same commit that changes implementation status.

## Code Review Guidance

When modifying control flow, especially in code generation, optimization, or debug-info generation, check whether the change can corrupt instrumentation profile data or invalidate branch/call debug information. Run reflection tests before and after such changes.

Recent work focuses on C++26 standard-library conformance. Header/module exports change frequently; regenerate files with `libcxx-generate-files` when tests fail unexpectedly.

## Commit and Release Policy

- Commit every coherent minor implementation step after its focused tests.
- Update the relevant GitHub issue (comment or close) in the same session
  whenever status changes — see Trackers above.
- Commit and push every completed major milestone after its required test gate.
- Never push a knowingly broken milestone state.
- Create and push annotated prerelease tags after completed epics or other release-worthy checkpoints.
- Use `cxx26-YYYY.MM.DD`, then `.2`, `.3`, and so on for additional tags on the same day.
- Stage only files belonging to the current change; never stage unrelated user changes implicitly.

**Commit message shape** (see `git log --oneline` for examples):
- Subject: `[libc++] <Verb> ...` or `[clang] <Verb> ...`, imperative mood
  (Implement / Rewrite / Add / Fix / Complete ...), with a trailing
  `(#NNN)` GitHub issue reference where one applies.
- Body: prose, not a template. Cover what paper/LWG issue/GitHub issue
  drove the change; a concrete list of what changed and why; how it was
  tested, narrated inline (e.g. "differential testing against std::list",
  "concretely reproduced the prior crash") — no separate "Test coverage:"
  header required, though one is fine if it aids clarity.
- If the change updates a CSV status row, say so explicitly, e.g. "Marked
  P0447R28 Complete in Cxx2cPapers.csv."
- Trailer per the attribution instructions given at session start (model
  name may vary by session).

## Answer Style

Reply in the most concise form possible. Skip pleasantries,

preambles, and recaps of my question. No phrases like

"I'd be happy to", "Great question", or "Let me explain".

Drop articles and filler words wherever the meaning stays clear.

Prefer short declarative sentences. If a tool call is needed,

run it first and show only the result. Do not narrate your steps.

Commit every significant change you make.

## Orchestration Policy

Claude (the agent reading this file) should act as an orchestrator/validator,

not a doer, on complex/research/multi-step tasks. Delegate implementation,

research, investigation, and parallelizable work to Codex (`codex exec`)

subagents or parallel Claude subagents. Reserve direct Claude turns for:

deciding what to delegate, reviewing/merging results, running verification

gates, committing, and updating trackers. This conserves Claude's own token

budget across long/multi-day sessions. Do not narrate every action taken —

report only decisions, results, and blockers.
