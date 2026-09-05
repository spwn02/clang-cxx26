# Codex Repository Context

## Project Overview

**CXX26 Clang** is an experimental LLVM fork focused on implementing C++26 and
earlier C++ standards. Its experimental static-reflection implementation
includes metafunctions, reflection operators, and splice expressions; see
[`docs/REFLECTION.md`](docs/REFLECTION.md).

This repository preserves the Apache-2.0 WITH LLVM-exception license, upstream
source headers and Git authorship, and the history of the Bloomberg-originated
reflection implementation. Do not remove or rewrite that provenance, and do
not imply Bloomberg endorses this fork.

## Command Dispatch

- `Continue`: read `docs/CXX26_GAPS.md` and `docs/REFLECTION.md`, then resume their recorded active work. (The LLVM 22 synchronization epic tracked in `docs/LLVM22_SYNC.md` finished 2026-08-30 and that file was deleted; see `docs/CXX26_GAPS.md`'s "Post-Contracts TODO" section for the fork regressions it left open. The Contracts (P2900R14) port epic tracked in `docs/CONTRACTS_PORT.md` finished 2026-09-04 and that file was deleted too; see `docs/CXX26_GAPS.md`'s Scope section and Tier 2 table for the status flip, and `git log` — commits prefixed `contracts:` — for full technical history.)
- `Begin PXXXX`: locate the requested paper in `docs/CXX26_GAPS.md`, `docs/REFLECTION.md`, and any linked status files; research its requirements, implement autonomously, run focused tests, update the active tracker, and commit. Begin work without introductory narration.

## Build Architecture

This repository uses a **two-build-tree architecture**:

1. **`build-nyx/`**: Main LLVM/Clang build (configured from `llvm/` with projects: `clang;clang-tools-extra`). Generates `clang`, `clang-tools-extra`, and `llvm-*` tools.
2. **`build-libcxx/`**: libc++ build (configured from `runtimes/` with `LLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind`). **Bootstraps from `build-nyx/bin/clang`** — front-end changes require rebuilding both trees in order. **`ninja -C build-libcxx cxx` has no dependency edge on the external `build-nyx/bin/clang` binary and can silently no-op after a fresh clang rebuild**, leaving a stale `libc++.so`/`.a` in place (the `libcxx-lit` wrapper's `cxx-test-depends` rebuild only runs `cmake --install` steps, which doesn't force a relink either). After any `clang/lib/Sema` or `clang/lib/AST` change, before trusting libc++ test results run `ninja -C build-libcxx -t clean cxx && ninja -C build-libcxx -j$(nproc) cxx` explicitly.

Both use Ninja and Release builds. Check actual configurations with:

```bash
grep CMAKE_BUILD_TYPE build-nyx/CMakeCache.txt build-libcxx/CMakeCache.txt
```

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

`clang/test/Reflection/splice-exprs.cpp` currently fails because the expected error at line 23 is not seen. This is a known pre-existing regression tracked in `docs/CXX26_GAPS.md` Tier 0.

### Archived test runs (`cxx26/dev/`)

Built for the Contracts epic (`docs/CONTRACTS_PORT.md`, deleted on
completion) but generically useful for any future gate that needs to prove
"zero new failures vs. a known-good baseline" rather than eyeballing a
failure list:

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

- `-freflection-latest`: all experimental features (recommended for testing)
- `-fparameter-reflection`: P3096 parameter reflection in metafunctions
- `-fexpansion-statements`: P1306 expansion statements over ranges
- `-fattr-reflection`: P3385 attributes reflection
- `-fdefine-enum`: P4033 compile-time enum/unscoped-enum definition

## Code Organization

- Reflection core (`clang/lib/AST/`): `ExprConstantMeta.cpp`, `Metafunction.h`; parsing via `CXXReflectExpr`, `CXXSpliceSpecifierExpr`, and `CXXMetafunctionExpr`.
- Metafunction table: `clang/include/clang/AST/Metafunction.h` (60+ functions, each with a `Metafunction::evaluate` implementation).
- Splice desugaring: reflection contexts (`Sema::isReflectionContext()`) handle special parsing rules for `^E` and `[:R:]`.
- Known limitation: the evaluation callback in `CXXMetafunctionExpr` is non-serializable, breaking precompiled headers and C++20 modules.

## Trackers

- `docs/CXX26_GAPS.md` is the living C++26 conformance tracker and `docs/REFLECTION.md` tracks reflection. Read both before starting relevant work.
- Update the active tracker in place when status changes and append a dated session-log entry before ending a work session.
- `std::execution` (P2300R10) requires a dedicated sub-plan; consult Tier 2 notes before starting. Contracts (P2900R14) was completed 2026-09-04, reopened by a production bug and fully hardened as of 2026-09-05 (see `docs/CXX26_GAPS.md`'s Scope section for both epics' full history); no sub-plan needed going forward.

## Code Review Guidance

When modifying control flow, especially in code generation, optimization, or debug-info generation, check whether the change can corrupt instrumentation profile data or invalidate branch/call debug information. Run reflection tests before and after such changes.

Recent work focuses on C++26 standard-library conformance. Header/module exports change frequently; regenerate files with `libcxx-generate-files` when tests fail unexpectedly.

## Commit and Release Policy

- Commit every coherent minor implementation step after its focused tests.
- Update the active tracker in the same commit whenever status changes.
- Commit and push every completed major milestone after its required test gate.
- Never push a knowingly broken milestone state.
- Create and push annotated prerelease tags after completed epics or other release-worthy checkpoints.
- Use `cxx26-YYYY.MM.DD`, then `.2`, `.3`, and so on for additional tags on the same day.
- Stage only files belonging to the current change; never stage unrelated user changes implicitly.

## Answer Style

Reply in the most concise form possible. Skip pleasantries,

preambles, and recaps of my question. No phrases like

"I'd be happy to", "Great question", or "Let me explain".

Drop articles and filler words wherever the meaning stays clear.

Prefer short declarative sentences. If a tool call is needed,

run it first and show only the result. Do not narrate your steps.

Commit every significant change you make.
