# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Clang/P2996** is an experimental fork of LLVM that implements ISO C++ proposal [P2996](https://wg21.link/p2996) (_Reflection for C++26_). The implementation includes metafunctions, reflection operators, and splice expressions. See [P2996.md](P2996.md) for complete details on the feature set, known limitations, and design decisions.

## Build Architecture

This repository uses a **two-build-tree architecture**:

1. **`build-nyx/`**: Main LLVM/Clang build (configured from `llvm/` with projects: `clang;clang-tools-extra`). Generates `clang`, `clang-tools-extra`, and `llvm-*` tools.
2. **`build-libcxx/`**: libc++ build (configured from `runtimes/` with `LLVM_ENABLE_RUNTIMES=libcxx;libcxxabi;libunwind`). **Bootstraps from `build-nyx/bin/clang`** — front-end changes require rebuilding both trees in order.

Both use Ninja and Release builds. Check actual configurations with:
```bash
grep CMAKE_BUILD_TYPE build-nyx/CMakeCache.txt build-libcxx/CMakeCache.txt
```

## Building

```bash
# Full rebuild (after clang changes, rebuild both)
ninja -C build-nyx
ninja -C build-libcxx libcxx-generate-files
ninja -C build-libcxx cxx

# Incremental clang-only
ninja -C build-nyx clang
```

Generated C++26 module files must be refreshed after upstream changes:
```bash
ninja -C build-libcxx libcxx-generate-files  # Regenerates std.cppm, std.compat.cppm, etc.
```

## Testing

Reflection tests live in two locations with different lit configurations:

```bash
# libc++ metafunction tests (e.g., std::meta::* functions, library conformance)
# Use the libcxx-lit wrapper, NOT bare llvm-lit — bare llvm-lit resolves
# %{include-dir} to a STAGED install (build-libcxx/libcxx/test-suite-install/
# include/c++/v1), so after editing libcxx/include/ headers a bare run
# silently tests STALE headers. The wrapper rebuilds cxx-test-depends first.
libcxx/utils/libcxx-lit build-libcxx -sv libcxx/test/std/experimental/reflection/entity-classification.pass.cpp

# Full libc++ suite
ninja -C build-libcxx check-cxx

# Clang reflection tests (e.g., reflect expressions, splices, operator precedence)
./build-nyx/bin/llvm-lit clang/test/Reflection/ -v

# Single test
./build-nyx/bin/llvm-lit clang/test/Reflection/splice-types.cpp -v

# Full clang test suite
ninja -C build-nyx check-clang
```

Note: `clang/test/Reflection/splice-exprs.cpp` currently fails (line 23
expected-error not seen) — a pre-existing regression unrelated to any
in-progress work, tracked in `docs/CXX26_GAPS.md` Tier 0. Not yet fixed.

## P2996 Features & Flags

Enabled with `-std=c++26 -freflection`. Extended features require additional flags:

- `-freflection-latest`: All experimental features (recommended for testing)
- `-fparameter-reflection`: P3096 (parameter reflection in metafunctions)
- `-fexpansion-statements`: P1306 (expansion statements over ranges)
- `-fattr-reflection`: P3385 (attributes reflection)
- `-fdefine-enum`: P4033 (compile-time enum/unscoped-enum definition)

## Code Organization

- **Reflection core** (`clang/lib/AST/`): `ExprConstantMeta.cpp`, `Metafunction.h`; parsing via `CXXReflectExpr`, `CXXSpliceSpecifierExpr`, `CXXMetafunctionExpr`
- **Metafunction table**: `clang/include/clang/AST/Metafunction.h` (60+ functions, each with `Metafunction::evaluate` impl)
- **Splice desugaring**: Reflection contexts (`Sema::isReflectionContext()`) handle special parsing rules for `^E` and `[:R:]`
- **Known limitation**: The evaluation callback in `CXXMetafunctionExpr` is non-serializable, breaking precompiled headers and C++20 modules

## Code Review Notes

When modifying control flow (especially in codegen, optimization, or debug-info generation):
- Could the change corrupt performance profile data tracked by instrumentation?
- Could the change invalidate debug information for branches and calls?

Run reflection tests before and after any such changes.

## Recent Work

Recent commits focus on C++26 standard library conformance (e.g., `std::optional<T&>`, `std::hive`, `std::inplace_vector`). The `Update C++26 transitive include expectations` commit series implies that module and header exports change frequently — regenerate files with `libcxx-generate-files` if tests fail unexpectedly.

## C++26 Conformance Gap-Closing

**[docs/CXX26_GAPS.md](docs/CXX26_GAPS.md) is the living tracker for closing remaining C++26 language and library gaps** (excluding P2996 reflection itself, which is tracked in `P2996.md`). Read it before starting any C++26-conformance session — it has the prioritized tier list, current status, build/test corrections, commit conventions, and a session log. Update it in place as work completes; append to its Session Log before ending a session. Contracts (P2900R14) and `std::execution` (P2300R10) are large enough to warrant dedicated sub-plans — see that document's Tier 2 notes before starting either.

## Answer Style
Reply in the most concise form possible. Skip pleasantries,

preambles, and recaps of my question. No phrases like

"I'd be happy to", "Great question", or "Let me explain".

Drop articles and filler words wherever the meaning stays clear.

Prefer short declarative sentences. If a tool call is needed,

run it first and show only the result. Do not narrate your steps.

Commit every significant change you make.
