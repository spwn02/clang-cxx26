# C++26 Conformance Gap-Closing Contract

Persistent, cross-session tracking document for closing this fork's C++26
language and library conformance gaps (excluding the P2996 reflection work
itself, which is tracked in [`P2996.md`](../P2996.md)). Read this document
first at the start of any C++26-conformance session; update it in place as
work completes; append a dated entry to the Session Log before ending a
session.

This is the **single source of truth** for what's done, what's next, and why.
Do not create parallel tracking files — edit this one.

## Scope

**In scope:** C++26 STL facilities under `libcxx/` and C++26 language
features under `clang/`, as tracked by:
- `libcxx/docs/Status/Cxx2cPapers.csv` (library papers) and
  `Cxx2cIssues.csv` (LWG issue resolutions)
- `clang/www/cxx_status.html` (§ C++2c implementation status, language side)

**Excluded — tracked elsewhere:** Reflection-family papers
(P2996R13, P3394R4, P3293R3, P3491R3, P3096R12, P1306R5 "Expansion
Statements") show as unimplemented ("No") in `cxx_status.html`, but that page
mirrors upstream Clang and was never updated for this fork's own reflection
work. This fork *does* implement substantial portions of these under
`-freflection`/`-freflection-latest` and related flags — see `P2996.md` and
root `CLAUDE.md` for actual status. **Do not re-implement these from this
document; consult P2996.md instead.**

**Deferred — not sequenced into this plan:** **Contracts (P2900R14).**
Touches Parser/Sema/CodeGen *and* library simultaneously, is one of the
largest single features in C++26, and is orthogonal to this fork's
reflection focus. Revisit as a separate, dedicated multi-session project
once the tiers below are substantially complete. Do not start on it as part
of routine gap-closing work without an explicit decision to open that
project.

## Tier 0 — Blocking prerequisite (must do first)

`build-nyx` was configured with `LLVM_INCLUDE_TESTS=OFF`. There is currently
**no `check-clang` ninja target and no `clang/test/` lit config generated at
all** — any clang-side (language feature) work in this document is
untestable until this is fixed. This does not block `libcxx/`-side work.

- [ ] Reconfigure `build-nyx` with `-DLLVM_INCLUDE_TESTS=ON`, reusing the
      existing cache (do not pass other flags — CMake will keep them from
      `CMakeCache.txt`):
      ```bash
      cmake -S llvm -B build-nyx -DLLVM_INCLUDE_TESTS=ON
      ninja -C build-nyx
      ```
- [ ] Verify: `ninja -C build-nyx -t targets | grep '^check-clang$'` succeeds
      and `build-nyx/bin/llvm-lit clang/test/Sema -v` runs (not a config
      error).
- Note: `LLVM_ENABLE_ASSERTIONS=OFF` in this tree — tests tagged
  `REQUIRES: asserts` will be silently skipped once enabled. Consider
  whether to also flip this on; not required to unblock testing, but some
  language-feature tests assume assertion-build diagnostics.

## Build & Test Reference

Full build architecture is in root `CLAUDE.md`. The commands below correct
two details discovered while building this contract that root `CLAUDE.md`
did not have right — **use these, not bare `llvm-lit`, for libcxx work:**

```bash
# Single libcxx test — use the wrapper, NOT bare llvm-lit.
# Bare llvm-lit resolves %{include-dir} to a STAGED install
# (build-libcxx/libcxx/test-suite-install/include/c++/v1), so after editing
# libcxx/include/ headers a bare run silently tests STALE headers.
libcxx/utils/libcxx-lit build-libcxx -sv <test-path>

# Full libcxx suite
ninja -C build-libcxx check-cxx

# Regenerate generated files (feature-test macros, std.cppm.in, etc.) after
# adding/removing feature-test macros or headers. No lit test catches
# staleness automatically — run this and `git diff` to check:
ninja -C build-libcxx libcxx-generate-files

# Single/full clang test — BLOCKED until Tier 0 is done.
build-nyx/bin/llvm-lit <path> -v
ninja -C build-nyx check-clang
```

**Status CSV mechanics:** `libcxx/docs/Status/Cxx2cPapers.csv` and
`Cxx2cIssues.csv` are hand-edited directly by this fork's commits (the
`synchronize_csv_status_files.py` GitHub-sync script exists but isn't the
normal path — ignore it unless specifically reconciling with the upstream
GitHub project board). Status vocabulary: empty string (not started),
`|In Progress|`, `|Partial|`, `|Complete|`, `|Nothing To Do|`. Every commit
that changes implementation status must update the CSV row in the same
commit — this is enforced by convention (`libcxx/docs/Contributing.rst`
pre-commit checklist), not by CI.

