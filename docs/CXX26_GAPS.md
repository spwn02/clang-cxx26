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

## Tier 0 — Blocking prerequisite (must do first) — DONE 2026-08-20

`build-nyx` was configured with `LLVM_INCLUDE_TESTS=OFF`. There was
**no `check-clang` ninja target and no `clang/test/` lit config generated at
all** — any clang-side (language feature) work in this document was
untestable until this was fixed. This never blocked `libcxx/`-side work.

- [x] Reconfigure `build-nyx`. **Correction to the original plan**:
      `-DLLVM_INCLUDE_TESTS=ON` alone is not enough — `CLANG_INCLUDE_TESTS`
      is a separate cached CMake option that only defaults from
      `LLVM_INCLUDE_TESTS` on a fresh configure; once cached `OFF` it stays
      `OFF` regardless of `LLVM_INCLUDE_TESTS`. Both flags are required:
      ```bash
      cmake -S llvm -B build-nyx -DLLVM_INCLUDE_TESTS=ON -DCLANG_INCLUDE_TESTS=ON
      ninja -C build-nyx
      ```
- [x] Verified: `check-clang` target now exists
      (`build-nyx/tools/clang/test/CMakeFiles/check-clang`), and
      `build-nyx/bin/llvm-lit clang/test/Sema/return.c -v` passes.
- Note: `LLVM_ENABLE_ASSERTIONS=OFF` in this tree — tests tagged
  `REQUIRES: asserts` will be silently skipped. Left as-is; flip on later
  if a specific gap's tests need assertion-build diagnostics.

**Known issue discovered while verifying (out of scope for this
contract — reflection regression, not a C++26 conformance gap):**
`clang/test/Reflection/splice-exprs.cpp` fails
(`build-nyx/bin/llvm-lit clang/test/Reflection/ -v` → 15/16 pass). Line 23
expects an `expected-error` diagnostic ("not derived from") that no longer
fires. Reproducible in isolation, not a parallelism/flake artifact. This
predates this contract and was invisible only because `check-clang` was
never runnable before today. Not fixed here — tracked as a known issue for
whoever picks up reflection-side maintenance; **do not confuse with the
Tier 1–6 items below**, which are new C++26 facilities, not regressions in
existing P2996 support.

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

# Single/full clang test — unblocked as of 2026-08-20 (Tier 0).
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
| [x] | P0792R14 | `function_ref` | Non-owning callable wrapper — done 2026-08-20 |
| [x] | P2548R6 | `copyable_function` | Owning type-erased callable — done 2026-08-20 |
| [ ] | P2363R5 | Heterogeneous lookup, remaining associative container overloads | Extends existing partial heterogeneous-lookup support |
| [ ] | P1901R2 | `weak_ptr` as unordered associative container key | Small, self-contained |
| [~] | P2944R3 | `reference_wrapper` comparisons | Partial — blocked on `optional`/`tuple` equality changes from P2165R4; check if P2988R11 work unblocked this |
| [~] | P1383R2 | `constexpr` for `<cmath>`/`<cstdlib>` | `<complex>` done; scalar math functions remain |
| [ ] | P3168R2 | `std::optional` range support | **Verify scope overlap with P2988R11 first** — range support for non-reference `optional<T>` may already be substantially covered; this may be a CSV-status-only fix plus a small test-coverage gap, not a fresh implementation |