## Commit Convention

Match this fork's existing style (see `git log --oneline` for examples like
`bcd5ad20bfd6`, `ac8ded1b3f7d`, `4fec85e052c6`):

- Subject: `[libc++] <Verb> ...` or `[Clang] <Verb> ...`, imperative mood
  (Implement / Rewrite / Add / Fix / Complete ...).
- Body: prose, not a template. Cover: what paper/LWG issue drove the change;
  concrete list of what changed and why; how it was tested (narrated
  inline — e.g. "differential testing against std::list", "concretely
  reproduced the prior crash" — no separate "Test coverage:" header is
  required, though one is fine if it aids clarity).
- Explicit line stating the CSV status change, e.g. `Marked P0447R28
  Complete in Cxx2cPapers.csv.`
- Trailer: `Co-Authored-By: Claude <model> <noreply@anthropic.com>`.

**Per user instruction: auto-commit at the end of each completed tier item**
(one paper/feature per commit, after its tests pass locally) — do not wait
for an explicit commit request for routine gap-closing work tracked in this
document. This does not apply to speculative/WIP changes, or to anything
outside the scope of this contract.

## Priority Tiers

Status legend: `[ ]` not started · `[~]` in progress · `[x]` complete ·
`[!]` blocked/deferred

### Tier 1 — High-impact library foundations

Small-to-medium scope, broadly used by other library code and user code.
Good starting point after Tier 0.

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [ ] | P0792R14 | `function_ref` | Non-owning callable wrapper, widely requested |
| [ ] | P2548R6 | `copyable_function` | Owning type-erased callable, pairs with function_ref |
| [ ] | P2363R5 | Heterogeneous lookup, remaining associative container overloads | Extends existing partial heterogeneous-lookup support |
| [ ] | P1901R2 | `weak_ptr` as unordered associative container key | Small, self-contained |
| [~] | P2944R3 | `reference_wrapper` comparisons | Partial — blocked on `optional`/`tuple` equality changes from P2165R4; check if P2988R11 work unblocked this |
| [~] | P1383R2 | `constexpr` for `<cmath>`/`<cstdlib>` | `<complex>` done; scalar math functions remain |
| [ ] | P3168R2 | `std::optional` range support | **Verify scope overlap with P2988R11 first** — range support for non-reference `optional<T>` may already be substantially covered; this may be a CSV-status-only fix plus a small test-coverage gap, not a fresh implementation |

### Tier 2 — Major standalone subsystems (large, phase separately)

Each of these is large enough to warrant its own multi-session sub-plan
(mini design doc or a dedicated section appended here once started) rather
than being tackled as a single commit.

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [ ] | P2300R10 | `std::execution` (sender/receiver) | Largest single item in scope. New `<execution>` header, many customization points, interacts with coroutines. Break into sub-milestones (schedulers → senders → algorithms → queries) before starting; do not attempt as one commit. |
| [ ] | P3325R5 | Execution environment utility | Depends on P2300R10 landing first |
| [ ] | P3396R1 | `std::execution` wording fixes | Depends on P2300R10 landing first |
| [!] | P2900R14 | Contracts | **Deferred** — see Scope section above |

### Tier 3 — Ranges, mdspan/linalg, format completions

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [ ] | P2542R8 | `views::concat` | |
| [ ] | P3138R5 | `views::cache_latest` | |
| [ ] | P3137R3 | `views::to_input` | |
| [ ] | P2846R6 | `reserve_hint` | |
| [ ] | P2630R4 | `submdspan` | |
| [ ] | P2642R6 | Padded `mdspan` layouts | |
| [ ] | P3355R1 | `submdspan` C++26 fixes | Depends on P2630R4 |
| [ ] | P3050R2 | `linalg::conjugated` optimization | |
| [ ] | P1673R13 | BLAS-based linear algebra interface | Large; consider its own sub-plan like Tier 2 items if scope proves big once started |
| [ ] | P2587R3 | `to_string` or not `to_string` | |
| [ ] | P2757R3 | Type-checking format args | |
| [ ] | P3107R5 | Efficient `std::print` implementation | |
| [ ] | P2845R8 | `std::filesystem::path` formatting | |
| [ ] | P3235R3 | `std::print` faster/leaner for more types | |

### Tier 4 — Atomics

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [ ] | P0493R5 | Atomic min/max | |
| [ ] | P2835R7 | `atomic_ref` object address exposure | |
| [ ] | P3323R1 | cv-qualified types in `atomic`/`atomic_ref` | |
| [ ] | P3309R3 | `constexpr atomic`/`atomic_ref` | |
| [ ] | P2869R4 | Remove deprecated `shared_ptr` atomic access APIs | Removal, not addition — low complexity |

### Tier 5 — Freestanding completeness

Lower priority (niche embedded/kernel audience), but self-contained.

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [ ] | P2198R7 | Freestanding feature-test macros | |
| [ ] | P2338R4 | Freestanding character primitives & C library | |
| [ ] | P2013R5 | Freestanding optional `::operator new` | |
| [ ] | P2407R5 | Freestanding partial classes | |
| [ ] | P2937R0 | Freestanding: remove `strtok` | |
| [ ] | P2833R2 | Freestanding `expected`/`span` | |
| [ ] | P2976R1 | Freestanding `algorithm`/`numeric`/`random` | |

### Tier 6 — Long tail (small, independent items)

Pick these up opportunistically; no particular ordering within the tier.

| Status | Paper | Feature |
|---|---|---|
| [ ] | P2592R3 | Hashing support for `std::chrono` value classes |
| [ ] | P1885R12 | `text_encoding` naming |
| [ ] | P2862R1 | `text_encoding::name()` should never return null |
| [ ] | P2641R4 | Checking if a `union` alternative is active |
| [ ] | P0952R2 | New spec for `std::generate_canonical` |
| [ ] | P2836R1 | `basic_const_iterator` convertibility |
| [ ] | P2264R7 | User-friendly `assert()` for C and C++ |
| [ ] | P2248R8 | List-initialization for algorithms |
| [ ] | P3217R0 | `find_last` addendum to P2248R8 |
| [ ] | P2810R4 | `is_debugger_present`, `is_replaceable` |
| [ ] | P1068R11 | Vector API for RNG |
| [ ] | P2075R6 | Philox RNG engine |
| [ ] | P3222R0 | Transposed special cases for P2642 mdspan layouts |
| [ ] | P3508R0 | Wording for constexpr specialized memory algorithms |
| [ ] | P3369R0 | `constexpr` for `uninitialized_default_construct` |
| [ ] | P3370R1 | New library headers from C23 |
| [ ] | P3349R1 | Converting contiguous iterators to pointers |
| [ ] | P3378R2 | `constexpr` exception types |
| [ ] | P3471R4 | Standard Library Hardening |

### Language-side gaps (clang/, blocked on Tier 0)

Not from the libcxx CSV — tracked from `clang/www/cxx_status.html`. Grouped
by rough priority; work these once Tier 0 is done, interleaved with library
tiers as makes sense.

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [~] | P2786R13 | Trivial relocatability | Partial — `__cpp_trivial_relocatability` feature-test macro not yet set |
| [ ] | P2841R7 | Concept and variable-template template-parameters | |
| [ ] | P2686R5 | `constexpr` structured bindings | |
| [ ] | P2795R5 | Erroneous behaviour for uninitialized reads | |
| [ ] | P2752R3 | Static storage for braced initializers (DR) | |
| [ ] | P3034R1 | Module declarations shouldn't be macros (DR) | |
| [ ] | P2843R3 | Preprocessing is never undefined | |
| [ ] | P3533R2 | `constexpr` virtual inheritance | |
| [ ] | P3074R7 | Trivial unions | Library-adjacent — coordinate with libcxx if relevant containers change |
| [ ] | P3475R2 | Defang and deprecate `memory_order::consume` | Coordinate with Tier 4 atomics work |
| [ ] | P1967R14 | `#embed` | |
| [!] | P1494R5 | Partial program correctness | Research-flavored, open-ended scope — consider deferring alongside Contracts once its actual scope is assessed |

## Session Log

Append a short dated entry each session — a few lines: what moved, what's
blocked, what's next. Do not remove old entries.

- **2026-08-20**: Contract created. Surveyed `Cxx2cPapers.csv`/`Cxx2cIssues.csv`
  (60 unstarted, 3 partial, 43 complete library papers) and
  `cxx_status.html` (language gaps). Identified Tier 0 blocker: `build-nyx`
  has `LLVM_INCLUDE_TESTS=OFF`, no `check-clang` target exists yet — no
  language-feature work has been verified testable. Verified libcxx testing
  requires `libcxx/utils/libcxx-lit`, not bare `llvm-lit`, to avoid stale
  staged headers. No implementation work started yet.