**Known issue found while implementing `copyable_function` (not fixed — pre-existing,
affects `move_only_function` too, out of scope for this contract):**
`move_only_function`'s and `copyable_function`'s "unwrap another
instance instead of double-wrapping" constructor optimization
(`__is_move_only_function_v<_StoredFunc>` / `__is_copyable_function_v<_StoredFunc>`
branches in `__functional/{move_only,copyable}_function_impl.h`) directly assigns
the source object's `__vtable_` pointer into `*this`'s `__vtable_` field.
The vtable struct type is parameterized on `<_BufferT, _ReturnT, _ArgTypes...>`
only (not cv/ref/noexcept, which is why the optimization works at all across
qualifier-only conversions), but if the source and target specializations have
genuinely different `_ReturnT`/`_ArgTypes...` — e.g.
`move_only_function<long(short)> dst{some_move_only_function<int(int)>}`, invocable
because `short`→`int` and `int`→`long` both convert implicitly — the two vtable
pointer types are incompatible and the assignment is a hard compile error inside
`if constexpr`, not a graceful SFINAE fallback to double-wrapping. Confirmed by
direct repro against `move_only_function` during this session (removed after
confirming). **Fixed for `copyable_function`** (nested `if constexpr` checks
`is_same_v<decltype(__func.__vtable_), const _VTable*>` before taking the unwrap
path, falling through to `__construct` — i.e. double-wrapping — otherwise; see
`libcxx/test/std/utilities/function.objects/func.wrap/func.wrap.copy/basic.pass.cpp`'s
"Genuinely different signature" case). **`move_only_function` still has the bug**
— apply the same nested-`if-constexpr` fix there if picked up; low priority since
the triggering pattern (converting between wrappers of different-but-convertible
signatures via the generic constructor, as opposed to same-signature
qualifier-only conversions) is rare in practice.

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
- **2026-08-20 (same day, second entry)**: Completed Tier 0. Reconfigured
  `build-nyx` with both `-DLLVM_INCLUDE_TESTS=ON` and
  `-DCLANG_INCLUDE_TESTS=ON` (the latter was the actual missing piece —
  it's a separately-cached option, not purely derived from
  `LLVM_INCLUDE_TESTS`). Rebuilt; `check-clang` target confirmed present;
  `llvm-lit` runs cleanly on `clang/test/Sema/return.c`. Also ran the full
  `clang/test/Reflection/` suite as a smoke test (not part of Tier 0's
  requirements, but cheap and worth doing while test infra was fresh):
  15/16 pass, one pre-existing regression found (`splice-exprs.cpp`,
  documented under Tier 0 as a known issue, not fixed — out of scope for
  this contract). **Next session: pick a Tier 1 item** (recommend
  `function_ref` (P0792R14) or `copyable_function` (P2548R6) — both
  self-contained, no cross-tier dependencies) or make a call on the
  `splice-exprs.cpp` regression if reflection maintenance takes priority.
- **2026-08-20 (third entry)**: Implemented P0792R14 `function_ref`. New
  `libcxx/include/__functional/function_ref{,_common,_impl}.h`, modeled on
  `move_only_function`'s macro-generation trick (deducing the `noexcept`
  bool template parameter from the abominable function type's
  exception-specifier) but simpler: only a `cv` axis (no ref-qualifiers, per
  [func.wrap.ref]). Added `nontype_t`/`nontype` alongside it (same header,
  same paper). Non-owning storage is an exposition-only
  `__function_ref_bound_entity` union (void* object pointer / generic
  function pointer via `reinterpret_cast`), with accessor choice
  (`__get_object` vs. `__get_function`) matched to how each constructor
  stored the value — this matters for the generic `F&&` constructor, which
  can bind an actual function *lvalue* (not just a pointer) and must read
  back through the function-pointer union member in that case. Caught by
  the advisor before commit: the deleted `operator=(T)` template was
  initially unconstrained, silently breaking the standard's carve-outs
  (assigning a function pointer, or a `nontype_t` value, must still work
  through the converting constructor + defaulted copy-assignment; only
  genuinely-unrelated `T` should be deleted to block the dangling-temporary
  footgun) — fixed with a `requires` clause using a new `__is_nontype_t_v`
  trait, and added a `constexpr` global `function_ref` test to exercise the
  bound-entity void* round-trip under constant evaluation (operator() itself
  is intentionally not `constexpr` per the synopsis). Registered the three
  new headers in `libcxx/include/CMakeLists.txt` (missing this is why the
  first lit run failed with a stale-staged-install "file not found" — new
  headers aren't installed until added there). Updated: `functional`
  synopsis, `__cpp_lib_function_ref` (flipped `unimplemented` off in
  `generate_feature_test_macro_components.py`, regenerated `version` +
  `FeatureTestMacroTable.rst` + the two `*.version.compile.pass.cpp` tests
  via `libcxx-generate-files`), `libcxx/modules/std/functional.inc` (C++26
  guarded block), `Cxx2cPapers.csv` (P0792R14 → Complete). New test:
  `libcxx/test/std/utilities/function.objects/func.wrap/func.wrap.ref/basic.pass.cpp`.
  Full `function.objects` suite (172 tests) green after the fix: 162 passed,
  9 unsupported (unrelated feature gating), 1 pre-existing xfail. **Next
  session: `copyable_function` (P2548R6)** — same Tier 1 pairing, owning
  counterpart to `function_ref`; can likely reuse `move_only_function`'s
  vtable/small-buffer machinery more directly than `function_ref` could.
- **2026-08-20 (fourth entry)**: Implemented P2548R6 `copyable_function`. New
  `libcxx/include/__functional/copyable_function{,_common,_impl}.h`, modeled
  directly on `move_only_function`'s file split and macro-generation shape
  (same 6 cv×ref include passes, noexcept deduced from the abominable
  function type). Extracted the exact wording from the paper's PDF (the HTML
  redirect from `wg21.link` 404s for this one; fetched the PDF via WebFetch,
  then read it with the `Read` tool's PDF support). Differences from
  `move_only_function`: adds a `__clone_` vtable slot (copy-constructs the
  held object into fresh storage — heap or inline, `__small_buffer` needed
  no changes since `__construct`/`__alloc` already work for any
  constructible type regardless of triviality) and a copy constructor/copy
  assignment (copy-and-swap, matching the paper's `Effects: Equivalent to:
  copyable_function(f).swap(*this)` wording verbatim — including that
  neither assignment operator is marked `noexcept` in the synopsis, unlike
  `move_only_function`'s move-assign, which surprised me enough to
  double-check the source rather than "fix" it). Folded the paper's
  `is_copy_constructible_v<VT>` Mandates into each constructor's
  `requires`-clause, matching how this codebase already folds
  `is_constructible_v` into `move_only_function`'s constraints rather than a
  separate Mandates `static_assert`.

  Advisor review before commit caught a real bug in the "avoid
  double-wrapping when constructing from another `copyable_function`"
  optimization (mirrors `move_only_function`'s own
  `__is_move_only_function_v` branch): naively assigning the source's
  `__vtable_` pointer type-checks fine for same-signature
  cv/ref/noexcept-only conversions (the vtable struct isn't parameterized on
  those), but hard-errors inside `if constexpr` — not a graceful SFINAE
  fallback — for a genuinely different signature that's still invocable via
  implicit conversions (e.g. `copyable_function<long(short)>` constructed
  from a `copyable_function<int(int)>`), which the standard requires to
  compile via double-wrapping. Fixed with a nested `if constexpr
  is_same_v<decltype(__func.__vtable_), const _VTable*>` gate that falls
  through to `__construct` (double-wrap) when the vtable types don't match;
  a top-level `&&` doesn't work here since `decltype(__func.__vtable_)` is
  ill-formed (hard error, not SFINAE) when `_StoredFunc` isn't a
  `copyable_function` specialization at all. Verified the fix with both the
  same-signature qualifier-only case (still takes the fast unwrap path —
  confirmed via a `CopyCounting` copy-counter probe showing exactly one
  copy) and the differing-signature case (now compiles and runs correctly
  via double-wrap) in the new test. **Confirmed by direct repro that
  `move_only_function` has the identical latent defect** (documented under
  Tier 1 above, in the CSV-adjacent notes) — not fixed there, out of scope
  for this session, but the exact same one-line nested-`if-constexpr` fix
  would apply if picked up.

  Registered the three new headers in `libcxx/include/CMakeLists.txt`;
  updated the `functional` synopsis (`copyable_function` class template,
  placed per the paper's synopsis diff — right after `move_only_function`,
  before `function_ref`); flipped `__cpp_lib_copyable_function`'s
  `unimplemented` off and regenerated `version`/`FeatureTestMacroTable.rst`/
  the two `*.version.compile.pass.cpp` tests via `libcxx-generate-files`;
  added the C++26-guarded export to `libcxx/modules/std/functional.inc`;
  marked P2548R6 Complete in `Cxx2cPapers.csv`. New test:
  `libcxx/test/std/utilities/function.objects/func.wrap/func.wrap.copy/basic.pass.cpp`
  — covers SBO and heap-fallback storage, copy ctor/assign independence
  (via a copy-counting probe), move ctor/assign, `nullptr` reset,
  const-qualified and both ref-qualified (`const&`, `&&`) specializations,
  `noexcept` specialization, both `in_place_type_t` constructors (including
  the `initializer_list` overload), function-pointer construction,
  conversion from `std::function` (double-wrap, unoptimized but
  conforming), and both unwrap-optimization cases described above. Full
  `function.objects` suite (173 tests) green: 163 passed, 9 unsupported
  (unrelated feature gating), 1 pre-existing unrelated xfail. Both Tier 1
  callable-wrapper papers (`function_ref`, `copyable_function`) are now
  done. **Next session: pick a remaining Tier 1 item** — `P2363R5`
  (heterogeneous lookup remaining overloads), `P1901R2` (`weak_ptr` as
  unordered map/set key), or the two partial items (`P2944R3`
  `reference_wrapper` comparisons, `P1383R2` `constexpr` `<cmath>`) — or
  make a call on the `move_only_function` vtable-mismatch bug noted above
  if callable-wrapper maintenance takes priority over new Tier 1 work.
