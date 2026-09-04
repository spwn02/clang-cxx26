# C++26 Conformance Gap-Closing Contract

Persistent, cross-session tracking document for closing this fork's C++26
language and library conformance gaps (excluding the CXX26 reflection work
itself, which is tracked in [the reflection documentation](REFLECTION.md)). Read this document
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
(reflection revision 13, P3394R4, P3293R3, P3491R3, P3096R12, P1306R5 "Expansion
Statements") show as unimplemented ("No") in `cxx_status.html`, but that page
mirrors upstream Clang and was never updated for this fork's own reflection
work. This fork *does* implement substantial portions of these under
`-freflection`/`-freflection-latest` and related flags — see `docs/REFLECTION.md` and
root `CLAUDE.md` for actual status. **Do not re-implement these from this
document; consult the reflection documentation instead.**

**Contracts (P2900R14): complete, 2026-09-04.** Was deferred out of this
plan's scope (touches Parser/Sema/CodeGen *and* library simultaneously, one
of the largest single features in C++26) and executed as its own dedicated
epic instead, per the note above. Ported from efcs/llvm-project's
contracts-nightly fork (upstream LLVM has not landed contracts). Full
technical history — the mechanical port, the three constification
mechanisms fixed, serialization/codegen/diagnostic bugs found and fixed,
library-side wiring, and the merge into `cxx26` — is in `git log` on this
branch (commit subjects prefixed `contracts:`) and in the individual commit
messages; the epic's own tracker, `docs/CONTRACTS_PORT.md`, was deleted on
completion per its stated policy since it had no open items to carry
forward. See the Tier 2 table below for the status-row flip.

Merged into `cxx26` locally at `64ad936fe5fe` (`--no-ff`), plus 5 more
commits fixing check-clang/check-cxx regressions surfaced by M6 and closing
out the docs (current tip before push: `365b6ec50c28`).

**M8 downstream verification — both passes complete, no regressions.**
Packaged a real reference-toolchain snapshot
(`cxx26/toolchain/build-linux-x86_64.sh` + `package.py`, identity
`cxx26-dev-365b6ec50c` since this predates the actual tag) and ran it
against Nyx, Miracle, and Switch (`~/dev/cpp/Nyx/...`, all three build via
their own `clang-tests`/`development-tests` CMake presets, none committed
or pushed to per standing policy — no fixes were needed there anyway).

Caught and fixed a real methodology bug along the way: Miracle's and
Switch's `build/tests` directories were pre-existing, from earlier
sessions, with `CMAKE_CXX_COMPILER` already cached to stale toolchains (one
to the pre-contracts `~/.local/opt/clang-cxx26-2026.09.04` install, one to a
scratchpad path from an unrelated prior session that no longer even
corresponds to this epic). A CMake toolchain file only takes effect on a
genuinely fresh configure — passing a new `P2996_CMAKE_TOOLCHAIN_FILE` env
var into an *existing* cache silently does nothing. The first "pass 1" run
against Miracle/Switch appeared to succeed but was actually exercising an
unrelated old compiler. Caught by explicitly grepping `CMAKE_CXX_COMPILER`
out of each `CMakeCache.txt` after configuring rather than trusting a green
build; fixed by deleting the stale `build/tests` directories (safe — pure
gitignored build output) and reconfiguring fresh, confirming the resolved
compiler path pointed at the new package before trusting any result.

- **Pass 1 (contracts off, no-regression):** Switch 100% (1/1), Miracle
  100% (1/1, using `FETCHCONTENT_SOURCE_DIR_SWITCH` to avoid a GitHub fetch
  of a commit not yet pushed there), Nyx's `development-tests` preset
  (vcpkg rebuilt its `x64-linux-cxx26` triplet's Vulkan/glslang/spirv-tools
  stack from source against the new compiler — triplet ABI hash changed
  with the toolchain path, ~20 min) built clean and its `unit_tests` binary
  passed 5/5 headless (RHI metadata + reflection/clap tests; no GPU/display
  needed for this subset).
- **Pass 2 (`-fcontracts` regression + integration):** Miracle and Switch
  both build and pass 100% with `-fcontracts` injected via
  `CMAKE_CXX_FLAGS` (had to replicate the toolchain file's full
  `-std=c++26 -stdlib=libc++ -freflection-latest` alongside it — a bare
  `-DCMAKE_CXX_FLAGS=-fcontracts` silently *replaces* the toolchain's
  `_INIT` flags rather than appending, another test-methodology trap worth
  remembering). Neither project has any existing contracts usage (Miracle's
  `contracts/` directory is an unrelated project design-charter, not C++
  Contracts). Wrote a standalone integration smoke test instead (not
  committed to either repo) combining real reflection
  (`nonstatic_data_members_of`) with real pre/post-conditions against the
  packaged toolchain end to end: precondition violation correctly routes
  through libc++'s `contract_violation` object (`kind()`/`comment()`
  correct) and continues under `observe` semantics — proof the *distributed
  package*, not just the build tree, wires compiler and library together
  correctly.

Pushed and tagged as `cxx26-2026.09.04.1` (not `cxx26-2026.09.04` — that
identifier is already an immutable published tag, at the pre-contracts
commit `33df47d52b81`, confirmed via `git ls-remote --tags origin` before
tagging).

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
existing CXX26 support.

## Post-Contracts TODO — carried over from the LLVM 22 sync

Epic A (LLVM 22 synchronization, `docs/LLVM22_SYNC.md`) finished 2026-08-30:
`integration/llvm-22.1.8` merged into `cxx26` (`--no-ff`, commit
`ca44e7b01b09`), pushed, tagged `cxx26-2026.08.30`. All 9 milestones closed;
`check-clang` (49778 tests) and `check-cxx` (12035 tests) both gated clean
against a fully documented, named exception list. That tracker file has been
deleted now that the epic is closed — recover its full text with
`git show dbc3036eea3c:docs/LLVM22_SYNC.md` (last commit to touch it) if any
item below needs more detail than is captured here. Contracts (P2900R14,
see Scope above) finished 2026-09-04; these two regressions are recorded so
they aren't lost, not because they're necessarily next in line — pick up as
routine gap-closing work when convenient.

**Minor open curiosity, not a regression:** the Contracts epic's pre-port
`check-cxx` baseline (captured 2026-09-04 at `97ea0acaee51`, just before the
epic branched) showed 50 failures, down from Epic A's recorded 199 — not
investigated (not the Contracts epic's job per its own M1 gate), and never
became confusing enough to need investigating: M4/M5/M6 all diffed cleanly
against this same 50-failure number with zero new failures throughout, so
whatever caused the drop (prior fixes landing, a config difference between
the two epics) did so before Contracts started and is orthogonal to it.
Revisit only if idle curiosity strikes; nothing currently depends on it.

**Two open fork regressions, both in `clang/lib/Sema/SemaExpr.cpp`'s
`HandleImmediateInvocations`/`Rec.ConstevalOnly` consteval-escalation
machinery** (introduced by the LLVM 22 merge, confirmed via before/after
testing against the pre-sync baseline — not pre-existing):

- **Self-reference escalation cluster** —
  `clang/test/AST/ByteCode/builtin-is-within-lifetime.cpp`,
  `clang/test/SemaCXX/constant-expression-cxx11.cpp`, and
  `libcxx/test/std/experimental/reflection/reflection-ex-parsing-command-line-options-2.sh.cpp`
  (same root cause, different context — `constexpr`-var-init vs. `template
  for` range-init). Root cause fully understood:
  `HandleImmediateInvocations` has two diagnosis buckets,
  `Rec.ImmediateInvocationCandidates` (ordinary consteval-call escalation —
  must run even inside a `constexpr`-var initializer) and `Rec.ConstevalOnly`
  (consteval-only-*type* escaping — must stay suppressed there, since a
  successfully-evaluated `std::meta::info` held in a `constexpr` variable is
  legitimate); both currently share one `isImmediateFunctionContext()` gate,
  so no fix can toggle it without breaking one bucket in favor of the other.
  A fix was attempted (`6b5f636e6ba1`), shipped, found to regress 9 libc++
  reflection tests, and reverted (`87bcf7d13116`). **Do not re-attempt that
  blanket `ActOnCXXEnterDeclInitializer` context-push revert** — proven
  net-negative. A correct fix needs `Rec.ConstevalOnly`'s diagnosis loop to
  recognize an expression that already evaluated successfully in the
  `Rec.ImmediateInvocationCandidates` loop moments earlier; no existing
  signal carries that today.
- **Template-instantiation escalation cluster** —
  `clang/test/SemaCXX/cxx2b-consteval-propagate.cpp`,
  `clang/test/SemaCXX/cxx2a-constexpr-dynalloc.cpp`. Reproduces with minimal
  repros; root cause not found (do not assume it's the same root cause as
  the cluster above, or that the two share one with each other — only
  confirmed as "breaks under template instantiation or implicit synthesis").

  Any fix to either cluster must be verified against the full libc++
  reflection suite (60 tests, forced-clean `build-libcxx` rebuild — see
  the `build-libcxx` staleness gotcha in `AGENTS.md`), not just the
  narrower clang suites — that's what caught the reverted fix's regression
  and its absence is what let it ship in the first place.

**Two more crashes found 2026-09-04 by the Contracts Hardening epic's M1**
(`docs/CONTRACTS_HARDENING.md`), which flipped `LLVM_ENABLE_ASSERTIONS=ON` in
`build-nyx` for the first time. Both are `check-clang` failures previously
invisible because they trip `llvm_unreachable`/`assert` — undefined behavior,
not a trap, in a Release/NDEBUG build — rather than producing a wrong-but-
non-crashing result. Neither is a contracts bug; recorded here rather than
fixed in that epic per its own scope discipline (each is a nontrivial,
self-contained compiler change, the same shape as the two clusters above):

- **`clang/test/Reflection/splice-namespaces.cpp`** — `UNREACHABLE executed at
  clang/include/clang/AST/NestedNameSpecifier.h:83! ("invalid prefix for
  namespace")`. Root cause: `NestedNameSpecifier::MakeNamespacePtrKind`
  handles a namespace nested-name-specifier prefixed by `Kind::Null`,
  `Kind::Global`, or another `Kind::Namespace`, but not one prefixed by
  `Kind::Splice`/`Kind::SpliceWithTemplate` — i.e. `[:some_ns_reflection:]::
  inner::x`, a splice used as the left-hand scope of a further-nested
  namespace name. Fixing it needs a new `StoredKind` (e.g.
  `NamespaceWithSplice`), threaded through the `PointerUnion` bit layout,
  storage struct, printing, and profiling in
  `clang/include/clang/AST/NestedNameSpecifier{,Base}.h` — at least 6 switch
  sites reference `Kind::Splice`/`SpliceWithTemplate` in that header alone.
  Reflection-area, so worth prioritizing whenever reflection gets its own
  hardening pass.
- **`clang/test/SemaCXX/PR98671.cpp`** — `Assertion 'IsExpectedEntity(FD1) &&
  FD2 && IsExpectedEntity(FD2) && "use non-instantiated function declaration
  for constraints partial ordering"' failed` in `Sema::IsAtLeastAsConstrained`
  (`SemaConcept.cpp:2518`). Pure vanilla C++20 concepts partial-ordering
  machinery — the test itself has zero contracts/reflection/expansion-
  statement references (confirmed by grep) and predates this fork's feature
  work (`c4724f603849`, upstream PR #98671, present since the LLVM 22
  baseline). General conformance/LLVM-sync territory, not this epic's.

**Re-verify before starting, don't assume still-open.** Three commits landed
after the epic closed (`55872c0fadcc`, `079b20780c79`, `33df47d52b81`,
2026-09-02/03), driven by downstream Nyx/Miracle testing rather than this
tracker, touching this exact `ConstevalOnly`/`InImmediateEscalatingFunctionContext`
subsystem (expansion-statement variables and reflection-substituted template
codegen, not the two clusters above by inspection — but not run against
`clang/test/SemaCXX/` or the libc++ reflection suite either). Rebuild and
re-run both named clusters plus the full libc++ reflection suite first; the
list above may already be partly stale.

**`check-cxx` 199-failure baseline** (post-merge, for reconciling a future
full run — a bare failure count of 199 means nothing without this): 145
clang-tidy bucket (144 `clang_tidy.gen.py`/`*.sh.cpp` + 1 `clang_tidy.sh.py`,
pure-upstream, see below) + 27 `std/execution/**` (Tier 2, out of scope) + 8
libc++ reflection-suite (6 Milestone-1-baseline names +
`reflection-ex-enum-to-string.pass.cpp` +
`reflection-ex-parsing-command-line-options-2.sh.cpp`, the latter also
counted above) + 2 `std`-module-gap (`module_std.gen.py`,
`module_std_compat.gen.py`, blocked on the clang-tidy bug below) + 17
"other" (`extensions/gnu/hash/specializations.verify.cpp`,
`system_reserved_names.gen.py/execution.compile.pass.cpp`, 4×
`atomic_fetch_{add,sub}{,_explicit}.verify.cpp`,
`gdb_pretty_printer_test.sh.cpp` [pure-upstream, GDB pretty-printer assumes
an `unordered_map` iteration order this fork doesn't control], 2×
`is_within_lifetime`, `atomics.ref/cv_qualified.pass.cpp`,
`element_access_transparent.pass.cpp` [pure-upstream], 3× `inplace.vector`,
3× `optional.iterator{,s}`).

**Unfiled upstream bug**: `docs/drafts/upstream-clang-tidy-ppcallbacks-crash.md`
— a vanilla `llvmorg-22.1.8` `clang-tidy` crash in `libcpp-cpp-version-check`/
`libcpp-internal-ftms` (both register `PPCallbacks`), confirmed pure-upstream
with a 100% vanilla binary and byte-identical check sources. Blocks the 2
`std`-module-gap failures above. Needs a human review pass before
`gh issue create` against `llvm/llvm-project`.

**`meta.inc` latent gap, no failing test**: `libcxx/modules/std/meta.inc`'s
reflection export guard is a single `#if __has_feature(reflection)`, but
`<meta>` itself gates several members behind finer-grained nested guards
(`__has_feature(annotation_attributes)`,
`__has_feature(attribute_reflection)`, `__has_feature(parameter_reflection)`).
Correct under `-freflection-latest` (the flag the packaged toolchain actually
bakes into `std.pcm`) — verified directly against
`build-libcxx/.../share/libc++/v1/std.cppm`. Fails (16 errors) under bare
`-freflection`, but no lit test combines `import std` with any reflection
flag, so this is latent, not live. Fix by mirroring the four nested guards
from `<meta>` into `meta.inc` line-for-line when a test is added that would
exercise this — not before, there's nothing to verify against yet.

**Disk hazard for a future full `check-cxx` run** (not in `AGENTS.md`):
`libcxx/test/extensions/clang/clang_modules_include.gen.py` reached ~9G
during generation in one run, dropping free disk from 18G to 8.5G in about
10 minutes. Gitignored and regenerated fresh each run — safe to `rm -rf`
proactively before a full `check-cxx`, rather than reactively once disk is
already tight.

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
| [x] | P2363R5 | Heterogeneous lookup, remaining associative container overloads | Done 2026-08-20 |
| [x] | P1901R2 | `weak_ptr` as unordered associative container key | Done 2026-08-20 |
| [x] | P2944R3 | `reference_wrapper` comparisons | Done 2026-08-22 — all Constraints (`pair`/`tuple`/`optional`/`variant`/`reference_wrapper`) were already implemented (mostly inherited from upstream commits); only the shared `__cpp_lib_constrained_equality` FTM flag and CSV status needed flipping |
| [!] | P1383R2 | `constexpr` for `<cmath>`/`<cstdlib>` | **Compiler-blocked** 2026-08-22 — `<complex>` done; scalar math functions need constexpr-evaluator support this Clang doesn't have. See notes below. |
| [x] | P3168R2 | `std::optional` range support | Done 2026-08-20 — implementation was already complete via P2988R11; added missing test coverage |

**P1383R2 scalar `<cmath>`/`<cstdlib>` — compiler-blocked, found 2026-08-22 (not
implemented, not scope-excluded):** probed this fork's constant evaluator
directly rather than guessing from the generator's `unimplemented` flag alone
(the flag only proves nobody's flipped it, not that the compiler can't). Repro
(compile with `build-nyx/bin/clang++ -std=c++26`):

```cpp
static_assert(__builtin_fabs(-1.0) == 1.0);          // OK
static_assert(__builtin_fmin(1.0, 2.0) == 1.0);      // OK
static_assert(__builtin_sqrt(4.0) == 2.0);           // error: not a constant expression
static_assert(__builtin_floor(1.5) == 1.0);          // error: not a constant expression
static_assert(__builtin_pow(2.0, 3.0) == 8.0);       // error: not a constant expression
```

Confirmed by grepping `clang/lib/AST/ExprConstant.cpp` for `Builtin::BI__builtin_`
cases directly (not inferred from the static_assert failures alone): this
fork's evaluator implements exactly `fabs`/`copysign`/`fmax`/`fmin`/
`fmaximum_num`/`fminimum_num`/`nan`/`nans` (floating) and `abs`/`labs`/`llabs`
(integer) — nothing else. `sqrt`/`pow`/`floor`/`ceil`/`trunc`/`round`/`fmod`/
`exp`/`log`/every trig function have **no case at all**, not a
disabled/guarded one. Line 15957's own `// FIXME: Builtin::BI__builtin_powi`
comment is upstream's own marker that this area is known-incomplete — this is
upstream LLVM's gap, not something introduced by this fork's reflection work.

**Do not attempt to close this by extending `ExprConstant.cpp`** — beyond the
sheer size (dozens of transcendental/rounding functions, each needing
correctly-rounded semantics matching the Cpp17 math-function requirements),
this file's neighborhood is exactly where this fork's own reflection
evaluator (`ExprConstantMeta.cpp`) lives; adding an unrelated upstream feature
here creates permanent rebase friction against a file this fork's actual
purpose depends on. A pure-library fallback (hand-rolled constexpr algorithms
for e.g. `floor`/`ceil`/`trunc`, dispatched via
`__builtin_is_constant_evaluated()`) is also not worth starting: **
`__cpp_lib_constexpr_cmath` is a single all-or-nothing macro** — implementing
a subset changes no observable status (CSV stays Partial, FTM stays
unimplemented) since the paper requires the whole surface area.

**Also blocked behind an undone C++23 prerequisite**, same shape as the
P2944R3/P2165R4 finding above: the generator's `__cpp_lib_constexpr_cmath`
entry has *only* a `c++23` value (P0533R9's own number) with `unimplemented:
True` — no C++26 bump exists yet for P1383R2's own value. `Cxx23Papers.csv`
confirms P0533R9 itself is only `|In Progress|` (just `isfinite`/`isinf`/
`isnan`/`isnormal`, which need no builtin folding at all — pure bit
manipulation on the float representation). So P1383R2 is a C++26 row sitting
on top of an incomplete C++23 row, tracked in a different CSV entirely — a
future session should not start P1383R2 thinking it's one paper deep.

**Tracked as compiler-blocked, not scope-excluded** — the repro above is the
regression test for "has this been fixed yet" (deliberately not landed as a
lit test: an assertion that a compiler limitation exists needs `XFAIL` and
would misfire confusingly, not usefully, once someone actually fixes the
evaluator). Revisit if this fork's Clang ever gains upstream's missing
builtin-folding support, or if P0533R9's own classification-function scope
expands first.

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
| [x] | P2300R10 | `std::execution` (sender/receiver) | Complete 2026-08-22 — see dedicated sub-plan below. Scope confirmed to collapse with P3325R5/P3396R1 into one effort (merged draft wording); landed across M1–M6c. |
| [x] | P3325R5 | Execution environment utility | Folded into the P2300R10 sub-plan below (its content is `[exec.envs]`/`prop`+`env`, M1) — flipped together with P2300R10 |
| [x] | P3396R1 | `std::execution` wording fixes | No separable content — already merged into current draft wording used by the sub-plan below; flipped to Complete alongside P2300R10 at M6c, not separately implemented |
| [x] | P2900R14 | Contracts | Complete 2026-09-04 — executed as its own dedicated epic, see Scope section above |

### Tier 2 Sub-Plan: P2300R10 `std::execution` (sender/receiver)

Started 2026-08-20. This is a **multi-session effort** — do not read a
partially-done milestone below as abandoned work; check this section's
status markers and the Session Log for where to pick up.

**Scope collapse (verified via `eel.is/c++draft/exec`, clause 33):** the
current draft's `[exec]` clause is the *merged* wording — it already
incorporates P3396R1's wording fixes and P3325R5's `prop`/`env`
utility additions. Treat all three CSV rows (P2300R10, P3325R5, P3396R1)
as **one implementation effort**; flip all three to `|Complete|` together
when M6 lands, not separately.

**Explicitly out of scope (separate, untracked papers merged into the same
draft clause after P2300R10 landed — do not implement here):**
- `[exec.coro.util]` 33.13.3–33.13.6: `execution::affine`,
  `execution::inline_scheduler`, `execution::task_scheduler`,
  `execution::task` — this is P3552, a distinct paper with no CSV row.
- `[exec.scope]` 33.14: execution scope / counting-scope utilities — P3149
  and related async-scope papers, no CSV row.
- `[exec.par.scheduler]` / `[exec.parschedrepl]` 33.15–33.16: parallel
  scheduler and `parallel_scheduler_replacement` — P3481 and related, no
  CSV row.

In scope: `[exec.queryable]`, `[exec.async.ops]`, `[execution.syn]`,
`[exec.queries]` (all of 33.5), `[exec.sched]`, `[exec.recv]`,
`[exec.opstate]`, `[exec.snd]` (all of 33.9, factories through consumers),
`[exec.cmplsig]`, `[exec.envs]` (`prop`/`env`, this is the P3325R5 part),
`[exec.ctx]` (`run_loop`), and only the first two coroutine-utility
clauses `[exec.as.awaitable]`/`[exec.with.awaitable.senders]` (33.13.1–2 —
these are P2300R10 core, unlike the `task`-family clauses after them).

**Structural constraint — verified by reading `libcxx/include/execution`
in full:** the existing C++17 PSTL execution-policy content
(`execution::seq`/`par`/`par_unseq`/`unseq`, `is_execution_policy`) is
guarded inside `#if _LIBCPP_HAS_EXPERIMENTAL_PSTL && _LIBCPP_STD_VER >= 17`
... `#endif`, and shares `namespace std::execution` with what P2300 adds.
New content must **not** go inside that guard (it would silently vanish
on any build without experimental PSTL enabled). Plan: new exposition
headers under `libcxx/include/__execution/`, included from the existing
`<execution>` top-level header inside a **separate**
`#if _LIBCPP_STD_VER >= 26` block, sharing the same `namespace
std::execution` but independently guarded. `libcxx/modules/std/execution.inc`
already exists (for the PSTL policies, under `_LIBCPP_ENABLE_EXPERIMENTAL`)
— add a second, independently-guarded C++26 export block there per
milestone, not a new file.

**Customization model — confirmed no precedent in this repo
(`grep -r tag_invoke libcxx/include` → empty):** R10 customization is
**member-function/member-typedef based**, not `tag_invoke`:
`sndr.connect(rcvr)`, `rcvr.set_value(...)`/`set_error`/`set_stopped`,
`env.query(q)`, `sndr.transform_env(...)`/`sndr.transform_sender(...)`
(domain-based customization). Each CPO must be hand-built as an
exposition-only niebloid/function-object type per `[exec.queryable.concept]`
`[exec.snd.expos]` etc., not adapted from any existing tag_invoke-shaped
code. `libcxx/include/__ranges/access.h` (the `ranges::begin` niebloid) is
this repo's closest existing style precedent for the CPO-as-hidden-`__fn`-
struct-with-`inline constexpr` pattern — reuse that shape, not tag_invoke.

**`<stop_token>` dependency:** `stoppable_token`, `stoppable_token_for`,
`unstoppable_token`, `never_stop_token` concepts, and
`inplace_stop_token`/`inplace_stop_source`/`inplace_stop_callback` are new
C++26 additions to `<stop_token>` introduced alongside P2300 — **not
present in this repo yet** (confirmed empty grep for
`stoppable_token`/`never_stop_token` in `__stop_token/stop_token.h`).
Note existing `stop_token`/`stop_source` (the allocating, type-erased
C++20 versions) are entirely guarded on `_LIBCPP_STD_VER >= 20 &&
_LIBCPP_HAS_THREADS` — check independently during M1 whether the new
concepts (generic, no allocation) and `inplace_stop_source` (spin-lock via
atomics, no OS thread primitives) actually need `_LIBCPP_HAS_THREADS` or
just atomics; don't copy the guard mechanically without checking.

**Milestone order (foundation-first, not the "schedulers → senders →
algorithms → queries" order originally sketched at the top of this
document — queries/environments and stop-token concepts are load-bearing
for everything downstream, and `schedule()` itself returns a sender so
schedulers can't come before sender concepts exist):**

- [x] **M1** — Queries & environments foundation: `queryable` concept,
  exposition query-object machinery, `forwarding_query`, `get_env`/
  `env_of_t`, `get_allocator`, `get_stop_token`, `prop`/`env` class
  templates (the P3325R5 part), plus the `<stop_token>` additions above
  (`stoppable_token` family, `inplace_stop_token`/`source`/`callback`).
  No sender/receiver concepts yet — this milestone is pure queryable
  infrastructure, testable via `static_assert`/concept checks alone.
  **Landed 2026-08-20 (two sessions)**: first session —`__queryable`
  concept, `forwarding_query_t`/`forwarding_query`, `prop<Query, Value>`,
  `env<Envs...>`, `get_env_t`/`get_env`/`env_of_t` — new
  `libcxx/include/__execution/{queryable,forwarding_query,env,get_env}.h`,
  included from `<execution>` in the independently-guarded
  `_LIBCPP_STD_VER >= 26` block per the structural constraint above. Tests
  under `libcxx/test/std/execution/{exec.queries/exec.fwd.env,exec.queries/
  exec.get.env,exec.envs/exec.prop,exec.envs/exec.env}/`. Second session
  (same day) — the rest of M1: `get_allocator`/`get_allocator_t` (new
  `libcxx/include/__execution/get_allocator.h`, with an exposition-only
  `__simple_allocator` concept per [allocator.requirements.general]) and
  `get_stop_token`/`get_stop_token_t` (new
  `libcxx/include/__execution/get_stop_token.h`), plus the `<stop_token>`
  additions they depend on: `stoppable_token`/`unstoppable_token` concepts
  (new `libcxx/include/__stop_token/stoppable_token.h`), `never_stop_token`
  (new `.../never_stop_token.h`), and `inplace_stop_source`/
  `inplace_stop_token`/`inplace_stop_callback` (new `.../inplace_stop_
  {source,token,callback}.h`). Reused the existing `__stop_state`/
  `__stop_callback_base`/`__atomic_unique_lock`/`__intrusive_list_view`
  machinery that already backs `stop_source`/`stop_token`/`stop_callback`
  rather than reimplementing the callback-registration race from scratch —
  `inplace_stop_source` holds a `__stop_state` by value (`mutable`, so
  callback (de)registration can mutate it through the `const
  inplace_stop_source*` that `inplace_stop_token`/`inplace_stop_callback`
  store) instead of going through `__intrusive_shared_ptr`, since
  "in-place" tokens are non-owning references with no allocation or
  ref-counting — the source object itself must outlive every token/callback
  referring to it, which is stated as a header comment (not "safe
  regardless of outstanding tokens/callbacks" — that would be wrong).
  Caught before it shipped: `inplace_stop_source`'s constructor must call
  `__state_.__increment_stop_source_counter()` (mirroring `stop_source`'s
  constructor) even though "in-place" sources aren't ref-counted — skipping
  it would leave `__stop_state`'s internal source-counter at 0 forever, and
  `__add_callback` unconditionally gives up (treating it as a "no
  stop-source exists" state) whenever that counter is 0, so every
  `inplace_stop_callback` registration would silently no-op. Also retrofit
  `template<class Fn> using callback_type = stop_callback<Fn>;` onto the
  existing C++20 `stop_token` class (guarded `_LIBCPP_STD_VER >= 26`) —
  verified against `eel.is/c++draft/stoptoken` (not inferred purely from
  the concept failing to compile) that the C++26 draft's `stop_token`
  synopsis actually adds this, needed so `stop_token` itself still models
  the new `stoppable_token` concept. Advisor caught two availability-marker
  gaps invisible in this environment's `libcpp-has-no-availability-markup`
  build (would break Apple-platform builds otherwise): `inplace_stop_
  callback`'s class itself needed `_LIBCPP_AVAILABILITY_SYNC` (its
  ctor/dtor call `__stop_state` methods that carry the marker, matching
  how `stop_callback` is marked — it was only on the deduction guide,
  which doesn't cover the class), and the `stop_callback` forward
  declaration added to `stop_token.h` needed the same marker as its real
  definition; added `// XFAIL: availability-synchronization_library-missing`
  to the new tests that exercise availability-marked code, matching the
  existing `stopsource/*.pass.cpp` convention. Tests: `libcxx/test/std/
  execution/exec.queries/{exec.get.allocator,exec.get.stop.token}/`, and
  `libcxx/test/std/thread/thread.stoptoken/{stoppable_token→stoptoken.
  concepts,stoptoken.never,stopsource.inplace,stoptoken.inplace,
  stopcallback.inplace}/`. Full `execution/` + `thread/` suites green
  (351/354, matching the pre-existing 3-unsupported baseline) after both
  sessions; `transitive_includes.gen.py`/`module_std.gen.py` clean
  (125/126, unchanged); verified `<execution>`'s C++26 content compiles
  and works with `-std=c++26` and no `-D_LIBCPP_ENABLE_EXPERIMENTAL` (the
  no-PSTL path the structural separation exists for) both times.

  **Load-bearing compiler-behavior finding, discovered empirically this
  session and reproduced independently against both this fork's Clang and
  stock upstream Clang 22 / GCC 16 (so it's not fork-specific) — record
  this before writing any more CPOs against this pattern:** a
  `requires { obj.method(args); }` **simple-requirement is evaluated
  eagerly, not as a substitution-failure-is-fine probe, whenever `obj`'s
  type and `args`' types are concrete (non-dependent) at that point** —
  e.g. checking a local variable's method directly from a `static_assert`
  in a plain function. If `method` exists but is a constrained template
  whose sole viable candidate gets removed by constraint-checking, this is
  a **hard compile error**, not a silent `false`, in both Clang and GCC —
  confirmed with `requires`-clauses, fold-expression `requires`-clauses,
  and classic `enable_if`-on-return-type SFINAE alike (all three behave
  identically here). The construct only behaves as a soft, SFINAE-style
  probe when the checked entities are genuinely dependent — i.e., the
  requires-expression's *own* parameter list must be substituted from an
  enclosing *template's* parameters (see `env.h`'s `__has_query` concept
  for the working pattern, and the `CanQuery` helper added to
  `env.pass.cpp` for how to write assertions against it). This affects
  every CPO built for the rest of this sub-plan (M2's `connect`/`sender`/
  `receiver` concepts, all of M3–M6) — write `requires{}` checks against
  a template's own parameters, never against concrete/already-resolved
  local objects, or downstream code that legitimately needs to probe "is
  this call well-formed" (as most exec CPOs do) will hard-error instead of
  falling back.

  Separately, also discovered and fixed: a function's **noexcept-specifier
  is not protected by SFINAE** the way a trailing requires-clause is
  (exception specifications are evaluated when forming the function's type
  for overload resolution, which is outside the "immediate context") — see
  the `_Idx == sizeof...(_Envs)` guard in `env::__query_is_noexcept`, and
  the regression test for it in `env.pass.cpp` (`CanQuery<env<prop<QueryA,
  int>>, QueryB>` must itself stay well-formed).

  **M1 correction, landed 2026-08-20 (same day, before M2 started):** the
  original M1 implementation put `forwarding_query_t`/`forwarding_query`,
  `get_allocator_t`/`get_allocator`, `get_stop_token_t`/`get_stop_token`, and
  the exposition-only `queryable` concept inside `namespace std::execution`.
  Cross-checking `eel.is/c++draft/execution.syn` directly (via `curl` +
  Python tag-strip, not WebFetch's summarizer — see process rule below)
  showed these are declared in plain `namespace std`, not `std::execution` —
  confirmed independently by the raw P2300R10 paper text too, which is the
  one point the two sources agree on. Fixed by moving all four out of the
  `namespace execution { ... }` wrapper in `__execution/{queryable,
  forwarding_query,get_allocator,get_stop_token}.h` (they're declared
  directly under `_LIBCPP_BEGIN_NAMESPACE_STD` now); `get_env`/`env_of_t`
  and `prop`/`env` correctly stay in `std::execution` per the same synopsis.
  Also added `stop_token_of_t<T>` (declared alongside `get_stop_token` in
  `namespace std` per the synopsis; not implemented in the original M1
  pass). Updated the 4 affected M1 tests (qualification + `using namespace
  std::execution;` → `using namespace std;` where those names were being
  brought in unqualified) and `libcxx/modules/std/execution.inc` (split into
  a `std` export block for the four moved names plus `stop_token_of_t`, and
  a separate `std::execution` block for `env`/`prop`/`get_env`/`env_of_t`).
  All 11 affected tests re-verified green; `libcxx-generate-files` clean
  (no unrelated diffs).

  **Process rule, established while researching M2 and worth keeping for
  M3–M6:** WebFetch's AI-summarized answers proved unreliable for this
  material — wrong namespaces, wrong tag names (`receiver_t` vs. the actual
  `receiver_tag`), wrong CPO shapes, and at least one invented declaration
  presented as real. The reliable method: `curl -s -A "Mozilla/5.0"
  https://eel.is/c++draft/<clause>` (or the P2300R10 paper HTML at
  `https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html`
  for prose/algorithm-body wording only — see below), then strip tags with a
  short inline `python3 -c "import re,html; ..."` and grep/read the raw
  text. **eel.is (the current merged working draft) wins over the R10 paper
  on what exists and what things are named** — confirmed by direct
  contradiction: R10 has `empty_env`/`receiver_t`/`sender_t`/
  `operation_state_t`; the current draft has neither `empty_env` (verified
  absent from `execution.syn` — `env<>` is the actual default everywhere,
  matching what M1 already implemented) nor `*_t`-suffixed tags (it's
  `receiver_tag`/`sender_tag`/`operation_state_tag`/`scheduler_tag`). Use
  the R10 paper only for wording of algorithm bodies where eel.is says "see
  below" — re-check every name against the current synopsis before typing
  it, since the draft has moved substantially past R10 (it now also
  contains entities belonging to separate, explicitly out-of-scope papers:
  `task_scheduler`/`affine` (P3149), `associate`/`spawn_future`,
  `bulk_chunked`/`bulk_unchunked`, `indeterminate_domain`,
  `get_start_scheduler`/`get_delegation_scheduler`/`get_completion_domain`/
  `get_await_completion_adaptor`, `inline_scheduler`, `with_error` — do not
  implement these; the scope rule is "implement an entity only if something
  in the P2300R10+P3325R5+P3396R1 surface actually names it").

- [x] **M2** — Core concepts, split like M1 (foundation sub-slice first,
  domain-dispatch sub-slice second). **Landed 2026-08-20.**

  **M2a** (`__execution/{completion_functions,completion_signatures,
  receiver,operation_state,sender,get_completion_signatures}.h`):
  `set_value`/`set_error`/`set_stopped` CPOs (ill-formed on lvalue/const-
  rvalue receiver, matching [exec.set.value]/[exec.set.error]/
  [exec.set.stopped]); `completion-signature` concept and
  `completion_signatures<Fns...>` (bare marker class -- `count-of`/
  `for-each` are themselves dash-named exposition-only helpers not
  referenced by anything else in scope, so not implemented); the full
  `gather-signatures`/META-APPLY/`indirect-meta-apply` machinery, transcribed
  literally (the indirection is load-bearing for non-variadic Tuple/Variant
  arguments, not decoration); `receiver_tag`/`receiver`/`valid-completion-
  for`/`has-completions`/`receiver_of`; `operation_state_tag`/
  `operation_state`/`start_t`/`start`; `sender_tag`/`is-sender`/`sender`/
  `tag_of_t` (via a real structured-binding pack, `auto&& [tag, data,
  ...children] = sndr` -- confirmed this fork's Clang supports P1061
  structured-binding packs); `dependent_sender_error : exception {}`;
  `get_completion_signatures<Sndr, Env...>()`/`completion_signatures_of_t`/
  `sender_in`; `value_types_of_t`/`error_types_of_t`/`sends_stopped` (with
  `__decayed_tuple`/`__variant_or_empty`/`__empty_variant` per
  [execution.syn]'s own prose definition of variant-or-empty, deduping via
  a `type_list`-based fold).

  **M2b** (`__execution/{domain,connect}.h`): `get_completion_domain_t`
  (declared with no `operator()` -- see deviations below); `default_domain`
  (`transform_sender`/`apply_sender` static members); `get_domain_t`/
  `get_domain`; the free `transform_sender`/`apply_sender` CPOs including
  the real `transform-recurse` fixed-point algorithm; `connect_t`/`connect`/
  `connect_result_t`; the exposition-only `sender-to` concept.

  Corrections to the M2 plan recorded below, discovered while implementing:
  tag types are `receiver_tag`/`operation_state_tag`/`sender_tag`/
  `scheduler_tag` in the *current* draft (not R10's `receiver_t`/etc., which
  the M1-correction commit's namespace fix already established eel.is wins
  on); `sender_in<Sndr, class... Env>` is genuinely variadic (0 or 1) as
  planned; `receiver` **does** require `is_nothrow_move_constructible_v`
  (an earlier note claimed otherwise -- wrong, corrected against the actual
  [exec.recv.concepts] fetch); `transform_sender`'s free-function form is
  confirmed 2-arg/no-domain (`transform_sender(Sndr&&, const Env&)`) with
  domain resolution happening inside the body via `get_domain`/
  `completion-domain`, not a 3-arg domain-first CPO.

  **Five deliberate, documented deviations from the letter of the spec**
  (each has an in-code comment at its exact location; recorded here too so
  a later session doesn't have to re-derive the reasoning):
  1. **`enable-sender`'s awaitable disjunct is omitted** (`__execution/
     sender.h`): `enable-sender = is-sender<Sndr> || is-awaitable<Sndr,
     env-promise<env<>>>`. The awaitable half needs the GET-AWAITER/
     `env-promise`/`with-await-transform` coroutine-integration machinery
     that's M6's whole job; every sender through at least M5 declares
     `sender_concept = sender_tag`, so `is-sender` always short-circuits
     true and the second disjunct is dead code until M6, where adding it
     back is a one-line change.
  2. **`dependent_sender`/`is-dependent-sender-helper` are not
     implemented**, and `get_completion_signatures` never throws
     `dependent_sender_error` for a genuinely dependent sender (one whose
     signatures can only be known once connected to a real environment).
     Root cause: [exec.getcomplsigs]'s Effects requires throwing an
     exception from a `consteval` function (`is-dependent-sender-helper`'s
     function-try-block, P3068 constexpr-exceptions), which this Clang does
     not support -- verified empirically:
     `consteval bool f() try { throw E{}; return false; } catch (E&) {
     return true; } static_assert(f());` fails to be a constant expression.
     Consequence: a sender with no viable `get_completion_signatures`
     dispatch simply has `sender_in` be false (soft), rather than
     `dependent_sender` being true. Tracked as **compiler-blocked**, not
     scope-excluded -- revisit if/when this fork gains constexpr-exception
     support. The repro above is the regression test for "has this been
     fixed yet."
  3. **`get_completion_domain`/`get_scheduler`-driven domain resolution is
     not implemented** (`__execution/domain.h`): `get_completion_domain_t`
     is declared with *no* `operator()`, purely so `completion-domain(s)`'s
     `requires{}` probe inside `transform_sender` is a soft SFINAE failure
     (not a hard "no such name in namespace" error) rather than needing a
     forward-declaration my own team would have to keep exception-spec-
     synchronized with a later real definition. `get_domain`'s branch (2.2)
     (deriving the domain from a scheduler's completion domain) is likewise
     unimplemented. Both fall back to `default_domain` unconditionally,
     which is correct for every sender/env in scope through M5 (none
     provide a completion scheduler). `operator()` lands with real
     schedulers.
  4. **`default_domain::transform_sender`'s tag-dispatch branch always
     takes the "otherwise" path** (`static_cast<Sndr>(forward<Sndr>(sndr))`,
     never `tag_of_t<Sndr>().transform_sender(...)`). This is the most
     interesting finding of the session: computing `tag_of_t<Sndr>` inside
     a `requires{ tag_of_t<Sndr>()...; }` probe **hard-errors instead of
     evaluating false** for a `Sndr` that doesn't decompose into at least
     (tag, data) -- confirmed empirically. Root cause: `tag_of_t` is a
     `decltype(auto)`-deduced alias over a *separate* helper function
     (`__sender_tag_of`) containing the actual structured-binding
     declaration; deducing that `auto` return type requires instantiating
     the helper's *body*, and body-instantiation errors are outside the
     "immediate context" of substitution that SFINAE covers -- unlike a
     substitution failure in a signature/return-type position, this is a
     genuinely hard error, empirically reproduced (a `void`-returning probe
     function does *not* trigger this, since its call's well-formedness
     never needs the body instantiated -- but a `void`-returning probe also
     can't accurately detect decomposability, so it doesn't help either).
     This is a second, structurally distinct instance of the M1
     eager-evaluation family of pitfalls -- "body instantiation triggered
     by return-type deduction is not immediate context," not "requires{}
     eagerly evaluates concrete entities" -- but has the same practical
     consequence (write code that's dependent-context-safe or it hard-
     errors instead of degrading). Since nothing in scope through M5
     defines a per-tag `.transform_sender` member (the branch this dispatch
     exists for), this deviation currently changes no observable behavior;
     revisit if a sender ever needs per-tag domain customization, using
     either an aggregate-member-count SFINAE trick or (cleaner) redesigning
     `__sender_tag_of` to make the "does this decompose" question checkable
     without instantiating a body.
  5. **`connect` only implements the member-`connect` dispatch** (branch
     6.1 of [exec.connect]'s Effects); the `connect-awaitable` fallback
     (branch 6.2, for senders that are awaitables but don't define a member
     `connect`) needs the same GET-AWAITER machinery as deviation 1 and is
     deferred to M6. Every sender connected through at least M5 defines a
     member `connect`, so this is unexercised, not incorrect.

  Also: `[exec.recv.concepts]`'s `valid-completion-for` is specified via a
  concept spelled `callable<Tag, remove_cvref_t<Rcvr>, Args...>` that does
  not appear defined anywhere in `<concepts>` or `[exec]` (grepped both);
  `invocable` is used as the closest standard equivalent (checking that
  `Tag{}(rcvr, args...)` is a valid call) -- noted in `__execution/
  receiver.h`.

  **New tests** (all passing under `libcxx-lit`, 52/52 in `execution/` +
  `thread.stoptoken/`): `exec.recv/exec.recv.concepts/receiver.pass.cpp`,
  `exec.opstate/exec.opstate.start/operation_state.pass.cpp`,
  `exec.snd/exec.snd.concepts/sender.pass.cpp` (includes a 3-member
  tag/data/child decomposition check, not just 2-member),
  `exec.getcomplsigs/get_completion_signatures.pass.cpp`,
  `exec.cmplsig/completion_signatures.pass.cpp`,
  `exec.domain.default/default_domain.pass.cpp`,
  `exec.snd.transform/transform_sender.pass.cpp`,
  `exec.snd.apply/apply_sender.pass.cpp`, `exec.connect/connect.pass.cpp`.
  Registered the 8 new headers in `CMakeLists.txt` and `<execution>`'s
  `_LIBCPP_STD_VER >= 26` include block; extended `libcxx/modules/std/
  execution.inc`'s export block; updated `transitive_includes/cxx26.csv`
  (execution now transitively pulls in cstdlib/cstring/exception/
  initializer_list/typeinfo/variant, from `<exception>`/`<variant>`
  themselves and glibc's `<cstring>`/`<cstdlib>` chain). `module_std.gen.py`
  and `transitive_includes.gen.py` both clean. Full `execution/` +
  `thread.stoptoken/` suites green; did not run the full `check-cxx`
  (blocked early by a pre-existing, unrelated `std.cppm`/`reflection_v2`
  module-build failure -- confirmed pre-existing via `git stash` on this
  commit's changes, unaffected either way, nothing to do with `<execution>`
  or reflection).

  **Next session: M3** — `just`/`just_error`/`just_stopped`, `read_env`,
  `schedule`. Note `read_env`'s zero-Env `get_completion_signatures` case is
  exactly the "genuinely dependent sender" scenario from deviation 2 above
  -- decide deliberately there (rather than rediscovering it) whether
  `read_env`'s completion signatures can be computed without invoking the
  now-missing `dependent_sender_error`-throwing path, or whether it needs
  its own workaround.

- [x] **M3** — First real senders: `just`/`just_error`/`just_stopped`,
  `read_env`, `schedule` (scheduler concept is exercised here for the
  first time via a trivial scheduler, not defined as its own milestone).
  **Landed 2026-08-21.**
- [x] **M4** — `run_loop` + `this_thread::sync_wait` (`sync_wait_with_variant`
  deferred, see below) + `then` (rode along per the original plan). **Landed
  2026-08-21.** Vertical-slice checkpoint confirmed working end-to-end:
  `sync_wait(just(42) | then([](int i){ return i+1; }))` (see note below on
  why it's `sync_wait(...)`, not a trailing `| sync_wait()`).

  **Scope correction caught before implementation started:** the checkpoint
  expression as originally written above, `... | sync_wait()`, doesn't
  parse — `sync_wait` is a sender *consumer*
  ([exec.sync.wait]: `this_thread::sync_wait(sndr)` →
  `apply_sender(Domain(), sync_wait, sndr)`), not a pipeable sender adaptor
  object; there is no nullary `sync_wait()` producing a closure. Corrected to
  `sync_wait(just(42) | then(f))` — a plain function call wrapping the piped
  sender chain, not part of the pipe itself.

  **`run_loop`** (`__execution/run_loop.h`): `run-loop-scheduler`/
  `run-loop-sender`/`run-loop-opstate<Rcvr>` hand-written per
  [exec.run.loop] (mutex + `condition_variable`-backed intrusive queue, not
  an aggregate — a class with a pure virtual function can't be one, so
  `run-loop-opstate-base` gained a small constructor). One clause-text
  literalism not followed: [exec.run.loop.types]p10.2 writes
  `get_stop_token(REC(o))` applied directly to the receiver, but
  `get_stop_token` ([exec.get.stop.token]) is defined only in terms of a
  queryable *environment*; implemented as `get_stop_token(get_env(REC(o)))`,
  matching how every other stop-token consumption in the draft is actually
  spelled — documented inline at the point of use, not treated as a new
  compiler-limitation finding.

  **Query CPOs** (`__execution/get_scheduler.h`): `get_scheduler_t`,
  `get_start_scheduler_t`, `get_delegation_scheduler_t`, and (needed by all
  three) `get_completion_scheduler_t<Cpo>` — implements both
  [exec.get.compl.sched] branches (5.1's TRY-QUERY+RECURSE-QUERY chain, not
  just 5.2's `auto(q)` fallback the original plan sketched): `get_scheduler`
  itself routes *through* `get_completion_scheduler`, and `run_loop`'s own
  self-consistency requirement
  (`get_completion_scheduler<C>(get_env(schedule(sch))) == sch` with an
  *empty* envs pack) only typechecks via branch 5.1, not 5.2 — confirmed
  both branches are actually exercised by real code in this milestone, not
  speculative completeness. `get_completion_domain` was deliberately left
  alone (still no `operator()`, per the M2 deviation) — sync_wait's own
  `Domain` resolution reuses the existing `execution::__completion_domain`
  soft-fallback helper from `<__execution/domain.h>` rather than calling
  `get_completion_domain` directly, so the M2 deviation's noexcept
  assumptions there stay valid.

  **`this_thread::sync_wait`** (`__execution/sync_wait.h`): `sync-wait-env`/
  `-state`/`-receiver` per [exec.sync.wait], dispatched through
  `default_domain::apply_sender` (already built at M2) via a member
  `sync_wait_t::apply_sender`. `AS-EXCEPT-PTR` ([exec.general]p8) is
  implemented locally in this header (its only consumer so far) rather than
  factored out. **`sync_wait_with_variant` is not implemented**: its
  `apply_sender` is specified directly in terms of `into_variant`
  ([exec.sync.wait.var]p3: `sync_wait(into_variant(sndr))`), an M5 sender
  adaptor — pulling it forward into M4 was explicitly rejected rather than
  silently skipped. Revisit once M5 lands `into_variant`.

  **Pipeable closures** (`__execution/sender_adaptor_closure.h`, new for
  this milestone — M1–M3 built only factories, no adaptors yet):
  `sender_adaptor_closure<D>` CRTP base + the two `operator|` overloads
  (`sndr | c` ≡ `c(sndr)`; `c | d` composes), modeled directly on
  `<__ranges/range_adaptor.h>`'s `range_adaptor_closure`/
  `__range_adaptor_closure` split (substituting "is a sender" for "is a
  range" as the disqualifying case) — this is the shared foundation every
  M5 adaptor's single-argument partial-application form rides on, not
  `then`-specific.

  **`then`** (`__execution/then.h`): hand-written aggregate sender (own
  `connect`/`get_completion_signatures`), matching the M1–M3 precedent of
  not routing through the draft's `basic-sender`/`impls-for` engine — same
  P3068 constexpr-exceptions blocker as before (`then`'s own `check-types`
  needs `throw unspecified-exception()` from a `consteval` function).
  `upon_error`/`upon_stopped` — the same `then-cpo` mechanism generalized to
  intercept `set_error`/`set_stopped` instead of `set_value` — were **not**
  built here despite sharing an exposition family with `then` in the
  standard; scoped to M5 per the tracker's original split, not
  speculatively generalized for. FWD-ENV ([exec.snd.expos]p4, needed by
  every single-child adaptor's `get_env`/receiver-environment per
  [exec.adapt.general]p3.2/3.4) is new this milestone too
  (`__execution/fwd_env.h`) — stores its wrapped env *by value*, not by
  reference like the standard's exposition sketch might suggest, since
  `FWD-ENV(get_env(rcvr))` is typically constructed from a temporary
  returned by `get_env` and a reference member would dangle past the
  full-expression that creates it.

  **Three new compiler-behavior/language findings from this session, distinct
  from M1–M3's:**
  1. A bare CPO call as the *first* operand of a `requires`-clause,
     immediately followed by `&&`, mis-parses on this fork's Clang: `requires
     std::forwarding_query(_Tag()) && requires(...) {...}` treats
     `std::forwarding_query` alone (type `const forwarding_query_t`) as the
     whole atomic constraint, leaving `(_Tag())` to dangle. Parenthesizing
     the call (`requires (std::forwarding_query(_Tag())) && requires(...)
     {...}`) fixes it — confirmed in isolation with a two-line repro
     independent of any execution-library code. Existing call sites of this
     `requires EXPR && requires(...) {...}` shape (e.g. `connect.h`'s
     `sender<_Sndr> && receiver<_Rcvr> && requires(...)`) were unaffected
     because their leading operands are *concept* checks, not calls — the
     parser doesn't stumble there. Only affected `<__execution/fwd_env.h>`;
     no other code in the tree used this exact shape.
  2. Forming a function type by substituting `void` into a template
     parameter used as a parameter type — e.g. `set_value_t(_ResultT)` with
     `_ResultT = void` — is a hard "argument may not have 'void' type"
     error, *unlike* the literal, unsubstituted source spelling `F(void)`
     (the standard C++ "no parameters" idiom), which is fine. This bit
     `then`'s completion-signature transform (mapping a void-returning `fn`
     to a datum-less `set_value_t()`): picking between `set_value_t()` and
     `set_value_t(_ResultT)` via `__conditional_t`/`conditional_t` doesn't
     help, since both branches are eagerly *formed* as types regardless of
     which is selected. Fixed with a `_ResultT`-specialized helper template
     (a `void` explicit specialization that never substitutes `void` into
     the primary template's `F(_ResultT)`) rather than a runtime/constexpr
     choice between two pre-formed types — the general pattern for "a
     function type whose parameter might be void" anywhere else in this
     sub-plan (M5's `upon_error`/`upon_stopped` will need the identical
     shape).
  3. Both a partial specialization and a full/explicit specialization of a
     member class template, declared *inside* the enclosing (still-open)
     class template's own body — as opposed to at namespace scope after the
     enclosing template closes, which is the textbook-safe location —
     compile and behave correctly on this fork's Clang (confirmed in
     isolation for both cases). Used throughout `then.h`'s
     `__then_sig_transform<_Fn>` (nested `__one<_Sig>`/`__dedup<_List>`/
     `__to_completion_signatures<_List>` specializations, and the
     `__then_value_sig<_ResultT>`/`<void>` pair from finding 2, all declared
     in-class). Not verified against the standard's letter one way or the
     other; flagging so a future session hitting an in-class member-template
     specialization that *doesn't* compile knows this fork's behavior here
     was empirically checked, not assumed.

  **New tests** (all passing under `libcxx-lit`; `execution/` suite now
  63/63 green including the new files, `thread.stoptoken/` still 37/37, no
  regressions): `exec.ctx/run_loop.pass.cpp` (behavioral: schedule/connect/
  start/finish/run, plus the `get_completion_scheduler` round-trip identity
  from [exec.run.loop.types]p5/p8.2), `exec.consumers/exec.sync.wait/
  sync_wait.pass.cpp` (all three completion paths — value/error/stopped —
  the latter two via small hand-written senders since neither
  `just_error(...)` nor `just_stopped()` alone has a value completion,
  which sync_wait's own Mandates require), `exec.queries/{exec.get.scheduler,
  exec.get.start.scheduler,exec.get.delegation.scheduler}/*.pass.cpp`,
  `exec.adapt/exec.then/then.pass.cpp` (the checkpoint expression itself,
  call-syntax equivalence, multi-datum/void-returning `fn`, pipe chaining,
  the throwing-`fn`-rethrows-through-sync_wait path, and both branches of
  the nothrow-vs-potentially-throwing completion-signature transform).
  Registered all 6 new headers in `CMakeLists.txt`, `<execution>`'s
  `_LIBCPP_STD_VER >= 26` include block (`get_scheduler.h`/`run_loop.h`
  gated internally on `_LIBCPP_HAS_THREADS`, matching `get_stop_token.h`'s
  existing pattern — not the whole file excluded from the include list),
  and `libcxx/modules/std/execution.inc`'s export block (including a new
  `export namespace std::this_thread { using std::this_thread::sync_wait;
  }` block, and an explicit `using std::execution::operator|;` — needed
  separately from the class exports per `range_adaptor_closure`'s own
  `ranges.inc` precedent). `transitive_includes/cxx26.csv`'s `execution`
  rows needed real changes this time (first since M1) —
  `sync_wait.h`'s `<system_error>`/`<optional>` pull in
  `cctype`/`cerrno`/`climits`/`cstddef`/`cstdio`/`ctime`/`cwchar`/`cwctype`/
  `iosfwd`/`optional`/`ratio`/`stdexcept`/`string`/`string_view`/
  `system_error` transitively; regenerated from the actual preprocessor
  trace, not hand-edited. `module_std.gen.py`/`transitive_includes.gen.py`
  125/126 (same pre-existing 1-unsupported baseline as M2/M3).
  `Cxx2cPapers.csv` intentionally untouched (stays `|In Progress|` per the
  sub-plan's FTM discipline — `__cpp_lib_senders` only flips at M6).

  **Next session: M5** — remaining sender adaptors (`upon_error`/
  `upon_stopped` first and cheaply, reusing this milestone's `then`
  machinery almost unchanged; then the rest per the list below). Re-read
  this session's three findings above before writing more completion-
  signature-transform or in-class-specialization code.
- [x] **M5** — Remaining sender adaptors: `upon_error`/`upon_stopped` **done
  2026-08-21**, `let_value`/`let_error`/`let_stopped` **done 2026-08-21**,
  `schedule_from`/`continues_on`/`starts_on` **done 2026-08-22**, `on`
  **done 2026-08-22, all forms** (2-arg `on(sch, sndr)`, 3-arg
  `on(sndr, sch, closure)`, and the 2-arg partial-application form
  `on(sch, closure)`), `write_env`/`unstoppable` **done 2026-08-22**,
  `stopped_as_optional`/`stopped_as_error` **done 2026-08-22**,
  `into_variant` **done 2026-08-22**, `when_all` **done 2026-08-22**,
  `when_all_with_variant` **done 2026-08-22**,
  `bulk`/`bulk_chunked`/`bulk_unchunked` **done 2026-08-22**.
  **M5 is complete — only M6 remains in this sub-plan.**
  (`associate`/`spawn`/
  `spawn_future` are part of the
  execution-scope paper family — reassess whether they're actually
  P2300R10-original or scope-family additions when this milestone starts;
  exclude if the latter, per the scope-collapse note above.)

  **`let_value`/`let_error`/`let_stopped`, landed 2026-08-21:** the first
  M5 adaptor that's genuinely more than "invoke fn, complete synchronously"
  (`then`/`upon_error`/`upon_stopped`'s shared shape) — [exec.let]'s `fn`
  returns a *new sender* that must be connected and started, so the
  overall operation's completion depends on that continuation's async
  completion, not on `fn`'s return value directly. New
  `libcxx/include/__execution/let.h` (one file for all three CPOs, mirroring
  `then.h`'s and `just.h`'s existing one-clause-one-file convention), hand-
  adapting the standard's exposition-only `let-state` (not routed through
  impls-for/basic-sender, same as every other adaptor here) with a real
  `variant`-backed operation state: `args_variant_t`/`ops_variant_t`
  ([exec.let]p12/p13) sized to every completion signature the child might
  produce for the intercepted tag, built by reusing
  `<__execution/get_completion_signatures.h>`'s existing `__gather_signatures`/
  `__decayed_tuple`/`__dedup_type_list_t` exposition machinery (already
  built for `value_types_of_t`/`error_types_of_t`) rather than
  reimplementing signature-gathering a third time.

  **Deliberate simplification (documented at length in let.h itself):**
  [exec.let]p2's three-branch `let-env` (giving the continuation sender an
  environment tied to the child's completion scheduler/domain) always takes
  the unconditionally-well-formed third branch, `env<>{}` — branch 2.2
  (domain) needs `MAKE-ENV`/a real `get_completion_domain` body, neither of
  which exist yet (M2 deviation); branch 2.1 (scheduler) is *not* similarly
  blocked — `SCHED-ENV(sched)` is just `prop<get_scheduler_t, Sched>` (M1)
  layered on `get_completion_scheduler_t` (M4) — but nothing in
  `let_value`/`let_error`/`let_stopped`'s own contract needs it, so it's
  left for whoever picks up `continues_on`/`on`/`schedule_from` later in
  this same milestone (those *do* need scheduler affinity to be
  meaningful). With `let-env` always `env<>{}`, the standard's `receiver2`
  (the receiver connecting the continuation sender) collapses exactly onto
  `<__execution/fwd_env.h>`'s existing `FWD-ENV(get_env(rcvr))` — reused
  directly as `__let_cont_rcvr`, not reimplemented.

  **Real engineering hazard, caught and fixed before the smoke test first
  compiled:** the standard's own `let-state::receiver` (the receiver
  connecting the *original* child sender) stores **both** a `let-state&`
  *and* its own direct `Rcvr&` member — not just the former. This isn't
  redundant. `connect_result_t<Sndr, receiver>` is computed *inside*
  `let-state`'s own body, while `let-state` (⇒ this fork's
  `__let_opstate`) is still incomplete; `<__execution/connect.h>`'s own
  `connect_t`/`__connect_impl` chain has an `auto`-returning (not trailing-
  decltype) link partway through it, which means checking `connect`'s
  constraints doesn't stay in an unevaluated/signature-only context the
  way a naive reading of "SFINAE probe" suggests — it actually compiles
  the receiver's `get_env()` *body* right then. A first attempt routed
  `__let_child_rcvr::get_env()` through `__state_->__rcvr_` (reaching into
  the enclosing, still-incomplete opstate) and hit exactly this: "member
  access into incomplete type" from deep inside `sync_wait`'s constraint-
  checking, confirming the failure mode empirically rather than taking the
  standard's two-member shape on faith. Fixed by giving
  `__let_child_rcvr` its own `_Rcvr&` member (constructed alongside the
  `_OpState*`), matching the standard exactly — `get_env()` then only
  touches this receiver's own already-complete member, no dependency on
  `_OpState` at all. **Lesson for later M5 adaptors that connect a child
  sender from inside their own operation state** (most of the rest of this
  milestone's list): if the standard's own exposition type stores a
  reference that looks redundant with something reachable through a
  back-pointer, don't simplify it away — it's very likely load-bearing for
  this exact incomplete-type-during-constraint-checking issue, not
  accidental duplication.

  **Runtime exception handling deliberately doesn't mirror the static
  signature computation:** `__let_opstate::__intercept` *unconditionally*
  wraps args-construction + `fn` invocation + `connect`-ing the
  continuation in `try`/`catch`, unlike `then.h`'s nothrow fast path —
  because connecting the continuation can also throw, and no sender's
  `connect()` in this tree is declared `noexcept` (checked: `just.h`,
  `then.h`, `read_env.h`, `run_loop.h` — none), so a static nothrow check
  here would almost never take the fast path anyway, and would be a real
  soundness gap for a future sender whose `connect()` genuinely throws.
  `get_completion_signatures` (via `__let_sig_transform`) still uses the
  `then`-style static nothrow check (args + fn only, *not* connect) to
  decide whether to advertise `set_error_t(exception_ptr)` — advisor-
  reviewed: sound in this tree specifically because no in-tree sender's
  `connect()` can throw when that check says nothrow, so runtime is
  strictly *more* permissive than what's statically advertised, never the
  reverse (documented inline in `let.h` as the load-bearing reason, not
  asserted as a general truth).

  **Also needed:** `emplace-from` ([exec.let]p10's own exposition helper),
  ported directly as `__emplace_from<Fn>` — operation states are neither
  movable nor copyable (`__let_opstate` explicitly deletes its copy ctor,
  matching `__just_opstate`'s existing precedent), so
  `variant::emplace<T>(...)`/`variant(in_place_type_t<T>, ...)` cannot
  construct one in place from a factory call returning `T` by value
  (forwarding-reference parameter materialization defeats guaranteed copy
  elision); a wrapper type with `operator T() &&` calling the factory
  sidesteps it, since direct-init from a prvalue of the *same* type *is*
  covered by mandatory elision.

  New tests: `exec.adapt/exec.let/{let_value,let_error,let_stopped}.pass.cpp`
  — payoff case (continuation sender's *own* async completion reaches the
  outer receiver, including a value-to-error transition for `let_value`
  exercised via a hand-rolled `maybe_errors_sndr`, matching
  `sync_wait.pass.cpp`'s own established pattern for testing completions
  `sync_wait` itself couldn't reach directly), call/pipe syntax
  equivalence, the absent-intercepted-completion no-op case, a throwing-fn
  path, and the nothrow/throwing completion-signature-transform branches.
  `execution/` suite 28/28 → 31/31 (3 new), `thread.stoptoken/` unaffected,
  `module_std.gen.py`/`transitive_includes.gen.py` 125/126 (pre-existing
  1-unsupported baseline, no diff from the new header despite pulling in
  `<variant>`/`<tuple>` — both already transitively reachable from
  `<execution>`), `libcxx-generate-files` clean. Registration: new header
  in `CMakeLists.txt` and `<execution>`'s `_LIBCPP_STD_VER >= 26` include
  block (both needed, unlike M5's `upon_error`/`upon_stopped` entry), plus
  `execution.inc`. `Cxx2cPapers.csv` untouched (flips at M6 only).

  **Next session: continue M5** — `starts_on`/`continues_on`/`on`/
  `schedule_from` next (the scheduler-affinity family this session's
  skipped `let-env` branch 2.1 was flagged for), or `when_all`/
  `into_variant`/`stopped_as_optional`/`stopped_as_error`/`write_env`/
  `unstoppable`/`bulk*` if scheduler plumbing isn't the priority. Re-read
  this session's "real engineering hazard" note above before writing any
  more adaptor that connects a child sender from inside its own operation
  state — it applies to most of what's left on the M5 list.

  **`upon_error`/`upon_stopped`, landed 2026-08-21:** [exec.then] is a
  single clause covering `then`/`upon_error`/`upon_stopped` together (same
  "intercept one completion tag, invoke `fn`, complete with the result as a
  value" mechanism, parameterized on which tag — [exec.then]p4's
  `set-cpo`), so generalized `<__execution/then.h>` in place rather than
  adding two near-duplicate files — mirrors `<__execution/just.h>`'s
  existing `_Tag`-templated shape (`just_t`/`just_error_t`/`just_stopped_t`
  sharing one `__just_sndr<_Tag, ...>`), which turned out to be the exact
  precedent to follow: `__then_sndr`/`__then_rcvr`/`__then_sig_transform`
  are now templated on `_Tag` (`then_t`/`upon_error_t`/`upon_stopped_t`)
  instead of hardcoded to `then_t`, dispatching with `if constexpr
  (same_as<_Tag, ...>)` in the receiver (matching `__just_opstate::start`'s
  style) and a derived `__then_set_cpo_t<_Tag>` alias (which completion-
  function tag `_Tag` intercepts) feeding the same `__one<_SetCpo(_Args...)>`
  partial-specialization pattern the original `then`-only version already
  used, just with `_SetCpo` promoted from a hardcoded `set_value_t` to an
  explicit template parameter — advisor flagged this explicit-parameter
  form over deriving `_SetCpo` from a member-typedef indirection inside the
  transform, since the former is structurally identical to the
  already-proven-working pattern rather than introducing a new dependent-
  alias-in-specialization-pattern shape. Also dissolved with this refactor:
  the original file's `then_t`-must-be-complete-before-`__then_sndr`
  ordering constraint (documented at length in a comment at the top of the
  pre-refactor file) no longer applies once `tag` is typed on a generic
  `_Tag` template parameter rather than the concrete `then_t` — the comment
  was deleted rather than left describing a constraint that no longer
  exists. Three CPO structs' near-identical two-arg `operator()` bodies
  factored through a shared `__then_make_sndr<_Tag>(sndr, fn)` helper; the
  one-arg pipeable-closure overload (`bind_back` + `__pipeable`) is small
  enough to duplicate three ways rather than factor.

  Only registration edit needed was `libcxx/modules/std/execution.inc`
  (four new `using` exports under the existing `// [exec.then]` block) —
  no new header file means no `CMakeLists.txt` entry, no `<execution>`
  include-block change, and (confirmed by rerunning
  `transitive_includes.gen.py`) no `transitive_includes/cxx26.csv` diff;
  much smaller registration surface than M4's new-header milestones.

  New tests: `exec.adapt/exec.then/{upon_error,upon_stopped}.pass.cpp`
  (existing `exec.then` directory, since all three CPOs are one clause) —
  each covers the payoff case (turning an absent-in-`just`/`just_stopped`
  completion into a value completion via `sync_wait`, without needing a
  hand-written sender the way M4's `sync_wait.pass.cpp` did for the error/
  stopped paths), call-syntax vs. pipe-syntax equivalence, void-returning
  `fn`, the "absent intercepted completion" no-op case (`fn` never invoked,
  signature passes through unchanged — confirms the non-matching `__one`
  partial specialization is simply never instantiated), a throwing-`fn`
  path through `sync_wait`'s exception, and both nothrow/potentially-
  throwing completion-signature-transform branches. `upon_error.pass.cpp`
  additionally covers a dedup-collision case (`just | then(throwing) |
  upon_error(...)`: child contributes `set_value_t(int) +
  set_error_t(exception_ptr)`; `upon_error` consumes the error into
  `set_value_t(int)` and re-adds `set_error_t(exception_ptr)` because its
  own `fn` can throw too) — exercises intercept + passthrough + dedup
  simultaneously, which nothing in `then.pass.cpp` alone does. Caught one
  real test bug before it was a real bug: the dedup-collision case's sender
  was stored in a local `auto sndr = ...` and passed to `sync_wait(sndr)`
  as an lvalue — `__then_sndr::connect` is `&&`-qualified (senders are
  single-use), so this fails to compile; fixed with `sync_wait(std::move(sndr))`.
  `then.pass.cpp` itself re-verified unchanged and still green (the
  refactor's regression guard). `execution/` suite 28/28 green (26
  pre-existing + 2 new), `thread.stoptoken/` still passing, no
  regressions; `module_std.gen.py`/`transitive_includes.gen.py` 125/126
  (same pre-existing 1-unsupported baseline); `libcxx-generate-files`
  produced no diff (expected — no FTM/header-list changes this session).
  `Cxx2cPapers.csv` untouched (still `|In Progress|`, flips only at M6 per
  the sub-plan's FTM discipline).

  **Next session: continue M5** — `let_value`/`let_error`/`let_stopped`
  next (the standard's next `[exec.adapt]` subclause after `[exec.then]`);
  re-read the M1 eager-`requires{}`-evaluation finding and the M4
  compiler-behavior findings before writing more completion-signature-
  transform or receiver code, since `let_*`'s "connect a sender-returning
  continuation and splice its operation state in" shape is more involved
  than `then`/`upon_error`/`upon_stopped`'s "just invoke `fn` and complete"
  shape.
- [x] **M6** — Coroutine integration: `as_awaitable`,
  `with_awaitable_senders`. `__cpp_lib_senders`'s `unimplemented` flag
  dropped and all three CSV rows flipped to `|Complete|` in the M6c-3
  follow-up commit — it's a single all-or-nothing `202406` value, held
  back until every M6c retrofit landed so conforming user code wouldn't
  detect a feature surface that wasn't fully there yet.
  - [x] M6a: `[exec.awaitable]` foundation (`GET-AWAITER`, `is-awaiter`,
    `is-awaitable`, `await-suspend-result`, `await-result-type`,
    `with-await-transform`, `env-promise`) — private, no public surface.
  - [x] M6b: `as_awaitable`, `with_awaitable_senders` — public surface,
    exported.
  - [x] M6c: retrofit `enable-sender`'s awaitable disjunct
    (`sender.h`) and `connect`'s `connect-awaitable` fallback
    (`connect.h`) — resolves M2 deviations 1 and 5. Landed as three
    separate commits: M6c-1 (`get_completion_signatures`'s awaitable
    fallback), M6c-2 (`enable-sender`'s disjunct), M6c-3
    (`connect-awaitable` itself, plus a real `__set_value_sig_t` bug
    found and fixed along the way — see Session Log).
    Only after M6c lands does the CSV flip happen.

**Threading & build-matrix notes for later milestones (recorded now so
they don't get discovered late):**
- M4's `run_loop`/`sync_wait` need real thread synchronization primitives
  → gate new tests with `test_suite_guard`/`libcxx_guard` on
  `_LIBCPP_HAS_THREADS`, matching the existing `stop_token`/`semaphore`
  pattern. Decide explicitly at M4 what a no-threads C++26 build exposes
  from `<execution>` (concepts/`just`/`then`/single-threaded composition
  presumably still work; `run_loop`/`sync_wait` presumably don't).
- `<execution>` in C++26 mode will newly pull in `<coroutine>`, `<tuple>`,
  `<variant>`, `<optional>`, `<stop_token>`, and threading headers —
  expect `transitive_includes.gen.py`/`module_std.gen.py` diffs to be
  larger than anything regenerated so far in this contract. Run and check
  both after **every** milestone, not just at the end.

**FTM discipline:** `Cxx2cPapers.csv` rows for P2300R10/P3325R5/P3396R1
were `|In Progress|` from this sub-plan's start (2026-08-20) until M6c
closed on 2026-08-22, when all three flipped to `|Complete|` and
`__cpp_lib_senders`'s `unimplemented` flag was dropped together, in one
commit. That flip regenerates files outside `execution/` (`libcxx/include/
version`, `libcxx/test/std/language.support/support.limits/
support.limits.general/{version,execution}.version.compile.pass.cpp`) —
run those tests too, not just `execution/`, when reproducing or
extending that commit.

### Tier 3 — Ranges, mdspan/linalg, format completions

Started 2026-08-22. Same shape as Tier 2: split into independent sub-blocks
rather than one 14-item sweep, so a session boundary doesn't leave the tier
in an ambiguous state.

- **Ranges block** (P2542R8, P3138R5, P3137R3, P2846R6) — **done
  2026-08-22, all four Complete.** All four already had headers in-tree from
  an earlier scaffolding pass (commits `d24292bc702a`, `5def9eee4a08`, both
  2026-08-17), but zero test coverage, and three
  (`__cpp_lib_ranges_cache_latest`, `__cpp_lib_ranges_reserve_hint`,
  `__cpp_lib_ranges_as_input`) had their feature-test macros already live in
  `<version>` with a blank CSV status — i.e. advertising conformance that
  had never been checked. Treated as a conformance pass + test-writing
  block, not bookkeeping — real bugs turned up in 3 of the 4 (see each
  paper's row below and the two commits landing this block); `views::concat`
  (P2542R8) was the one exception, already solid. Full `ranges/` suite green
  (453/454, 1 pre-existing unsupported) plus `transitive_includes.gen.py`
  and `support.limits.general/` (208/208) both times.
- **mdspan/linalg block, split 2026-08-22 per the gate above:**
  - **mdspan proper** (P2630R4 `submdspan`, P2642R6 padded layouts, P3355R1)
    — **not started, confirmed from-scratch, and blocked on an untracked
    prerequisite — see the dedicated sub-plan below before starting this.**
  - **linalg** (P1673R13, P3050R2) — **P1673R13 assessed, not a from-
    scratch project; P3050R2 done 2026-08-22.** `libcxx/include/linalg`
    is 3150 lines with 14 pre-existing test files (`algorithms.pass.cpp`,
    `triangular_solves.pass.cpp`, `rank_updates.pass.cpp`,
    `structured_algorithms.pass.cpp`, etc., 17 tests total) that were
    **already passing before this session touched anything** — this is
    the opposite of the ranges block's "scaffolded but untested" trap.
    `__cpp_lib_linalg` is also already live in `<version>` (no
    `unimplemented` flag) despite the CSV row being blank, so the
    blank status is very likely stale bookkeeping, not an accurate
    "not started". Confirming that needs a real audit against the
    paper's full wording/API surface — not done this session, don't
    assume Complete without doing it — but whoever picks this up next
    should start from "probably mostly done, verify the gaps" rather
    than "implement from scratch". P3050R2 (a small, independently-
    scoped DR-shaped fix to `linalg::conjugated`) was completed as part
    of this assessment once the fix was understood; see its row below.

### Tier 3 Sub-Plan: mdspan `submdspan`/padded layouts (P2630R4/P2642R6/P3355R1) — BLOCKED, do not start

**Blocked 2026-08-24, before any code was written — this is a scoping
finding, not an implementation attempt.** Do not pick this up as a normal
"from-scratch feature" session; read this whole section first, since the
blocker is a prerequisite chain, not a size problem.

**What was checked, in order:**
1. Confirmed (again) that `libcxx/include/__mdspan/` has only
   `layout_left.h`/`layout_right.h`/`layout_stride.h`/`extents.h`/
   `aligned_accessor.h`/`mdspan.h` — no `submdspan`, `submdspan_mapping`,
   `strided_slice`, or padded-layout types anywhere.
2. Checked whether this is just a backport gap by querying upstream LLVM
   `main` directly via the GitHub API
   (`api.github.com/repos/llvm/llvm-project/contents/libcxx/include/__mdspan`)
   rather than assuming: upstream `main` has the exact same 7 files as this
   fork. **No major libc++ tree has implemented any of this yet** — this
   isn't a fork-specific backlog item, it's a genuinely unimplemented
   corner of C++26 industry-wide as of this session.
3. Fetched P2630R4 (submdspan base proposal, 2023) and P2642R6 (padded
   layouts, 2024 — PDF-only, no HTML mirror, converted via `pdftotext
   -layout`) directly, read their wording sections in full.
4. **Before implementing against those two papers' wording, cross-checked
   against the live draft** (`eel.is/c++draft/mdspan.sub` — this fetches as
   one large page covering the entire `[mdspan.sub]` clause tree including
   the padded-layout submdspan-mapping specializations, not just the base
   paper's scope) per this tracker's established discipline of verifying
   against the current draft before trusting a paper's own text. **This is
   where the blocker was found**, not in the implementation itself.

**The blocker:** the current draft's submdspan wording has moved
substantially beyond P3355R1 (the tracked "C++26 fixes" paper) via further,
untracked committee changes:
- `strided_slice` (P2630R4's type) has been renamed `extent_slice`, and a
  new sibling type `range_slice` has been added
  (`[mdspan.sub.range.slices]`).
- `submdspan_extents` has been renamed `subextents`, and a new function
  `canonical_slices` and clause `[mdspan.sub.canonical]` ("submdspan slice
  canonicalization") have been added — this is real new machinery, not a
  rename.
- Critically, the *current* wording's "canonical submdspan index type"
  definition, `range_slice`'s default `StrideType` template argument, and
  the static-extent computation in `[mdspan.sub.map.sliceable]` all depend
  on **`std::constant_wrapper`** — confirmed via
  `grep -rn "constant_wrapper\|constexpr_v" libcxx/include/` to not exist
  anywhere in this fork. `constant_wrapper` is P2781 ("`std::constexpr_v`",
  renamed by LEWG during review) — **an entire separate library feature,
  not tracked in `Cxx2cPapers.csv`/`Cxx2cIssues.csv` at all**, discovered
  only because this session went looking for it. It is not incidental to
  the wording; it is load-bearing in exactly the three places listed
  above.

**Why "implement P2630R4's original wording instead" is not a valid
workaround** (this was considered and rejected, not just skipped): the
original papers' `strided_slice`/`submdspan_extents` and the missing
`range_slice`/`canonical_slices` machinery would all need to be renamed,
restructured, or added on top once `constant_wrapper` and the newer
wording eventually get implemented — i.e. this would be writing code
whose shape is already known to be wrong, destined for a rewrite rather
than a refinement. This is the same category of decision as declining to
invent P3471R4's placeholder FTM names, at a much larger scale (a whole
API surface instead of a handful of macro values).

**What unblocks this:** either (a) implement `std::constant_wrapper`
(P2781) first as its own tracked item — add it to `Cxx2cPapers.csv`, which
doesn't have a row for it at all yet — then revisit submdspan/padded
layouts against the current draft; or (b) a future session re-checks
whether upstream libc++ `main` has picked up `constant_wrapper` or
submdspan by then (the GitHub API check above is cheap to repeat) and
backports rather than reimplements. Do not start on `strided_slice`/
`submdspan_extents`-shaped code against the old papers as a stopgap.

P2630R4, P2642R6, and P3355R1 stay `[ ]` (not started) in the table below;
P3222R0 (Tier 6, "transposed special cases for P2642 mdspan layouts")
stays blocked transitively on this same chain.

- **format/print block, worked partially 2026-08-22:**
  - P2845R8 (`formatter<path, charT>`) and P2587R3 (`to_string`/
    `to_wstring` float overloads) — both **Complete**, see rows below.
  - P2757R3 (type-checking format args) — **assessed, not completable
    this session, same shape as Tier 1's P1383R2 finding.** `Cxx2cPapers.
    csv`'s existing P2637R3 row (inherited from upstream, not written
    this session) already states outright: "Change of `__cpp_lib_format`
    is blocked by P2419R2." Confirmed independently: P2419R2 ("Clarify
    handling of encodings in localized formatting of chrono types") is a
    *C++23* paper (`Cxx23Papers.csv`), still blank/unstarted, not tracked
    anywhere in this Tier list. `__cpp_lib_format`'s C++26 bump is a
    single cumulative value shared across P2510R3 (formatting pointers,
    already `|Complete|`, LLVM 17), P2757R3 itself, P2637R3 (member
    `visit`, already `|Complete|`), and P2918R2 (runtime format strings
    II, already `|Complete|`) — meaning even though P2757R3's own scope
    might be implementable, the shared macro can't advance past it
    without P2419R2 also landing, and P2419R2 isn't scoped or started.
    Don't attempt P2757R3 in isolation next time without first doing
    P2419R2 (itself untracked — add it to a future C++23-gap pass) or
    deciding to split `__cpp_lib_format`'s single value into something
    finer-grained (nonstandard, would need real justification).
  - P3107R5 / P3235R3 (`std::print` efficiency) — **assessed, out of
    scope for this session, deserves a dedicated implementation session
    of its own.** These are pure implementation-strategy papers with no
    new API surface — "Complete" can only mean a structural claim
    (writes through to the stream instead of materializing a `string`
    first), not an observable-output test. Checked the actual mechanism:
    `libcxx/include/__ostream/print.h`'s `__print::__vprint_nonunicode`
    does `string __str = std::vformat(__fmt, __args); ...; fwrite(__str.
    data(), ...)` — it fully materializes the formatted string before
    writing, exactly the pattern P3107R5 exists to eliminate. Fixing
    this means redesigning the internals around an output iterator that
    writes through to the `FILE*` incrementally (and P3235R3 adds
    per-type fast paths on top of that), which is a real redesign of
    `__vprint_nonunicode`/`__vprint_unicode_posix`/
    `__vprint_unicode_windows`, not a conformance-test pass. `__cpp_lib_
    print`'s C++26 bump lines for both papers (202403, 202406) are
    commented out in the generator, consistent with this. Record the
    "verify mechanism, not just output" criterion for whoever takes this
    on, so a future session doesn't write tests that pass trivially
    against the unoptimized path.

**Correction found this session, worth recording before the next person
trusts a paper's own title:** P3137R3's paper title is `views::to_input`,
but WG21 wording review renamed the adopted feature to `views::as_input`
(`__cpp_lib_ranges_as_input`, confirmed against `eel.is/c++draft/version.syn`
directly, not the paper). This fork's scaffolding (`as_input_view`,
`views::as_input`) already used the *correct* final name — do not "fix" it
back to `to_input` based on the paper title alone.

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [x] | P2542R8 | `views::concat` | Complete 2026-08-22 — scaffolded implementation was already solid (unlike the other three in this block, it already had `_LIBCPP_HIDE_FROM_ABI` throughout and no wrong `enable_borrowed_range`); added tests and flipped the FTM, no header bugs found |
| [x] | P3138R5 | `views::cache_latest` | Complete 2026-08-22 — scaffolded but untested; fixed missing `_LIBCPP_HIDE_FROM_ABI` throughout, a wrongly-added `enable_borrowed_range` specialization, and two private constructors missing `constexpr` |
| [x] | P3137R3 | `views::to_input` (adopted as `views::as_input`) | Complete 2026-08-22 — same conformance-pass fixes as P3138R5 (missing `_LIBCPP_HIDE_FROM_ABI`, wrong `enable_borrowed_range` specialization) |
| [x] | P2846R6 | `reserve_hint` | Complete 2026-08-22 — CPO/concept were already scaffolded but untested; added tests, fixed a `_LIBCPP_HIDE_FROM_ABI` gap and `ranges::to` using `sized_range`/`ranges::size` instead of `approximately_sized_range`/`ranges::reserve_hint` |
| [ ] | P2630R4 | `submdspan` | Confirmed from-scratch, no scaffolding in-tree — see mdspan/linalg block note |
| [ ] | P2642R6 | Padded `mdspan` layouts | Confirmed from-scratch |
| [ ] | P3355R1 | `submdspan` C++26 fixes | Depends on P2630R4 |
| [x] | P3050R2 | `linalg::conjugated` optimization | Complete 2026-08-22 — `conjugated()` always wrapped in `conjugated_accessor` even for arithmetic/no-`conj(E)` element types; now returns the argument unchanged for those (reuses the existing `__has_adl_conj` trait). No FTM of its own. |
| [~] | P1673R13 | BLAS-based linear algebra interface | Partial 2026-08-24 — audited: name-set diff clean, found and fixed a real SFINAE-conformance gap (~90 functions retrofitted with concept constraints) plus (via P3371R5 below) a real-if-needed gap in the hermitian rank-1/2/k/2k updates; `|Partial|` because the FTM chain to `202511L` traces through P3222R0, which is genuinely blocked on P2642R6/`constant_wrapper` — see Session Log |
| [x] | P3371R5 | Consistent rank-1/2/k/2k updates | Complete 2026-08-24 — found via tracing the `__cpp_lib_linalg` FTM chain (not previously in this CSV). 3 of its 4 required changes were already correct in this fork; fixed the 4th (`real-if-needed(alpha)` and diagonal `real-if-needed(E[i, i])` missing from the 4 hermitian rank-update E-taking overloads) — see Session Log |
| [x] | P2587R3 | `to_string` or not `to_string` | Complete 2026-08-22 — float/double/long double overloads used `sprintf("%f", ...)` (fixed 6 decimals); now `format("{}", val)` per wording, shortest round-trip. Integer overloads already matched via `to_chars`, untouched |
| [ ] | P2757R3 | Type-checking format args | Assessed 2026-08-22, blocked — shared `__cpp_lib_format` bump can't advance without C++23's P2419R2 (untracked, unstarted) also landing — see format/print block note |
| [ ] | P3107R5 | Efficient `std::print` implementation | Assessed 2026-08-22 — confirmed `__vprint_nonunicode` materializes a full `string` before writing, the exact thing this paper eliminates; real redesign, deserves its own session — see format/print block note |
| [x] | P2845R8 | `std::filesystem::path` formatting | Complete 2026-08-22 — new `formatter<path, charT>` in `__filesystem/path_format.h`, path-format-spec grammar (fill-and-align, width, `?`, `g`) |
| [ ] | P3235R3 | `std::print` faster/leaner for more types | Assessed 2026-08-22, same redesign as P3107R5 above, bundle with it |

### Tier 4 — Atomics

Started and mostly worked 2026-08-23. Ran the same assessment-gate discipline
as Tier 3 before touching code — see session log entry below for what it
found (in particular, P0493R5's fetch_max/fetch_min were a pure library gap:
the compiler side, `__c11_atomic_fetch_max/min` and its `atomicrmw fmax/
fmin` lowering, was already fully in place).

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [x] | P0493R5 | Atomic min/max | Complete 2026-08-23. `fetch_max`/`fetch_min` added to `atomic<Integral>`, `atomic<Floating-point>`, `atomic_ref<Integral>`, `atomic_ref<Floating-point>`, plus the `atomic_fetch_max(_explicit)`/`atomic_fetch_min(_explicit)` free functions. Integral routes through the pre-existing `__c11_atomic_fetch_max/min`/`__atomic_fetch_max/min` compiler builtins (single `atomicrmw`, no CAS loop); floating-point follows `fmaximum_num`/`fminimum_num` NaN semantics via the same CAS-loop fallback pattern `fetch_add`/`fetch_sub` already used for the fp80-long-double case. **Both `atomic<T*>::fetch_max`/`fetch_min` and `atomic_ref<T*>::fetch_max`/`fetch_min`** were missed originally (this row was wrongly marked `\|Complete\|` over the `atomic_ref<T*>` half of that gap until the P3323R1 session's [atomics.ref.pointer] wording read caught it, and the `atomic<T*>` half — confirmed real via [atomics.types.pointer]'s own synopsis, `T* fetch_max(T*, memory_order = seq_cst) volatile noexcept;` etc. — went unnoticed until an advisor review of the `atomic_ref<T*>`-only fix pushed back on reusing this same tracker as the authority for a `\|Complete\|` flip a second time) — **both now implemented via a manual CAS loop** (`libcxx/include/__atomic/atomic_ref.h`'s and `libcxx/include/__atomic/atomic.h`'s pointer specializations), since `clang/lib/Sema/SemaChecking.cpp`'s `IsAllowedValueType` gives `__atomic_fetch_max`/`__atomic_fetch_min`'s `ArithAllows` as `AOEVT_FP` only (no `AOEVT_Pointer`) — the compiler builtin rejects pointer operands for these two operations specifically (unlike `fetch_add`/`fetch_sub`, which do allow `AOEVT_Pointer`). Both sections' wording (confirmed against eel.is) specifies the computation "as if by max and min algorithms... with the object value and the first parameter as the arguments" — i.e. plain `operator<` via `std::max`/`std::min`'s own tie-breaking (first argument wins on equality), not a NaN-aware comparison, so each loop body is a single ternary (`__old < __arg ? __arg : __old` for max, `__arg < __old ? __arg : __old` for min) with no helper function needed, unlike the floating-point specializations' three-branch NaN-aware `__maximum_num`/`__minimum_num`. `atomic<T*>` needed its own copy of the loop (not shared with `atomic_ref<T*>`'s): `is_integral<_Tp*>` is false, so `atomic<_Tp*>` never inherits from the integral `__atomic_base<_Tp, true>` base and instead implements `fetch_add`/`fetch_sub` directly, so `fetch_max`/`fetch_min` were added right alongside them, as both volatile and constexpr-under-C++26 overloads. Tests: extended `atomics.ref/fetch_max.pass.cpp`/`fetch_min.pass.cpp` from `is_arithmetic_v<T>`-only to also cover `TestEachPointerType` (`int*`, `const int*`), replacing the old `TestDoesNotHaveFetchMax/Min<T*>` static-assert coverage; `fetch_min`'s existing acquire-release/seq-cst thread tests needed their `x.store(T(100), ...)` sentinel-before-fetch_min hack (to force the CAS loop to actually lower the value, since the two racing threads' `old_val`/`new_val` are naturally increasing) generalized to `x.store(new_val + 1, ...)` — works for both arithmetic and pointer T, since forming (not dereferencing) one-past-the-end of the shared 2-element test array is well-defined and compares greater than both `make_value<T>(0)`/`make_value<T>(1)`; extended `atomics.types.generic/pointer.compile.pass.cpp`'s synopsis/exercised-API and the four `atomics.types.operations.req/atomic_fetch_max(_explicit)`/`atomic_fetch_min(_explicit)` free-function test files with a `testp<T>()` pointer case each, mirroring `atomic_fetch_add.pass.cpp`'s own existing `testp<T>()`; extended both `atomics.ref/constexpr.pass.cpp`'s and `atomics.types.generic/constexpr.pass.cpp`'s existing `test_pointer()` functions with a fetch_max/fetch_min round trip verified against a standalone compile probe; extended `atomics.ref/cv_qualified.pass.cpp`'s `int* volatile` read/write block with the same. Full `atomics/` suite green (131 tests, 1 pre-existing unsupported — same count throughout, all additions were to existing test files). `Cxx2cPapers.csv` flipped to `\|Complete\|` |
| [x] | P2835R7 | `atomic_ref` object address exposure | Complete 2026-08-23 — `constexpr T* address() const noexcept` added to `__atomic_ref_base<T>` (inherited by every specialization). Per the paper itself the return type is `T*`, not the `COPYCV(T,void)*` shown on eel.is — that form comes from a later merge (likely P3323R1-adjacent wording polish), not this paper; don't "fix" this back if a future session compares against the live draft |
| [x] | P3323R1 | cv-qualified types in `atomic`/`atomic_ref` | Complete 2026-08-23 — see session log. Fetched the paper's own wording diff directly (`www.open-std.org/.../p3323r1.html`) rather than trusting eel.is, which resolved the prior session's "extra converting constructor" concern: that constructor is not in this paper at all (it's P3309R3-adjacent, unrelated). `atomic<T>`: added `same_as<T, remove_cv_t<T>>` to `__check_atomic_mandates`'s static_assert, and routed the floating-point partial specialization through that same check too (it previously bypassed it entirely, since `is_floating_point_v<const double>` is true — `atomic<const double>` would otherwise have silently skipped the mandate). `atomic_ref<T>`: `__atomic_ref_base<T>::value_type = remove_cv_t<T>`; `__ptr_` keeps `T`'s cv-qualification (so a `const T*`/`volatile T*` is what's actually passed to the atomic builtins and what the compiler enforces constness against), while the internal `__clear_padding`/`__compare_exchange` scratch-buffer helpers were re-typed from `T`/`T*` to `value_type`/`value_type*` — the exact internals re-threading the prior session flagged as required, now done. `store`/`operator=`/`exchange`/`compare_exchange_weak`/`compare_exchange_strong`/`notify_one`/`notify_all` gained `requires(!is_const_v<T>)`; `load`/`operator value_type()`/`wait` stay unconstrained, matching the paper's Constraints elements exactly. Added a class-scope `static_assert(is_always_lock_free \|\| !is_volatile_v<T>, ...)` in `__atomic_ref_base` (inherited by every specialization). **Found and fixed a latent bug exposed by this paper's own wording, not present before cv-support existed**: the integral specialization's bool-exclusion (`!same_as<bool, T>`) only ever excluded exactly `bool`, not `const bool`/`volatile bool` — since `same_as<bool, const bool>` is false, `atomic_ref<const bool>` was wrongly routing to the integral specialization (picking up `fetch_add` etc. that `atomic_ref<bool>` shouldn't have) instead of the primary template Note 1 requires. The paper's own diff makes this explicit ("except cv bool", not just "except bool"), confirming it's this paper's scope, not a separate fix; changed the exclusion to `!same_as<bool, remove_cv_t<T>>`.

**Pointer specialization initially left unchanged, which was wrong — caught by advisor review before commit, not by the test suite.** The first pass carried forward the prior session's note that `atomic_ref<T* const>` "falls through to the primary template" as if that settled the matter for this paper too. It doesn't: `[atomics.ref.pointer]`'s own wording diff deletes the `template<class T>` parameter from `atomic_ref<T*>`'s synopsis entirely, replacing the deduced-pattern notation with the same "for all pointer-to-object types" / placeholder-type convention already used for `[atomics.ref.int]`/`[atomics.ref.float]` (each paired with `value_type = remove_cv_t<placeholder>`, `is_const_v<placeholder>`-gated Constraints, and the same `is_always_lock_free`/`is_volatile_v` Mandates) — confirmed by re-fetching the raw diff HTML and inspecting the literal `<del>`/`<ins>` tags around that synopsis line, not just the crude text-stripped version. A real C++ `template<class T> struct atomic_ref<T*>` partial specialization can never match a top-level cv-qualified argument like `int* const` — that's a structural fact about partial-specialization matching, unrelated to what the standard's prose says — so keeping that literal pattern would have kept silently excluding cv-qualified pointers from ever getting `fetch_add`/`fetch_sub`/the increment operators, unlike `atomic_ref<const int>`, which already reaches `[atomics.ref.int]` because `std::integral<const int>` strips cv. Fixed by switching the pointer specialization to `template<class T> requires is_pointer_v<T> struct atomic_ref<T>` (T now the *whole*, possibly cv-qualified, pointer type, mirroring the integral/floating specializations' own shape exactly), with a `__pointee` alias (`remove_pointer_t<value_type>`) replacing the old scaling type for `fetch_add`/`fetch_sub`'s `sizeof(_Tp)` (previously `_Tp` was the pointee since the old pattern deduced it from `T*`; now `_Tp` is the pointer type itself, so the pointee has to be recovered explicitly). `fetch_max`/`fetch_min` for pointers were *not* added — the paper's synopsis lists them, but the existing pointer specialization never had them at all (a P0493R5 scope gap, not this paper's), so adding them now would be scope creep beyond "make cv-qualification work"; left as a separate, still-open gap. Confirmed the fix against three-line discriminating checks the advisor supplied (`is_const_v<int* volatile>` false ⇒ full read/write API present; `atomic_ref<int* const>` exposes only the read-only subset) before folding coverage into the real test file.

One real Clang-parser finding, not a libc++ design issue: combining a trailing `requires`-clause with the pre-existing `_LIBCPP_CHECK_*_MEMORY_ORDER` diagnose_if attribute macro on the same declaration only parses if `requires(...)` comes *before* the GNU-style `__attribute__((...))`, not after — reproduced minimally (`t1.cpp`/`t2.cpp` swap test) before touching the real header, since the "expected ';' at end of declaration list" error pointed at the attribute, not the requires-clause, and was otherwise easy to misattribute. Also confirmed (per the advisor's push) that a raw inline `static_assert(!requires(...) { obj.constrained_member(); })` hard-errors in this compiler instead of evaluating to `false` — calling a constrained non-template member function whose constraints fail behaves like calling a deleted function for requires-expression purposes, not a SFINAE-friendly substitution failure — while the exact same check wrapped in a named `concept` (this file's actual style throughout) evaluates cleanly; a real, reproducible compiler quirk worth remembering, not a bug in this session's code.

New tests: `atomics.ref/cv_qualified.pass.cpp` (value_type identity across all four cv-combinations; `const T` exposes only load/conversion/wait via `requires`-expression concepts checking non-participation, not just call failure; `volatile T` exercises the full read/write API end-to-end including `wait()`, restricted to `T(0)`/`T(1)` throughout so the same test works for `bool`, which only has two distinct values; pointer cv-of-*pointee* is a distinct, already-working case; pointer cv-of-the-pointer-itself is the case this paper actually adds, checked separately with its own read/write/RMW sequence) and `atomics.types.generic/cv_qualified.verify.cpp` (`atomic<const T>` static_assert fires for both the primary-template and floating-point-specialization paths; deliberately excludes `atomic<volatile T>` from the verify test, since forming the primary template's own pre-existing volatile-qualified member overloads for a volatile `T` trips C++20's unrelated `[depr.volatile.type]` deprecation warning under `-verify`'s exact-match mode). No FTM — the paper defines none. Verified: full `atomics/` suite (131 tests incl. both new files) green, including under `--param hardening_mode=extensive` for `cv_qualified.pass.cpp` specifically (the P3309R3 session's finding that the default `none` hardening mode never compiles the constructor's guarded `reinterpret_cast<uintptr_t>` alignment check applies here too, now exercised through a `const`/`volatile` referenced-object constructor path that didn't exist before this session); `utilities/memory/` + `thread/` + `support.limits.general/` (641 of 650, 9 unsupported) green; `transitive_includes.gen.py`/`module_std.gen.py` (125 of 126, 1 unsupported) green; `libcxx-generate-files` + `git diff` clean, no drift |
| [x] | P3309R3 | `constexpr atomic`/`atomic_ref` | Complete 2026-08-23 for the paper's core surface, with two deliberate, documented scope cuts — see session log. `atomic<T>`'s storage is `_Atomic(T)`; none of `__c11_atomic_*`/`__atomic_*` are constexpr-evaluable in this compiler (confirmed by direct probe, not inferred from the "does not need a clang change" note in this row's prior assessment — that was the paper's own claim, not an audit), so the consteval branches (support/c11.h) read/write `__a_value` directly. That direct read only type-checks when T is scalar (`_Atomic(ClassType)` has no implicit conversion back to `ClassType`), so **`atomic<T>` constexpr is scoped to scalar T** (int, bool, pointer, floating-point incl. fp80 `long double` on this target, which exercises the separate CAS-loop path in `__rmw_op`) — arbitrary trivially-copyable class T stays non-constexpr, a real compiler gap, not scope-timidity. `atomic_ref<T>` has a different storage model (plain `T*`, no `_Atomic` qualification) so its `store`/`load`/`exchange`/`operator=` consteval branches (`*__ptr_` direct access) work for **any** trivially copyable T, including class types; only `compare_exchange_weak/strong` and `wait` stay scalar-gated, since they compare via `==` (unlike the bytewise `__atomic_compare_exchange`/`__clear_padding`/memcmp machinery used at runtime, which needs no such operator and isn't constexpr-usable itself). **`atomic_flag` is explicitly NOT covered** — its `wait()` calls `std::__atomic_wait` before `__atomic_waitable_traits<atomic_flag>`'s specialization is declared later in the same header (atomic_flag.h), relying on that call's *body* not being instantiated until end-of-TU; making the shared `__atomic_wait`/`__atomic_notify_one`/`__atomic_notify_all` templates (atomic_sync.h) `constexpr` breaks that — Clang instantiates constexpr function templates eagerly — producing "explicit specialization after instantiation" errors. Fixed by keeping those shared atomic_sync.h templates non-constexpr and inlining the consteval wait/notify logic locally into `atomic<T>`/`atomic_ref<T>`'s own `wait()`/`notify_one()`/`notify_all()` (calling `this->load()` directly) instead of routing through them — `atomic_flag`'s own wait/notify were left untouched rather than risk restructuring its declaration order. The `gcc.h` backend (dead code in this fork — clang always selects `c11.h`) was not touched, so a hypothetical GCC build would advertise `__cpp_lib_constexpr_atomic` without honoring it; recorded rather than fixed, same shape as the P0493R5 row's compiler-layer scoping. FTM: `__cpp_lib_constexpr_atomic` (202411L, *not* `__cpp_lib_atomic_constexpr` — that name came from the paper's own R3 text, "location TBD by LEWG"; verified against eel.is's actual `<version>` synopsis before adding) |
| [x] | P2869R4 | Remove deprecated `shared_ptr` atomic access APIs | Complete 2026-08-23 — the tracker's own "low complexity" label undersold this: these functions had never been given `_LIBCPP_DEPRECATED_IN_CXX20` in this fork despite the standard deprecating them since C++20 ([depr.util.smartptr.shared.atomic]), so the work was adding that plus the `_LIBCPP_ENABLE_CXX26_REMOVED_SHARED_PTR_ATOMICS` escape-hatch gate (same pattern as allocator/string/codecvt/strstream) plus updating 11 existing test files with the escape-hatch flag, not a bare deletion. `__sp_mut`/`__get_sp_mut` stay unconditional — implementation plumbing, not part of the removed public surface. Verified empty via `grep -rn "std::atomic_load\|atomic_store\|atomic_exchange\|atomic_compare_exchange" libcxx/src libcxx/test` (excluding the shared_ptr test dir itself) that no other in-tree code calls the now-deprecated overloads under `-Werror` |

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

**`text_encoding` block (P1885R12 + P2862R1), done 2026-08-23:** same
"scaffolded but untested" shape as Tier 3's ranges block. `libcxx/include/
text_encoding` was already an 831-line, fully-worked implementation
(IANA charset table, `comp-name` alias-matching algorithm, `aliases_view`,
`environment()`/`environment_is()`, `hash<text_encoding>`) with the FTM
already live and correct in `<version>` (`202306L`) — but the generator's
own `unimplemented: True` flag was still set (same trap as before: the
flag only proves nobody flipped it, not that the code doesn't exist), and
there was **zero test coverage** beyond the version-macro compile check.
Audited the implementation against the raw eel.is `[text.encoding]`
wording (`curl -A "Mozilla/5.0"` + tag-strip, not WebFetch, per the Tier 2
process rule) member-by-member — postconditions of both constructors, the
`[text.encoding.general]p6` round-trip invariant, the `comp-name` worked
examples, `aliases_view`'s range/duplicate-freedom guarantees, and the
`operator==` "both `other`" special case — found no bugs, everything
matches. **P2862R1 needed no code change at all**: `name()` returns a
pointer into a fixed `char __name_[max_name_length + 1]` member array,
which is structurally non-null in every code path (default-constructed,
`id::unknown`, `id::other`) — the paper's own fix was already the
implementation's only possible shape. New tests under `libcxx/test/std/
text/text.encoding/text.encoding.class/{text.encoding.members/{basic,
environment}.pass.cpp, text.encoding.cmp/equal.pass.cpp,
text.encoding.aliases/aliases.pass.cpp, text.encoding.hash/hash.pass.cpp}`
(5 files, mirroring the draft's own subclause breakdown). One snag worth
recording: `std::strlen`/`std::strcmp` are **not constexpr** in this
libc++ (they resolve to the C library's non-constexpr symbols, not a
constexpr-foldable builtin), so using them inside a function under
`static_assert` fails with "non-constexpr function cannot be used in a
constant expression" — since `text_encoding` itself is fully `constexpr`,
tests exercising it at compile time need `std::string_view`'s
`empty()`/`operator==` instead (both are constexpr wrappers over the same
operation). `environment.pass.cpp` is `UNSUPPORTED: no-localization`
(runtime-only, calls the real `environment()`). Flipped generator's
`unimplemented` flag off and ran `libcxx-generate-files` — 5-file diff
(`<version>`, `FeatureTestMacroTable.rst`, both affected
`*.version.compile.pass.cpp` tests, the generator script itself), nothing
unrelated. Full `text/` suite green (7/7 including the pre-existing
version test); `Cxx2cPapers.csv` P1885R12 and P2862R1 rows flipped to
`|Complete|`.

**Advisor review before the flip caught three real gaps in the first test
pass, all fixed in a follow-up commit:** (1) the hash test's only
equal-encoding pair (`ASCII`/`US-ASCII`) both had `mib() != other`, so it
couldn't exercise `[unord.hash]`'s equal-implies-equal-hash requirement
for the `operator==` "both `other`, compare by `comp-name`" branch
([text.encoding.cmp]p1) — a pair like `TE("UTF-8-Bogus")`/
`TE("u.t.f-8-bogus")` (both resolve to `other`, comp-name-equal, distinct
`__name_` bytes) is the one case that could actually catch a hash
implementation that only hashes `mib_`. Added it. (2) `literal()`'s test
only checked it compiled (`(void)lit`), which can't distinguish the real
`__clang_literal_encoding__`-driven branch from the `#else` fallback
(default-constructed, `mib() == unknown`) — confirmed via `-dM -E` that
this compiler defines `__clang_literal_encoding__` as `"UTF-8"`, so
strengthened to `static_assert(TE::literal().mib() == ID::UTF8)`. (3)
checked `__cpp_lib_text_encoding`/the class both survive compiling
*without* `-D_LIBCPP_ENABLE_EXPERIMENTAL` (lit always passes that flag, so
the suite alone can't prove the header isn't silently gated behind it) —
confirmed clean via a standalone compile+run, consistent with the header
having no `_LIBCPP_ENABLE_EXPERIMENTAL` guard of its own. **Did not**
apply the fourth suggestion (an `XFAIL: availability-<feature>-missing`
matching the `stopsource` precedent for `_LIBCPP_AVAILABILITY_SYNC`) —
checked `_LIBCPP_AVAILABILITY_TEXT_ENCODING_ENVIRONMENT` in
`__configuration/availability.h` first rather than pattern-matching
blindly, and unlike `_LIBCPP_AVAILABILITY_SYNC`
(`_LIBCPP_INTRODUCED_IN_LLVM_11_ATTRIBUTE`), this macro is defined as
**unconditionally empty** — no per-platform value exists anywhere in the
tree, so there is no real availability attribute on `environment()`/
`environment_is()` to gate a test against yet, on any target. No matching
`availability-*-missing` Lit feature exists in `features.py` either.
Recorded here so a future session wiring up real Apple-platform
availability for this facility knows to add both the attribute and the
test guard together, not just one. Re-verified full `text/` suite (5/5)
under both default and `--param hardening_mode=extensive`, plus
`transitive_includes.gen.py`/`module_std.gen.py` (125/126, unchanged
baseline) after these fixes.

**P2592R3, done 2026-08-23 — genuinely from-scratch (confirmed via `grep -rn
"struct hash" libcxx/include/__chrono/` returning nothing before this
session), unlike the two conformance-pass items above.** Fetched
`[time.syn]`/`[time.hash]` from eel.is (raw `curl` + tag-strip, per the
established process rule) to get the exact list of 18 specializations:
`hash<duration<Rep,Period>>` (enabled iff `hash<Rep>` enabled),
`hash<time_point<Clock,Duration>>` (enabled iff `hash<Duration>` enabled),
14 unconditionally-enabled calendar types (`day`, `month`, `year`,
`weekday`, `weekday_indexed`, `weekday_last`, `month_day`,
`month_day_last`, `month_weekday`, `month_weekday_last`, `year_month`,
`year_month_day`, `year_month_day_last`, `year_month_weekday`,
`year_month_weekday_last`), plus `hash<zoned_time<Duration,TimeZonePtr>>`
(enabled iff both `hash<Duration>` and `hash<TimeZonePtr>` enabled) and
`hash<leap_second>` (unconditional) which live behind the TZDB
experimental gate. New `libcxx/include/__chrono/hash.h` holds the 16
non-TZDB specializations (included unconditionally right after
`__chrono/exception.h` in the top-level `<chrono>` header — safe because
`day.h`/`month.h`/etc. self-guard their content to `_LIBCPP_STD_VER>=20`
already, so including them from `hash.h` regardless of caller context is a
no-op outside C++20); the two TZDB-gated ones were added directly inside
`__chrono/leap_second.h` and `__chrono/zoned_time.h` (matching those
files' own `_LIBCPP_HAS_EXPERIMENTAL_TZDB` gate) rather than fighting a
conditional-include shape for a single specialization each. All 16 use
the existing `std::__hash_combine(size_t, size_t)` helper (already used by
`variant`'s hash, at `__functional/hash.h:332`) to combine per-field
hashes computed via each type's public accessors (`.month()`, `.day()`,
`.weekday_indexed()`, etc.) — no friend access or private-member reads
needed. `duration`/`time_point`/`zoned_time` reuse the `__enable_hash_helper<Type,
Keys...>` conditional-SFINAE pattern `optional`'s hash already established
(`hash<__enable_hash_helper<optional<_Tp>, remove_const_t<_Tp>>>` at
`optional:1527`) rather than inventing a new one. Verified [time.hash]p3's
Note 1 ("meet Cpp17Hash even when `k.ok()` is false") holds by
construction, since every specialization hashes raw field values with no
validity branch. Flipped the generator's `__cpp_lib_chrono` C++26 value
(`202306`, previously commented out pending exactly this paper) and ran
`libcxx-generate-files` — 9-file diff, all directly related (`<version>`,
`FeatureTestMacroTable.rst`, `CMakeLists.txt` registering the new header,
both affected `*.version.compile.pass.cpp` tests, the generator script,
the 3 header edits). New tests: `libcxx/test/std/time/time.syn/
hash.pass.cpp` (all 16 non-TZDB specializations, using the existing
`test_hash_enabled<Key>()`/`poisoned_hash_helper.h` idiom already
established by `optional`'s own hash tests rather than inventing bespoke
Cpp17Hash-requirement assertions — some calendar types like
`month_weekday`/`year_month_day_last` aren't default-constructible, so
those calls pass an explicit key), plus TZDB-gated
`time.zone.leap/hash.pass.cpp` and `time.zone.zonedtime.
time.zone.zonedtime.nonmembers/hash.pass.cpp` (both `XFAIL: libcpp-has-no-
experimental-tzdb` / `XFAIL: availability-tzdb-missing`, matching the
existing `leap_seconds.pass.cpp` convention; `leap_second` test objects
built via the existing `test_chrono_leap_second.h` support header's
`test_leap_second_create`, since the standard doesn't specify a public
constructor). Confirmed (standalone compile+run, not just lit's
always-on `-D_LIBCPP_ENABLE_EXPERIMENTAL`) that the 16 non-TZDB
specializations compile and run without the experimental flag — matches
the discipline established in the text_encoding follow-up commit. Full
`time/` suite green (611 tests, matching pre-existing unsupported count);
`transitive_includes.gen.py`/`module_std.gen.py` (125/126, unchanged
baseline); the three new test files also verified under `--param
hardening_mode=extensive`. `Cxx2cPapers.csv` P2592R3 row flipped to
`|Complete|`.

**Advisor review before the flip caught two real bugs and one test gap,
all fixed before committing:** (1) `__chrono/zoned_time.h` used
`hash<_Duration>` in its `hash<zoned_time<...>>` specialization's
constraint and body without including `__chrono/hash.h` (only
`<__functional/hash.h>`, for the primary template) — it happened to work
today only because the top-level `<chrono>` header includes `hash.h`
before the tzdb block, so `__has_enabled_hash<_Duration>` was already
true by the time `zoned_time.h`'s own specialization was parsed. If
`zoned_time.h` were ever reached first (e.g. a different include order,
or the header used standalone), the constraint would have silently
evaluated false and the specialization would vanish with no diagnostic —
exactly the "SFINAE eats a real bug" failure mode this sub-plan's own M1
findings warned about. Added the missing include. (2) Every test written
was a positive case; nothing verified the "enabled iff" half of
[time.hash]p1/p2 actually gates on `hash<Rep>`/`hash<Duration>` rather
than being unconditionally enabled. Added `testDisabled()` to
`hash.pass.cpp` using the existing `test_hash_disabled<Key>()` idiom
(from the same `poisoned_hash_helper.h` used above) against
`duration<Class>`/`time_point<system_clock, duration<Class>>`, where
`Class` is that header's existing empty-struct-with-no-hash-
specialization — confirmed the `__enable_hash_helper` SFINAE is actually
load-bearing, not decorative. (3) `leap_second`'s test only used
same-date/same-value pairs; since `operator==` compares solely `date()`
(not `value()`, which the class also stores), nothing would have caught
a hash that folded in `value()`. Added a same-date/different-value pair
that must be both `==` and hash-equal — the same "does the equal-hash-
equal pair actually exercise the discriminating branch" gap the
text_encoding follow-up commit caught for `operator==`'s "both `other`"
case. Also verified `__cpp_lib_chrono`'s `202306` value directly against
`eel.is/c++draft/version.syn` (not just the generator's own comment)
before trusting it, per the P3309R3-session rule established after that
paper's `__cpp_lib_constexpr_atomic` name mixup — confirmed single value,
no missing intermediate. Re-verified full `time/` suite (444/444) and
the three new/changed test files under `--param
hardening_mode=extensive` after all three fixes.

**P3369R0 + P3508R0 (constexpr specialized `<memory>` algorithms), done
2026-08-23 — bigger than a Tier 6 "small item," treated as its own block.**
P3369R0's own narrow scope (`uninitialized_default_construct`/`_n`) turned
out to sit inside P3508R0's much larger DR wording (all of
`[specialized.algorithms]`: `uninitialized_copy(_n)`,
`uninitialized_move(_n)`, `uninitialized_fill(_n)`,
`uninitialized_default_construct(_n)`, `uninitialized_value_construct(_n)`,
and every one of their `ranges::` CPO equivalents in
`__memory/ranges_uninitialized_algorithms.h`) — confirmed by grepping the
whole family in `libcxx/include/__memory/{uninitialized_algorithms.h,
ranges_uninitialized_algorithms.h}` and finding **zero** `constexpr`
anywhere in either file, not just on the one function P3369R0 names.
Implemented the full P3508R0 scope in one pass rather than leaving
P3369R0 "|Complete|, P3508R0 still open" — same shape, same fix.

**First checked whether this was actually compiler-blocked, since Tier 1/3
already found two "single all-or-nothing FTM sitting on a real compiler
gap" cases (P1383R2, P2757R3) with this exact same shape.** A naive probe —
`::new (voidify(buf)) T()` reusing a stack `unsigned char[]` array's
storage inside a `constexpr` function — hard-errors ("placement new would
change type of storage"), which looked like the same P2747R2
(`__cpp_lib_constexpr_new`) compiler gap at first. **It isn't**: that
failure is specific to reusing a *declared object's* storage for a
different type, an unrelated core-language restriction. Retested against
storage obtained via `std::allocator<T>::allocate` (the actual, intended
use pattern — matching how `vector`/`basic_string` already do C++20
constexpr construction) and both `std::construct_at`/`destroy_at` *and*
raw `::new (voidify(*it)) T` compile and evaluate correctly in a
`static_assert`. Confirmed with a standalone probe before touching any
header. This means the gap was a real, implementable library omission,
not a compiler limitation — worth recording precisely so a future
P2747R2-adjacent investigation doesn't rediscover the same false trail.

**Mechanical fix**: added `_LIBCPP_CONSTEXPR_SINCE_CXX26` to all 20
functions in `uninitialized_algorithms.h` (10 public + 10
`__`-prefixed implementation helpers) and all 15 `operator()` overloads
across the 10 CPO structs in `ranges_uninitialized_algorithms.h` (via a
scripted regex pass, then `clang-format -i` to fix line wrapping — hand
-verified the diff was exactly the intended 35 insertions, nothing
reflowed unexpectedly). No other logic changes; `std::destroy`/`destroy_n`
underneath were already `_LIBCPP_CONSTEXPR_SINCE_CXX20` from a prior
session, so nothing needed touching there.

**Test strategy note, since the existing runtime tests in this directory
use a `char[]` stack buffer via `alignas(T) char pool[...]`** (the classic
pre-C++20 "manual placement new" idiom) **— exactly the storage pattern
confirmed above as NOT constexpr-usable.** Rather than rewrite ~18
existing runtime test files to go through an allocator (large, unrelated
diff, real risk of breaking the exception-safety/counted-construction
tests they already cover well), added 6 new,
narrowly-scoped `constexpr_uninitialized_*.pass.cpp` files (one per
sibling directory: `uninitialized.construct.default`,
`uninitialized.construct.value`, `uninitialized.fill`,
`uninitialized.fill.n`, `uninitialized.copy`, `uninitialized.move`),
matching this directory's own existing `constexpr_addressof.pass.cpp`
naming precedent. Each file exercises both the classic and `ranges::`
forms (iterator-pair and range overloads, `_n` where it exists) through a
shared `with_allocated<T>(n, fn)` helper that allocates via
`std::allocator<T>`, runs `fn`, then `destroy`s and deallocates — the only
storage-acquisition shape confirmed to actually constexpr-evaluate.
Verified all 6 both as standalone `static_assert`-driven compiles (against
freshly-rebuilt `cxx-test-depends` staged headers — bare compiles against
a stale staged install would have silently tested pre-edit headers, per
the wrapper's own warning in root `CLAUDE.md`) and through the real
`libcxx-lit` harness. Full `specialized.algorithms/` suite green (38/38,
32 pre-existing + 6 new); broader `utilities/memory/` +
`containers/sequences/vector/` + `strings/basic.string/` +
`containers/sequences/deque/` sweep green (609/609, unchanged unsupported
baseline) to catch any container internals relying on the touched
algorithms; all 6 new files also verified under `--param
hardening_mode=extensive`.

**FTM value verification, flagged by advisor review before commit — worth
recording the resolution, not just the value.** `__cpp_lib_constexpr_memory`
had no C++26 entry in the generator at all (only `c++20`/`c++23`); fetched
`eel.is/c++draft/version.syn` directly and found `202506L`. The advisor
correctly flagged that `202506` (Sofia, June 2025) doesn't match either
paper's own CSV "Meeting" column (`2024-11`, Wrocław) — the exact shape of
this fork's own P2757R3/P1383R2 "don't trust an FTM value without
checking what it actually covers" rule, applied from the opposite
direction (over-claiming instead of under-claiming). Investigated rather
than assumed: grepped the CSV for any other `2025-06`-dated paper that
could plausibly own this bump (only hit was P2988R11, `optional<T&>`,
unrelated to `<memory>` constexpr-ness) and fetched P3508R0's own paper
text directly (no FTM value stated in the paper itself — LWG assigns FTM
values during wording adoption, not paper authors). **Resolved by
precedent already in this exact CSV**: `P2835R7`'s own row (also
`2024-11 Wrocław`) already carries `__cpp_lib_atomic_ref`'s C++26 value of
`202603` (March 2026) — a paper's presentation meeting and its wording's
actual adoption-into-draft meeting are tracked separately by WG21 and
routinely differ by multiple cycles; this fork already accepted that gap
for P2835R7 in an earlier session. No second candidate paper exists for
`202506`, and the live draft's `[specialized.algorithms]` wording (already
directly fetched during this session, see above) is exactly this
implementation's scope. Kept `202506`. Re-ran the two directly-affected
version tests (`memory.version.compile.pass.cpp`,
`version.version.compile.pass.cpp`) after resolving this, since they
weren't covered by the `utilities/memory/` sweep above (different
top-level test path). `Cxx2cPapers.csv` P3369R0 and P3508R0 rows flipped
to `|Complete|`.

**P3349R1 (contiguous iterator → pointer conversion), done 2026-08-23 —
`|Nothing To Do|`, confirmed empirically rather than assumed.** Fetched the
paper directly (`curl -A "Mozilla/5.0"` + tag-strip, per the established
process rule): this is a *core*-wording addition to
[iterator.requirements.general], not a library clause, and grants pure
permission — implementations *may* replace `[i, s)` with
`[to_address(i), to_address(i + ranges::distance(i, s)))` for a
`contiguous_iterator` `i`, they aren't required to. No FTM (confirmed via
`grep -n "contiguous\|to_address" generate_feature_test_macro_components.py`
— the one hit, `__cpp_lib_to_address`, is the unrelated pre-existing C++20
macro for `std::to_address` itself) and `libcxx-generate-files` produced no
diff, as expected. The R1 revision (over R0) adds one real constraint: the
library must perform the advance-by-`n` on the *original* iterator (`i +
ranges::distance(i, s)`, or `i + n` for a counted range) rather than only
computing an offset from `to_address(i)`, so that a "checked" iterator that
throws/asserts/calls a violation handler on an out-of-bounds advance still
gets a chance to do so before the algorithm drops to raw-pointer operation.

Traced this constraint through libc++'s existing lowering path rather than
trusting the "it probably already does this" assumption from two sessions
ago. `__unwrap_range_impl::__unwrap` (`libcxx/include/__algorithm/
unwrap_range.h:37-42`) computes `ranges::next(__first, __sent)` — advancing
the *original* iterator type — before calling `__unwrap_iter` (which then
calls `to_address`) on both endpoints; `copy_n`'s random-access overload
(`libcxx/include/__algorithm/copy_n.h:55`) computes `__first +
difference_type(__n)` on the original iterator before ever calling
`std::copy`. Wrote a standalone ~90-line probe (not committed — no FTM, no
behavior change, nothing for a lit test to assert per advisor review, so no
test file added) defining a `contiguous_iterator` whose `operator++`/
`operator+=` increments a static counter, then ran it through `std::copy`
and `std::copy_n` on an 8-element range: `std::copy` recorded exactly 1
advance call (not 8), `std::copy_n` recorded 2 (one from `copy_n`'s own `+
n`, one from `copy`'s internal `unwrap_range`) — confirming both that the
pointer-lowering optimization is genuinely active (not just present in the
header but dead code) and that every advance happens through the original
iterator's own arithmetic operators, exactly the shape R1 requires for a
checked iterator to get its chance to fire. Result: `|Nothing To Do|`, not
`|Complete|` — advisor caught this distinction before the edit; the paper
changes what's *permitted*, and libc++ was already exercising that
permission before this paper existed to grant it.

**Recorded, not fixed, since it's orthogonal to this paper's disposition**:
`unwrap_iter.h:43`'s existing `// TODO(hardening): make sure that the
following unwrapping doesn't unexpectedly turn hardened iterators into raw
pointers` flags a real, still-open concern in exactly this area — whether
`_LIBCPP_HARDENING_MODE` bounds-checked iterators lose their checks when
unwrapped to a raw pointer mid-algorithm. That TODO is about whether
libc++'s *own* hardened iterators are among the "checked iterators" R1
protects, which is a separate question from whether the permission-granting
wording itself needs any library change (it doesn't). Left as-is; a future
hardening-mode session should read `unwrap_iter.h:43` before touching this
area again. `Cxx2cPapers.csv` P3349R1 row flipped to `|Nothing To Do|`.

**Next session**: the `<__random/log2.h>` `__uint128_t` "bug" carried over
from the `generate_canonical` session **was investigated and retracted**
(see the correction dated 2026-08-23 in the `generate_canonical`/Tier 6
narrative below — it doesn't reproduce; no fix needed) — no longer a
candidate. Pick fresh from the Tier 6 table above.

**P2836R1, P2075R6, P3378R2 — three assessed, done 2026-08-23 (seventh
session), none implemented.** Picked P2836R1 as the fresh Tier 6 candidate
after the log2.h retraction; before implementing, checked whether the
tree had anything to convert. It doesn't: `grep -rl "basic_const_iterator"
libcxx/include/` returns nothing. P2836R1 only adds two converting
operators to an *existing* `basic_const_iterator<Iterator>` — but that
type, `const_iterator<I>`, `as_const_view`, and the `ranges::cbegin`/`cend`
fix are all P2278R4 (C++23, tracked in `Cxx23Papers.csv`, not this
document's C++26 tier list at all), and P2278R4 itself is unimplemented:
`generate_feature_test_macro_components.py`'s `__cpp_lib_ranges_as_const`
entry carries `"unimplemented": True`, with P2836R1's own `202311` DR bump
sitting commented out directly beneath the `202207` (P2278R4) value,
confirming the dependency in the generator's own data. Tier 6's "small,
independent items" framing doesn't hold for this row — it's gated on a
from-scratch C++23 ranges feature this document doesn't otherwise track.
Not implementing either paper this session; flagged both so a future
session scopes P2278R4 deliberately (as its own block, sized like the
`mdspan`/`submdspan` or BLAS items) rather than someone reaching for
P2836R1 expecting a two-operator patch.

Checked P2075R6 (Philox engine) next, since it was also flagged `[ ]` and
untouched: same shape, `__cpp_lib_philox_engine` is `"unimplemented":
True` in the generator, and `grep -rln "philox" -i libcxx/include/__random/`
returns nothing — a full counter-based RNG engine implemented from the
paper's wording, not a conformance pass over existing scaffolding. Also
not small; also not attempted.

Checked P3378R2 (`constexpr` exception types) third. Its own motivating
example (catching a `constexpr`-thrown `std::out_of_range` during constant
evaluation) depends on P3068 (throw-expressions usable in constant
expressions) — probed directly with a minimal `throw 42;` inside a
`constexpr` function, independent of any libc++ header: hard error
(`subexpression not valid in a constant expression`), confirming this
Clang has no P3068 support at all (no relevant flag found via `--help` or
in `clang/include/clang/Basic/LangOptions.def`). **But that alone doesn't
make this row `[!]`-compiler-blocked in the P1383R2 sense** — advisor
caught that the paper's actual normative content (adding `constexpr` to
`<stdexcept>`'s constructors/`what()`/destructors) doesn't itself require
`throw`-in-`constexpr` to work; only the paper's *motivating scenario*
does. Ran the narrower discriminator directly: `constexpr
std::out_of_range e("msg"); static_assert(std::string_view(e.what()) ==
"msg");` (no `throw` anywhere) — still a hard error, but a different one:
`non-constexpr constructor 'out_of_range' cannot be used in a constant
expression`, pointing at `stdexcept:165`'s plain (non-`constexpr`)
constructor. That's a **library** gap, not a compiler one. The real blocker
is the restructuring the paper's own "libc++" section describes: `logic_
error`/`runtime_error`'s reference-counted-string storage (atomic refcount,
`reinterpret_cast`-accessed `_Rep_base`, allocated in a `.cpp`) isn't
`constexpr`-compatible and would need to fall back to a plain copy during
constant evaluation; the `what()`/dtor implementations currently live in
`.cpp` files and moving them to headers drops the symbols libc++.so/
libc++abi.so currently export, needing explicit compatibility symbols; and
`<stdexcept>`'s `std::string`-taking constructors create a `<stdexcept>`/
`<string>` circular-include problem the paper says must be resolved by
reordering. Three real, independent, ABI-sensitive changes — session-sized
on its own, matching the `P3107R5`/`P3235R3` "needs its own session" shape
from the format/print block, not a Tier 6 pickup. Not implemented. No CSV
changes for any of the three — none of them flip status this session.

`[x]` complete, `[ ]` not started, `[!]` assessed and rejected as too large
for a Tier 6 pickup (see Notes), `[~]` partially implemented and left that
way deliberately (see Notes for what's done vs. remaining).

| Status | Paper | Feature | Notes |
|---|---|---|---|
| [x] | P2592R3 | Hashing support for `std::chrono` value classes |
| [x] | P1885R12 | `text_encoding` naming |
| [x] | P2862R1 | `text_encoding::name()` should never return null |
| [x] | P2641R4 | Checking if a `union` alternative is active |
| [x] | P0952R2 | New spec for `std::generate_canonical` |
| [!] | P2836R1 | `basic_const_iterator` convertibility | **Not small** — depends on unimplemented P2278R4 (see block below) |
| [x] | P2264R7 | User-friendly `assert()` for C and C++ | Nothing To Do 2026-08-24 — see Session Log |
| [x] | P2248R8 | List-initialization for algorithms | Complete 2026-08-24 — `ranges::fold_right` untouched (P2322R6 gap, not implementable here); see Session Log |
| [x] | P3217R0 | `find_last` addendum to P2248R8 | Complete 2026-08-24 — see Session Log |
| [x] | P2546R5 | Debugging Support (`<debugging>`, not its own CSV-tracked row here) |
| [x] | P2810R4 | `is_debugger_present`, `is_replaceable` |
| [x] | P1068R11 | Vector API for RNG | Complete 2026-08-24 — see Session Log |
| [!] | P2075R6 | Philox RNG engine | **Not small** — from-scratch engine, `unimplemented: True`, no scaffold (see block below) |
| [ ] | P3222R0 | Transposed special cases for P2642 mdspan layouts |
| [x] | P3508R0 | Wording for constexpr specialized memory algorithms |
| [x] | P3369R0 | `constexpr` for `uninitialized_default_construct` |
| [x] | P3370R1 | New library headers from C23 | Complete 2026-08-23 — see Session Log |
| [x] | P3349R1 | Converting contiguous iterators to pointers |
| [!] | P3378R2 | `constexpr` exception types | **Session-sized** — library-side ABI restructure, not compiler-blocked (see block below) |
| [~] | P3471R4 | Standard Library Hardening | Partial 2026-08-24 — audit found nearly everything already implemented (upstream-inherited); fixed 2 real gaps (`inplace_vector`, `mdspan::operator[]` array/span overloads), documented 1 deliberate non-fix (`forward_list`); `|Partial|` because §10.11's FTMs are unimplementable placeholders in the paper itself — see Session Log |

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
| [ ] | P3475R2 | Defang and deprecate `memory_order::consume` | Coordinate with Tier 4 atomics work — the 2026-08-23 Tier 4 session did not touch `memory_order.h`, so this is still fully deferred, not partially covered |
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
- **2026-08-20 (fifth entry)**: Implemented P1901R2 (`weak_ptr` as unordered
  associative container key). Fetched the paper's wording via WebFetch
  (`wg21.link/P1901R2` redirects to the open-std HTML). Contrary to what the
  paper title suggests, it does **not** add a `std::hash<weak_ptr<T>>`
  specialization — `weak_ptr` still isn't directly usable as a bare
  `unordered_map`/`unordered_set` key. Instead it adds owner-based-identity
  member functions (`owner_hash()`, `owner_equal()`) to both `shared_ptr`
  and `weak_ptr`, plus two free transparent function-object structs
  (`owner_hash`, `owner_equal`) meant to be passed explicitly as the
  container's `Hash`/`KeyEqual` template arguments — e.g.
  `unordered_set<weak_ptr<T>, owner_hash, owner_equal>` — enabling
  heterogeneous `shared_ptr`/`weak_ptr` lookup the same way `owner_less`
  already enables it for ordered containers. Implementation reused the
  existing `__cntrl_` (`__shared_weak_count*`) identity field that
  `owner_before`/`__owner_equivalent` already compare on
  (`libcxx/include/__memory/shared_ptr.h`): `owner_hash()` is
  `hash<__shared_weak_count*>()(__cntrl_)`, `owner_equal()` is the same
  `__cntrl_ ==` comparison `__owner_equivalent` already did (just exposed
  publicly, templated on the argument's element type, and given the
  standard's name). The two free structs are thin dispatchers to the new
  member functions, mirroring `owner_less`'s existing placement and shape.
  Added `#include <__functional/hash.h>` for `hash<T*>`; advisor review
  caught that this was redundant (`__memory/unique_ptr.h`, already included,
  pulls it in for `hash<unique_ptr>`), so removed it — verified no
  transitive-include-graph shift via `transitive_includes.gen.py` and
  `module_std.gen.py` (125/126 pass, unchanged). Also caught by advisor: an
  initial test asserted `owner_hash()` returns *different* values for
  distinct control blocks — not a standard guarantee (only "equal owner
  implies equal hash" is required; collisions are permitted) — removed
  those assertions, keeping only the guaranteed equal-hash-for-equal-owner
  checks. Updated: `<memory>` synopsis (both class synopses plus new
  `owner_hash`/`owner_equal` struct synopses, placed after
  `owner_less<void>` per the paper). Flipped
  `__cpp_lib_smart_ptr_owner_equality`'s `unimplemented` off
  (`generate_feature_test_macro_components.py`) and regenerated
  `version`/`FeatureTestMacroTable.rst`/the two
  `*.version.compile.pass.cpp` tests via `libcxx-generate-files`. Added the
  C++26-guarded `owner_hash`/`owner_equal` exports to
  `libcxx/modules/std/memory.inc`. Marked P1901R2 Complete in
  `Cxx2cPapers.csv`. New tests: `owner_hash.pass.cpp`/`owner_equal.pass.cpp`
  in both `util.smartptr.shared.obs` and `util.smartptr.weak.obs` (member
  functions), plus a new `util.smartptr.owner/` directory (sibling to
  `util.smartptr.ownerless/`) with `owner_hash.pass.cpp`/
  `owner_equal.pass.cpp` for the free structs, including a heterogeneous
  `unordered_map`/`unordered_set` lookup case (`shared_ptr` key, `weak_ptr`
  lookup) in each. Full `util.smartptr` suite (111 tests) green: 109
  passed, 2 unsupported (unrelated). Ran the pre-existing
  `clang_modules_include.gen.py` header-modules suite as a caution after
  advisor flagged the untested module surface; confirmed via `git stash`
  that its 122/143 failures (including a `memory.compile.pass.cpp` failure
  citing this session's `owner_hash()`) are **pre-existing and unrelated**
  — the same file fails to build the Clang header module at baseline too,
  for independent reasons (e.g. `__memory/indirect.h`'s `remove_const_t`
  visibility). Not investigated further — that whole suite is already
  broken independent of any Tier 1 library work and is out of scope for
  this contract. **Next session: pick a remaining Tier 1 item** —
  `P2363R5` (heterogeneous lookup remaining overloads), or the two partial
  items (`P2944R3` `reference_wrapper` comparisons, `P1383R2` `constexpr`
  `<cmath>`) — or investigate the pre-existing `clang_modules_include.gen.py`
  breakage (122/143 failing at baseline, unrelated to any tracked Tier
  work) if header-modules infrastructure maintenance takes priority.
- **2026-08-20 (sixth entry)**: Implemented P2363R5 (remaining heterogeneous
  associative-container overloads). Confirmed via WebFetch against
  `eel.is/c++draft` (the paper's own HTML doesn't carry the wording — that
  lives in the referenced P2077R2/prior heterogeneous-lookup papers, so the
  actual signatures had to come from the current draft standard) that scope
  is: `map`/`unordered_map` gain heterogeneous `try_emplace`,
  `insert_or_assign` (both with and without a hint), `operator[]`, and
  `at`; `set`/`unordered_set` gain heterogeneous `insert` (with and without
  hint); all four unordered containers (`unordered_map`,
  `unordered_multimap`, `unordered_set`, `unordered_multiset`) gain
  heterogeneous `bucket`. Explicitly **not** in scope: `multimap`/
  `multiset` get nothing under this paper (confirmed by fetching
  `multiset.overview` directly — no heterogeneous `insert` there) and the
  FTM's `headers` list in the generator only names `map`/`set`/
  `unordered_map`/`unordered_set` (multimap/multiset share headers with
  their unique counterparts, so this is consistent). Erase/extract
  heterogeneous overloads are a **different**, separately-gated FTM
  (`__cpp_lib_associative_heterogeneous_erasure`, P2077R2, C++23) that is
  still `unimplemented: True` in this repo's generator — left untouched,
  out of scope for this paper and not tracked in this C++26 gap document.

  Pre-implementation blocking check (per advisor): verified
  `__tree::__emplace_unique_key_args`/`__emplace_hint_unique_key_args` and
  `__hash_table::__emplace_unique_key_args` are **already** templated on
  the search-key type (`template <class _Key, class... _Args>`), with the
  actual search (`__find_equal`/`hash_function()(__k)` +
  `key_eq()(...)`) also already generic — i.e., the internal emplace
  machinery already supported heterogeneous search transparently (same
  path `find`/`count`/etc. use); no plumbing changes were needed, only the
  new public-facing overloads. This turned what could have been a
  multi-session plumbing project into a single mechanical (if large)
  fan-out session, confirmed empirically before writing the 20+ new
  overloads.

  Added, each guarded `#if _LIBCPP_STD_VER >= 26` and constrained on
  `__is_transparent_v<_Compare, _K2>` (ordered) or
  `__is_transparent_v<hasher, _K2> && __is_transparent_v<key_equal, _K2>`
  (unordered): `map`/`unordered_map`'s `try_emplace`/`insert_or_assign`
  non-hint overloads additionally exclude `is_convertible_v<_K2&&,
  const_iterator>`/`is_convertible_v<_K2&&, iterator>` (per the standard's
  Constraints, to disambiguate a 2-arg call from the 2-arg hint overload);
  `set`/`unordered_set`'s hint `insert` excludes the same pair (to
  disambiguate from the 2-iterator range-insert overload); non-hint
  `insert`/`insert_or_assign`/`operator[]`/`at`/`bucket` need no such
  exclusion (arity alone disambiguates, verified by writing out the
  argument counts rather than trusting a lossy WebFetch summarizer, which
  also incorrectly annotated every function `constexpr` on the same
  eel.is page — a live example of why the summarizer needed
  cross-checking here). `operator[](K&&)`/`at(const K&)` are defined
  inline in terms of `try_emplace`/`__tree_.__find_equal`/`find`,
  mirroring the standard's own Effects wording
  (`try_emplace(std::forward<K>(x)).first->second`) rather than
  duplicating tree-search logic. Added `#include
  <__type_traits/is_convertible.h>` to all four headers; ran
  `transitive_includes.gen.py`/`module_std.gen.py` after — 125/126 pass
  (1 pre-existing unsupported), unchanged from baseline, confirming
  `is_convertible` was already transitively reachable so the explicit
  include didn't shift the graph.

  Verified incrementally per advisor's guidance: implemented and tested
  `map` alone first (lit-testable via `libcxx-lit`, not raw
  `clang++` — a raw invocation hits the `__config_site` staged-header trap
  CLAUDE.md warns about) before fanning out to `set`/`unordered_map`/
  `unordered_set`/`unordered_multimap`/`unordered_multiset`. Note: this
  session's sandbox runs a background static-analysis pass (surfaced as
  `<new-diagnostics>` system reminders) that flagged every new heterogeneous
  call site as "no matching member function" — cross-checked against the
  real `libcxx-lit` compile line and confirmed the false positives are from
  that checker running without `-std=c++26`/the libc++-specific defines
  (so the new `_LIBCPP_STD_VER >= 26`-guarded overloads are invisible to
  it); the actual lit-driven compiles all passed. Worth remembering for
  future sessions: don't trust that diagnostic channel for anything gated
  behind a std-version or libc++-internal macro — always confirm via a
  real `libcxx-lit` run.

  New tests (12 files, all passing under the real `libcxx-lit` run, full
  `containers/associative` + `containers/unord` suites re-run clean
  afterward: 783 passed / 2 unsupported, no regressions):
  `map/map.modifiers/try_emplace_transparent.pass.cpp`,
  `map/map.modifiers/insert_or_assign_transparent.pass.cpp`,
  `map/map.access/element_access_transparent.pass.cpp` (operator[]/at),
  `set/insert_transparent.pass.cpp`,
  `unord.map/unord.map.modifiers/try_emplace.transparent.pass.cpp`,
  `unord.map/unord.map.modifiers/insert_or_assign.transparent.pass.cpp`,
  `unord.map/element_access.transparent.pass.cpp`,
  `unord.map/bucket.transparent.pass.cpp`,
  `unord.multimap/bucket.transparent.pass.cpp`,
  `unord.set/insert.transparent.pass.cpp`,
  `unord.set/bucket.transparent.pass.cpp`,
  `unord.multiset/bucket.transparent.pass.cpp`. Flipped
  `__cpp_lib_associative_heterogeneous_insertion`'s `unimplemented` off
  and regenerated `version`/`FeatureTestMacroTable.rst`/the five
  `*.version.compile.pass.cpp` tests via `libcxx-generate-files`. Marked
  P2363R5 Complete in `Cxx2cPapers.csv`. All of Tier 1's originally-listed
  callable-wrapper and heterogeneous-lookup items are now done except the
  two partials. **Next session: pick a remaining Tier 1 item** —
  `P2944R3` (`reference_wrapper` comparisons, blocked on `optional`/
  `tuple` P2165R4 equality changes — note P2165R4 is itself a *C++23*
  paper only partially done in this repo, scoped to `zip_view`; extending
  it to `optional`/`tuple` comparisons is a real, possibly nontrivial
  sub-task, not just a status-flip) or `P1383R2` (`constexpr` `<cmath>`
  scalar functions — **scope check done this session**: the generator's
  `__cpp_lib_constexpr_cmath` entry only has a `c++23` value with
  `unimplemented: True` and no C++26 bump at all, meaning *zero* `<cmath>`
  functions are `constexpr` in this repo currently — likely relies on
  Clang's constexpr-evaluator support for `__builtin_<math>` intrinsics,
  which should be checked for availability in this Clang version before
  scoping the work; this is probably larger than a single session, touches
  many files in `libcxx/include/__math/`, and was flagged as such rather
  than started).

- **2026-08-20 (fourth entry)**: Closed the flagged `P3168R2` scope-check
  item. Verified the suspicion directly: `libcxx/include/optional`
  already defines `begin()`/`end()`/`iterator`/`const_iterator` for both
  the primary `optional<T>` (guarded `#if _LIBCPP_STD_VER >= 26`) and the
  `optional<T&>` partial specialization, plus
  `ranges::enable_view<optional<_Tp>> = true` (covers both specializations,
  since `_Tp` can itself be a reference type — no separate declaration
  needed for `optional<T&>`) and
  `ranges::enable_borrowed_range<optional<_Tp&>> = true` (reference-only,
  correctly excluding the owning primary template). `<version>`'s
  `__cpp_lib_optional_range_support` is `202406L`, not `unimplemented` —
  all landed as part of the P2988R11 (`optional<T&>`) session, confirming
  this was purely a leftover status/test-coverage gap, not fresh
  implementation work. What was actually missing: zero test coverage
  beyond the feature-test-macro checks in `optional.version.compile.pass.cpp`
  — no test exercised `begin`/`end` behavior or verified the range/view/
  borrowed_range concepts. Added
  `libcxx/test/std/utilities/optional/optional.iterators/begin_end.pass.cpp`
  (disengaged/engaged behavior for both specializations, iterator
  mutation reaching the contained/referenced object, `constexpr`-ness,
  interop with `std::ranges::begin`/`end`/`distance` and range-based
  `for`) and
  `.../optional.iterators/range_concept_conformance.compile.pass.cpp`
  (`static_assert`s that `optional<T>`/`optional<T&>` and their `const`
  forms model `contiguous_range`/`sized_range`/`common_range`/`view`, and
  that only `optional<T&>` additionally models `borrowed_range`). Both
  pass under the real `libcxx-lit` run (bare llvm-lit / the sandbox's
  background static-analysis channel both falsely flag this file with
  "no member `begin`" / "no member `ranges`" errors — same known false-
  positive pattern as prior sessions, since neither uses `-std=c++26`;
  confirmed via the wrapper instead). `libcxx-generate-files` produced no
  diff (macro state was already correct). Flipped `P3168R2` to
  `|Complete|` in `Cxx2cPapers.csv` and `[x]` in this document. **Tier 1
  is now fully done except the two genuine partials**: `P2944R3`
  (`reference_wrapper` comparisons — blocked on extending P2165R4's
  `optional`/`tuple` equality changes) and `P1383R2` (`constexpr`
  `<cmath>` scalars — needs a Clang builtin-support check before scoping,
  likely multi-session). **Next session: either pick up one of those two
  partials, or move to Tier 2** (`P2300R10` `std::execution` — largest
  remaining item, needs its own sub-plan per the Scope section — or
  `P2900R14` Contracts, deferred).
- **2026-08-20 (Tier 2 kickoff)**: Started Tier 2 (`P2300R10` `std::execution`),
  per user request to move to Tier 2. This is a **multi-session effort** —
  see the dedicated sub-plan under Tier 2 above for full scope, milestone
  breakdown, and structural findings; do not read a partially-done
  milestone as abandoned. Surveyed `eel.is/c++draft/exec` to scope and
  order the work; set `P2300R10`/`P3325R5`/`P3396R1` to `|In Progress|` in
  `Cxx2cPapers.csv` and committed the sub-plan on its own before any code.
  Then implemented the first slice of **M1**: `__queryable` concept,
  `forwarding_query`, `prop<Query, Value>`, `env<Envs...>`, `get_env`/
  `env_of_t` — new `libcxx/include/__execution/` headers, wired into
  `<execution>` in a separate C++26-guarded block outside the existing
  PSTL execution-policy guard (verified that structural separation was
  necessary before writing any code). `get_allocator`/`get_stop_token`/
  the `stoppable_token` concept family/`inplace_stop_token` are the
  explicitly deferred remainder of M1 — not started.

  Along the way, discovered (and documented in the sub-plan, since it's
  load-bearing for every future milestone) that `requires { obj.method(); }`
  hard-errors instead of evaluating `false` when `obj`/its arguments are
  concrete rather than genuinely template-dependent, reproduced
  independently against stock upstream Clang 22 and GCC 16 (not a
  fork-specific bug) — this shaped both `env.h`'s implementation (the
  `_Idx == sizeof...(_Envs)` guards, needed because noexcept-specifiers
  aren't SFINAE-protected either) and how its tests had to be written (the
  `CanQuery` concept helper in `env.pass.cpp`, routing every "this query is
  unsupported" assertion through a template parameter rather than a
  concrete local variable). Advisor caught two real bugs before commit:
  aggregate-breaking `-Wdeprecated-copy` from deleting `operator=` without
  a user-declared copy constructor (fixed by making `prop`'s/`env`'s data
  members `const`, which implicitly deletes assignment without an explicit
  declaration and keeps `prop` an aggregate per its synopsis), and the
  noexcept-specifier SFINAE gap above.

  New tests (all passing under `libcxx-lit`, `libcxx/test/std/execution/`
  now 4/4 green): `exec.queries/exec.fwd.env/forwarding_query.pass.cpp`,
  `exec.queries/exec.get.env/get_env.pass.cpp`,
  `exec.envs/exec.prop/prop.pass.cpp`, `exec.envs/exec.env/env.pass.cpp`.
  Added the new headers to `libcxx/include/CMakeLists.txt`, a C++26-guarded
  export block to `libcxx/modules/std/execution.inc`, and updated
  `libcxx/test/libcxx/transitive_includes/cxx26.csv` (`execution` now also
  transitively pulls `compare`/`cstdint`/`limits`/`tuple` in C++26 mode) —
  `transitive_includes.gen.py` and `module_std.gen.py` both clean
  afterward (125/126 and 126/126 respectively, matching this repo's
  existing 1 pre-existing unsupported baseline). Left
  `__cpp_lib_senders`'s `unimplemented` flag untouched per the sub-plan's
  FTM discipline (only flips at M6). **Next session: finish M1**
  (`get_allocator`, `get_stop_token`, `stoppable_token`/
  `stoppable_token_for`/`unstoppable_token`/`never_stop_token`,
  `inplace_stop_token`/`inplace_stop_source`/`inplace_stop_callback` in
  `<stop_token>`) before moving to M2's `sender`/`receiver`/
  `operation_state` concepts.
- **2026-08-20 (Tier 2, M1 completion)**: Finished the deferred remainder
  of M1 — `get_allocator`, `get_stop_token`, and the `<stop_token>`
  additions they depend on (`stoppable_token`/`unstoppable_token`
  concepts, `never_stop_token`, `inplace_stop_source`/`inplace_stop_
  token`/`inplace_stop_callback`). Full details are inline under M1's
  entry in the sub-plan above rather than duplicated here. Headline
  points: reused this repo's existing `__stop_state`/`__atomic_unique_
  lock`/`__intrusive_list_view` machinery (already backing `stop_source`/
  `stop_token`/`stop_callback`) for `inplace_stop_source` instead of
  reimplementing the callback-registration race, which turned a
  genuinely tricky concurrency problem into a much smaller
  wire-it-together session; caught (before shipping) that skipping
  `__stop_state`'s source-counter increment in `inplace_stop_source`'s
  constructor would have silently broken every callback registration;
  and retrofit `stop_token` with a `callback_type` member alias
  (verified against the actual C++26 draft, not inferred) so the
  pre-existing C++20 class keeps modeling the new `stoppable_token`
  concept. Advisor caught two availability-marker gaps invisible in this
  environment's build config (`libcpp-has-no-availability-markup`) that
  would have broken Apple-platform builds — both fixed. **All of M1 is
  now complete.** `libcxx/test/std/execution/` (10 tests) and
  `libcxx/test/std/thread/` (354 tests) both fully green; no regressions.
  **Next session: M2** — `completion_signatures`, `get_completion_
  signatures`, `receiver`/`operation_state`/`sender`/`sender_in`
  concepts, `connect`/`start`, `default_domain`/`indeterminate_domain`/
  `transform_sender`/`apply_sender`. Read M1's `requires{}`
  eager-evaluation finding (in the sub-plan above) before writing any of
  M2's CPOs — it applies directly to `sender`/`receiver` concept checks.
- **2026-08-20 (Tier 2, M1 namespace correction + M2 research)**: Before
  starting M2, cross-checked M1's shipped code against
  `eel.is/c++draft/execution.syn` fetched directly via `curl` + Python
  tag-strip (abandoned WebFetch's summarizer after it returned wrong
  namespaces/names/CPO shapes and at least one fabricated declaration for
  `[exec.connect]`/`[exec.snd.transform]`). Found and fixed a real M1 bug:
  `forwarding_query`, `get_allocator`, `get_stop_token`, and the
  exposition-only `queryable` concept belong in plain `namespace std`, not
  `std::execution` — moved all four, added the previously-missing
  `stop_token_of_t`, updated 4 tests and `execution.inc`'s export block, all
  green. Also resolved an open question from the previous session: the
  current draft has no `empty_env` (R10 had it; the draft's actual default
  is `env<>`, matching what M1 already implemented — nothing to change
  there), and tag names are `receiver_tag`/`sender_tag`/
  `operation_state_tag`/`scheduler_tag`, not R10's `receiver_t`/`sender_t`/
  `operation_state_t` — corrected in the M2 plan below. Established the
  process rule (eel.is beats the R10 paper on names; R10 paper only for
  algorithm-body wording) and confirmed the current draft carries several
  out-of-scope papers' entities in the same synopsis (`task_scheduler`,
  `affine`, `associate`/`spawn_future`, `bulk_chunked`/`bulk_unchunked`,
  `indeterminate_domain`, `get_start_scheduler`/`get_delegation_scheduler`)
  — scope rule: implement only what the P2300R10+P3325R5+P3396R1 surface
  actually names. Re-split M2 into M2a (completion_signatures/receiver/
  operation_state/sender concepts, no domain dependency) and M2b
  (get_domain/get_scheduler/default_domain/transform_sender/apply_sender/
  connect) — see the sub-plan above for full detail. No M2 code written
  yet; full details and the exact fetch commands are recorded inline in the
  sub-plan above so the next session doesn't re-derive any of this.
  **Next session: M2a**, starting with fetching `[exec.cmplsig]` and
  `[exec.snd]` as raw HTML.
- **2026-08-20 (Tier 2, M2 landed)**: Implemented M2a (completion_signatures/
  gather-signatures, set_value/set_error/set_stopped, receiver/receiver_of,
  operation_state/start, sender/tag_of_t, get_completion_signatures/
  sender_in/completion_signatures_of_t, value_types_of_t/error_types_of_t/
  sends_stopped) and M2b (default_domain/get_domain, transform_sender/
  apply_sender, connect) in one session, in 8 new headers. Five deliberate,
  documented deviations from strict conformance (enable-sender's awaitable
  disjunct, dependent_sender/dependent_sender_error's throw path, get_domain
  branch 2.2/get_completion_domain's operator(), default_domain::
  transform_sender's tag-dispatch branch, connect's connect-awaitable
  fallback) — all traced to either M6 coroutine-integration machinery not
  existing yet, or this Clang lacking P3068 constexpr exceptions (verified
  empirically), or a newly-discovered structured-binding/SFINAE hard-error
  interaction distinct from M1's eager-requires{} finding. Full details,
  each deviation's exact reasoning, and the regression-test repro for the
  constexpr-exceptions gap are recorded in M2's own entry above — read it
  before M3, since M3's `read_env` directly exercises the dependent-sender
  deviation. 52/52 new + existing execution/thread.stoptoken tests green;
  module_std.gen.py/transitive_includes.gen.py clean. **Next session: M3**
  — just/just_error/just_stopped, read_env, schedule.
- **2026-08-21 (Tier 2, M3 landed)**: Implemented M3 in 6 new headers
  (`__execution/{movable_value,get_forward_progress_guarantee,schedule,
  scheduler,just,read_env}.h`): `__movable_value` (namespace std, per
  [exec.general]); `forward_progress_guarantee` enum + `get_forward_
  progress_guarantee_t`/`get_forward_progress_guarantee`; `schedule_t`/
  `schedule`; `scheduler_tag`/`scheduler`/`schedule_result_t`; `just_t`/
  `just_error_t`/`just_stopped_t`/`just`/`just_error`/`just_stopped`;
  `__read_env_t` (read_env's type is unspecified in [execution.syn])/
  `read_env`.

  **Architecture decision (confirmed with advisor before writing code):**
  the *current* draft (fetched fresh via eel.is, not re-derived from the
  R10 paper or M1/M2's notes) has moved to a generic `basic-sender`/
  `impls-for`/`default-impls`/`basic-operation`/`basic-receiver`/
  `make-sender` engine ([exec.snd.expos]) that every sender factory and
  adaptor from here through M5 is meant to plug into via `impls-for<Tag>`
  specializations. Did **not** build this engine: its failure path
  (`basic-sender::get_completion_signatures`, [exec.snd.expos]p47) relies
  on `throw unspecified-exception()` from a `consteval` function — the
  exact P3068 constexpr-exceptions gap already found compiler-blocked at
  M2 (deviation 2) — and the exposition text is even internally
  inconsistent in this revision (p43's `impls-for<Tag>::get-attrs` is
  never declared by p35's `impls-for`/`default-impls`). Continued M1/M2's
  precedent instead: each M3 sender is a hand-written aggregate with
  public `tag`/`data` members (matching the `product-type`/structured-
  binding-decomposable shape `tag_of_t` already expects), its own
  `connect` member, and its own `get_completion_signatures` static
  member. Revisit factoring into a shared engine at M4, once `then`
  supplies the first adaptor with a child sender to design against.

  **New compiler-limitation finding (distinct from M1's eager-`requires{}`
  and M2's body-instantiation-outside-immediate-context findings):** a
  `static_assert(!requires(T t) { some_cpo(t); })`, where `some_cpo` is a
  global CPO object and the constrained call to its sole `operator()`
  candidate fails due to unsatisfied template constraints, **hard-errors**
  on this fork's Clang instead of the requires-expression quietly
  evaluating to `false` — reproduced in isolation with a two-line
  unrelated repro (`inline constexpr Foo foo{};` with one constrained
  `operator()`; `static_assert(!requires(NoBar n) { foo(n); })` fails to
  compile, "no matching function for call to object of type 'const
  Foo'"). Wrapping the identical check in a named `concept` (`template
  <class T> concept has_foo = requires(T t) { foo(t); }; static_assert(
  !has_foo<NoBar>);`) works around it — confirmed both in isolation and
  in `get_forward_progress_guarantee.pass.cpp`'s negative test. Root cause
  not fully isolated (unlike M1/M2's findings, no consteval-exception or
  body-instantiation angle identified yet); flagging here so a future
  session investigating unrelated `static_assert(!requires{...})`
  failures checks this pattern first, and always prefer a named concept
  over an inline anonymous `requires(...) {...}` passed directly to
  `static_assert` for "this call must be ill-formed" checks in this repo
  going forward. Does not affect any library code (`scheduler.h`'s
  `scheduler` concept — a named concept — hits the identical CPO-call-
  inside-requires shape correctly for its own `NoGuaranteeScheduler`
  negative test).

  **Two more findings surfaced while ordering just.h:**
  1. `get_forward_progress_guarantee_t`'s own trailing requires-clause
     cannot construct `get_forward_progress_guarantee_t{}` (needs the
     class complete; a member function template's requires-clause is not
     a complete-class context the way a function *body* is) — used `*this`
     instead, and a requires-expression hypothetical parameter of the same
     reference type for the SFINAE probe. Same root cause independently
     also broke `just_stopped_t::operator()() -> __just_sndr<just_stopped_t>`
     (a **non-template** member function, so — unlike `just_t`'s and
     `just_error_t`'s templated `operator()`s — its return type is needed
     eagerly, deferred only to `just_stopped_t`'s own closing brace, not to
     first call): fixed by moving `__just_sndr`'s *full* definition before
     the `just_t`/`just_error_t`/`just_stopped_t` struct definitions
     (forward-declaring the three tag types earlier, since `__just_opstate`
     and `__just_sndr` — both templates — only need them declared, not
     complete, for `same_as<_Tag, just_t>`-style comparisons).
  2. `get_forward_progress_guarantee`/`schedule_t` deliberately avoid
     depending on the `scheduler` concept even though the draft phrases
     both in terms of it ([exec.get.fwd.progress]p2, "ill-formed unless Sch
     satisfies scheduler") — `scheduler`'s own requires-clause calls both
     of them, which would make the definitions mutually recursive.
     Constrained each directly on its own underlying syntax instead (documented
     inline in both headers).

  Also confirmed, contrary to a plausible-sounding first guess: `<execution>`'s
  transitive-includes CSV row needed **no changes** for M3 (`transitive_includes.
  gen.py` passed 125/125 clean on the first try) — `<tuple>`/`<exception>`, the
  only new standard headers M3's sources use, were already pulled in
  transitively by M2's `get_completion_signatures.h`.

  **New tests** (all passing under `libcxx-lit`; full `execution/` suite now
  20/20 green, `thread.stoptoken/` still 37/37, no regressions):
  `exec.queries/exec.get.fwd.progress/get_forward_progress_guarantee.pass.cpp`,
  `exec.sched/scheduler.pass.cpp`, `exec.factories/exec.schedule/
  schedule.pass.cpp`, `exec.factories/exec.just/just.pass.cpp` (real runtime
  behavioral asserts via manual `connect`+`start`, not just `static_assert` —
  the earliest point in this sub-plan that's been possible), `exec.factories/
  exec.read.env/read_env.pass.cpp` (also behavioral; plus `static_assert`s
  confirming `sender<read_env(...)>` is true but `sender_in` with no Env is
  false — the "dependent-sender-as-soft-failure" deviation from M2 exercised
  for real for the first time). Registered the 6 new headers in
  `CMakeLists.txt` and `<execution>`'s `_LIBCPP_STD_VER >= 26` include block;
  extended `libcxx/modules/std/execution.inc`'s export block.
  `module_std.gen.py` 125/126 (same pre-existing 1-unsupported baseline as
  M2), `transitive_includes.gen.py` 125/125 clean. Did not run full
  `check-cxx` (same pre-existing, unrelated `std.cppm`/`reflection_v2`
  module-build failure noted at M2, confirmed still unrelated to
  `<execution>`). **Next session: M4** — `run_loop` +
  `this_thread::sync_wait`/`sync_wait_with_variant`, plus `then` (rides
  along per the sub-plan's vertical-slice checkpoint). Get
  `just(42) | then([](int i){ return i+1; }) | sync_wait()` compiling and
  running end-to-end before fanning out to M5. Decide explicitly there
  whether `run_loop`/`sync_wait` should be gated on `_LIBCPP_HAS_THREADS`
  (per the sub-plan's threading note above) and, if a shared `impls-for`-
  style engine is worth factoring out now that `then` gives a second
  (non-childless) sender to design against — re-read this session's
  "Architecture decision" note above first.
- **2026-08-21 (second entry)**: Completed M4: `run_loop`, `this_thread::
  sync_wait`, and `then` (`sync_wait_with_variant`, `upon_error`,
  `upon_stopped` explicitly deferred — see the M4 entry above for why).
  Vertical-slice checkpoint confirmed: `sync_wait(just(42) | then([](int
  i){ return i+1; }))` compiles and runs correctly end-to-end (note the
  checkpoint is a plain function call around the piped chain, not a
  trailing `| sync_wait()` — sync_wait is a consumer, not a pipeable
  adaptor; caught before implementation started). New headers:
  `get_scheduler.h`, `run_loop.h`, `sync_wait.h`, `sender_adaptor_closure.h`
  (new pipeable-closure foundation every M5 adaptor will reuse), `then.h`,
  `fwd_env.h`. Three new compiler-behavior/language findings recorded in
  the M4 entry above (a `requires CALL(...) && requires(...) {...}`
  mis-parse needing extra parens; `F(_ResultT)` with `_ResultT=void`
  hard-erroring on substitution unlike literal source `F(void)`; in-class
  member-template specializations working fine on this fork's Clang).
  `execution/` suite 63/63 green, `thread.stoptoken/` 37/37, no
  regressions; `module_std.gen.py`/`transitive_includes.gen.py` 125/126
  (same pre-existing baseline); `transitive_includes/cxx26.csv`'s
  `execution` rows regenerated (first real change since M1, from
  `sync_wait.h` pulling in `<system_error>`/`<optional>`).
  `Cxx2cPapers.csv` untouched (still `|In Progress|`, per plan — flips
  only at M6).
- **2026-08-21 (third entry)**: Post-M4 review caught and fixed two real
  bugs before they shipped further. (1) `execution.inc`'s export block
  named `run_loop` and `std::this_thread::sync_wait` unconditionally, but
  both headers self-guard on `_LIBCPP_HAS_THREADS` — a no-threads module
  build would have failed to resolve those `using` declarations. Wrapped
  both in `#if _LIBCPP_HAS_THREADS`/`#endif`, matching the file's existing
  `get_stop_token` guard right above. Verified two ways: preprocessing
  `execution.inc` directly with `_LIBCPP_HAS_THREADS` forced to 0/1 (confirms
  the guard actually strips the right lines), and compiling real TUs against
  a staged-header copy with `__config_site`'s `_LIBCPP_HAS_THREADS` patched
  to 0 (confirms `then`/`just`/`get_scheduler` still work and `run_loop`/
  `sync_wait` correctly fail to resolve). Did not stand up a full no-threads
  *runtime* build tree (expensive; the two checks above exercise the actual
  guard logic without it). (2) `run_loop::run()` read-then-wrote `__state_`
  (the `starting`→`running` transition) without holding `__mtx_`, racing
  against `finish()`'s locked write from another thread — a real violation
  of [exec.run.loop.members]p7's data-race-freedom Remark, since cross-
  thread push/run is `run_loop`'s entire reason to exist. Fixed with a
  `lock_guard` around the transition. Added a genuine cross-thread test to
  `run_loop.pass.cpp` (producer thread pushes + finishes while the main
  thread blocks in `run()`) — every prior test pushed work before calling
  `run()`, so none exercised the blocking `__pop_front` wait or the race.
  Also added an `operator|(closure, closure)` composition test to
  `then.pass.cpp` (`then(f) | then(g)` composed before ever piping a sender
  through it) — the existing chained-pipe test is left-associative and only
  ever exercised the sender-pipe-closure overload, never this one; every M5
  adaptor depends on both overloads working. All fixes re-verified:
  `execution/` + `thread.stoptoken/` 63/63, `module_std.gen.py`/
  `transitive_includes.gen.py` 125/126 (same baseline). **Next session:
  M5** — start with `upon_error`/`upon_stopped` (cheap, reuses `then`'s
  machinery almost unchanged), then work through the rest of the M5
  adaptor list.
- **2026-08-21 (fourth entry)**: Started M5. Implemented `upon_error`/
  `upon_stopped` by generalizing `<__execution/then.h>` in place (all three
  are one standard clause) onto `just.h`'s `_Tag`-templated precedent,
  rather than adding near-duplicate files. See the M5 entry above for full
  detail: `__then_sndr`/`__then_rcvr`/`__then_sig_transform` now take
  `_Tag` (`then_t`/`upon_error_t`/`upon_stopped_t`) as a template parameter
  and dispatch with `if constexpr (same_as<_Tag, ...>)`; advisor-reviewed
  design choice (`_SetCpo` as an explicit template parameter, not derived
  via a member-typedef indirection) landed without issue. New tests
  `exec.adapt/exec.then/{upon_error,upon_stopped}.pass.cpp`; caught and
  fixed one test bug (`sync_wait` needs an rvalue sender — a locally-named
  sender must be `std::move`d in). `execution/` 28/28,
  `module_std.gen.py`/`transitive_includes.gen.py` 125/126, no
  `libcxx-generate-files` diff. Only registration touch was
  `execution.inc` (no new header ⇒ no CMakeLists/transitive-includes
  changes) — smaller surface than M4. `Cxx2cPapers.csv` untouched. **Next
  session: `let_value`/`let_error`/`let_stopped`** — the next `[exec.adapt]`
  subclause; expect it to need real "connect a child operation state
  dynamically" plumbing, unlike the intercept-and-complete shape `then`/
  `upon_error`/`upon_stopped` shared.
- **2026-08-21 (fifth entry)**: Continued M5: implemented `let_value`/
  `let_error`/`let_stopped`. New `libcxx/include/__execution/let.h`
  (one file, all three CPOs, per-clause convention), hand-adapting the
  standard's `let-state` with a real `variant`-backed operation state
  (reusing `get_completion_signatures.h`'s existing `__gather_signatures`/
  `__decayed_tuple`/`__dedup_type_list_t` rather than rebuilding
  signature-gathering). `let-env` always takes the `env<>{}` fallback
  branch (documented deviation; branch 2.1/SCHED-ENV is cheap from
  existing M1/M4 pieces and flagged for `continues_on`/`on`/
  `schedule_from` later in this milestone, not blocked). Hit and fixed a
  real incomplete-type trap: `connect_result_t<Sndr, ChildRcvr>`, computed
  inside the still-incomplete opstate, transitively compiles `ChildRcvr::
  get_env()`'s body (an `auto`-returning, non-trailing-decltype link
  inside `<__execution/connect.h>` forces this) — fixed by giving the
  child receiver its own direct `Rcvr&` member instead of reaching through
  a back-pointer to the opstate, matching the standard's own (initially
  taken-on-faith, then empirically justified) two-member `receiver` shape.
  Ported `emplace-from` ([exec.let]p10) as `__emplace_from<Fn>` to
  construct non-movable operation states in place inside the variants.
  New tests `exec.adapt/exec.let/{let_value,let_error,let_stopped}.pass.cpp`,
  including a hand-rolled `maybe_errors_sndr` (matching `sync_wait.pass.cpp`'s
  own established pattern) to exercise a continuation completing with an
  error rather than a value. `execution/` 28/28 → 31/31,
  `module_std.gen.py`/`transitive_includes.gen.py` 125/126 (no diff
  despite the new header), `libcxx-generate-files` clean. Full
  registration this time (new header ⇒ `CMakeLists.txt` +
  `<execution>`'s include block + `execution.inc`), unlike the previous
  `upon_error`/`upon_stopped` entry. `Cxx2cPapers.csv` untouched. **Next
  session: continue M5** — `starts_on`/`continues_on`/`on`/`schedule_from`
  (the scheduler-affinity family, natural next target given this
  session's `let-env` branch-2.1 note) or the remaining adaptors
  (`when_all`/`into_variant`/`stopped_as_optional`/`stopped_as_error`/
  `write_env`/`unstoppable`/`bulk*`) if scheduler plumbing isn't the
  priority. Re-read this session's incomplete-type note before writing
  any adaptor that connects a child sender from inside its own operation
  state.
- **2026-08-22**: Continued M5: implemented `schedule_from`/`continues_on`/
  `starts_on`, and `on` (2-arg `on(sch, sndr)` form only — see below).
  New `libcxx/include/__execution/{schedule_from,continues_on,starts_on,
  on}.h`.

  **`schedule_from`** ([exec.schedule.from]): the standard's own clause
  defines *no* impls-for/connect/completion-signature algorithm at all —
  its entire behavior is domain-based customization, [Note 1]: "used by
  schedulers to control how to transition off of their schedulers'
  associated execution contexts". Since `default_domain::transform_sender`
  on this fork always takes the identity branch (documented in
  `domain.h` from M2), `schedule_from(sndr)` is unconditionally
  identity-forwarding here — same completion signatures and attributes as
  `sndr`, `connect()` relays straight through. Still a real, distinctly-
  typed sender (not literally `return sndr;`) so `continues_on` can wrap
  its input in one per [exec.continues.on]p3's literal wording, rather
  than collapsing the wrapper away as a second deviation stacked on the
  first.

  **`continues_on`** ([exec.continues.on]) — the actual new engineering
  this session: unlike every prior M5 adaptor's single-child shape, its
  opstate owns *two* connected child operations simultaneously (the input
  sender, started first; `schedule(sch)`, connected up front but started
  only once the input completes) — a hand-adaptation of the standard's own
  `state-type`/`receiver-type`/`get-state`/`complete`. Applied the M1
  eager-`requires{}` and M5 `let.h` incomplete-type findings directly:
  both new receiver types (`__continues_on_sched_rcvr`,
  `__continues_on_child_rcvr`) store a direct `_Rcvr&` for `get_env()`
  (not reached through the opstate pointer), since `connect_result_t<...>`
  for both child operations is computed inside the still-incomplete
  opstate.

  **Real, empirically-discovered bug, not merely a design choice — record
  before writing another adaptor whose receiver captures *every*
  completion tag into a type-dependent variant (as opposed to
  intercepting exactly one, the way `then`/`let_value` do):**
  `<__execution/run_loop.h>`'s `__run_loop_opstate::__execute()` decides
  at *runtime* whether to call `set_value()` or `set_stopped()` on its
  receiver, but the dispatch is a plain `if`, not `if constexpr` — so
  *both* branches must be well-formed at *compile time* for whatever
  receiver type ends up connected to `schedule(sch)`, regardless of
  whether the environment's stop token could ever actually report
  `stop_requested()` (i.e. regardless of what `unstoppable_token` says,
  and regardless of what the sender's own advertised completion
  signatures say). `then.h`/`let.h` never hit this because their
  "completion tag not intercepted" branches are a type-independent direct
  forward (`execution::set_stopped(std::move(__rcvr_))`), always
  well-formed no matter what. `continues_on`'s child receiver intercepts
  *every* tag (that's the whole point — capture-and-replay whichever one
  fires) by emplacing into `__async_result_t`, a variant sized only for
  the tag/arg combinations the child sender's own completion signatures
  actually advertise — so when a forced-but-statically-unreachable
  `set_stopped()` call (originating from `run_loop`'s unconditional `if`,
  and propagating outward through a chain of otherwise-trivial
  direct-forwarding receivers in `on`'s composition, which is where this
  was actually caught — a standalone `continues_on(sch, sndr)` never
  reaches a second run_loop-backed hop) reaches a child sender that never
  advertises `set_stopped_t()` (e.g. `just`/`just(42)`), `tuple<set_stopped_t>`
  isn't a variant alternative and the `emplace` hard-errors. Confirmed
  this isn't fork-specific bad reasoning: the standard's own
  `impls-for<continues_on_t>::complete` lambda has no `requires
  callable<Tag, Rcvr, Args...>` guard either (unlike `default-impls::complete`,
  which does) — a fully-conforming implementation using the real
  `basic-sender`/`connect-all` machinery avoids this because the
  *generated* per-child receiver only ever has overloads matching that
  child's own advertised signatures (so `complete<set_stopped_t>` is
  simply never instantiated for a child that doesn't advertise it) — a
  guarantee this fork's hand-written, non-generic receivers don't get for
  free. Fix: `__on_child_complete<Tag>` now probes
  `requires { __async_result_.emplace<tuple<Tag, decay_t<Args>...>>(...); }`
  via `if constexpr` and, when the tag/arg combination isn't representable
  (provably unreachable at runtime for the case that actually triggered
  this — `unstoppable_token<never_stop_token>` is true, so
  `__execute()`'s stopped branch never *executes*, only *compiles*),
  skips the scheduling hop and completes directly instead of hard-erroring
  the whole translation unit. `variant::emplace<T>`'s own SFINAE-friendly
  default template argument (a substitution failure in
  `__find_unambiguous_index_sfinae<T,...>::value` when `T` isn't an
  alternative) makes this `requires{}` probe a soft, per-instantiation
  check rather than a hard error, matching the `__try_query`-style split
  used elsewhere in this sub-plan for the same reason.

  Completion-signature derivation (hand-derived, not the standard's
  operational "completion operations potentially evaluated as a result of
  `op.start()`" spec style, matching every prior adaptor in this
  sub-plan): each child signature `Tag(Args...)` replays as
  `Tag(decay_t<Args>...)`, plus `set_error_t(exception_ptr)` *per
  signature* if that signature isn't statically nothrow-decay-copyable
  (mirrors `then.h`'s own TRY-SET-VALUE pattern) — unioned with
  `schedule(sch)`'s own signatures minus its `set_value_t()` (which only
  triggers the internal redispatch, never propagates as-is). The internal
  storage variant (`__async_result_t`) separately follows
  [exec.continues.on]p9's literal formula: `tuple<Tag, decay_t<Args>...>`
  per child signature (tag included, unlike `let.h`'s tag-less
  `__decayed_tuple` — continues_on captures *any* of value/error/stopped
  and must remember which one fired), plus a single shared
  `tuple<set_error_t, exception_ptr>` alternative gated on whether *any*
  signature (not each individually) might fail to decay-copy nothrow —
  the standard's own storage-efficiency simplification, distinct from the
  per-signature check driving the advertised signature set above.

  **`starts_on`** ([exec.starts.on]): per p4, `starts_on(sch, sndr)` is
  expression-equivalent to a sender whose *own* `transform_sender` (fired
  via domain dispatch) rewrites it to
  `let_value(continues_on(just(), sch), [sndr]() mutable { return
  std::move(sndr); })`. Since domain dispatch never fires on this fork
  (same `default_domain` identity-branch deviation as `schedule_from`
  above), a `starts_on_t`-tagged sender built the usual way would be
  dead on arrival — so this computes the *rewrite's result* directly, at
  CPO-call time, rather than building an intermediate sender nothing
  would ever transform. **Deviation, same class as `let.h`'s let-env 2.3
  fallback:** the result's `tag_of_t` is `let_value_t`'s own tag, not
  `starts_on_t`; `sender_for<decltype(starts_on(...)), starts_on_t>` is
  false. Nothing in scope through M5 inspects `tag_of_t`/`sender-for` on
  a `starts_on` result. No new opstate/sender class needed at all — the
  whole file is one CPO struct.

  **`on`** ([exec.on]) — **only the `on(sch, sndr)` 2-arg form
  implemented; the pipeable-closure 3-arg form (`on(sndr, sch, closure)`,
  [exec.on]p1.2) and the argument-disambiguation rules that let a single
  2-arg call resolve to either form (p2.1–2.3) are deferred** — a
  deliberate scope cut (flagged in advance as the likely one) to land a
  complete, tested `on(sch, sndr)` rather than a half-wired overload set.
  Unlike `starts_on`, `on` genuinely needs a real sender/opstate rather
  than a pure call-time composition: [exec.on]p6's transform_sender body
  needs `get_start_scheduler(env)`, which isn't available until connect
  time (env comes from the real receiver) — so `__on_sndr::connect()`
  calls `execution::get_start_scheduler(execution::get_env(rcvr))`
  directly (matching [exec.on]p7's literal operational wording, which has
  no `call-with-default` fallback either) and builds
  `continues_on(starts_on(sch, sndr), orig_sch)` right there, forwarding
  the real receiver into it unchanged (no intermediate FWD-ENV-wrapping
  receiver, unlike every single-child adaptor elsewhere in this sub-plan
  — there's nothing generic to wrap since `on`'s own behavior is entirely
  this one composition). `get_completion_signatures` mirrors the same
  composition at the type level via `declval`.

  Tests: new `exec.adapt/{exec.schedule.from,exec.continues.on,
  exec.starts.on,exec.on}/*.pass.cpp`. `continues_on`'s and `starts_on`'s
  tests deliberately avoid threads — `run_loop::start()` + `finish()` +
  `run()` in that order drains a single-threaded queue synchronously
  (matching `exec.ctx/run_loop.pass.cpp`'s own first test block), and for
  `on`'s test specifically, using the *same* `run_loop` for both the
  "start on" and "resume on" schedulers means the second scheduling hop
  (queued mid-drain, from inside the first hop's own `execute()` call
  stack) is picked up by the same `run()` call before it returns — no
  producer thread needed, confirmed empirically by running the test, not
  just reasoned through. `continues_on.pass.cpp` also exercises error
  passthrough (not just value) with a hand-written always-errors sender,
  matching `let_value.pass.cpp`'s own `maybe_errors_sndr` precedent.
  Caught one *test* bug before it was real: a local (in-function) class
  with an abbreviated-function-template member (`void set_error(auto&&)`)
  is ill-formed — `[class.local]`: local classes may not have member
  templates — fixed by using a concrete `std::exception_ptr` parameter in
  every test receiver's `set_error`, matching what these compositions can
  actually produce.

  Advisor caught two real coverage gaps before this was called done,
  both fixed in a same-day follow-up: (1) `on.pass.cpp`'s original test
  used the *same* `run_loop` for both `sch` and `orig_sch`, which made
  [exec.on]p7.3's "transfer back to the remembered scheduler" hop
  unobservable — a `connect()` that dropped the `continues_on(...,
  orig_sch)` wrapper entirely and just ran `starts_on(sch, sndr)` would
  have passed the same test. Fixed with *two* distinct run_loops and an
  `assert(!completed)` after draining only the first one — the
  discriminating assertion that actually depends on the second hop
  landing on the second loop. (2) Every completion-signature test used an
  env that falls through `get_stop_token` to `never_stop_token`
  (`env<>`, `on_env`), so `__continues_on_sched_gather`'s "keep
  non-`set_value_t` sched signatures" union branch — the whole reason
  `continues_on`'s signature computation unions in anything from
  `schedule(sch)` at all — was dead code as far as the test suite could
  tell. Added a `stop_env` (answers `get_stop_token` with a real
  `inplace_stop_token`) static_assert confirming
  `continues_on(just(1), sch)`'s signatures include `set_stopped_t()` in
  the stoppable case. Both fixes re-verified green empirically (not just
  reasoned through) before landing. Also ran the no-
  `_LIBCPP_ENABLE_EXPERIMENTAL` compile check every prior M5 session
  ran (the lit suite itself always passes `-D_LIBCPP_ENABLE_EXPERIMENTAL`,
  so it doesn't exercise this path) — `<execution>` plus all four new
  CPOs compile clean under plain `-std=c++26` with no experimental
  define.

  `execution/` + `thread.stoptoken/` + `module_std.gen.py`/
  `transitive_includes.gen.py` 197/198 green (same pre-existing
  1-unsupported baseline, no diff despite the four new headers),
  `libcxx-generate-files` clean. Full registration for all four (new
  headers ⇒ `CMakeLists.txt` + `<execution>`'s include block +
  `execution.inc`). `Cxx2cPapers.csv` untouched (flips at M6 only).

- **2026-08-22 (second entry)**: Finished `on` — added the 3-arg
  pipeable-closure form (`on(sndr, sch, closure)`, [exec.on]p1.2/p4/p8)
  and its 2-arg partial-application form (`on(sch, closure)`, so
  `sndr | on(sch, closure)` equals `on(sndr, sch, closure)`, matching
  every other pipeable adaptor's single-arg-overload convention but with
  two bound arguments via `std::__bind_back` instead of one). New
  `__on2_sndr<_Tag, _Sndr, _Sch, _Closure>` in `<__execution/on.h>`,
  same `tag`/`data`/`child` shape as `__on_sndr` (`data` here is
  `tuple<_Sch, _Closure>`, matching [exec.on]p4's literal
  `product-type{sch, closure}`). `connect()` computes
  `get_completion_scheduler<set_value_t>(get_env(child), get_env(rcvr))`
  directly (matching [exec.on]p8.1's literal wording, no
  `call-with-default` fallback — same established pattern as the 2-arg
  form's `get_start_scheduler` lookup) and builds
  `continues_on(closure(continues_on(child, sch)), orig_sch)` right there;
  `get_completion_scheduler` must be looked up from `sndr`'s own
  environment *before* `sndr` is moved into `continues_on`, so it's its
  own statement rather than inlined into the `return`, where argument
  evaluation order would be unspecified. Disambiguating the 2-arg
  overload set (`on(sch, sndr)` vs. `on(sch, closure)`) needed no extra
  machinery: `sender` and `__sender_adaptor_closure_object`
  (`<__execution/sender_adaptor_closure.h>`) are already mutually
  exclusive by construction, so ordinary overload-constraint SFINAE
  reproduces [exec.on]p2.2/p2.3's exclusion for free.

  **Mandate discovered while writing the test, not obvious from the
  wording alone:** `on(sndr, sch, closure)` genuinely requires `sndr`'s
  own attributes to answer `get_completion_scheduler<set_value_t>`
  directly — a bare `just(42)` does *not* satisfy this (its env is
  `env<>`, which answers nothing), and `get_completion_scheduler_t`'s own
  fallback path (`static_assert(scheduler<_Q>, ...)`) would fail loudly
  for it. This is semantically correct, not a bug: the whole point of the
  3-arg form is "remember the scheduler `sndr` completes on", which is
  only meaningful for a sender that actually carries scheduler affinity.
  `schedule(some_run_loop.get_scheduler())` is the one sender in this
  fork's `execution/` subsystem whose env answers this query
  ([exec.run.loop.types]p5), so it's what the test uses as `sndr`.

  **Real, empirically-caught test bug — a genuine deadlock, not a
  reasoning error caught before running anything:** the first draft of
  both new test blocks called `run_loop::run()` a second time on an
  already-fully-drained loop without calling `finish()` again first, and
  the resulting binary hung forever (caught by literally running it, with
  a hard `timeout`, after the built-in review process's "would this
  actually work" reasoning said it should be fine). Root cause:
  `__pop_front()`'s wait predicate only unblocks on `state == __finishing`;
  the moment a loop's queue empties, it downgrades state to the *distinct*
  `__finished` value, and nothing except another `finish()` call moves it
  back to `__finishing`. So a loop that has already drained to empty once
  needs `finish()` called again before it can be safely `run()` a second
  time, even though the *item itself* is already sitting in the queue by
  then. Fixed by re-`finish()`-ing before every subsequent `run()` call
  (including, for the polling-loop version of the pipe-syntax test,
  inside the loop body itself, on every iteration — calling `finish()` on
  an already-empty loop is a harmless no-op, so this is always safe
  regardless of which loop actually has pending work that iteration).
  This is a genuine trap in `run_loop`'s own API (present since M4,
  unrelated to `on` or `continues_on`'s own logic) that any future
  multi-hop, multi-`run()`-call test against the same loop should watch
  for — record here since this is the first test in the sub-plan to
  actually call `run()` more than once per loop.

  Tests: extended `exec.adapt/exec.on/on.pass.cpp` with the 3-arg direct
  call (three-phase drain across two distinct `run_loop`s, discriminating
  each hop the same way the 2-arg form's test does) and the 2-arg
  pipe-equivalence form (drain-until-done polling loop). Verified by
  actually running the compiled binary with a `timeout` wrapper before
  trusting the reasoning, both while broken (confirmed the hang) and
  after the fix (confirmed it completes). `execution/` +
  `thread.stoptoken/` + `module_std.gen.py`/`transitive_includes.gen.py`
  197/198 green (unchanged baseline), `libcxx-generate-files` clean, no
  `-D_LIBCPP_ENABLE_EXPERIMENTAL` compile also re-verified for the
  updated `on.h`. No new header ⇒ no `CMakeLists.txt`/`<execution>`
  changes needed, only `on.h` and its test changed. `Cxx2cPapers.csv`
  untouched.

  **`on` is now fully implemented — M5's scheduler-affinity family
  (`schedule_from`/`continues_on`/`starts_on`/`on`) is complete. Next
  session: continue M5** with the remaining adaptors (`when_all`/
  `when_all_with_variant`/`into_variant`/`stopped_as_optional`/
  `stopped_as_error`/`write_env`/`unstoppable`/`bulk`/`bulk_chunked`/
  `bulk_unchunked`). `write_env`/`unstoppable` are genuinely small (both
  are one-clause "expression-equivalent to X" definitions with no new
  opstate) — a good pairing for a short session. Before writing any more
  adaptors whose receiver captures more than one completion tag into a
  type-dependent variant, re-read this file's run_loop-forced-
  set_stopped() finding (first M5 entry, 2026-08-22); it's not
  `continues_on`-specific. And before writing any test that calls
  `run_loop::run()` more than once on the same loop, re-read this
  entry's `finish()`-must-be-re-armed finding.
- **2026-08-22 (third entry)**: Implemented `write_env`/`unstoppable`
  (new `<__execution/write_env.h>`, `<__execution/unstoppable.h>`).
  `write_env`'s `impls-for<write-env-t>::join-env(state, env)`
  ([exec.write.env]p4: `e.query(q)` is `state.query(q)` if valid, else
  `env.query(q)`) needed no new queryable-combinator type — it's exactly
  `execution::env<_Envs...>`'s existing first-match-wins forwarding
  (`<__execution/env.h>`), so `__write_env_join(state, env)` is just
  `execution::env(state, env)`. `unstoppable(sndr)` is literally
  `write_env(sndr, prop(get_stop_token, never_stop_token{}))`
  ([exec.unstoppable]p2) — the whole file is a four-line CPO plus a
  paragraph of ordering/threading commentary.

  **Pipe-form question, resolved with a concrete control case, not just
  reasoning:** neither clause says "denotes a pipeable sender adaptor
  object" (the phrase [exec.then]/[exec.on]/[exec.let] use to grant the
  `adaptor(args...)` partial-application overload per
  [exec.adapt.obj]p4-5) — only "is a customization point object". Read
  in isolation, [exec.adapt.obj]p4's *definition* ("a customization
  point object that accepts a sender as its first argument and returns
  a sender") looks like it could auto-grant the classification to
  anything shaped that way, which would make `write_env`/`unstoppable`
  pipeable regardless of the missing phrase. Settled by checking
  [exec.schedule.from]p1 (already implemented, `<__execution/
  schedule_from.h>`, no pipe support): it uses the *identical*
  "denotes a customization point object" phrasing for a single-
  argument, sender-first CPO that would trivially satisfy p4's shape
  and would therefore have to be unconditionally pipeable if the shape
  alone were sufficient (p4's last sentence: a one-argument pipeable
  sender adaptor object *is* a pipeable sender adaptor closure object,
  no partial application even needed) — and it isn't implemented that
  way. So p4 defines the *term*; a clause's own "denotes a pipeable
  sender adaptor object" is the actual per-CPO grant. `write_env`/
  `unstoppable` are therefore call-only: no `write_env(env)` closure,
  no `sndr | write_env(env)`, no `sndr | unstoppable`. Full reasoning
  and the `schedule_from` control case are recorded in
  `<__execution/write_env.h>`'s header comment (long — read it before
  re-litigating this for a future adaptor).

  **Real pre-existing bug found and fixed, one layer down:**
  [exec.unstoppable]p2's own canonical implementation
  (`write_env(sndr, prop(get_stop_token, never_stop_token{}))`) didn't
  compile against this fork's `get_stop_token_t`
  (`<__execution/get_stop_token.h>`). Root cause: `prop::query()` is
  specified ([exec.prop]) to return `const ValueType&` (a genuine
  reference, not a decayed copy), but `get_stop_token_t::operator()`'s
  Mandates check was `static_assert(stoppable_token<decltype(__env.query(*this))>,
  ...)` — no `remove_cvref_t`, so for any env answering via `prop` this
  checked `stoppable_token<const T&>`, which is *unconditionally false*
  (`stoppable_token` requires `copyable`, which requires
  `movable`/`is_object_v`, false for every reference type). That would
  make `prop(get_stop_token, tok)` permanently unusable, contradicting
  the standard's own canonical `unstoppable` composition — so this was
  a bug in `get_stop_token_t`, not in `prop` or in this session's new
  code. Fixed with one `remove_cvref_t` (matches the `auto`, not
  `decltype(auto)`, return type on the same function, which already
  decays the same way in the actual `return` statement — only the
  static_assert had the mismatch). This is a **separate commit** from
  the `write_env`/`unstoppable` addition, since it changes a shared
  query CPO used by `run_loop`, `sync_wait`, and every future stop-
  token-aware adaptor.

  Grepped `__execution/*.h` for the same missing-decay shape
  (`static_assert` / `{ expr } -> Concept` checking a query's result
  type without stripping references) to see whether this is one bug or
  a class of them: `get_completion_scheduler_t`/`get_scheduler_t`
  (`get_scheduler.h`) are **not** affected — they deduce a template
  parameter from a by-value-ish `const _Q&` function parameter rather
  than taking `decltype` of a call expression, so reference-ness is
  already stripped by deduction. `get_allocator_t` (`get_allocator.h`)
  and `get_forward_progress_guarantee_t`
  (`get_forward_progress_guarantee.h`) **are** structurally the same
  shape as the `get_stop_token_t` bug (`{ __env.query(__self) } ->
  Concept`, with `__simple_allocator`/`same_as<forward_progress_
  guarantee>` as the concept) — both would plausibly break the same
  way if ever answered via `prop(get_allocator, some_alloc)` or
  `prop(get_forward_progress_guarantee, guarantee)`, since `copyable`/
  `__simple_allocator`'s `copy_constructible` both bottom out in the
  same `is_object_v` requirement. **Not fixed this session** (out of
  scope, unconfirmed by an actual failing test) — flagged for whoever
  next touches either of those two files.

  **Test-writing trap, worth remembering for any future test that uses
  `read_env` as a probe child:** a query object with an unconstrained
  `const auto&` parameter and a *fixed* (non-deduced) return type
  makes `read_env`'s own `get_completion_signatures` constraint
  (`requires { { _Query()(__env) }; requires !is_void_v<...>; }`)
  vacuously true for *any* env, including one that doesn't actually
  answer the query — forming the call never needs the body
  (`env.query(...)`) to compile, only *invoking* it would fail, and
  `get_completion_signatures` only ever does `decltype(...)`/
  `noexcept(...)` on the call (both unevaluated contexts). First hit
  while trying to write a negative (`!sender_in`) check for
  `write_env`'s joined-environment `get_completion_signatures` formula
  ([exec.write.env]p5) using the same permissive query object the
  positive/runtime checks used. Fix pattern used in
  `write_env.pass.cpp`: keep the permissive query object (`get_value_t`)
  for runtime tests, and add a *second*, genuinely constrained caller
  (`read_value_t`, `requires requires(const _Env& e) { e.query(get_value); }`)
  purely for the type-level discrimination checks — referencing the
  already-complete `get_value` object rather than constructing a fresh
  `get_value_t{}` inside its own class's constraint (which would need
  `get_value_t` complete at a point inside its own definition).

  Also hit, unrelated to the above: `__write_env_sndr`'s `tag` member
  is `__write_env_t tag;` — a *non-dependent* by-value field naming the
  CPO's own type directly (write_env has only one CPO, unlike
  then/on/schedule_from's `_Tag`-templated senders). A non-dependent
  by-value member must be a complete type at the point its enclosing
  class *template* is defined, not deferred to instantiation the way a
  dependent member would be — so `__write_env_t` had to be fully
  defined *before* `__write_env_sndr` in the header (matching
  `<__execution/read_env.h>`'s existing ordering, for the same reason).

  Tests: new `exec.adapt/{exec.write.env,exec.unstoppable}/*.pass.cpp`.
  `write_env.pass.cpp` covers: state-overrides-outer-env priority,
  fallback-to-outer-env when state doesn't answer, sender-level
  attributes coming from the child only (not the written env), the
  no-partial-application-form static_assert, and three joined-env
  `get_completion_signatures` static_asserts (state-answers,
  neither-answers ⇒ `!sender_in`, only-outer-answers-via-fallback).
  `unstoppable.pass.cpp` covers: the child seeing `never_stop_token`
  regardless of the outer receiver's own (real, stoppable)
  `inplace_stop_token`, plus a completion-signatures cross-check
  against `write_env` directly, tying [exec.unstoppable]p2's
  expression-equivalence to the implementation.

  `execution/` + `thread.stoptoken/` 74/74 green,
  `module_std.gen.py`/`transitive_includes.gen.py` 125/126 (same
  pre-existing 1-unsupported baseline, no diff despite the two new
  headers), `libcxx-generate-files` clean (no feature-test-macro
  change — `__cpp_lib_senders` stays gated to M6), no
  `-D_LIBCPP_ENABLE_EXPERIMENTAL` compile re-verified for both new
  headers. New headers registered in `CMakeLists.txt`,
  `<execution>`'s M5 include block, and the C++26-guarded exports in
  `libcxx/modules/std/execution.inc` (`unstoppable`'s export gated on
  `_LIBCPP_HAS_THREADS`, matching `get_stop_token.h`'s own gating).
  `Cxx2cPapers.csv` untouched (flips at M6 only).

  **Next session: continue M5** with the remaining adaptors
  (`when_all`/`when_all_with_variant`/`into_variant`/
  `stopped_as_optional`/`stopped_as_error`/`bulk`/`bulk_chunked`/
  `bulk_unchunked`). `stopped_as_optional`/`stopped_as_error` are
  single-child, single-completion-tag-rewrite adaptors similar in
  shape to `then`/`upon_error` — likely the next good small pairing.
  `when_all`/`when_all_with_variant` are the first *multi*-child
  adaptors in this sub-plan (M3/M4 and M5 so far have all been single-
  or dual-child) and will need real new machinery for joining multiple
  child operation states and completion-signature sets — budget more
  time for that one than for the others.

- **2026-08-22 (Tier 2, M5 continued — stopped_as_optional/stopped_as_error)**:
  Implemented `[exec.stopped.opt]`/`[exec.stopped.err]` (new
  `libcxx/include/__execution/{stopped_as_optional,stopped_as_error}.h`).
  `stopped_as_error(sndr, err)` is a pure call-time composition (like
  `starts_on`/`on`): `let_stopped(sndr, [err]() mutable noexcept(...) {
  return just_error(std::move(err)); })`, expression-equivalence verbatim,
  no new sender type needed since its result doesn't depend on Env.
  `stopped_as_optional(sndr)` is not: [exec.stopped.opt]p3's
  transform_sender body needs `V = single-sender-value-type<child, Env>`
  to build both `let_stopped(then(child, [](Ts...){ return
  optional<V>(in_place, ts...); }), []{ return just(optional<V>()); })`
  branches with a *matching* V, and Env isn't known until connect()/
  get_completion_signatures() see a real receiver/queried environment —
  so (unlike every other composed-at-call-time M5 adaptor so far) this
  needed its own hand-rolled `__stopped_as_optional_sndr<Sndr>`, pinning V
  once Env is available and building the composed sender against it in
  both connect() (from `env_of_t<Rcvr>`) and get_completion_signatures()
  (from the `_Env` template parameter) — same "not routed through
  impls-for/make-sender" precedent as every other M5 adaptor. Also added
  `single-sender-value-type`/the exposition-only `single-sender` concept
  to `<__execution/get_completion_signatures.h>` (`__single_sender_value_type`/
  `__single_sender`) as shared machinery, since both this and (per M5's
  remaining list) `bulk`/`into_variant` need it.

  **Compiler-behavior finding, worth remembering for any future gather-
  signatures-style trait:** the standard's own single-sender-value-type
  (2.1) alternative is `gather-signatures<set_value_t, CS, decay_t,
  type_identity_t>` — `decay_t<Args...>` applied per-signature, which is
  ill-formed whenever a signature's arity isn't exactly one. The first
  implementation attempt used this literally (via an overload-priority
  `int`/`long`/`...` SFINAE dispatch, expecting the ill-formed
  `decay_t<>`/`decay_t<A,B>` to just drop that candidate) and it does
  *not* SFINAE away — it's a **hard compile error**. Reason: forming
  `decay_t<Args...>` happens inside `__gather_one`'s implicit
  class-template instantiation (`<__execution/completion_signatures.h>`),
  not in the immediate context of the outer alias-template substitution
  being probed — so the "immediate context only" SFINAE rule doesn't
  cover it, the same way M1's `requires{}`-on-concrete-objects finding
  didn't. Confirmed by an actual `libcxx-lit` build failure (not just
  reasoning), showing the error nested ~13 frames deep through
  `__gather_signatures_impl`/`__gather_one`/`__meta_apply`. Fixed by
  computing entirely through `__decayed_tuple` (always well-formed, any
  arity) and gathering into a `type_list` of one tuple per signature,
  then extracting the final answer via ordinary (genuinely SFINAE-safe)
  partial specialization on that `type_list`'s shape
  (`__single_sender_value_type_impl`) — never calling anything ill-formed
  at any arity. Generalize: prefer "gather into something always
  well-formed, then pattern-match the shape" over "gather directly with
  the possibly-ill-formed target trait" whenever a gather-signatures
  `_Tuple`/`_Variant` argument might not accept every arity/count it
  could see.

  A second, unrelated hazard surfaced only by this session's own tests
  (not fixed — flagged for whoever next touches `sync_wait`):
  `<__execution/sync_wait.h>`'s own `__sync_wait_result_type` computes
  `value_types_of_t<Sndr, Env, __decayed_tuple, type_identity_t>`
  directly — the exact same `type_identity_t`-applied-to-a-gathered-list
  shape, hitting the exact same hard-error-not-SFINAE landmine whenever
  `sync_wait` is called on a sender with zero set_value completions at
  all (e.g. `sync_wait(stopped_as_error(just_stopped(), err))` alone,
  before this session's tests were adjusted to route through a
  value-carrying test sender instead). Pre-existing, not introduced this
  session; `sync_wait`'s own Mandates already require a single-sender in
  principle, so this should eventually reuse
  `__single_sender_value_type` above rather than recomputing the same
  fragile shape inline — not done here since it's out of scope for a
  stopped_as_optional/stopped_as_error session and no existing test
  exercised the gap.

  Tests: new `exec.adapt/{exec.stopped.opt,exec.stopped.err}/*.pass.cpp`,
  each with a small hand-rolled `value_or_stopped_sndr` (single set_value
  + set_stopped completion, runtime-switchable) to exercise both branches
  through `sync_wait` without hitting the `sync_wait` landmine above.
  Full `execution/` suite 39/39 green (up from 37 — the two new files);
  `libcxx-generate-files` clean (no feature-test-macro change —
  `__cpp_lib_senders` stays gated to M6). New headers registered in
  `CMakeLists.txt`, `<execution>`'s M5 include block, and
  `libcxx/modules/std/execution.inc` (both CPO and `_t` type exported,
  unlike `write_env`/`unstoppable` — [execution.syn] names
  `stopped_as_optional_t`/`stopped_as_error_t` outright).

  **Next session: continue M5** with the remaining adaptors (`when_all`/
  `when_all_with_variant`/`into_variant`/`bulk`/`bulk_chunked`/
  `bulk_unchunked`). `into_variant` is probably the next good small one
  (single-child, and `sync_wait_with_variant`'s own still-unimplemented
  M4 stub — docs/CXX26_GAPS.md's M4 entry — is specified directly in
  terms of it, so landing it also unblocks that). `when_all`/
  `when_all_with_variant` remain the first genuinely multi-child
  adaptors — see the previous session's note; still budget extra time
  there. `bulk`/`bulk_chunked`/`bulk_unchunked` haven't been scoped in
  detail yet this sub-plan; read `[exec.bulk]` fresh via the `curl`
  process rule before starting.
- **2026-08-22 (Tier 2, M5 continued — into_variant)**: Implemented
  `[exec.into.variant]` (new `libcxx/include/__execution/into_variant.h`).
  `into_variant(sndr)` maps every `set_value_t(Args...)` completion into one
  combined `set_value_t(V)`, where `V` is `value_types_of_t<child, Env>`
  called with its *default* Tuple/Variant arguments
  (`__decayed_tuple`/`__variant_or_empty`) — this is exactly the standard's
  own `V`, so no new gathering machinery was needed, just reusing
  `<__execution/get_completion_signatures.h>`'s existing alias directly.
  `set_error_t`/`set_stopped_t` completions pass through unchanged. Same
  "V depends on Env, not known until connect()/get_completion_signatures()"
  shape as `stopped_as_optional` (previous session) — hand-rolled sender
  type, not a call-time composition, `V` pinned independently in both
  `connect()` (from `env_of_t<Rcvr>`) and `get_completion_signatures()`
  (from `_Env`).

  Completion-signature transform (`__into_variant_signatures`) follows
  `<__execution/then.h>`'s/`<__execution/let.h>`'s established
  `__one<_Sig>`-partial-specialization-plus-`__concat_type_lists` shape, but
  simpler: no dedup needed, since `set_value_t` signatures are dropped
  outright (each subsumed by the single prepended `set_value_t(V)`) rather
  than remapped-and-deduped. The receiver (`__into_variant_rcvr`) converts
  `Args...` to `V` via a two-step `__decayed_tuple<Args...>` →
  `V`-converting-constructor, matching `[exec.into.variant]p6`'s
  `variant_type(decayed-tuple<Args...>{args...})` verbatim; TRY-SET-VALUE
  semantics hand-written (try/catch → `set_error(current_exception())`),
  matching every prior adaptor's convention since this fork has no shared
  TRY-SET-VALUE macro.

  Hit the same class-completeness ordering constraint
  `<__execution/stopped_as_optional.h>`/`<__execution/write_env.h>` already
  document: `__into_variant_sndr`'s non-dependent `into_variant_t tag`
  member needs `into_variant_t` complete at the sndr class's own definition
  point, but `into_variant_t::operator()`'s body needs `__into_variant_sndr`
  complete too — broken by declaring `into_variant_t` in full first
  (`operator()` only declared, trailing-return-type-only reference to the
  not-yet-complete sndr type, which is fine), defining
  `__into_variant_sndr` in full second, then defining
  `into_variant_t::operator()`'s body out-of-line last. First attempt got
  this backwards (sndr class first, referencing an incomplete
  forward-declared `into_variant_t` as a by-value member) and would not
  have compiled had it not been caught by rereading the two existing
  files' own ordering comments before writing the test.

  New test: `exec.adapt/exec.into.variant/into_variant.pass.cpp`, with a
  hand-rolled `multi_value_sndr` (two distinct value-completion shapes —
  `set_value_t(int)` and `set_value_t(double, char)` — plus an error and a
  stopped completion, runtime-switchable) since no existing factory
  advertises more than one value shape at once. Covers both variant
  alternatives via `sync_wait`, error passthrough (caught as a rethrown
  `int` through `sync_wait`'s `AS-EXCEPT-PTR`), stopped passthrough, call-
  vs-pipe-syntax equivalence, the full completion-signatures static-assert
  (two value shapes collapsing into one `set_value_t(variant<...>)` plus the
  untouched `set_error_t`/`set_stopped_t`), and a zero-value-completion
  sender (`just_stopped()`) still typechecking as `sender_in` (its `V` is
  `__empty_variant`, never actually constructed). `execution/` suite
  39/39 → 40/40 green; `thread.stoptoken/` unaffected; `libcxx-generate-files`
  clean (no FTM change — `__cpp_lib_senders` stays gated to M6);
  `module_std.gen.py`/`transitive_includes.gen.py` 125/126 (same
  pre-existing 1-unsupported baseline, unchanged — no new transitive
  includes). Registered the new header in `CMakeLists.txt`, `<execution>`'s
  M5 include block, and `libcxx/modules/std/execution.inc` (both CPO and
  `_t` type exported — `[execution.syn]` names `into_variant_t` outright,
  same as `stopped_as_optional_t`/`stopped_as_error_t`).

  **Next session: continue M5** — only `when_all`/`when_all_with_variant`
  and `bulk`/`bulk_chunked`/`bulk_unchunked` remain. `when_all` is the
  first genuinely multi-child adaptor in this sub-plan (every adaptor
  through this session has had exactly one child sender) — budget extra
  time for it per the prior session's own flag; read `[exec.when.all]`
  fresh via the `curl` process rule before starting, and expect the
  operation-state shape (N child operation states, synchronized completion
  via an atomic counter or similar) to need real new machinery, not a
  reuse of any existing one-child adaptor's shape.
- **2026-08-22 (Tier 2, M5 continued — when_all)**: Implemented `[exec.when.all]`
  (new `libcxx/include/__execution/when_all.h`, ~500 lines) — `when_all` only;
  `when_all_with_variant` (a pure call-time composition,
  `when_all(into_variant(sndrs)...)`, per p18/p19) deliberately deferred to a
  follow-up commit on the advisor's recommendation, to get a working
  checkpoint landed first given the size of this milestone. Also added
  `stop_callback_for_t<T, CallbackFn>` (new alias in
  `libcxx/include/__stop_token/stoppable_token.h`, exported from
  `<stop_token>`/`stop_token.inc`) — a real M1 gap discovered while scoping
  this session: `[thread.stoptoken.syn]` declares it alongside
  `stoppable_token`/`unstoppable_token` (confirmed via the `curl`+strip
  process against `eel.is/c++draft/thread.stoptoken.syn`), but M1's original
  pass only added the two concepts. Needed here for `__when_all_state`'s
  `on_stop` member (`optional<stop_callback_for_t<stop_token_of_t<Env>,
  __when_all_on_stop_request>>`). Added test coverage to the existing
  `stoptoken.concepts/concepts.pass.cpp` rather than a new file (same header,
  same clause).

  **Design, confirmed with the advisor before writing code:** `values_tuple`/
  `errors_variant`/the value completion signature are all computed by
  classifying each child's own `completion_signatures` via ordinary partial
  specialization on the *always-well-formed* shape gathered by
  `value_types_of_t<..., __decayed_tuple, type_list>` (0, 1, or N elements,
  never ill-formed regardless of arity) — never by naming
  `value_types_of_t<..., optional>` directly (`optional` takes exactly one
  type argument; a child with 0 or 2+ set_value shapes makes that formation
  hard-error, not SFINAE-fail, deep inside `__gather_signatures_impl`'s
  implicit instantiation, the same "gather into something always
  well-formed, then pattern-match" lesson `__single_sender_value_type`
  already established). Both 0-shape and 2+-shape children collapse into the
  *same* "otherwise tuple<>" fallback per [exec.when.all]p13's literal
  wording — check-types' Mandates-diagnostic for the 2+ case specifically is
  not implemented (same P3068 gap as every other adaptor).

  **`copy-fail`** ([exec.when.all]p12) scans every child's *both* value and
  error datums for nothrow-decay-copyability, independent of the
  all-single-value classification above — confirmed necessary (not just
  simpler) by re-reading p12/p17 together: TRY-EMPLACE-ERROR's own fallback
  assumes `exception_ptr` is always a valid `errors_variant` alternative
  whenever an error datum's own decay-copy might throw, which only holds if
  copy-fail's scan covers error datums too, not just the ones feeding
  `values_tuple`.

  **Real engineering hazard, caught empirically (first full build attempt
  failed exactly here):** constructing `tuple<OpState_1, ..., OpState_N>`
  directly from N `execution::connect(...)` calls passed through tuple's own
  variadic `(_Up&&... u)` constructor defeats guaranteed copy elision --
  materializing each `connect()` call's prvalue result through a forwarding-
  reference parameter -- and every operation state in this tree (matching
  `__just_opstate`/`__let_opstate`) has copy/move explicitly deleted, so this
  is a hard compile error, not a silent extra copy. Exact same hazard
  `<__execution/let.h>`'s `__emplace_from` already documents at length for
  `variant::emplace<T>(...)`; fixed with a local `__when_all_emplace_from`
  wrapper (identical shape: `operator T() &&` calling a factory closure) so
  the *wrapper* -- cheap and trivially movable -- is what passes through
  tuple's constructor hops, and the actual non-movable `OpState` is only
  produced at the final direct-initialization step, which mandatory elision
  does cover. Confirms the pattern generalizes beyond `variant::emplace` to
  any "construct N non-movable objects from N factory calls" site.

  **Per-child receiver** (`__when_all_rcvr<Index, State, Rcvr>`) stores
  direct pointers to `State`/`Rcvr` (both already-complete, ordinary types at
  the point children are connected), not a pointer back to the enclosing
  `__when_all_opstate` class template -- deliberately avoiding the
  incomplete-type-during-constraint-checking hazard `<__execution/let.h>`'s
  own "real engineering hazard" note (M5, previous entries) warns about for
  exactly this shape.

  **when-all-env** (`__when_all_env<Env>`, [exec.when.all]p5-7): modeled
  directly on `<__execution/fwd_env.h>`'s FWD-ENV query-forwarding shape,
  with `get_stop_token` intercepted ahead of the generic forward (answering
  with this operation's own `inplace_stop_source`'s token, not whatever the
  outer environment would otherwise answer) -- this is the actual mechanism
  by which requesting stop on one child's error/stopped completion reaches
  every other still-running child's environment.

  **Verified under a genuine multi-child stopped/error case, not just the
  sequential happy path** (per the advisor's explicit ask): the new test's
  last case uses a hand-rolled `error_sndr` (completes `set_error(99)`
  synchronously on `start()`) alongside a `stop_check_sndr` that records,
  via a caller-owned `bool*`, whether `get_stop_token(get_env(rcvr))` already
  reports `stop_requested()` at the moment *it* starts -- since `when_all`
  starts every child unconditionally via a fold expression
  (`(execution::start(ops), ...)`, no short-circuit), and both complete
  synchronously, `error_sndr` (declared first) fully completes -- including
  requesting stop on the shared `inplace_stop_source` -- before
  `stop_check_sndr`'s own `start()` runs; the test asserts the flag came
  back `true`, directly confirming the propagation path works end-to-end,
  not just that the types compile.

  New test: `exec.adapt/exec.when.all/when_all.pass.cpp` -- value-datum
  concatenation across two children (`when_all(just(1,2), just(3.5))` →
  `tuple(1,2,3.5)`, both at runtime via `sync_wait` and statically via a
  `completion_signatures_of_t` assert), the zero-shape collapse case
  (`when_all(just_stopped(), just(1))` → stopped, no value, completion
  signature `<set_value_t(), set_stopped_t()>`), a no-error/no-stopped-
  capable case showing copy-fail correctly stays `none-such` (no
  `set_error_t(exception_ptr)` advertised when every datum is trivially
  nothrow-copyable), and the error+stop-propagation case above. `execution/`
  suite 40/40 → 41/41 green, `thread.stoptoken/` 37/37 → 37/37 (same count,
  new assertions added to an existing file), 78/78 total, no regressions.
  `libcxx-generate-files` clean (no FTM change -- `__cpp_lib_senders` stays
  gated to M6). `transitive_includes/cxx26.csv` needed one real addition
  this time (first since M4): `execution` now also transitively pulls in
  `atomic` (from `<__stop_token/inplace_stop_source.h>`'s/`__when_all_state`'s
  own `atomic<size_t>`/`atomic<disposition>` members) -- confirmed via the
  actual preprocessor trace (`transitive_includes.gen.py`'s diff output), not
  guessed; `module_std.gen.py`/`transitive_includes.gen.py` both 125/126
  after the fix (same pre-existing 1-unsupported baseline). Registered the
  new header in `CMakeLists.txt`, `<execution>`'s M5 include block, and
  `libcxx/modules/std/execution.inc` (both `when_all`/`when_all_t` exported,
  guarded `#if _LIBCPP_HAS_THREADS` matching `unstoppable`'s own precedent,
  since `when_all.h` itself is gated `_LIBCPP_STD_VER >= 26 &&
  _LIBCPP_HAS_THREADS` — it uses `inplace_stop_source` unconditionally, same
  as `get_stop_token.h`). `Cxx2cPapers.csv` untouched (flips at M6 only).

  **Next session: `when_all_with_variant`** — should be small (pure
  call-time composition over `when_all`+`into_variant`, both now landed; no
  new sender/receiver/state machinery expected, matching
  `stopped_as_error`'s/`starts_on`'s own call-time-composition precedent
  rather than `stopped_as_optional`'s/`into_variant`'s/this session's
  hand-rolled-sender precedent). After that, only
  `bulk`/`bulk_chunked`/`bulk_unchunked` remain in M5 — read `[exec.bulk]`
  fresh via the `curl` process rule before starting; unscoped in detail so
  far this sub-plan.
- **2026-08-22 (Tier 2, M5 continued — when_all_with_variant)**: Implemented
  `[exec.when.all]`'s second CPO (added to the existing
  `libcxx/include/__execution/when_all.h`, same clause as `when_all`).
  Confirmed the prediction from the previous entry: this needed no new
  sender/receiver/state machinery at all. `operator()` returns
  `when_all(into_variant(sndrs)...)`'s own concrete type directly, matching
  `stopped_as_error_t`'s/`starts_on_t`'s established call-time-composition
  shape — not the standard's literal `make-sender(when_all_with_variant, {},
  sndrs...)` + domain-based `transform_sender` customization, which would
  rely on `tag_of_t<Sndr>().transform_sender(...)` actually firing, and
  `<__execution/domain.h>`'s M2 deviation 4 permanently disables exactly that
  branch in `default_domain::transform_sender` on this fork (always takes
  the "otherwise" static_cast path). Same `tag_of_t`/`sender_for` deviation
  as `stopped_as_error`/`starts_on`: the result's tag is `when_all_t`'s, not
  `when_all_with_variant_t`'s — undocumented anywhere new, just the third
  instance of an already-recorded pattern.

  New test: `exec.adapt/exec.when.all/when_all_with_variant.pass.cpp` —
  confirms `when_all_with_variant(just(1,2), just(3.5))` produces
  `tuple(variant<tuple<int,int>>(tuple(1,2)), variant<tuple<double>>(tuple(3.5)))`
  via `sync_wait`, plus the matching `completion_signatures_of_t` static
  assert. `execution/` suite 41/41 → 42/42 green, `thread.stoptoken/`
  unaffected, 79/79 total. `libcxx-generate-files` clean; no new header, so
  `module_std.gen.py`/`transitive_includes.gen.py` needed no changes either
  (confirmed by rerunning both — still 125/126, unchanged). Registration:
  `libcxx/modules/std/execution.inc` only (both `when_all_with_variant`/
  `when_all_with_variant_t` exported, under the existing `#if
  _LIBCPP_HAS_THREADS` `[exec.when.all]` block) — no `CMakeLists.txt` or
  `<execution>` include-list change, since no new file. `Cxx2cPapers.csv`
  untouched (flips at M6 only).

  **M5 is now down to exactly one item: `bulk`/`bulk_chunked`/
  `bulk_unchunked`.** Once that lands, M5 is complete and M6 (coroutine
  integration: `as_awaitable`, `with_awaitable_senders`) is the only
  remaining milestone before `__cpp_lib_senders`/`P2300R10`/`P3325R5`/
  `P3396R1` all flip to `|Complete|` together. **Next session:** read
  `[exec.bulk]` fresh via the `curl` process rule (not yet scoped in detail
  this sub-plan) before starting `bulk`/`bulk_chunked`/`bulk_unchunked`.
- **2026-08-22 (Tier 2, M5 complete — bulk/bulk_chunked/bulk_unchunked)**:
  Implemented `[exec.bulk]` (new `libcxx/include/__execution/bulk.h`,
  ~370 lines) — the last M5 item. `bulk_chunked`/`bulk_unchunked` are
  hand-rolled one-child adaptors (own connect()/get_completion_signatures(),
  per the M3 precedent); `bulk` is a pure call-time composition over
  `bulk_chunked` (matching `when_all_with_variant`'s/`stopped_as_error`'s
  established shape — the standard's own `bulk.transform_sender(...)`
  domain-dispatch mechanism is exactly the branch
  `<__execution/domain.h>`'s M2 deviation 4 permanently disables in this
  fork). `bulk_chunked`'s own default behavior — invoking `f` exactly once
  with the *whole* `[0, shape)` range — isn't a fork-specific simplification;
  it's literally what `[exec.bulk]p5`'s own `impls-for<bulk_chunked_t>`
  reference lambda does (`f(Shape(0), shape, args...)`, unconditionally,
  no chunking loop of its own) — there's no parallel-scheduler machinery in
  scope through M5 to make bulk_chunked actually invoke `f` more than once
  per completion, so this fork's `Policy` parameter is accepted (and
  Mandates-checked via `is_execution_policy_v`) but not yet exploited for
  parallelism, consistent with `[exec.par.scheduler]`/
  `parallel_scheduler_replacement` being explicitly out of scope for this
  whole sub-plan (see the Tier 2 scope-collapse note above).

  `Policy` storage ([exec.bulk]p3's own rule, not simplified): every
  concrete execution policy this fork ships (`execution::seq`/`par`/
  `par_unseq`/`unseq`, under `_LIBCPP_HAS_EXPERIMENTAL_PSTL` in
  `<execution>`) explicitly deletes its copy constructor, so `__bulk_data`
  stores `Policy` by value only if `copy_constructible`, else `const
  Policy&` — in practice always the reference branch, safe because every
  real policy is an `inline constexpr`, static-duration singleton.

  **Two real bugs caught and fixed during this session, both self-inflicted
  (not compiler limitations) but both instances of the same "ternary
  operands aren't SFINAE-protected the way if-constexpr branches are"
  mistake:**
  1. The pipe-form (3-arg) overloads' first attempt used
     `std::__bind_back(*this, policy, shape, f)` — `__bind_back` decay-copies
     every bound argument, and decay-copying a deleted-copy-ctor `Policy`
     hard-errors immediately. Fixed by writing a custom closure instead,
     but the *first* fix attempt used a lambda init-capture
     `[__policy = __bulk_policy_storage_t<_Policy>(...)]`, which still
     failed: init-capture always deduces the captured member's type via
     `auto`, degrading a reference-typed initializer to a value — so it
     tried to *copy* the policy into that auto-deduced value member anyway.
     Final fix: capture one whole `__bulk_data` object by value instead
     (its `policy` member has an *explicitly declared* type, never
     auto-deduced, so copying the enclosing struct just copies the
     reference, correctly rebinding rather than attempting to copy the
     referent).
  2. `__bulk_sig_transform`'s and `__bulk_rcvr::set_value`'s nothrow checks
     both originally used `_Chunked ? is_nothrow_invocable_v<Func&, Shape,
     Shape, Args&...> : is_nothrow_invocable_v<Func&, Shape, Args&...>` —
     a ternary, not `if constexpr`. A ternary's *untaken* operand is not
     SFINAE-protected; both are substituted regardless of the condition's
     value. For `bulk_t`'s own `new_f` (a *generic* lambda whose `auto&...
     vs` parameter silently absorbs a mismatched argument count instead of
     failing to match), `is_nothrow_invocable_v` needs to instantiate the
     lambda's body to deduce its `auto` return type before it can answer
     the trait at all — and evaluating the *wrong*-arity operand
     instantiated `new_f`'s body with `vs` bound to the wrong split of
     arguments, calling the user's original two-argument callback with
     only one argument inside that body: a hard compile error from body
     instantiation, not a graceful SFINAE failure. Confirmed by an actual
     build failure (not just reasoning) on the `bulk_t` pipe-form test.
     Fixed by dispatching through an ordinary class-template partial
     specialization on `_Chunked` instead (`__bulk_nothrow_invocable`) — the
     untaken specialization's body is never instantiated at all, unlike a
     ternary's untaken operand. Same "immediate context" pitfall family
     recorded repeatedly elsewhere in this sub-plan (M1, M2 deviation 4),
     but self-inflicted here, not a compiler limitation being worked
     around.

  New test: `exec.adapt/exec.bulk/bulk.pass.cpp` (gated `UNSUPPORTED:
  libcpp-has-no-incomplete-pstl`, matching the existing PSTL test
  convention, since `execution::seq` etc. only exist under
  `_LIBCPP_HAS_EXPERIMENTAL_PSTL`) — covers `bulk_unchunked` (per-index
  invocation order and mutation visibility), `bulk_chunked` (single-chunk
  `[0, shape)` invocation), `bulk` (same per-index semantics as
  `bulk_unchunked`, reached through `bulk_chunked`'s machinery), call/pipe
  syntax equivalence, and a completion-signatures static assert (the
  child's `set_value_t(int)` passing through unchanged, plus the added
  `set_error_t(exception_ptr)` since a capturing lambda is never statically
  nothrow-invocable here). `execution/` suite 42/42 → 43/43 green,
  `thread.stoptoken/` unaffected, 80/80 total. `libcxx-generate-files`
  clean (no FTM change — `__cpp_lib_senders` stays gated to M6);
  `module_std.gen.py`/`transitive_includes.gen.py` both 125/126 (same
  pre-existing 1-unsupported baseline, unchanged — no new transitive
  includes). Registered the new header in `CMakeLists.txt`, `<execution>`'s
  M5 include block, and `libcxx/modules/std/execution.inc` (all six names —
  `bulk`/`bulk_t`, `bulk_chunked`/`bulk_chunked_t`,
  `bulk_unchunked`/`bulk_unchunked_t` — exported, per `[execution.syn]`
  naming all three types outright). `Cxx2cPapers.csv` untouched (flips at
  M6 only).

  **M5 is now complete.** Every sender factory, query, and adaptor in this
  sub-plan's scope (see the Tier 2 sub-plan's own scope list near the top)
  is implemented. **Only M6 remains**: coroutine integration
  (`as_awaitable`, `with_awaitable_senders`) — the M2 deviation 1 "awaitable
  disjunct omitted from enable-sender" and deviation 5 "connect-awaitable
  fallback not implemented" both get resolved there. Once M6 lands, flip
  `__cpp_lib_senders`'s `unimplemented` off and all three CSV rows
  (`P2300R10`/`P3325R5`/`P3396R1`) to `|Complete|` together, per this
  sub-plan's FTM discipline recorded at its start. **Next session: start
  M6** — re-read the M2 deviation notes (1 and 5) before starting, and the
  M1 process rule about verifying eel.is over any paper/summarizer text,
  since the coroutine-integration clauses (`[exec.as.awaitable]`,
  `[exec.with.awaitable.senders]`) haven't been fetched/scoped yet this
  sub-plan.

- **2026-08-22 (Tier 2, M6 started — M6a `[exec.awaitable]` foundation)**:
  Fetched and read `[exec.awaitable]`, `[exec.as.awaitable]`, and
  `[exec.with.awaitable.senders]` in full via the eel.is process rule.
  Confirmed scope: only 33.13.1-2 (`as_awaitable`,
  `with_awaitable_senders`) are in this sub-plan; 33.13.3-6 (`affine`,
  `inline_scheduler`, `task_scheduler`, `task`) belong to the separate
  P3552 paper, matching what this file already recorded. Also confirmed
  `[exec.awaitable]` (which defines `GET-AWAITER`/`is-awaiter`/
  `is-awaitable`/`env-promise`/etc.) is itself 33.9.4, under [exec.snd] —
  foundational sender machinery, not part of [exec.coro.util] — which is
  why `enable-sender`'s awaitable disjunct (M2 deviation 1) depends on it.

  Split M6 into three commits on advisor's recommendation, since the
  pieces have different risk profiles and two retrofit files
  (`sender.h`/`connect.h`) sit on the instantiation path of every test in
  `execution/`:
  - **M6a** (this entry): the exposition-only foundation, new file
    `libcxx/include/__execution/awaitable.h` — no public surface, touches
    nothing existing. Implements `GET-AWAITER` as two overloads of
    `__get_awaiter` (one/two-argument forms, matching the standard's own
    overload split) that simulate the compiler's own await-expression
    transformation: try the promise's `await_transform` first, then member
    `operator co_await`, then free `operator co_await`, then identity.
    Plus `__await_suspend_result`, `__is_awaiter`, `__is_awaitable`,
    `__await_result_type`, `__with_await_transform`, `__env_promise`.

  One deliberate divergence, called out in a comment at the point of use:
  the standard's `await_transform`-lookup step is "if this search is
  performed and finds at least one declaration, `a = p.await_transform(c)`"
  — meaning a promise declaring *some* `await_transform` member that isn't
  callable with this particular `c` should be a hard error, not a
  fallback to `a = c`. `requires{ p.await_transform(c); }` can't
  distinguish "no such member" from "member exists but unusable here";
  this implementation accepts that divergence rather than reproducing
  member-lookup fidelity — the same class of approximation already
  accepted for `__valid_completion_for` in `receiver.h`.

  Tested via `libcxx/test/libcxx/execution/awaitable.pass.cpp`, which
  includes the private header directly and exercises it with hand-written
  awaiter/promise types (no senders involved) — there's no public surface
  yet to test from `test/std`, matching the established `test/libcxx`
  precedent for exposition-only utilities (e.g. `__utility/no_destroy.h`'s
  own test). `libcxx-generate-files` clean (no FTM change — still gated at
  M6c); full `execution/`+`thread.stoptoken/` suite (81 tests) and the
  transitive-includes generator (125/125) both pass with no diff. New
  header registered in `CMakeLists.txt` and `<execution>`'s M6 include
  block (no `execution.inc` export — nothing here is public).
  `Cxx2cPapers.csv` untouched.

  **Next session: M6b** — `as_awaitable`, `with_awaitable_senders`, per
  the split above.

- **2026-08-22 (Tier 2, M6 continued — M6b `as_awaitable`/`with_awaitable_senders`)**:
  Implemented `libcxx/include/__execution/as_awaitable.h`
  (`__sender_awaitable`/`__awaitable_receiver`, `as_awaitable_t`) and
  `libcxx/include/__execution/with_awaitable_senders.h`
  (`with_awaitable_senders<Promise>`) — both public surface, both
  exported from `libcxx/modules/std/execution.inc`.

  `__sender_awaitable<Sndr, Promise>` follows this sub-plan's usual
  receiver shape (mirrors `sync_wait.h`'s `__sync_wait_receiver`): a
  `variant<monostate, result-type, exception_ptr>` result slot plus a
  `connect_result_t<Sndr, __awaitable_receiver>` operation state,
  constructed by direct member-initialization from a single `connect(...)`
  call — no `__emplace_from`-style elision wrapper needed here (unlike
  `when_all.h`'s N-way tuple case) since this is a single direct-init, not
  routed through an intermediate forwarding-reference constructor.
  `__awaitable_receiver::set_error` reuses `AS-EXCEPT-PTR`, which is now
  shared (moved from `sync_wait.h`, unconditionally available, into
  `completion_functions.h`) rather than duplicated — the "generalize once
  a second consumer needs it" note left on that helper when it was first
  written (M4) predicted exactly this.

  `as_awaitable_t`'s dispatch implements (7.1) member `.as_awaitable(p)`,
  (7.3) already-directly-awaitable passthrough (via the one-argument
  `GET-AWAITER`, independent of `Promise`'s own `await_transform`), (7.4)
  sender wrapping via `__sender_awaitable`, (7.5) identity fallback.
  Branch (7.2) is omitted: it and (8.1) both route through
  `get_await_completion_adaptor`/`adapt-for-await-completion`, which are
  out of scope (no scheduler in this fork customizes a completion
  adaptor); with `adapt-for-await-completion(s)` always taking the (8.2)
  fallback (`s` unchanged), (7.2)'s condition becomes identical to
  (7.1)'s own condition on the same object, so it can never fire when
  (7.1) doesn't. The exposition-only `awaitable-sender<Sndr, Promise>`
  concept is also not implemented: it is never cited by (7.1)-(7.5)'s own
  dispatch conditions, only by the block introducing `sender-awaitable`,
  so skipping it costs only diagnostic quality (a `Promise` lacking
  `unhandled_stopped()` now surfaces as a hard error inside
  `__awaitable_receiver::set_stopped()`'s body, only when actually
  ODR-used — i.e. only when a sender that can complete with `set_stopped`
  is really awaited — rather than being SFINAE'd out earlier).

  `with_awaitable_senders<Promise>` is a direct, mostly mechanical port of
  the standard text, with the exposition-only private members renamed
  `__continuation_`/`__stopped_handler_` (the standard's own text reuses
  the bare name `continuation` for both the private data member and its
  public accessor, which is exposition shorthand, not literal C++).

  Tested via two new `test/std` files (public surface, so real coroutines
  this time, not private-header access): `exec.coro.util/exec.as.awaitable/
  as_awaitable.pass.cpp` drives a minimal hand-rolled promise (its own
  `await_transform` calling `as_awaitable` directly, not through
  `with_awaitable_senders`) through value/chained-adaptor/error paths;
  `exec.coro.util/exec.with.awaitable.senders/with_awaitable_senders.pass.cpp`
  exercises the inherited path plus `set_continuation`/`continuation()`/
  `unhandled_stopped()` end to end using two real coroutines (a `Task`
  co-awaiting `just_stopped()`, wired via `set_continuation` to a second
  `Sink` coroutine whose own `unhandled_stopped()` records that it fired
  and returns `noop_coroutine()` — deliberately avoiding ever exercising
  the *default* stopped-handler, which calls `std::terminate()`). Confirms
  the standard's own note that the awaiting coroutine is never resumed
  past the `co_await` point when stopped: `t.h.done()` is false after the
  stopped path runs.

  Both new tests pass; full `execution/`+`thread.stoptoken/`+
  `libcxx/execution/` suite is now 83/83 (up from 81, the two new `test/std`
  files). `libcxx-generate-files` clean; transitive-includes generator
  still 125/125, no diff (both new headers are only reachable via
  `<execution>`, already included there). New headers registered in
  `CMakeLists.txt`, `<execution>`'s M6 include block, and
  `libcxx/modules/std/execution.inc` (`as_awaitable`/`as_awaitable_t`,
  `with_awaitable_senders`). `Cxx2cPapers.csv` still untouched — M6c (the
  `enable-sender`/`connect` retrofits) remains before the CSV flip.

  **Next session: M6c** — retrofit `enable-sender`'s awaitable disjunct
  in `sender.h` (`is-sender<Sndr> || is-awaitable<Sndr,
  env-promise<env<>>>`) and `connect`'s `connect-awaitable` fallback in
  `connect.h`, per the M2 deviation notes. Land both in their own commit,
  separate from any other change, and run the full `execution/` suite
  before and after: per the advisor's guidance when M6 was scoped, this
  is the check that discriminates whether the awaitable disjunct is
  escaping its intended domain (e.g. `static_assert(!sender<int>)` and
  similar existing negative cases in `exec.snd.concepts/sender.pass.cpp`
  must still hold and must not hard-error). Once M6c lands and the suite
  is still green, flip `__cpp_lib_senders`'s `unimplemented` off and all
  three CSV rows (`P2300R10`/`P3325R5`/`P3396R1`) to `|Complete|`
  together — the last remaining step in this Tier 2 sub-plan.

- **2026-08-22 (Tier 2, M6 continued — M6c-1 awaitable completion-signatures fallback)**:
  Discovered, while starting M6c, that the two-file plan above was
  incomplete: `get_completion_signatures.h`'s M2-era comment ("a sender
  with no viable get_completion_signatures dispatch, and that isn't
  itself awaitable, once M6 adds that") had already flagged a third
  retrofit — `[exec.getcomplsigs]` branch (3.3), the awaitable fallback
  for computing completion signatures. Confirmed via eel.is this is a
  *prerequisite* for the other two, not a peer: `enable_sender`'s
  awaitable disjunct makes an awaitable a `sender`, but `sender_in`
  additionally requires `get_completion_signatures` to be viable — without
  (3.3), `connect_t`'s own Mandates `static_assert(sender_in<...>)` would
  fire first, making `connect-awaitable` unreachable through the public
  CPO regardless of M6c's other two changes.

  Implemented (3.3) alone in this commit:
  `completion_signatures<SET-VALUE-SIG(await-result-type<NewSndr,
  env-promise<Env>...>), set_error_t(exception_ptr), set_stopped_t()>`,
  reached only when neither existing member-`get_completion_signatures`
  branch is viable. `SET-VALUE-SIG(T)` is `set_value_t()` for void `T`,
  else `set_value_t(T)` (no tuple-unwrapping — confirmed via
  `[exec.snd.concepts]`, simpler than a first guess). Hit one real bug
  making this a genuinely necessary fix rather than a mechanical port:
  `__await_result_type<NewSndr, __env_promise<Env>...>` doesn't compile
  when `__await_result_type`'s second parameter is defaulted rather than
  a true pack — "pack expansion used as argument for non-pack parameter"
  — even though the pack here only ever holds 0 or 1 elements. Fixed by
  making `__await_result_type` itself fully variadic (matching
  `__is_awaitable`'s existing shape), selecting between `__get_awaiter`'s
  one- and two-argument overloads based on the pack being empty or not;
  this is a source-compatible change for M6a/M6b's existing explicit
  1-arg/2-arg call sites.

  Landed alone, ahead of the `sender.h`/`connect.h` retrofits, since this
  branch is reachable by nothing currently in the tree (both existing
  branches already cover every sender in scope) — the safest of the three
  changes to verify in isolation, and it also caught the regression
  early: an initial version without the `__await_result_type` fix broke
  45 of 83 tests suite-wide (every file that transitively includes
  `get_completion_signatures.h`, i.e. nearly everything), confirming the
  advisor's warning that this disjunct sits on a hot path. Full
  `execution/`+`thread.stoptoken/` suite (83 tests) and
  `libcxx-generate-files` both pass, clean, after the fix.
  `Cxx2cPapers.csv` still untouched.

  **Next session: M6c-2 and M6c-3** — `enable_sender`'s awaitable
  disjunct in `sender.h`, then `connect-awaitable` in `connect.h` plus an
  end-to-end test that actually connects a bare (non-sender) awaitable
  through `execution::connect` and runs it to completion. Land each in
  its own commit; run the full suite after each. Only after M6c-3's test
  demonstrates `connect-awaitable` actually working does the
  `__cpp_lib_senders`/CSV flip happen — M6c-1 and M6c-2 are type-level
  only.

- **2026-08-22 (Tier 2, M6 closed — M6c-2, a real `__set_value_sig_t`
  bug, M6c-3, and the CSV/FTM flip)**: Finished M6c and closed Tier 2's
  P2300R10 sub-plan, in four commits.

  **M6c-2** (`sender.h`): one-line retrofit exactly as M6c-1's own
  header comment anticipated — `enable_sender<_Sndr> = __is_sender<_Sndr>
  || __is_awaitable<_Sndr, __env_promise<env<>>>`. Added `awaitable.h`/
  `env.h` includes (no cycle — neither includes `sender.h`). Full suite
  still 83/83, `libcxx-generate-files` clean.

  **`__set_value_sig_t` bug** (its own commit, ahead of M6c-3): while
  writing M6c-3's test with a *void*-returning bare awaitable, hit a
  hard error inside `sender_in`'s own `requires` check —
  `__set_value_sig_t<void>` doesn't SFINAE away, it hard-errors.
  Root cause: `conditional_t<is_void_v<_T>, set_value_t(), set_value_t(_T)>`
  requires *both* alternatives to be well-formed types before picking
  one, and `set_value_t(void)` is ill-formed regardless of which branch
  wins — the same "non-immediate-context SFINAE trap" class as M6c-1's
  `__await_result_type` bug, just one alias template over. M6c-1's own
  83/83-green suite never instantiated this alias with `_T = void`, so
  it shipped unnoticed. Fixed with an explicit `__set_value_sig<void>`
  partial specialization instead of `conditional_t`, which never forms
  the ill-formed alternative. Dropped the now-dead `conditional_t`/
  `is_void_v` includes. This is the *second* time on this milestone that
  a green suite concealed a fallback branch nothing in-tree reached —
  worth remembering for any future fallback-branch work in this area:
  a passing suite only covers what it actually instantiates.

  **M6c-3** (`connect.h`): ported `connect-awaitable-promise`,
  `operation-state-task`, and `suspend-complete` from `[exec.connect]`p3–5
  close to verbatim (renamed to this fork's `__`-prefixed convention).
  The `Sigs`/`__set_value_sig_t` and `__with_await_transform` machinery
  slotted in unchanged from M6c-1/M6a. One design deviation from a naive
  if-constexpr port: `connect_t::operator()`'s single `noexcept(noexcept(...))`
  needs exactly one well-formed expression to name, and computing it from
  `__connect_impl` alone (as pre-M6c-3 code did) would hard-error once a
  *second*, `__connect_impl`-nonviable branch exists — so the (6.1)/(6.2)
  dispatch is two overloads of a new `__connect_dispatch`, distinguished
  by a `requires(!__connect_impl-viable)` constraint on the fallback
  overload, rather than an if-constexpr inside one function. Each
  overload's own trailing decltype/noexcept-specifier is then only ever
  substituted when that overload is the one actually selected.

  New test `exec.connect/connect.awaitable.pass.cpp` connects a bare,
  non-sender awaitable (no `sender_concept`, no member `.connect()`, a
  `sender` only via M6c-2's disjunct) through `execution::connect` and
  drives it to completion via `start()`, covering value (`int` and
  `void` await-result — the latter is what surfaced the
  `__set_value_sig_t` bug above), an exception thrown from
  `await_resume` delivered as `set_error(exception_ptr)`, and
  `unhandled_stopped()` delivering `set_stopped()`. The advisor caught,
  before this landed, that the first draft exercised only
  value/void/error — `unhandled_stopped()` is a real member function of
  a class template and instantiates only on use, so it had never once
  been compiled. Adding the stopped case surfaced a *test* bug (not a
  library bug): reusing the `int`-valued `MyReceiver` for a
  `void`-returning `StoppedAwaitable` fails `connect-awaitable`'s own
  `receiver_of<DR, Sigs>` Mandate, because `Sigs`'s value channel is
  computed structurally from the await-expression's static result type
  regardless of which branch actually runs at runtime — fixed by giving
  `StoppedAwaitable::await_resume()` an `int` return type that matches
  `MyReceiver`, even though `unhandled_stopped()` means it's never
  actually reached. Full `execution/`+`thread.stoptoken/` suite is now
  84/84 (up from 83).

  **CSV/FTM flip** (its own commit, last): dropped
  `__cpp_lib_senders`'s `unimplemented: True` in
  `generate_feature_test_macro_components.py` and flipped all three
  `libcxx/docs/Status/Cxx2cPapers.csv` rows (`P2300R10`/`P3325R5`/
  `P3396R1`) to `|Complete|`. `libcxx-generate-files` regenerated
  `libcxx/include/version`, `FeatureTestMacroTable.rst`, and the two
  generated `support.limits.general/{version,execution}.version.compile.pass.cpp`
  tests — files outside `execution/` that no earlier M6 commit's test
  run touched (the advisor flagged this ahead of time: earlier commits'
  three-directory lit invocation wouldn't see this fallout). Ran
  `execution/` + `thread.stoptoken/` + `libcxx/execution/` +
  `support.limits.general/` together for this commit (167 tests, all
  green); `libcxx-generate-files` re-run afterward produces no further
  diff (idempotent).

  **Tier 2's P2300R10 sub-plan is now closed.** All three CSV rows are
  `|Complete|`, `__cpp_lib_senders` is live, and M1–M6c are all `[x]`
  above. Next session should return to the Tier list for the next
  gap-closing target rather than this sub-plan.

- **2026-08-22 (Tier 1, closing both partials)**: Per user request, closed
  out Tier 1's two remaining partial items.

  **P2944R3 (`reference_wrapper` comparisons) — now `[x]` Complete.**
  Investigation found the entire technical scope was already implemented:
  `reference_wrapper`'s own comparisons, plus the drive-by Mandates→
  Constraints changes for `pair`/`tuple`/`variant` (all inherited from
  upstream LLVM commits — `#136672`, `#141396`, `#145677`, none authored in
  this fork), and `optional`'s relops (which have used `enable_if_t<
  is_convertible_v<...>>` SFINAE since the original N4606 implementation,
  predating this paper entirely — never actually "Mandates" in this
  codebase). `expected` was already `|Complete|` via the separate P3379R0
  row. Fetched P2944R3's actual wording diff directly (not the CSV note,
  which had drifted) to confirm scope: only `==`/`<=>` on same-type
  `pair`/`tuple`/`optional`/`variant` overloads, nothing about the
  heterogeneous `tuple`-like comparison overloads visible in the current
  eel.is draft — those belong to the separate C++23 paper P2165R4
  (`Cxx23Papers.csv`, still `|Partial|` there), which the previous
  session's CSV note had conflated with this paper's own scope. Confirmed
  via advisor consultation before trusting the header-reading alone. Only
  actual work needed: drop `__cpp_lib_constrained_equality`'s
  `unimplemented: True` in `generate_feature_test_macro_components.py`
  (the FTM covers 5 headers — `expected`/`optional`/`tuple`/`utility`/
  `variant` — so the flip regenerates 6 `*.version.compile.pass.cpp`
  files, not 1, matching the M6c CSV-flip precedent) and rewrite the CSV
  note to state the P2165R4 divergence explicitly rather than leaving it
  looking like unfinished work. Verified pre- and post-flip: full
  `tuple`/`variant`/`pairs`/`optional`/`expected`/`refwrap`/
  `support.limits.general` suites green both times (509/514, 5
  pre-existing unsupported), `module_std.gen.py`/
  `transitive_includes.gen.py` 125/126 unchanged. Added missing test
  coverage advisor flagged: `optional`'s relops had no negative-SFINAE
  test at all (unlike `tuple`/`variant`, which got dedicated coverage from
  their own upstream commits) and no coverage of `optional<T&>` (this
  fork's own P2988R11 extension, added after P2944R3 originally landed
  upstream) specifically — new
  `optional.relops/types.compile.pass.cpp` confirms both specializations
  SFINAE away softly for a `NonComparable` contained/referenced type.
  Landed as its own commit, ahead of P1383R2.

  **P1383R2 (`constexpr` `<cmath>`/`<cstdlib>`) — now `[!]` compiler-
  blocked, not completable this session.** Probed this fork's constant
  evaluator directly rather than guessing: `static_assert(__builtin_sqrt(
  4.0) == 2.0)` and similar for `pow`/`floor`/`ceil`/`fmod`/`exp`/`log`/
  every trig function all fail "not a constant expression"; only `fabs`/
  `fmin`/`fmax`/`copysign`/`nan`/integer `abs`/`labs`/`llabs` fold.
  Confirmed by grepping `clang/lib/AST/ExprConstant.cpp` directly for
  `Builtin::BI__builtin_` cases — the missing functions have no case at
  all, not a disabled one; line 15957's own `// FIXME:
  Builtin::BI__builtin_powi` comment is upstream's marker that this is
  known-incomplete upstream work, not a fork regression. Full reasoning,
  the repro, and why neither remediation path (extending `ExprConstant.cpp`
  — rebase-risk against the file this fork's own reflection evaluator
  lives next to; or a partial library-only fallback — `__cpp_lib_
  constexpr_cmath` is one all-or-nothing macro, so a subset changes no
  status) is worth attempting are recorded at length under Tier 1's table
  above. Also discovered: this is gated behind an *undone C++23
  prerequisite*, same shape as the P2165R4 finding above — the
  generator's `__cpp_lib_constexpr_cmath` entry has only a `c++23` value
  (P0533R9's own number, `unimplemented: True`), and `Cxx23Papers.csv`
  confirms P0533R9 itself is only `|In Progress|` (just the
  classification functions `isfinite`/`isinf`/`isnan`/`isnormal`, which
  need no builtin folding). Rewrote the `Cxx2cPapers.csv` note to state
  the compiler blocker and the P0533R9 dependency explicitly. No lit test
  added (a compiler-limitation assertion needs `XFAIL` and would misfire
  confusingly, not usefully, once someone fixes the evaluator) — the repro
  in this file is the "has this been fixed yet" check for a future
  session. Landed as its own commit (tracker-only, no code change).

  **Tier 1 is now 8 of 9 items complete, 1 (P1383R2) compiler-blocked.**
  This is as far as Tier 1 can be honestly closed without either
  upstream Clang gaining constexpr-evaluator support for the missing
  `<cmath>` builtins, or a decision to hand-write a full constexpr
  numerical-algorithm library for every affected function (large,
  numerically risky, and — per the all-or-nothing FTM — wouldn't even
  change status until every function in scope were done). **Next
  session: move to Tier 3** (ranges/mdspan/format completions) or revisit
  the deferred Contracts (P2900R14) project per an explicit decision, per
  the Scope section's guidance.

- **2026-08-22 (Tier 3, ranges block — begun and completed in one
  session):** Split Tier 3 into three sub-blocks per the note in that
  section (ranges / mdspan+linalg / format+print) before starting, same
  shape as Tier 2's sub-plan. Worked the ranges block only: P2542R8
  (`views::concat`), P3138R5 (`views::cache_latest`), P3137R3
  (`views::as_input` — adopted name, not the paper's own `to_input` title),
  P2846R6 (`reserve_hint`). All four landed `|Complete|`, three commits
  (`438c01bb5bd2`, `b554dce17c8e`, `762c07926a39`).

  All four already had header scaffolding in-tree from a 2026-08-17 session
  (commits `d24292bc702a`, `5def9eee4a08`) but zero test coverage, and three
  of the four FTMs were already live in `<version>` with a blank CSV status
  — i.e. this fork was advertising conformance for `cache_latest`,
  `reserve_hint`, and `as_input` that had never actually been checked.
  Advisor's framing going in (don't pattern-match to the P2944R3
  "already-implemented, just flip the flag" case from Tier 1 — that one was
  safe because the code was inherited from upstream LLVM; this scaffolding
  was this fork's own untested code) was correct: writing real conformance
  tests surfaced concrete bugs in 3 of the 4 papers, not just missing CSV
  bookkeeping:

  - **P2846R6**: `ranges::reserve_hint`'s CPO (`__ranges/size.h`) was
    missing `_LIBCPP_HIDE_FROM_ABI` on all three overloads (every sibling
    CPO in the same file has it) — an ABI-export gap. Separately,
    `ranges::to` (`__ranges/to.h`) still gated its `reserve()` call on
    `sized_range`/`ranges::size`, not `approximately_sized_range`/
    `ranges::reserve_hint` as `[range.utility.conv.to]` normatively
    requires (confirmed against eel.is directly) — a range exposing only
    `reserve_hint()` silently got no pre-reservation at all. Fixed
    version-gated (C++26 path only; reserve_hint doesn't exist pre-C++26).
  - **P3138R5 / P3137R3**: both `cache_latest_view.h` and `as_input_view.h`
    had **zero** `_LIBCPP_HIDE_FROM_ABI` anywhere in either file (vs. 44 in
    the sibling `concat_view.h` scaffolding) — the same ABI-export gap at
    file scope. Both also wrongly specialized
    `enable_borrowed_range<...View<V>> = enable_borrowed_range<V>`; neither
    paper's wording specializes it at all (confirmed against eel.is) — for
    `cache_latest_view` in particular this isn't just a wording nicety,
    since its iterator holds a pointer back to the parent view for the
    cache, so a "borrowed" iterator would genuinely dangle.
    `cache_latest_view`'s private iterator/sentinel constructors were also
    missing `constexpr` (caught immediately by a `static_assert` on the new
    test — begin()/end() weren't usable in constant expressions at all).
  - **P2542R8**: the one exception — already solid (HIDE_FROM_ABI
    throughout, no incorrect borrowed-range specialization). Only needed
    tests and the FTM flip.

  New test coverage: `range.access/reserve_hint.pass.cpp`,
  `range.approximately.sized/approximately_sized_range.compile.pass.cpp`
  (using `forward_iterator`-based ranges rather than raw pointers, since a
  raw-pointer range is accidentally `sized_sentinel_for` and defeats the
  "non-sized" test cases), a new block in the existing
  `range.utility.conv/to.pass.cpp` fixture, `range.cache.latest/
  cache_latest.pass.cpp` (proves memoization via a call-counting input
  range), `range.as.input/as_input.pass.cpp` (proves the `views::all`
  passthrough condition is exactly `input_range && !common_range &&
  !forward_range`, not just "is a forward_range" — a common-but-input-only
  custom range still gets wrapped), `range.concat/concat.pass.cpp`
  (random-access/bidirectional/forward degradation, non-common trailing
  range via `default_sentinel_t`, heterogeneous `common_reference`
  resolution, 3-range concatenation, CTAD). Full `ranges/` suite green
  (453/454, 1 pre-existing unsupported) plus `transitive_includes.gen.py`
  and `support.limits.general/` (208/208), checked after each of the three
  commits.

  **Ranges block of Tier 3 is fully closed.** Next: mdspan/linalg block
  (assess whether P1673R13 needs a P2300R10-style dedicated sub-plan before
  starting) or format/print block — both per the split recorded at the top
  of the Tier 3 section, not started this session.

- **2026-08-22 (later same day, third entry): mdspan/linalg gate assessed,
  format/print block worked partially.** Continuation of the same day's
  Tier 3 work. Ran the mdspan/linalg gate the ranges-block note called for
  before picking off small items: `submdspan`/padded layouts (P2630R4/
  P2642R6/P3355R1) are confirmed genuinely from-scratch (no scaffolding at
  all in `__mdspan/`), but `<linalg>` (P1673R13) turned out to already be a
  3150-line implementation with 17 pre-existing *passing* tests — the
  opposite of what the ranges block found, and a caution against assuming
  "blank CSV status" always means "untested scaffolding": here it much more
  likely means stale bookkeeping. Didn't flip P1673R13 without auditing it
  properly (out of scope this session), but landed the one paper in that
  block precise enough to do safely: **P3050R2** (`linalg::conjugated`
  no-op for noncomplex element types) — `conjugated()` was unconditionally
  wrapping in `conjugated_accessor` even for `int`/`double` or types with no
  ADL `conj()`; fixed by reusing the existing `__has_adl_conj` trait (which
  already computed exactly this condition for `conjugated_accessor`'s own
  `element_type`) as the top-level branch condition. No FTM of its own
  (folds into `__cpp_lib_linalg`'s existing value), so this was a CSV flip
  plus two new test cases, not a macro flip.

  On the format/print block, landed the two independently-scoped items and
  found the other two genuinely blocked/out-of-scope rather than forcing
  them:
  - **P2845R8** (`formatter<filesystem::path, charT>`) — new `__filesystem/
    path_format.h`, following the `__thread/formatter.h` precedent for
    where a type's formatter specialization lives. Implements the
    path-format-spec grammar (`fill-and-align width ? g`, per
    `[fs.path.fmtr]`, fetched from eel.is directly) by reusing the
    existing `__fields_fill_align_width` parser for the common prefix and
    hand-parsing the trailing `?`/`g` (which don't fit the shared parser's
    single-type-char model — confirmed by reading `__parse_type` in
    `parser_std_format_spec.h` before assuming a shared-parser fields-flag
    would work). Required registering the new header in **both**
    `CMakeLists.txt` and `module.modulemap.in` — missing either broke the
    staged/module build with a `file not found`, not something a header-
    only smoke test would have caught. Also had to fix
    `concept.formattable.compile.pass.cpp`, which explicitly asserted
    `path` was *not* formattable under the (unrelated, abandoned) P1636
    test group — moved that assertion out into its own `test_P2845`
    function gated on `TEST_STD_VER >= 26`.
  - **P2587R3** (`to_string`/`to_wstring` float overloads) — wording is
    `Returns: format("{}", val)` verbatim (fetched from eel.is, then cross-
    checked against the paper itself since eel.is showed an unrelated
    `constexpr` addition on the integer overloads from a different, untracked
    paper — did not touch that, out of scope). Old behavior was `sprintf(...,
    "%f", val)`, always six fixed decimals; new behavior is the shortest
    round-trip representation. This is a real *behavior* change (existing
    tests asserted the old `"0.000000"`-style output), exactly the risk the
    advisor flagged going in. Rewrote `libcxx/src/string.cpp`'s six float/
    double/long-double overloads in terms of `std::format`, and deleted the
    `as_string`/`initial_string<S>`/`get_swprintf` sprintf-plumbing that only
    those six functions used (confirmed via grep before deleting — nothing
    else in the file referenced them). Integer overloads were already
    `to_chars`-based and already matched the new wording's observable output,
    so left untouched rather than rewritten for wording-purity alone.
  - **P2757R3** (type-checking format args) — assessed, not attempted.
    `__cpp_lib_format`'s C++26 bump is one cumulative value shared with
    P2510R3/P2637R3/P2918R2 (all already `|Complete|`), but the *existing*
    upstream-inherited P2637R3 CSV note already says the shared macro's bump
    is blocked by P2419R2, a still-unstarted *C++23* paper not tracked
    anywhere in this Tier list. Same shape as Tier 1's P1383R2 finding
    (undone prerequisite outside this tier's own scope) — recorded rather
    than attempted in isolation.
  - **P3107R5 / P3235R3** (`std::print` efficiency) — assessed, not
    attempted. Read `__print::__vprint_nonunicode` in `__ostream/print.h`
    directly: it does `string __str = std::vformat(...)` then `fwrite`,
    i.e. fully materializes the formatted output before writing — exactly
    what P3107R5 exists to eliminate. This is a real internals redesign
    (output iterator writing through to the `FILE*` incrementally, plus
    P3235R3's per-type fast paths on top), not a conformance-test pass;
    recorded the "verify mechanism via reading the internals, not just
    observable `std::print` output" criterion so a future session doesn't
    write tests that pass trivially against the unoptimized path.

  Verified after each commit: targeted suites all green — `filesystems/`
  (all pass), `utilities/format/` (262 then 345 tests as the FTM flips
  landed, all pass, including fixing the one pre-existing
  `concept.formattable` regression from adding the path formatter),
  `strings/string.conversions/` (10/10), `numerics/linalg/` (17/17, both
  before and after the P3050R2 fix), `support.limits.general/` (unchanged
  count, all pass), `transitive_includes.gen.py`/`module_std.gen.py`
  (126/126, no drift). A background full `check-cxx` run separately hit a
  pre-existing, unrelated reflection-module (`std.cppm`/CXX26) failure
  before timing out — not caused by this session's changes (none of them
  touch reflection), and the narrower `module_std.gen.py` run covering the
  same generated module content passed cleanly, so treated as inconclusive
  noise rather than a regression signal.

  **Format/print block: 2 of 5 papers Complete, 1 blocked on an untracked
  C++23 prerequisite, 2 assessed and scoped out as a dedicated future
  session.** mdspan/linalg block: 1 of 5 papers Complete (P3050R2),
  P1673R13 reassessed as likely-mostly-done-but-unaudited rather than
  not-started, mdspan proper (P2630R4/P2642R6/P3355R1) confirmed as a real
  from-scratch project for a future session. Next: either the mdspan-
  proper from-scratch implementation, a proper P1673R13 audit against its
  full wording (don't just flip the CSV on the strength of "tests already
  pass"), the P3107R5/P3235R3 `std::print` internals redesign, or move to
  Tier 4 (atomics) — all independent starting points, no forced ordering
  between them.

- **2026-08-23: Tier 4 (atomics) started, 3 of 5 papers Complete.** Ran
  the assessment gate across all five papers before touching code (per
  the advisor's push-back going in — Tier 4's Notes column was completely
  empty, unlike Tier 3 where the gate was partly pre-run). Landed
  **P0493R5** (atomic min/max), **P2835R7** (`atomic_ref::address()`),
  and **P2869R4** (remove deprecated `shared_ptr` atomic access APIs) as
  three separate commits; see their Tier 4 rows above for what each
  actually involved. The standout finding: P0493R5 looked like it might
  need clang changes (the tier's biggest open risk per the advisor), but
  reading `Builtins.td`/`SemaChecking.cpp`/`CGAtomic.cpp` directly showed
  `__c11_atomic_fetch_max/min` and their `atomicrmw fmax/fmin` lowering
  were already fully implemented in this fork's clang — a pure libc++
  header gap, not a compiler gap. Worth remembering next time a paper
  looks compiler-adjacent: check the builtin/codegen layer before assuming
  it's out of scope.

  **P3323R1** (cv-qualified `atomic`/`atomic_ref`) and **P3309R3**
  (`constexpr atomic`/`atomic_ref`) were assessed, not attempted — both
  real blockers, not scope-timidity, recorded in detail in their Tier 4
  rows. Short version: P3323R1's `eel.is` synopsis is post-P3309R3-merge
  (has a converting constructor the paper itself doesn't add, confirmed
  by fetching the paper text directly rather than trusting the draft), and
  its conformant `value_type`-typed signatures collide with
  `__atomic_ref_base`'s internal `T*`-typed `__compare_exchange`/
  `__clear_padding` helpers for `atomic_ref<volatile T>` — a real
  internals-threading problem, not something a `requires` clause papers
  over. P3309R3 doesn't need a clang change (the paper's own strategy is
  an `if consteval` library-side fallback) but touches every RMW/wait/
  notify path across five files — a dedicated session, same shape as
  Tier 3's `std::print` redesign finding.

  Verified: full `atomics/` + `utilities/memory/` + `thread/` +
  `support.limits.general/` + `libcxx/atomics/` suites (803 tests) all
  green both before commits (baseline) and after all three; confirmed via
  targeted compile check that `std::atomic_load(&shared_ptr)` etc. now
  hard-error under plain C++26 and compile clean with the escape-hatch
  macro; grepped the whole tree for any other in-tree caller of the newly
  gated `shared_ptr` atomic functions (none, so the `-Werror`-under-test
  build stays clean); `libcxx-generate-files` + `git diff` clean with no
  drift left unstaged. One concurrent-writer note: another process was
  committing to this same working tree mid-session (`ae2dfc5b1ce0`, a
  reflection-range fix, plus a `.github/workflows/` edit) — never
  reverted or touched, staged this session's work by explicit path each
  time, waited out one `index.lock` collision rather than removing it.
  Next: P3323R1 (internals re-thread first), P3309R3 (five-file RMW
  redesign), or Tier 5/6 — independent starting points, no forced
  ordering.

- **2026-08-23 (Tier 4, P3309R3 landed): `constexpr atomic`/`atomic_ref`,
  scoped to scalar `atomic<T>` + all-T `atomic_ref<T>` store/load/exchange.**
  The prior entry's "does not need a clang change" line was the paper's own
  claim, not an audit of this fork — re-derived it from scratch this
  session via three direct compile probes (see row for what each found):
  (1) none of `__c11_atomic_*`/`__atomic_*` are constexpr-evaluable here,
  confirming the `if consteval` direct-storage-access strategy is
  mandatory, not optional; (2) `_Atomic(T)` only implicitly converts back
  to `T` for scalar `T` (a real, compiler-level restriction, not a libc++
  choice) — the reason `atomic<T>`'s consteval reads are scalar-only, while
  `atomic_ref<T>` (plain `T*` storage, no `_Atomic` wrapping at all) has no
  such restriction and got store/load/exchange for arbitrary trivially
  copyable `T`; (3) `__builtin_bit_cast` also rejects `_Atomic(T)` as "not
  trivially copyable" even for `T=int`, ruling out that route entirely.
  Landed as one commit spanning `support/c11.h` (the shared consteval
  primitives, gated `is_scalar_v<_Tp>`, including a NaN-aware
  `fetch_max`/`fetch_min` helper duplicated from `<atomic>`'s own since
  this is the single call site reached by both the integral
  direct-RMW-builtin path and the floating-point CAS-loop-via-`__rmw_op`
  path), `atomic.h`, `atomic_ref.h`, `<atomic>`'s and `<atomic_ref>`'s
  free-function wrappers, and the FTM. Hit and fixed one real regression
  along the way, not caught until
  actually running the suite (not by reasoning about the design): marking
  the *shared* `atomic_sync.h` `__atomic_wait`/`__atomic_notify_one/all`
  templates `constexpr` broke `atomic_flag` — its `wait()` (atomic_flag.h)
  calls `std::__atomic_wait` textually *before*
  `__atomic_waitable_traits<atomic_flag>`'s specialization is declared
  later in the same header, which only worked because an ordinary function
  template's body-instantiation is deferred past that point; a `constexpr`
  template gets instantiated eagerly instead, so it hit the still-primary
  (deleted) trait and produced "explicit specialization after
  instantiation" errors when the real specialization was reached. Fixed by
  reverting `atomic_sync.h` to byte-identical-with-HEAD (confirmed via
  `git diff`) and inlining the consteval wait/notify-one/notify-all logic
  directly into `atomic<T>`/`atomic_ref<T>`'s own member functions instead
  (calling `this->load()` directly, no `__atomic_waitable_traits` needed
  there) — `atomic_flag` itself was left completely untouched rather than
  risk restructuring its declaration order for a type the paper doesn't
  even name.

  **Two deliberate, documented scope cuts, not oversights:** (1)
  `atomic<T>` for non-scalar trivially-copyable class `T` stays
  non-constexpr — a real compiler gap (no way to read `_Atomic(ClassType)`
  at compile time with this compiler's current capabilities), not
  something a future *library* session can close alone; would need a
  clang-side constexpr extension for `__c11_atomic_load` (or equivalent)
  on class-typed `_Atomic`. (2) `atomic_flag` is entirely out of scope,
  for the declaration-ordering reason above. Both are called out plainly
  in the row rather than silently absorbed into a "Complete" claim that
  overstates what landed — matches this tracker's established practice
  (e.g. P2835R7's `T*`-vs-`COPYCV` caveat, P0493R5's compiler-layer note).

  Added `libcxx/test/std/atomics/atomics.types.generic/constexpr.pass.cpp`
  (int, bool, pointer, `double`, and `long double` — the last specifically
  to exercise the CAS-loop path, since fp80 extended precision on this
  target's `long double` makes `__has_rmw_builtin()` false, unlike
  `double` — plus a `test_free_functions()` static_assert hitting every
  free `atomic_*`/`atomic_*_explicit` wrapper including the
  `__enable_if_t<is_integral...>`-constrained `fetch_and/or/xor` family, a
  compile-time `fetch_max`/`fetch_min` NaN-propagation check against
  `<atomic>`'s own `__maximum_num`/`__minimum_num` semantics since c11.h's
  copy of that logic isn't otherwise linked to it, and a non-constexpr
  `test_class_type_still_compiles()` regression check that `atomic<T>` for
  a non-scalar class `T` still compiles and runs normally at runtime — the
  `if constexpr (is_scalar_v<_Tp>)` gating must not break that) and
  `libcxx/test/std/atomics/atomics.ref/constexpr.pass.cpp` (scalar
  `int`/pointer plus a `TrivialPoint` class type exercising the
  store/load/exchange-only class-T path). Verified: both new tests pass
  under both the default hardening mode and (critically —
  `_LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN` compiles to `((void)0)` under
  the default `none` mode, so the default run alone never actually
  compiled the `atomic_ref` constructors' guarded
  `reinterpret_cast<uintptr_t>` alignment check) `--param
  hardening_mode=extensive`, which maps that macro to a real assertion
  that evaluates its argument — confirming the
  `__libcpp_is_constant_evaluated()` guard added around it is genuinely
  load-bearing, not inert; full `atomics/` suite green both before (127,
  re-baselined fresh at session start since the tree had moved since the
  last run) and after (129 — 127 plus the two new files), including
  `atomic_flag`'s existing tests, confirming the regression above is
  actually fixed and not just removed from the diff; `utilities/memory/` +
  `thread/` + `support.limits.general/` (650 tests) green;
  `transitive_includes.gen.py` (125 tests, covers the new
  `is_scalar.h`/`is_floating_point.h`/`is_constant_evaluated.h` includes)
  green; `atomic.version.compile.pass.cpp`/`version.version.compile.pass.cpp`
  (the FTM-specific tests) green; `libcxx-generate-files` + `git diff`
  clean, no drift left unstaged; `clang-format --style=file` applied to
  all three touched headers and both new test files before the final
  verification pass. FTM:
  `__cpp_lib_constexpr_atomic` = `202411L` — verified the name against
  eel.is's actual `<version>` synopsis rather than trusting the paper
  text's own "`__cpp_lib_atomic_constexpr`, location TBD by LEWG" (wrong
  name, would have been a real bug). `gcc.h` (this fork's dead
  GCC-atomics backend — clang always selects `c11.h`) was not touched, so
  a hypothetical GCC build would advertise the FTM without honoring it;
  recorded as a stated scope limit rather than fixed, same shape as the
  P0493R5 row. Next: P3323R1 (`atomic`/`atomic_ref` cv-qualification —
  needs `__atomic_ref_base` internals re-threaded first, per its row), or
  Tier 5/6 — independent starting points.

- **2026-08-23: Tier 6, P2641R4 (`std::is_within_lifetime`) landed.**
  Assessed first per the established gate: `__builtin_is_within_lifetime`
  and its Sema/`ExprConstant.cpp` support already existed in full in this
  fork's clang (`clang/test/SemaCXX/builtin-is-within-lifetime.cpp` passes
  unmodified) — a pure library gap, not compiler work. New
  `libcxx/include/__type_traits/is_within_lifetime.h`:
  `template<class T> requires (!is_function_v<T>) consteval bool
  is_within_lifetime(const T* p) noexcept`, gated `_LIBCPP_STD_VER >= 26 &&
  __has_builtin(__builtin_is_within_lifetime)`, wired into `<type_traits>`,
  `CMakeLists.txt`, `module.modulemap.in`, and the FTM generator
  (`libcxx_guard`/`test_suite_guard` both set, mirroring the
  `is_virtual_base_of` row already in the generator).

  **Signature reconciliation, not guessed:** eel.is's current
  `[meta.const.eval]` shows a *different*, two-parameter signature
  (`template<class U = void, class T> consteval bool
  is_within_lifetime(const T* p)`, Mandates on `static_cast<const volatile
  U*>(p)`) — fetched twice independently, consistent both times. Traced
  this to LWG4138 ("`is_within_lifetime` should mandate `is_object`"),
  confirmed via the actual issue page: **Status "Ready", not yet WP** —
  i.e. LWG-approved but not yet voted into the working paper this fork
  should track as "current C++26". Went with the original P2641R4
  single-parameter signature instead, for two independent reasons, not
  just "paper text wins": (1) it's what the paper itself specifies
  (`template<class T> consteval bool is_within_lifetime(const T*)
  noexcept`), and (2) it's what this fork's *own compiler* was actually
  built against — `clang/test/SemaCXX/builtin-is-within-lifetime.cpp`
  hand-rolls exactly `template<typename T> requires (!is_function_v<T>)
  consteval bool is_within_lifetime(const T* p)` as `#std-definition` and
  asserts diagnostics like `call to consteval function
  'std::is_within_lifetime<int>'` and (for an explicit function-type
  argument) `constraints not satisfied ... because
  '!is_function_v<void ()>' evaluated to false` — single template
  parameter, requires-clause not Mandates-static_assert. Implementing
  LWG4138's `U`-parameter form now would satisfy neither: it doesn't match
  the paper this row tracks, and it doesn't match what the compiler's own
  test suite was written against. **Not a full write-off** — noted here so
  a future session doesn't have to re-derive it: if LWG4138 is later voted
  in, it's a distinct, separate follow-up (same shape as P3323R1 being
  kept separate from P3309R3 in Tier 4), not a silent addition to this
  paper's scope.

  **Load-bearing compiler-behavior finding, confirmed by direct probing,
  not inferred from the wording alone:** per [meta.const.eval]'s Remarks
  ("...unless p points to an object usable in constant expressions or
  whose complete object's lifetime began within E"), *E* is scoped to a
  **consteval (immediate-function) invocation boundary**, not merely "any
  manifestly-constant-evaluated expression". A plain `constexpr` helper
  function with a local variable, called only via `static_assert(helper())`,
  fails with "read of non-const variable ... is not allowed in a constant
  expression" when it calls `is_within_lifetime` on that local — even
  though the enclosing `static_assert` is itself unquestionably a constant
  expression. Declaring the helper `consteval` instead (matching every
  helper function in the upstream `builtin-is-within-lifetime.cpp` test)
  fixes it. Confirmed with a minimal two-line repro compiled directly
  (`constexpr` helper fails, `consteval` helper of the same shape
  succeeds) before writing this into the test file, not assumed from the
  first failure. All four `consteval` helpers in the new pass test
  document this reasoning inline as a comment so it isn't rediscovered the
  hard way again.

  Also found and fixed in passing, unrelated to this paper's own scope:
  **four already-landed Tier 4 papers (P0493R5, P2835R7, P2869R4, P3309R3)
  never got their `Cxx2cPapers.csv` Status/`First released version`
  columns flipped**, despite root `CLAUDE.md`'s explicit "every commit
  that changes implementation status must update the CSV row in the same
  commit" convention and despite this tracker's own Tier 4 table already
  marking them `[x]` — confirmed via `git show <commit> --
  .../Cxx2cPapers.csv` on each of their four landing commits that none
  touched the CSV at all. Backfilled in a separate housekeeping commit
  (not folded into this paper's own commit, to keep history scoped): three
  (P0493R5, P2835R7, P2869R4) flipped to `|Complete|`; **P3309R3 flipped to
  `|Partial|`**, not `|Complete|` — its own Tier 4 row documents two
  deliberate scope cuts (class-typed `atomic<T>` stays non-constexpr,
  `atomic_flag` entirely excluded), and this tracker's established
  practice is that scope cuts get called out in the row rather than
  absorbed into an overstated `|Complete|`, so the backfill preserves that
  distinction instead of flattening it. A future session diffing the CSV
  against this tracker for accuracy would otherwise have hit four false
  negatives and, had P3309R3 been marked blindly `|Complete|`, one false
  positive too.

  Verified: new `meta.const.eval/` suite (4/4: `is_constant_evaluated.*`
  plus the two new `is_within_lifetime.*` files) green;
  `utilities/utility/` + `language.support/support.limits/
  support.limits.general/` (257 tests) green; `transitive_includes.gen.py`
  + `module_std.gen.py` (126, 125 passed/1 unsupported, matching the
  established baseline) green; `libcxx-generate-files` + `git diff` clean,
  no drift left unstaged. Next: any other Tier 5/6 item, or Tier 4's
  P3323R1 — independent starting points, no forced ordering.

- **2026-08-23: Tier 4, P3323R1 landed (cv-qualified `atomic`/`atomic_ref`),
  closing out Tier 4.** Fetched the paper's own wording diff directly
  rather than trusting eel.is, resolving the prior session's "extra
  converting constructor" blocker — that constructor belongs to a
  different (P3309R3-adjacent) merge, not this paper. `atomic<T>` gained
  a `same_as<T, remove_cv_t<T>>` mandate (routed through the
  floating-point specialization too, which previously bypassed it).
  `atomic_ref<T>`'s internals were re-threaded exactly as flagged:
  `value_type = remove_cv_t<T>` for all by-value parameters/returns and
  scratch buffers, while `__ptr_` keeps `T`'s real cv-qualification. Found
  and fixed a latent bool-cv bug this paper's own wording exposed
  (`atomic_ref<const bool>` was wrongly integral-specialized). An advisor
  review before commit caught a real gap the first pass missed: the
  pointer specialization needed the exact same "requires is_pointer_v<T>"
  treatment as integral/floating (not left as a deduced `T*` pattern,
  which structurally can never match a cv-qualified pointer argument) —
  fixed and verified against discriminating checks before folding into
  the real test. See the Tier 4 table row for full detail (compiler
  quirks found, test structure, verification). Full `atomics/` suite (131
  tests) green including under `--param hardening_mode=extensive`;
  `utilities/memory/`+`thread/`+`support.limits.general/` and
  `transitive_includes.gen.py`/`module_std.gen.py` green;
  `libcxx-generate-files` clean. **Tier 4 (atomics) is now fully assessed
  and landed** — all five papers (P0493R5, P2835R7, P2869R4, P3309R3,
  P3323R1) have real, accurate rows: three `|Complete|`
  (P2835R7/P2869R4/P3323R1), two `|Partial|` (P3309R3's scalar-only
  constexpr scope; P0493R5, downgraded from an overstated `|Complete|`
  this session after confirming, via the P3323R1 paper's own
  [atomics.ref.pointer] synopsis, that `atomic_ref<T*>::fetch_max`/
  `fetch_min` are real pre-existing standard surface this implementation
  never added — a genuine gap the P0493R5 row had silently missed, not
  something P3323R1 itself needed to fix, so left open for a future
  session rather than folded in here). Next: Tier 5/6, the
  `atomic_ref<T*>::fetch_max`/`fetch_min` CAS-loop addition just found, or
  the larger Tier 3 follow-ups (mdspan-proper, P1673R13 audit, `std::print`
  internals redesign) — independent starting points, no forced ordering.

- **2026-08-23 (Tier 4, P0493R5 closed): `atomic_ref<T*>::fetch_max`/
  `fetch_min` CAS-loop addition.** Closed the gap the previous P3323R1
  session flagged: added `fetch_max`/`fetch_min` to the pointer
  specialization in `libcxx/include/__atomic/atomic_ref.h`, right after
  `fetch_sub` and gated the same as the floating-point specialization's
  (`#if _LIBCPP_STD_VER >= 26`). Confirmed via `clang/lib/Sema/
  SemaChecking.cpp`'s `IsAllowedValueType` that `__atomic_fetch_max`/
  `__atomic_fetch_min`'s `ArithAllows` really is `AOEVT_FP` only (no
  `AOEVT_Pointer`), so — unlike the integral specialization, which routes
  straight through the builtin — this needed a manual CAS loop, same
  shape as `fetch_add`/`fetch_sub`'s existing `load` + `compare_exchange_
  weak` pattern in that struct. Fetched the current eel.is
  [atomics.ref.pointer] wording before writing the comparison: the
  computation is "as if by max and min algorithms... with the object
  value and the first parameter as the arguments" — i.e. `std::max`/
  `std::min`'s own `operator<`-based tie-breaking (first argument wins on
  equality), not NaN-aware like the floating-point specialization's
  `__maximum_num`/`__minimum_num` — so the loop body is a single ternary
  each (`__old < __arg ? __arg : __old` for max, `__arg < __old ? __arg :
  __old` for min), no helper function needed.

  Extended `atomics.ref/fetch_max.pass.cpp` and `fetch_min.pass.cpp`
  (previously `is_arithmetic_v<T>`-only, asserting pointer types did
  *not* have these members via `TestDoesNotHaveFetchMax/Min`) to cover
  `TestEachPointerType` (`int*`, `const int*`) with a real read/write
  sequence mirroring `fetch_add.pass.cpp`'s existing pointer branch (a
  9-element array, indices exercised via `&t[n]`). One real snag:
  `fetch_min`'s pre-existing acquire-release/seq-cst thread tests
  synchronize by racing `old_val`→`new_val` (both derived from
  `make_value<T>(0)`/`make_value<T>(1)`, an *increasing* pair for both
  arithmetic and pointer T, since they come from the same 2-element
  array by ascending index) and relied on a `x.store(T(100), relaxed)`
  sentinel before the `fetch_min` call to force the CAS loop to actually
  lower the value — `T(100)` doesn't compile for pointer T. Generalized
  to `x.store(new_val + 1, relaxed)`, which works for both: forming (not
  dereferencing) one-past-the-end of the shared test array is
  well-defined and compares greater than both `make_value` outputs,
  exactly like the arithmetic literal did. Verified this is still a
  meaningful test (not just "make it compile") by confirming the
  fetch_min pass/fail path is actually exercised — the loop must observe
  the sentinel and correctly select the smaller `new_val`, not just
  happen to already equal it.

  Called the advisor before flipping the CSV to `|Complete|`, since the
  tracker had already carried a wrong `|Complete|` for this exact paper
  once before (until the P3323R1 session's wording read caught the
  `atomic_ref<T*>` gap). The advisor pushed back on reusing that same
  tracker as the authority again and asked whether plain `atomic<T*>` —
  not just `atomic_ref<T*>` — has `fetch_max`/`fetch_min` in the
  standard. It does: fetching the raw synopsis for
  `https://eel.is/c++draft/atomics.types.pointer` shows `T* fetch_max(T*,
  memory_order = seq_cst) volatile noexcept;`/`fetch_min` declared right
  alongside `fetch_add`/`fetch_sub` (that section's synopsis also lists
  `store_add`/`store_sub`/`store_max`/`store_min` — void-returning
  variants that are *not* part of P0493R5, judging by the mismatched
  naming and the fact they don't appear in [atomics.ref.pointer]'s
  synopsis either; left untouched as out of scope, presumably a later,
  not-yet-C++26 paper the live draft has already picked up). This was a
  second, larger instance of the exact gap this row already had: nobody
  had checked plain `atomic<T*>` either.

  Fixed the same way: `libcxx/include/__atomic/atomic.h`'s `atomic<_Tp*>`
  specialization (`_Tp*` here isn't routed through the integral
  `__atomic_base<_Tp, true>` base at all — `is_integral<_Tp*>` is false,
  so it lands in the generic `false` base with no `fetch_add`/`fetch_max`
  of its own, and `atomic<_Tp*>` implements `fetch_add`/`fetch_sub`
  directly against `__cxx_atomic_fetch_add`/`sub`) got `fetch_max`/
  `fetch_min` added right after `fetch_sub`, both volatile and
  constexpr-under-C++26 overloads, as a CAS loop over the inherited
  `load`/`compare_exchange_weak` (same builtin-rejects-pointers
  constraint, confirmed against the same `IsAllowedValueType` check).
  Verified the constexpr overload works stand-alone with a small
  probe (`static_assert` calling `fetch_max` on a `constexpr`-evaluated
  `atomic<int*>`) before touching any test file, since the class-template
  member-instantiation laziness that let the *floating-point* `fetch_max`
  silently coexist with `atomic<T*>` doing nothing for pointers (never
  instantiated unless called) meant a plain compile of the header proved
  nothing either way.

  Test surface for the `atomic<T*>` side: `atomics.types.generic/
  pointer.compile.pass.cpp`'s synopsis comment and exercised-API `test()`
  function gained `fetch_max`/`fetch_min` lines (`#if TEST_STD_VER >= 26`
  guarded, matching the paper's actual availability); the four free
  function test files (`atomic_fetch_max(_explicit).pass.cpp`,
  `atomic_fetch_min(_explicit).pass.cpp`, all under `atomics.types.
  operations.req/`) each gained a `testp<T>()` pointer overload mirroring
  the existing `atomic_fetch_add.pass.cpp`/`_explicit.pass.cpp`'s own
  `testp<T>()` pattern exactly (a 3-element array, `int*`/`const int*`
  instantiations, non-volatile and volatile variants) — these free
  functions needed no header change themselves, since they're already
  templated generically on the atomic's value_type and just forward to
  `->fetch_max`/`fetch_min`. Also extended `atomics.types.generic/
  constexpr.pass.cpp`'s existing `test_pointer()` (which already covered
  `fetch_add`/`fetch_sub` under `constexpr`) with a self-contained
  fetch_max/fetch_min round trip, verified against a standalone compile
  probe first rather than inferred from the analogous `atomic_ref` case.

  Advisor's other two findings, both addressed: (1) the constexpr path
  for the *`atomic_ref<T*>`* loop itself was unverified — `atomics.ref/
  constexpr.pass.cpp`'s existing `test_pointer()` (which already covers
  `fetch_add`/`fetch_sub`) got a `fetch_max`/`fetch_min` round trip added
  and confirmed passing; (2) `atomics.ref/cv_qualified.pass.cpp`'s
  `int* volatile` read/write block, which already exercises
  `fetch_add`/`fetch_sub`/`compare_exchange_strong` under the paper's
  `is_const_v<T>`-gated Constraints, got a matching `fetch_max`/
  `fetch_min` round trip so the newly-added members are covered by the
  same cv-qualification check as everything else in that block.

  Full `atomics/` suite green (131 tests, 1 pre-existing unsupported,
  same count as before — all additions were to existing test files, no
  new ones needed); `libcxx-generate-files` clean, no drift. Flipped
  `Cxx2cPapers.csv`'s P0493R5 row from `|Partial|` to `|Complete|` — this
  time covering both the `atomic<T*>` and `atomic_ref<T*>` pointer gaps,
  not just the one the row's text had previously named. All five Tier 4
  papers (P0493R5, P2835R7, P2869R4, P3309R3, P3323R1) are now either
  `|Complete|` or `|Partial|` with an accurately-scoped reason (only
  P3309R3 remains `|Partial|`, for its documented scalar-only constexpr
  scope — a real compiler gap, not something this session touched). Tier
  4 is now fully closed out. Next: Tier 5/6, or the larger Tier 3
  follow-ups (mdspan-proper, P1673R13 audit, `std::print` internals
  redesign) — independent starting points, no forced ordering.

- **2026-08-23 (Tier 5/6 session)**: Started Tier 5/6. Closed four papers
  across three commits: P1885R12 + P2862R1 (`text_encoding` conformance
  pass — implementation was already complete and correct, just untested
  and carrying a stale generator `unimplemented` flag, same shape as
  Tier 3's ranges block), P2592R3 (hashing support for `std::chrono`
  value classes — genuinely from-scratch, all 18 `[time.hash]`
  specializations), and P3369R0 + P3508R0 together (constexpr for the
  whole `[specialized.algorithms]` family — P3369R0's narrow scope
  turned out to be a strict subset of P3508R0's, so implemented both at
  once; confirmed via a standalone probe that this was a real, fixable
  library gap and not a compiler limitation, unlike the two
  already-documented Tier 1/3 compiler-blocked cases it superficially
  resembled). Advisor review caught real, fixable issues before every
  CSV flip in this session (a missing include that made a `zoned_time`
  hash specialization work only by include-order accident, two missing
  discriminating test cases, and an FTM-value sanity check) — see each
  paper's own note above for specifics. Assessed but did not implement:
  the remaining Tier 5 freestanding papers and most of Tier 6 are still
  open. Next: continue down Tier 6's small items (P2836R1
  `basic_const_iterator`, P0952R2 `generate_canonical`, P2810R4
  `is_debugger_present`/`is_replaceable`, P3349R1 contiguous-iterator-
  to-pointer conversion look like the next small, self-contained
  candidates from a first pass) or start Tier 5's freestanding sweep —
  neither assessed in depth yet, no forced ordering.

- **2026-08-23 (second session)**: Assessed P2836R1 first (`basic_const_iterator`
  convertibility) — **blocked, not a small item**: `grep -rn
  "basic_const_iterator" libcxx/include/ libcxx/test/` returns nothing. The
  base facility (P2278R4, `basic_const_iterator`/`as_const_view`/`cbegin`
  changes — a C++23 paper with no row anywhere in `Cxx2cPapers.csv`, since
  this CSV only tracks C++26 papers) is entirely unimplemented, so P2836R1's
  DR against it has nothing to patch. Skipped; a future session that wants
  this needs to scope it as its own multi-file `<ranges>`/`<iterator>`
  effort, not a Tier 6 pickup.

  Implemented **P2546R5** (`<debugging>`: `is_debugger_present`,
  `breakpoint`, `breakpoint_if_debugging`) **+ P2810R4** (is_debugger_present
  should be replaceable) together, same shape as the text_encoding
  combination: the CSV had P2810R4 as the only Tier 6 row for this facility,
  but P2546R5 itself (the header's base content) had never been marked
  `|Complete|` either despite `libcxx/include/debugging` already existing —
  and per [debugging.utility]p5, P2810R4 isn't a documentation-only DR here:
  it requires `is_debugger_present` to actually be link-time replaceable,
  which the prior `_LIBCPP_HIDE_FROM_ABI inline` definition structurally
  could not be (inline functions get baked into every call site; a user's
  out-of-line redefinition in another TU is simply ignored). Zero test
  coverage existed for any of the three functions beforehand (only the
  `debugging.version.compile.pass.cpp` FTM check) — same "scaffolded but
  untested" shape this tracker keeps finding.

  **Two build-specific pitfalls, both caught by advisor review before
  writing any code, worth recording since neither is obvious from reading
  the existing replaceable-function precedent (`<new>`'s `operator new`)
  alone:** (1) `_LIBCPP_BEGIN_NAMESPACE_STD` is the *versioned* inline
  namespace (`std::__1`) — leaving `is_debugger_present` in that block would
  mangle it as `std::__1::is_debugger_present`, a different symbol than the
  `std::is_debugger_present` a user's replacement defines; the two would
  silently coexist with the user's version never called. Fixed by declaring
  it via `_LIBCPP_BEGIN_UNVERSIONED_NAMESPACE_STD`, matching
  `__new/new_handler.h`'s `set_new_handler`/`get_new_handler` (confirmed via
  `nm -D` on the built `.so`: `_ZSt19is_debugger_presentv`, no `__1`, no ABI
  tag). (2) `libcxx/src` compiles at a fixed `-std=c++23`
  (`libcxx/CMakeLists.txt:502`, `CXX_STANDARD 23`) regardless of the
  `_LIBCPP_STD_VER` a *user's* program is compiled at — so a `.cpp` giving
  the out-of-line definition can't simply `#include <debugging>` and expect
  to see the declaration, since the whole header (and hence the
  `_LIBCPP_OVERRIDABLE_FUNC_VIS` visibility attribute it carries) sits
  behind `#if _LIBCPP_STD_VER >= 26`. No existing C++26-only free function
  in this repo had an out-of-line library definition yet to copy (grepped
  `_LIBCPP_STD_VER >= 26` across `libcxx/src/` — zero hits), so there was no
  precedent to lift directly. Fixed by carving the single-line declaration
  into a new internal header, `libcxx/include/__debugging/
  is_debugger_present.h`, that carries no `_LIBCPP_STD_VER` gate of its own
  (matching how other internal headers, e.g. `__memory/aligned_alloc.h`,
  gate on feature macros rather than std-version) — `<debugging>` includes
  it from inside its own `>= 26` block as before, and the new
  `libcxx/src/debugging.cpp` includes it directly, unconditionally, the
  same way `libcxx/src/new.cpp` reaches straight for `<__memory/
  aligned_alloc.h>` instead of the public `<new>`. This sidesteps the
  std-version mismatch entirely rather than fighting it.

  Verified the fix empirically rather than by ABI reasoning alone, per
  advisor's suggested before/after: wrote a replacement-function test
  (`bool std::is_debugger_present() noexcept { return true; }` in a
  separate TU, dynamically linked against `libc++.so`) and confirmed it
  **fails to compile** against the pre-change tree (`git stash`'d
  momentarily) with `error: redefinition of 'is_debugger_present'` pointing
  at the old inline header definition, then confirmed it **passes** once
  the fix is applied — this is the only evidence that actually settles
  replaceability, since a successful build alone doesn't prove a user's
  definition wins at link time. `breakpoint()`/`breakpoint_if_debugging()`
  stay `_LIBCPP_HIDE_FROM_ABI inline` in the versioned namespace as before —
  per the freshly-fetched `[debugging.utility]` wording, only
  `is_debugger_present` carries the "Remarks: This function is replaceable"
  clause.

  New tests under `libcxx/test/std/diagnostics/debugging/debugging.utility/`
  (`is_debugger_present.pass.cpp`, `breakpoint_if_debugging.pass.cpp`,
  `is_debugger_present.replaceable.pass.cpp`) — matching advisor's hygiene
  note, `breakpoint()` itself is never called (it traps), and
  `breakpoint_if_debugging()` is only exercised guarded behind
  `if (!is_debugger_present())`, avoiding a spurious SIGTRAP under any
  ptrace-based test runner. Registered the new header
  (`libcxx/include/CMakeLists.txt`) and source
  (`libcxx/src/CMakeLists.txt`) file, added a `debugging` submodule entry
  for the new internal header to `module.modulemap.in` (required by
  `libcxx/test/libcxx/headers_in_modulemap.sh.py`, which otherwise flagged
  it missing — that same test run also surfaced three **pre-existing,
  unrelated** missing-from-modulemap headers from the earlier
  `std::execution` M6 work, `__execution/{sender,connect,
  get_completion_signatures}.h`; not fixed here, out of scope, flagging for
  a future session). Flipped `__cpp_lib_debugging` from `202311L` to
  `202403L` (the generator already had the P2810R4 value written in as a
  commented-out alternative from an earlier session, confirmed correct
  against eel.is `[version.syn]`'s live value directly rather than trusted
  blindly) and ran `libcxx-generate-files` — clean 5-file diff
  (`<version>`, `FeatureTestMacroTable.rst`, the generator script, both
  affected `*.version.compile.pass.cpp` tests). Confirmed (standalone
  compile+run, not just lit's always-on `-D_LIBCPP_ENABLE_EXPERIMENTAL`)
  that both functions work without the experimental flag. Full new test
  directory (5/5) and the two affected version tests green;
  `transitive_includes.gen.py`/`module_std.gen.py` (125/126, unchanged
  baseline, no drift from the new internal header). `Cxx2cPapers.csv`
  P2546R5 and P2810R4 rows flipped to `|Complete|`.

  **Next session**: P0952R2 (`generate_canonical`), P3349R1 (contiguous
  iterator → pointer conversion), or fixing the three now-documented
  pre-existing `__execution/*.h` modulemap gaps, are the next
  self-contained candidates — none assessed in depth yet.

- **2026-08-23 (third session)**: Implemented P0952R2 (new spec for
  `std::generate_canonical`). This turned out to be substantially bigger
  than a typical Tier 6 pickup — flagged as such by the advisor going in —
  because the pre-C++26 formula in `libcxx/include/__random/
  generate_canonical.h` isn't a status/test-coverage gap like several
  recent Tier 6 items; it's a materially different, non-uniform algorithm
  (`k = ceil(digits / floor_log2(R))`, no rejection sampling, can return
  exactly `1.0`) that the paper replaces with rejection sampling against
  the exact integer formula in [rand.util.canonical] (`k` = smallest
  integer with `R^k >= r^d`, `x = floor(R^k / r^d)`, retry until
  `S < x*r^d`, return `floor(S/x)/r^d`).

  **Gated the new algorithm behind `_LIBCPP_STD_VER >= 26`** (advisor's
  call, made before writing any code) rather than replacing the old
  formula unconditionally: this is a C++26 paper per `Cxx2cPapers.csv`,
  and an unconditional swap would silently change `uniform_real_
  distribution`/`exponential_distribution` output for every C++11–23 user
  of this repo (both call `generate_canonical` internally). The old
  algorithm survives verbatim in an `#else` branch; the pre-existing test
  (which hardcodes the old formula's exact output) got `// UNSUPPORTED:
  c++26` added and otherwise untouched, and a new file,
  `generate_canonical.p0952r2.pass.cpp` (`// UNSUPPORTED: c++03 .. c++23`),
  covers the new algorithm.

  Two correctness traps, both surfaced by the advisor before coding:
  (1) `R = g.max() - g.min() + 1` overflows in the URNG's native
  `result_type` for any generator whose range spans it fully (e.g.
  `mt19937_64`, range exactly `2^64`) — fixed by computing `R` in a type
  wide enough that `+1` can't wrap (`__uint128_t` when
  `_LIBCPP_HAS_INT128`, else `unsigned long long`), rather than
  reproducing the dispatch-based avoidance libstdc++ uses (a
  power-of-two-range special case). (2) `digits == 0`: the wording gives
  `r^0 == 1`, so `k` (smallest integer with `R^k >= 1`) is `0` — zero
  invocations of `g`, result trivially `0`, with no rejection loop at all.
  Verified this reading against the fetched wording before coding it
  (`generate_canonical<F, 0>` in the old test asserted one draw happened —
  that assumption doesn't survive into the new algorithm) and covered it
  explicitly in the new test (checks both the `0` result and, by reading
  the engine's next output afterward, that no draw was consumed).

  **Scoped the intermediate-precision limit rather than replicating
  libstdc++'s chunked >128-bit fallback** (`__generate_canonical_any`'s
  `else` branch in `random.tcc`, which does windowed 128-bit-safe
  multiply-accumulate to support `R^k` that doesn't fit in 128 bits) —
  but got the *shape* of the limit wrong on the first pass, caught by
  advisor review before commit: an initial version asserted the loose
  bound `bits(R) + d <= 127` (derived from `R^(k-1) < r^d` ⟹
  `R^k < R * r^d`), which is a valid upper bound on `R^k` but is loosest
  exactly where the true `R^k` is *smallest* — a large, e.g.
  full-width-power-of-two `R` makes `k` tiny. Concretely, `long double`
  (`digits == 64` on this fork's x86-64 target) combined with
  `mt19937_64` (`R == 2^64`) hits `bits(R) + d == 128`, failing that
  bound, even though the real computation is trivial: `k == 1`,
  `R^k == 2^64`, fits `__uint128_t` with headroom to spare. That bound
  would have hard-errored on an entirely ordinary
  `uniform_real_distribution<long double>` + `mt19937_64` program.
  Replaced with a `constexpr` immediately-invoked lambda that computes
  the *actual* `k` and `R^k` at compile time (both `R` and `r^d` are
  already compile-time constants) with a standard pre-multiply overflow
  check (`__rk > ~_UInt(0) / __rp`) instead of an approximated bound, and
  `static_assert`s on that computed `__fits` flag. This accepts every
  instantiation the arithmetic genuinely supports and only rejects the
  cases that would actually overflow — still narrow and still documented
  as an out-of-scope, this-session deviation (no chunked >128-bit
  fallback), just no longer over-broad. Added `long double` (with
  `mt19937_64`, the discriminating combination above) to the `[0, 1)`
  range sweep, plus a `uniform_real_distribution<long double>` case
  exercising the real call path a user hits.

  **Found and routed around a live, previously-uncovered bug in
  `<__random/log2.h>`'s `__uint128_t` specialization of `__log2_imp`**
  while computing that `static_assert`'s bound: its `?:` expression
  `(_Xp >> 64) ? (64 + __log2_imp<unsigned long long, (_Xp >> 64), 63>::
  value) : __log2_imp<unsigned long long, _Xp, 63>::value` unconditionally
  instantiates *both* branches (a `?:` is not `if constexpr` — both
  operands must be type-checked regardless of which one's value is
  actually selected), and the untaken `unsigned long long` branch is
  instantiated with the *unshifted*, full-width `_Xp` — so any `_Xp >=
  2^64` (exactly the `R == 2^64` case from `mt19937_64` this session hit
  first) fails to narrow at compile time even though that branch is never
  the one whose value is used. This utility is also used by
  `independent_bits_engine`/`uniform_int_distribution`, but only ever
  with values that fit in 64 bits in existing call sites, so the bug was
  latent and untriggered before this session. **Not fixed** — out of
  scope for this paper, and fixing `__log2_imp` correctly needs its own
  focused pass (the fix likely means masking `_Xp` to the low 64 bits
  before the untaken-branch instantiation, or restructuring away from the
  eager-`?:` pattern entirely, mirroring the `if constexpr`-vs-`requires{}`
  eager-evaluation lesson from the M1 `std::execution` session).
  `generate_canonical.h` routes around it entirely with a small
  self-contained `constexpr` loop (`__log2_floor`, a local lambda) instead
  of reusing `__log2<>`. Flagging `<__random/log2.h>`'s `__uint128_t`
  branch as a known, reproducible, still-open bug for whoever next touches
  `independent_bits_engine`/`uniform_int_distribution` with a genuinely
  128-bit-magnitude range.

  **Correction (2026-08-23, sixth session): this "known, reproducible, still-
  open bug" claim does not reproduce and is retracted.** Picked up as the
  queued Tier 6 candidate; before touching `log2.h`, reconstructed the exact
  failure this note describes rather than trusting the "probably still
  broken" framing. Four escalating probes, all against the unchanged
  `build-nyx/bin/clang` binary and unchanged `log2.h` (confirmed via `git log
  -1 -- libcxx/include/__random/log2.h`, last touched 2024-10-31, long before
  this fork's own sessions) that this note itself was written against:
  `std::__log2<__uint128_t, __uint128_t(1) << 64>::value` (the exact `R ==
  2^64` case named above) compiles clean and yields `64`; the same with `<<
  100` (forcing the branch that keeps genuine high bits) yields `100`; the
  literal call shape from the pre-fix `generate_canonical` formula —
  `constexpr __uint128_t __rp = static_cast<__uint128_t>(mt19937_64::max()) -
  static_cast<__uint128_t>(mt19937_64::min()) + __uint128_t(1); __log2<
  __uint128_t, __rp>::value` — also compiles clean and yields `64`; and the
  two real call sites named above as latent risks,
  `independent_bits_engine<mt19937_64, 100, unsigned __int128>` and
  `uniform_int_distribution<unsigned __int128>`, both compile and run
  correctly end-to-end. `-Wnarrowing`/`-Wc++11-narrowing` produce no
  diagnostic on any of these. The `?:`-both-branches-instantiated structural
  observation in the note above is accurate (that part isn't wrong), but the
  predicted consequence — the untaken branch's narrowing conversion of `_Xp`
  to `unsigned long long` failing to compile — doesn't happen: this Clang
  silently truncates the discarded branch's template argument instead of
  diagnosing it as narrowing, and since only the taken branch's `::value` is
  ever read, the final result is correct in every case tried. Not chasing
  whether that truncate-instead-of-diagnose behavior is itself
  standard-conformant per `[temp.arg.nontype]`/`[expr.const]`'s narrowing
  rules — that's a Clang frontend question independent of this paper's scope
  and doesn't change what's observable here. **No fix applied**: there is no
  reproducing defect to fix, and speculatively restructuring a header shared
  by `independent_bits_engine`/`uniform_int_distribution` on the strength of
  a claim that didn't hold up would be exactly the unmotivated-change risk
  this document's own conventions warn against. `generate_canonical.h`'s
  `__log2_floor` workaround is unaffected by this correction — it remains
  correct, self-contained code, simply no longer justified by "routing
  around a known bug." Leaving `<__random/log2.h>` untouched.

  New test `generate_canonical.p0952r2.pass.cpp`: the `digits == 0` case
  above; a scripted small-range (`R == 3`, non-power-of-two) custom URNG
  that forces one rejected attempt before an accepted one and checks both
  the total draw count and the accepted result exactly (the only
  rejecting input for `digits == 2` against `R == 3` is the draw pair
  `(2, 2)`, worked out by hand from the formula); a power-of-two-range
  (`R == 4`) sanity check confirming single-attempt exactness per
  [rand.util.canonical] Note 1; boundary-value cases (`digits-1`,
  `digits`, `digits+1`) against `minstd_rand0`'s real, well-known output
  sequence, cross-checked with an independent Python reimplementation of
  the formula rather than hand-derived by inspection; and a
  range-membership sweep (`[0, 1)`, 1000 iterations) against both
  `mt19937_64` (the `R == 2^64` overflow-trap case) and `minstd_rand0`
  (the large-`k`, many-draws-per-attempt case) for both `float` and
  `double`. Full `libcxx/test/std/numerics/rand/` suite green (492 tests:
  491 passed, 1 unsupported — the pre-C++26 test file, correctly excluded
  at this build's default `c++26` mode); manually re-ran the pre-C++26
  test file at `c++17`/`c++20`/`c++23` via `--param std=` to confirm it's
  untouched by the gating (`c++17` hit a pre-existing, unrelated failure —
  `<optional>`'s `enable_view` reaching an unguarded `ranges::` reference
  via a `<numeric>`/pstl-backend include chain — confirmed via `git
  stash` to reproduce identically at baseline, not caused by this
  session). `libcxx-generate-files` produced no diff, confirmed correct
  in advance: P0952R2 carries no feature-test macro (grepped
  `generate_feature_test_macro_components.py`, no entry). Flipped
  `Cxx2cPapers.csv`'s P0952R2 row to `|Complete|` and this document's
  Tier 6 checklist entry to `[x]`.

  **Next session**: P3349R1 (contiguous iterator → pointer conversion),
  the three pre-existing `__execution/*.h` modulemap gaps, or the newly
  found `<__random/log2.h>` `__uint128_t` bug (independent of any single
  paper — worth a standalone fix-and-regression-test session given it's
  now a documented, reproducible latent bug) are the next candidates.

- **2026-08-23 (fourth session)**: Picked up the `__execution/*.h`
  modulemap gap. **The "three headers" figure from the last session was a
  partial read** — actually 55 headers were missing from
  `libcxx/include/module.modulemap.in`, confirmed by running
  `headers_in_modulemap.sh.py`'s own check directly (39 `__execution/*.h`,
  5 `__stop_token/*.h`, 6 `__functional/*.h` from the copyable_function/
  function_ref work, 2 `__memory/*.h` from the P3508R0 session's
  `indirect`/`polymorphic`, 1 `__chrono/hash.h` — this session's own
  P2592R3 work registered that header in `CMakeLists.txt` but not the
  modulemap; **adding a header needs both, every time**). Added all 55,
  alphabetically within each existing submodule block, `textual header`
  for the two X-macro-repeated-inclusion headers
  (`function_ref_impl.h`/`copyable_function_impl.h`, matching
  `move_only_function_impl.h`'s existing precedent).

  **`headers_in_modulemap.sh.py` is a substring check on the source `.in`
  file, not a validity check** (advisor caught this before any real
  verification work) — it goes green the instant the path text is present,
  before anything compiles. Real verification needed a `-fmodules` build.
  That build surfaced two classes of finding:

  1. **The 39 `__execution/*.h` submodules are far more tightly
     interdependent than `chrono`/`functional`'s existing per-header
     blocks** (e.g. `completion_functions.h`'s `set_value_t`/`set_error_t`
     are used throughout the other 38 files) — naive per-header exports
     (hand-tracing `tuple`/`variant`/`optional`/`atomic`/`coroutine_handle`
     usage into `export std.X` lines) produced cascading "declaration must
     be imported" errors. Fixed by giving all 41 `execution` submodules
     `export *`, matching this same file's existing `allocator`/
     `unique_temporary_buffer`/`high_resolution_clock` precedent for
     exactly this shape of problem (their own comments cite
     https://github.com/llvm/llvm-project/issues/120108). Also applied to
     the new `chrono.hash`/`memory.indirect`/`memory.polymorphic` entries
     for consistency, since their hand-picked exports couldn't be
     independently validated either (see next point) — better to match
     the established workaround than leave narrowly-scoped exports that
     look verified but aren't.

  2. **The `-fmodules` build cannot currently be driven to completion at
     all, independent of anything in this session.** `module std [system]`
     is one monolithic top-level Clang module wrapping the *entire*
     library; building it the first time compiles every `header` line in
     the file as one synthetic TU (`<module-includes>`), so a single
     `#include <execution>` under `-fmodules` transitively surfaced
     unrelated pre-existing bugs in `hive`, `inplace_vector`, and (this
     session's own) `__memory/indirect.h`/`polymorphic.h` (missing
     `#include <__memory/allocator_arg_t.h>` for a type used in public
     constructor signatures — fixed via `export *` as above, not
     independently verified beyond that the compile error moved past that
     point) — all "worked by textual accident" bugs, same shape as the
     `zoned_time.h` finding from the P2592R3 session. **Measured the actual
     scale via `libcxx/test/libcxx/clang_modules_include.gen.py`, which
     exercises this same path per top-level header: 20 passed / 122 failed
     / 1 unsupported out of 143, and — checked via `git stash` before and
     after this session's changes — this ratio is byte-for-byte identical
     with or without this session's modulemap additions.** This is a
     pre-existing, undocumented-until-now condition affecting the large
     majority of the library, not something this session introduced or
     regressed. Making it pass is its own project, sized like the
     `std::execution` sub-plan (M1–M6) above — **not a Tier 6 pickup** —
     and is out of scope here.

  Two real, independent bugs found and fixed along the way, both the same
  "declaration visible only by textual-inclusion-order accident" shape:
  `__execution/receiver.h` used the `derived_from` concept without
  including `<__concepts/derived_from.h>` (added); `__memory/shared_ptr.h`
  used `hash<_Tp*>` (in `owner_hash()`) without including
  `<__functional/hash.h>` (added). Kept both — genuinely correct
  independent of modules — but verified the `shared_ptr.h` one carefully
  before keeping it, since advisor flagged it as the single riskiest line
  in the diff for `<memory>`'s transitive-include set (one of the most
  widely-pulled headers in the tree): reran
  `libcxx/test/libcxx/transitive_includes.gen.py` after the change — 125/125
  passed, no drift, confirmed `__functional/hash.h` was already reachable
  transitively through some other already-included header, so this fix
  adds no new top-level transitive include.

  Verification actually run, given the `-fmodules` build itself can't
  finish: `headers_in_modulemap.sh.py` (1/1, mechanical check now green);
  full default-mode (non-modules) sweep of `libcxx/test/std/execution/`,
  `libcxx/test/std/time/`, `libcxx/test/std/utilities/memory/`,
  `libcxx/test/std/thread/thread.stoptoken/`,
  `libcxx/test/std/utilities/function.objects/` (925/925, no failures);
  `libcxx/test/std/containers/sequences/vector`,
  `libcxx/test/std/strings/basic.string`,
  `libcxx/test/std/containers/sequences/deque` plus
  `libcxx/test/libcxx/module_std.gen.py` (393/393) as the `shared_ptr.h`
  blast-radius check; `transitive_includes.gen.py` (125/125) as above.

  **Next session**: the `hive`/`inplace_vector`/wider modules-build
  breakage (122/143 failing, confirmed pre-existing and undocumented until
  this session — candidates for a dedicated, `std::execution`-sub-plan-sized
  effort, not Tier 6); P3349R1 (contiguous iterator → pointer conversion —
  confirmed this session to be a pure "permission, not requirement" DR with
  no FTM, so likely just a CSV-flip-and-note once someone confirms
  `__unwrap_iter`'s existing memmove-style optimizations satisfy it); or
  the `<__random/log2.h>` `__uint128_t` bug from two sessions ago.

- **2026-08-23 (fifth session)**: Closed P3349R1. Confirmed via direct
  paper fetch that it's core wording (not library), grants permission only
  (implementations *may* lower `contiguous_iterator` ranges to pointer
  ranges, not required to), carries no FTM, and `libcxx-generate-files`
  produces no diff — the `|Nothing To Do|` shape, not `|Complete|` (advisor
  caught the wrong badge before the edit). Didn't just trust the prior
  session's "probably satisfies it" note: traced `__unwrap_range_impl`
  (`unwrap_range.h:37-42`, advances via `ranges::next` on the *original*
  iterator before `to_address`) and `copy_n`'s random-access overload
  (`copy_n.h:55`, `__first + difference_type(__n)` on the original iterator
  before delegating to `copy`) by hand, then wrote a standalone,
  uncommitted probe — a `contiguous_iterator` instrumented to count
  `operator++`/`operator+=` calls — and measured `std::copy`/`std::copy_n`
  over an 8-element range doing 1 and 2 advance calls respectively, not 8:
  empirical confirmation that libc++'s memmove-style lowering already
  advances through the original iterator's own arithmetic (giving a
  checked/throwing iterator its chance to fire) rather than skipping
  straight to raw-pointer math, exactly R1's constraint over R0. No test
  file added (advisor: no FTM, no behavior change, nothing for lit to
  assert). Recorded but left untouched: `unwrap_iter.h:43`'s pre-existing
  `TODO(hardening)` about hardened iterators losing checks when unwrapped —
  a real open question, but about libc++'s own hardening story, orthogonal
  to whether this paper needs a library change. `Cxx2cPapers.csv` P3349R1
  row flipped to `|Nothing To Do|`; Tier 6 table checkbox and a narrative
  block added.

  **Next session**: the `<__random/log2.h>` `__uint128_t` bug
  (`independent_bits_engine`/`uniform_int_distribution`-adjacent, from the
  `generate_canonical` session, still open and well-scoped) is the last
  carried-over candidate; otherwise pick fresh from the Tier 6 table, or
  scope the `hive`/`inplace_vector`/modules-build breakage as its own
  dedicated effort per the note above.

- **2026-08-23 (sixth session)**: Picked up the carried-over
  `<__random/log2.h>` `__uint128_t` candidate — and retracted it, rather
  than implementing the fix its own description already prescribed. Before
  touching the header, reconstructed the exact failure the prior session's
  note described: a `?:` in `__log2_imp<__uint128_t, _Xp, _Rp>` eagerly
  instantiates both branches, and the untaken branch narrows the full-width
  `_Xp` to `unsigned long long` as a non-type template argument. Four
  probes against the unchanged `build-nyx/bin/clang` and unchanged
  `log2.h` (verified via `git log -1 -- libcxx/include/__random/log2.h`:
  last touched 2024-10-31, well before this fork existed) — a direct
  `__log2<__uint128_t, 2^64>` and `2^100` call, the literal
  `constexpr`-variable call shape from `generate_canonical`'s pre-fix
  formula, and the two real call sites the note flagged as latent risks
  (`independent_bits_engine<mt19937_64, 100, unsigned __int128>`,
  `uniform_int_distribution<unsigned __int128>`) — all compiled cleanly
  (including under `-Wnarrowing -Wc++11-narrowing`) and produced correct
  results. The structural observation (both `?:` branches get instantiated)
  holds, but the predicted consequence doesn't: this Clang truncates the
  discarded branch's argument silently instead of diagnosing it as
  narrowing, and since only the taken branch's `::value` is ever read, the
  final answer is unaffected. Not chasing whether that's itself
  standards-conformant per `[temp.arg.nontype]` — orthogonal to this
  fork's C++26-conformance scope. **No fix applied** — there's no
  reproducing defect, and speculatively restructuring a header shared by
  two other facilities on the strength of a claim that didn't hold up would
  be its own unmotivated-change risk. Amended the `generate_canonical`
  session's Tier 6 narrative in place with a dated correction (this
  repo's first such retraction — no prior precedent existed to match, so
  marked explicitly as "Correction" rather than silently edited) rather
  than deleting the original text, so the reasoning trail (what was
  claimed, what was actually tested, what was found) stays intact for
  anyone who lands on the original claim first. `generate_canonical.h`'s
  `__log2_floor` local workaround is untouched and still correct — just no
  longer justified by "routing around a known bug." No CSV/checklist change
  (nothing here was ever a tracked paper).

  **Next session**: no carried-over candidate remains. Pick fresh from the
  Tier 6 table, or scope the `hive`/`inplace_vector`/modules-build breakage
  (122/143 failing under `-fmodules`, confirmed pre-existing, `std::execution`-
  sub-plan-sized) as its own dedicated effort.

- **2026-08-23 (seventh session)**: Assessed three Tier 6 rows, implemented
  none — all three turned out bigger than the tier implies, and recording
  that (so the next session doesn't reach for any of them expecting a small
  patch) is the deliverable. **P2836R1** (`basic_const_iterator`
  convertibility): the type it modifies doesn't exist in the tree at all —
  it's gated on P2278R4 (C++23, `Cxx23Papers.csv`, not tracked in this
  document's tier list), confirmed via `generate_feature_test_macro_
  components.py`'s `__cpp_lib_ranges_as_const` entry (`"unimplemented":
  True`, with P2836R1's own `202311` DR value already commented out
  directly beneath P2278R4's `202207`). **P2075R6** (Philox engine): same
  `"unimplemented": True` shape, no scaffold anywhere under
  `libcxx/include/__random/` — a full counter-based engine from the paper's
  wording, not a conformance pass. **P3378R2** (`constexpr` exception
  types): confirmed this Clang has no P3068 (`throw`-in-`constexpr`)
  support via a standalone `throw 42;` probe (hard error, no relevant flag
  found), but advisor caught that this alone doesn't make the row
  compiler-blocked in the P1383R2 sense — the paper's actual normative
  surface (constexpr constructors/`what()`/dtors) doesn't require `throw`
  support itself, only its motivating example does. Ran the narrower
  discriminator (`constexpr std::out_of_range e("msg"); static_assert(...)`,
  no `throw`) separately: still fails, but on `stdexcept:165`'s plain
  non-`constexpr` constructor — a **library** gap. The real blocker is
  three independent ABI-sensitive restructures the paper's own text
  describes (non-`constexpr`-compatible refcounted-string storage in
  `logic_error`/`runtime_error`; moving `.cpp`-defined `what()`/dtors into
  headers without dropping libc++.so/libc++abi.so's existing exported
  symbols; resolving `<stdexcept>`'s circular include on `<string>`) —
  session-sized on its own, not a Tier 6 pickup. Tier 6 table: all three
  rows marked `[!]` with inline notes pointing at the narrative block above
  (`P2836R1`/`P2075R6`: "not small"; `P3378R2`: "session-sized, not
  compiler-blocked"); table header gained a `Notes` column to hold them,
  matching the Tier 1/2/3 tables' existing shape. No CSV changes — nothing
  here flips a tracked status.

  **Next session**: no small Tier 6 item confirmed ready as of this
  session — the three checked this session and the `<__random/log2.h>`
  candidate from the last are all closed out (implemented, retracted, or
  reclassified as bigger). Remaining unchecked Tier 6 rows not yet assessed:
  P2264R7 (user-friendly `assert()`), P2248R8 + P3217R0 (list-initialization
  for algorithms + `find_last` addendum), P1068R11 (vector API for RNG),
  P3222R0 (transposed mdspan layouts), P3370R1 (new C23 headers), P3471R4
  (Standard Library Hardening) — any of these is a reasonable next pickup,
  unassessed rather than confirmed-small, so scope-check each before
  committing to an implementation the way this session did.

- **2026-08-23 (eighth session)**: Implemented P3370R1 (new C23 library
  headers `<stdbit.h>`/`<stdckdint.h>`), the best-scoped of the six
  candidates the prior session left unassessed. No FTM: confirmed via grep
  that `generate_feature_test_macro_components.py` has no `__cpp_lib_*` row
  for this paper at all — it uses its own `__STDC_VERSION_STDBIT_H__`/
  `__STDC_VERSION_STDCKDINT_H__` macros instead. Every primitive it needs
  was already in the tree (`<stdbit.h>`'s 14 function families wrap
  `std::countl_zero`/`countl_one`/`countr_zero`/`countr_one`/`popcount`/
  `has_single_bit`/`bit_width`/`bit_floor`/`bit_ceil`; `<stdckdint.h>`'s
  three templates wrap `__builtin_add_overflow`/`sub_overflow`/
  `mul_overflow`, already used in `__charconv/traits.h`/`__mdspan/*.h`) —
  ruling out the P1383R2 compiler-blocked failure mode before starting.

  Used the system's installed GCC 16 libstdc++ (`/usr/include/c++/16/
  stdbit.h`, `stdckdint.h`) as a reference implementation of this exact
  paper rather than re-deriving the 14 functions' edge-case semantics from
  scratch. Confirmed via the paper's actual wording (fetched from
  open-std.org) that declarations belong at global scope, not `std::` —
  matching libstdc++'s shape and this fork's `stdatomic.h` precedent, but
  written directly at global scope (no internal namespace) since there's no
  `<cstdbit>`/`<cstdckdint>` std-namespace counterpart to reflect. Used this
  fork's own `std::__unsigned_integer`/`std::__signed_or_unsigned_integer`
  concepts as template constraints (matching `__bit/*.h`'s existing style)
  rather than libstdc++'s `static_assert` form. Matched libstdc++'s choice
  to leave every function non-`constexpr`, confirmed deliberate (not an
  omission) via a failing `static_assert` probe. Checked `<mdspan>`/
  `<expected>` (no `__cxx03/` mirror) against `stdatomic.h`/`stdbool.h`
  (have one) to confirm the frozen-cxx03 split only applies to headers that
  predate C++11 — `<stdbit.h>`/`<stdckdint.h>` correctly get no mirror.

  **Found and fixed a real bug in the reference implementation before it
  could be copied in**: libstdc++'s `stdc_bit_ceil` computes
  `constexpr T msb = T(1) << (digits - 1); return (value & msb) ? 0 :
  bit_ceil(value);` — using "top bit set" as the not-representable test.
  That's wrong at the boundary: `msb` itself (e.g. `0x80` for
  `unsigned char`) is a valid, representable power of two, but has its top
  bit set, so this formula wrongly maps `stdc_bit_ceil_uc(0x80)` to `0`
  instead of `0x80`. Verified this is a genuine libstdc++ bug, not a
  misunderstanding of the paper's semantics, by cross-checking against
  glibc's C `<stdbit.h>` (the actual C23 reference, compiled with
  `gcc -std=c23`): `stdc_bit_ceil_uc(0x80)` correctly returns `0x80` there.
  Fixed by changing the guard to `value > msb` (only values strictly
  greater than the largest representable power of two are non-representable);
  added boundary-case asserts (`0x80`/`0x81` for `unsigned char`, and the
  generic-template equivalent for every tested width) to both new test
  files so a future regression here would be caught. Also verified no
  macro collision between `<stdbit.h>`'s unconditional `__STDC_ENDIAN_*`
  `#define`s and `<bit>`/`<endian.h>` by compiling all four headers
  together under `-Werror` (advisor-suggested check; clean).

  New: `libcxx/include/stdbit.h`, `libcxx/include/stdckdint.h`, registered
  in `libcxx/include/CMakeLists.txt` and as `[system]` modules in
  `libcxx/include/module.modulemap.in` (outside `module std {}`, matching
  `stdatomic.h`/`stdbool.h`). New tests: `libcxx/test/std/numerics/
  stdbit.h.pass.cpp`, `libcxx/test/std/numerics/stdckdint.h.pass.cpp`. No
  `.version.compile.pass.cpp` (no FTM to generate one from; confirmed
  `libcxx-generate-files` produces no diff). Full `libcxx/test/std/
  numerics/` sweep: 939/941 passed (2 pre-existing unsupported,
  unrelated); `transitive_includes.gen.py`, `module_std.gen.py`,
  `headers_in_modulemap.sh.py` all green. `Cxx2cPapers.csv` P3370R1 row
  flipped to `|Complete|`; Tier 6 table checkbox flipped alongside it.

  **Next session**: five unassessed Tier 6 rows remain — P2264R7
  (user-friendly `assert()`), P2248R8 + P3217R0 (list-initialization for
  algorithms + `find_last` addendum), P1068R11 (vector API for RNG),
  P3222R0 (transposed mdspan layouts, blocked on the mdspan/submdspan work
  in Tier 3 not having started), P3471R4 (Standard Library Hardening,
  likely session-sized on its own given its breadth). P1068R11 is probably
  the next-best-scoped pick using the same lens as this session (library-
  only, no compiler dependency) — scope-check it first rather than assume.

- **2026-08-24 (ninth session)**: Implemented P1068R11 (Vector API for
  random number generation), confirmed `unimplemented: True` in
  `generate_feature_test_macro_components.py` and library-only per the
  prior session's scope-check. Fetched the actual paper PDF (R11, LWG wording
  final) rather than trusting a first-pass WebFetch summary of it, which
  invented a plausible-but-wrong API shape (per-engine/per-distribution
  `.generate(first, last)`/`operator()(first, last)` member functions) —
  the real wording is much narrower: a single new algorithm,
  `std::ranges::generate_random`, in a new `[rand.alg]`/`[rand.alg.generate]`
  clause, with four overloads (range/iterator-pair, each with and without a
  distribution argument). Extending engines/distributions themselves is
  explicitly out of scope for this paper — confirmed by its own "Extension
  and/or modification of the list of supported Engines and/or Distributions
  is out of the scope of this proposal" line — so no changes to any engine
  or distribution class were needed or made.

  Implemented as a CPO-shaped function object (matching
  `__algorithm/ranges_generate.h`'s style) in new
  `libcxx/include/__random/generate_random.h`: each overload checks, via an
  `if constexpr (requires {...})`, whether the generator (or distribution)
  exposes its own `generate_random` member function — the paper's actual
  customization point, for `.generate_random(r)`/`.generate_random(r, g)` —
  and calls that when well-formed; otherwise it falls back to
  `ranges::generate(r, ref(g))` or `ranges::generate(r, [&d,&g]{ return
  invoke(d,g); })`, exactly matching the wording's `Remarks:` equivalence
  clause. No engine or distribution in this tree defines a `generate_random`
  member, so today every call takes the fallback path — the member-check
  branch exists for forward compatibility with a future paper/vendor
  extension, and is exercised in the new test via two hand-rolled
  `constexpr`-friendly mock types (a generator and a distribution) that
  each define one. The iterator-pair overloads are implemented as literal
  transcriptions of the wording's `Effects: Equivalent to: return
  generate_random(subrange<O, S>(std::move(first), last), g[, d]);` (a
  recursive call into `(*this)`), rather than a private shared-impl helper
  like `ranges_generate.h` uses — closer to the standard text, and the
  extra call layer is free under `_LIBCPP_HIDE_FROM_ABI` inlining.

  Synopsis: added the `namespace ranges { ... }` block to `<random>`'s
  header-comment synopsis, placed immediately after `generate_canonical`
  (matching the paper's own synopsis diff position, before the
  `// Distributions` section). Registered the new header in
  `libcxx/include/CMakeLists.txt` and `libcxx/include/module.modulemap.in`;
  confirmed (by diffing `__cxx03/__random/` against `__random/`) that
  `uniform_random_bit_generator.h` — a C++20 concept — already has no
  `__cxx03` mirror, so `generate_random.h` (C++26-only) correctly gets none
  either, consistent with the `<stdbit.h>`/`<stdckdint.h>` precedent that
  the frozen-cxx03 split is for pre-C++11 facilities only. Added the
  `namespace ranges { using std::ranges::generate_random; }` export, guarded
  `#if _LIBCPP_STD_VER >= 26`, to `libcxx/modules/std/random.inc` (matching
  `functional.inc`'s existing guard style for `copyable_function`/
  `function_ref`). Flipped `__cpp_lib_generate_random`'s `unimplemented`
  off and regenerated `version`/`FeatureTestMacroTable.rst`/the two
  `*.version.compile.pass.cpp` tests via `libcxx-generate-files` (value
  `202403L`, already present as a placeholder — no macro-value change).
  `Cxx2cPapers.csv` P1068R11 row flipped to `|Complete|`.

  Advisor review before commit caught a real bug: both member-customization
  branches originally did `return ranges::end(__r);` after calling the
  member, but the declared return type is `borrowed_iterator_t<R>` (i.e.
  `iterator_t<R>`), while `ranges::end` yields `sentinel_t<R>` — those types
  differ for a non-common range, making it a hard compile error rather than
  a graceful failure. Reachable through the paper's own iterator-pair
  overload (which delegates through `subrange`, itself borrowed) whenever
  the sentinel and iterator types differ and the generator/distribution
  customizes `generate_random` — not an exotic case. The test as first
  written didn't catch this because every case used `std::array`/pointer
  ranges, where sentinel == iterator. Fixed by returning
  `ranges::next(ranges::begin(__r), ranges::end(__r))` in both
  customization branches instead (the non-customized fallback branches were
  already correct, since `ranges::generate` itself returns the iterator).
  Added a non-common-range test case (`subrange` over a pointer and
  `sentinel_wrapper`, from `test_iterators.h`) exercising both the fallback
  and customization paths through this exact gap.

  Also deliberately **not implemented**: the paper's other customization
  form, `g.generate_random(s)` for `s` a `span<invoke_result_t<G&>, N>`
  (the shape a SIMD/vectorized backend would actually use for chunked
  generation) — only the arbitrary-range member form
  (`g.generate_random(r)`) is dispatched. This is conforming (the `Remarks:`
  clause pins observable behavior to plain `ranges::generate` when no
  customization exists), and libc++ has no vectorized RNG backend to
  benefit from wiring up span-based chunking, but it means a future engine
  that only implements the span form won't be picked up by this
  implementation. Flagging here so a future session doesn't assume the
  customization point is fully wired.

  New test: `libcxx/test/std/numerics/rand/rand.alg/rand.alg.generate/
  generate_random.pass.cpp` (new `rand.alg`/`rand.alg.generate` directories
  — first paper to populate this clause). Covers all four overloads'
  constraints (`HasGenerateRandomRange`/`HasGenerateRandomIter`/
  `HasGenerateRandomRangeDist` SFINAE-friendly concepts), the no-
  customization fallback path (call-count and output assertions), the
  member-customization dispatch path (via the two mock types), and the
  non-common-range case described above, all under `static_assert(test())`
  for compile-time coverage. One thing this needed:
  `std::uniform_int_distribution`'s constructor isn't `constexpr` in this
  implementation, so the sub-tests using it were split into a separate
  runtime-only `test_uniform_int_distribution()` rather than folded into
  the main `constexpr test()` — the rest of the coverage uses only
  hand-rolled `constexpr`-friendly generator/distribution mocks and stays
  in the `static_assert`ed path. Full `libcxx/test/std/numerics/rand/`
  sweep: 492/493 passed (1 pre-existing unsupported, unrelated);
  `transitive_includes.gen.py` and `headers_in_modulemap.sh.py` both green;
  `module_std.gen.py` unsupported (pre-existing, gated on a lit feature
  this build tree doesn't have — unrelated to this change, matches the
  Tier 6 `<stdbit.h>` session's same observation).

  **Next session**: four unassessed Tier 6 rows remain — P2264R7
  (user-friendly `assert()`), P2248R8 + P3217R0 (list-initialization for
  algorithms + `find_last` addendum), P3222R0 (transposed mdspan layouts,
  still blocked on Tier 3 mdspan/submdspan work not having started),
  P3471R4 (Standard Library Hardening, likely session-sized on its own
  given its breadth — scope-check before committing). None of these four
  has been scope-checked yet; P2264R7 or P2248R8/P3217R0 are the most
  promising to check first (both plausibly library/front-end-diagnostic
  scoped rather than large ABI or from-scratch-engine work), but confirm
  before implementing rather than assuming, per this tier's established
  pattern.

- **2026-08-24 (tenth session)**: Checked P2264R7 (user-friendly variadic
  `assert()`). **Nothing to do**: libc++'s `<cassert>` never defines
  `assert` itself — its own comment says so directly (`// <assert.h> is not
  provided by libc++`) — it just forwards to the platform's `<assert.h>`.
  The paper's entire normative surface is the `assert` macro definition,
  which is therefore the C library's responsibility, not libc++'s. Verified
  transitively satisfied rather than just assumed: grepped the system's
  glibc 2.44 `/usr/include/assert.h` and found it already guards a
  `#if __ASSERT_VARIADIC` variadic definition for C++/GNU-extension builds;
  confirmed empirically by compiling `assert(std::is_same<int,int>::value)`
  (a comma-containing expression with no extra parens — the paper's whole
  point) with `-fsyntax-only` against this fork's `clang-nyx`/libc++
  headers — compiles clean. Same shape as the `P3349R1` precedent: marked
  `|Nothing To Do|` in `Cxx2cPapers.csv` with a Notes-column rationale
  rather than `|Complete|`, since libc++ contributes zero lines to
  satisfying this paper. No FTM exists for this paper (confirmed via grep
  of `generate_feature_test_macro_components.py` — none), so no `version`
  regeneration needed. Tier 6 table checkbox flipped alongside it.

  Also scope-checked P2248R8 (list-initialization for algorithms) +
  P3217R0 (its `find_last` addendum) using the actual wording (fetched the
  paper's raw HTML via `curl` rather than trusting `WebFetch`'s summarizer
  model a second time — the first summary of P1068R11 earlier in this
  effort had already proven capable of inventing a plausible-but-wrong API
  shape; a `WebFetch` retry on this paper produced a similar artifact,
  flattening `<del>`/`<ins>` diff markup into single garbled signatures
  with duplicate template parameters). The real technique: add a defaulted
  template type parameter (`projected_value_t<I, Proj>` for
  projection-aware `ranges::` algorithms, plain
  `iterator_traits<It>::value_type` for classic non-range algorithms)
  so a braced-init-list argument — which is a non-deduced context against a
  bare template parameter — falls through to the default instead of
  failing to deduce. ~69 signature touch-points across `<algorithm>`,
  `<numeric>`-adjacent range algorithms, and 5 container `erase()` free
  functions (`<string>`/`<vector>`/`<deque>`/`<list>`/`<forward_list>`),
  confirmed via the paper's own itemized table — objectively larger than
  every other single-session Tier 6 pickup so far, but advisor pushed back
  on deferring wholesale: the real question is whether libc++'s current
  declared parameter order already matches the paper's pre-diff text, not
  the raw touch-point count. Checked directly: it does **not**, for every
  `ranges::` site inspected. `ranges_find.h` declares `<_Iter, _Sent, _Tp,
  _Proj = identity>` (paper's new order: `_Proj` before `_Tp`, which gets
  the default). `ranges_fill.h` declares `<_Type, output_iterator<const
  _Type&> _Iter, _Sent>` — `_Type` **first**, using itself to constrain the
  iterator's constrained-parameter shorthand; the paper's version puts the
  iterator/sentinel first and moves the `output_iterator<...>` constraint
  into a trailing `requires`-clause so `_Type` can move last and get a
  default. `ranges_replace.h` and `ranges_lower_bound.h` show the same
  pattern (value type(s) declared before `_Proj`/`_Comp`, not after). So
  every `ranges::` algorithm site needs individual restructuring — not a
  literal transcription of the paper's diff — which is genuinely
  session-sized (and P3217R0's `find_last` family inherits the same
  hazard). **Not started this session** — left `unimplemented: True` on
  `__cpp_lib_default_template_type_for_algorithm_values` and both CSV rows
  unflipped, so a future session picks this up as a clean, correctly-scoped
  unit rather than a half-migrated one.

  The other three slices carry no reordering risk (no `_Proj`/`_Comp`
  parameter to jump over) and are queued as the concrete next pickup:
  (1) add `projected_value_t<I, Proj>` as a new alias in `<iterator>`
  (needed by the `ranges::` slice too, so doing it now isn't wasted); (2)
  append `class U = T`-shaped defaults to the 5 `erase()` free functions;
  (3) append `class T = typename iterator_traits<It>::value_type`-shaped
  defaults to the classic non-range algorithm signatures (`fill`, `find`,
  `count`, `search_n`, `replace`, `replace_if`, `remove`, `remove_copy`,
  `replace_copy_if`, `lower_bound`, `upper_bound`, `equal_range`,
  `binary_search` — no `Proj` involved, so nothing to reorder). Per
  `Cxx2cPapers.csv`'s existing `|Partial|` convention (used for
  `P1383R2`/`P2714R1`/`P3309R3`), this paper should land as `|Partial|`
  once slices 1–3 are done, not flipped to `|Complete|` until the
  `ranges::`/`find_last` slice also lands.

  **Next session**: implement projected_value_t + the 5 erase() defaults +
  the non-range algorithm defaults (slices 1–3 above) for P2248R8, mark
  `|Partial|` in the CSV, leave the FTM `unimplemented: True`. The
  `ranges::`/`find_last` reordering slice (P2248R8's remainder + P3217R0)
  is its own follow-up — budget real per-site care, not a mechanical
  sweep, and note the FTM spans 7 headers so flipping it eventually
  regenerates 7 header-specific `.version.compile.pass.cpp` files plus
  `version.version.compile.pass.cpp`. P3222R0 (still blocked on Tier 3
  mdspan work) and P3471R4 (Standard Library Hardening, unassessed) remain
  the other two open Tier 6 rows.

- **2026-08-24 (eleventh session, same day as the tenth)**: Implemented
  slices 1–3 of P2248R8 (list-initialization for algorithms) — the parts
  the prior session identified as free of the `ranges::` reordering
  hazard. New `std::projected_value_t<I, Proj>` alias in
  `<__iterator/projected.h>` (`remove_cvref_t<invoke_result_t<Proj&,
  iter_value_t<I>&>>`, C++26-guarded, matching the paper's wording
  verbatim — fetched via raw `curl` of the paper's HTML rather than
  `WebFetch`'s summarizer, which had already produced garbled/duplicated
  template-parameter diffs for this exact paper on a retry mid-session;
  the raw `<del>`/`<ins>` markup gave unambiguous per-site wording
  instead). Appended `class T = typename iterator_traits<It>::value_type`
  (or the paper's specific per-algorithm variant — `OutputIterator` for
  `replace_copy_if`, `InputIterator` for `remove_copy`) to the classic
  non-range overloads of `find`, `count`, `search_n` (both overloads),
  `replace`, `replace_if`, `replace_copy_if`, `fill`, `fill_n`, `remove`,
  `remove_copy`, `lower_bound`, `upper_bound`, `equal_range`,
  `binary_search` (both overloads each) — 14 `__algorithm/*.h` files, all
  pure appends since none of these involve a `Proj`/`Comp` parameter that
  the value-type parameter would need to jump over. Appended `class U = T`
  (`= charT` for `basic_string`) to the 5 container `erase()` free
  functions (`<vector>`'s is split into `__vector/erase.h`; `<deque>`/
  `<list>`/`<forward_list>`/`<string>` have theirs inline in the main
  header). Updated the `<algorithm>` synopsis comment block and the 5
  container synopses to match. All defaults guarded
  `#if _LIBCPP_STD_VER >= 26` using the `class _Tp #if ... = default
  #endif` mid-declaration form (avoids duplicating the whole parameter
  list across two branches) — verified this doesn't leak the behavior into
  earlier standard modes with a direct `-std=c++23` compile of a
  braced-init-list call, which correctly still fails to deduce.

  Advisor review caught two real gaps before this was called done. First:
  the paper's non-range algorithms *also* have `ExecutionPolicy` overloads
  with the identical default, and this fork actually implements 7 of them
  (`count`, `fill`, `fill_n`, `find`, `replace`, `replace_if`,
  `replace_copy_if`, all in `__algorithm/pstl.h`, gated
  `_LIBCPP_HAS_EXPERIMENTAL_PSTL` — confirmed via `grep -rln
  "is_execution_policy_v\|_ExecutionPolicy"`; `search_n`/`remove`/
  `remove_copy`/the `alg.binary.search` family have no PSTL overloads in
  this tree at all, nothing to do there). Checked each site: `_Tp` sits
  right before the already-defaulted `_RawPolicy`/`enable_if_t<...>`
  parameters in every one, so this was the same pure-append edit, not a
  reordering — done, and exercised in the test via
  `std::execution::seq`, gated on `_LIBCPP_HAS_EXPERIMENTAL_PSTL` (the
  *value*, not `defined(...)` — that macro is unconditionally defined to 0
  or 1, and the first test-file draft used the wrong check, silently
  compiling the PSTL block out under the ad hoc verification `clang++`
  invocation that didn't pass `-D_LIBCPP_ENABLE_EXPERIMENTAL`; caught by
  re-deriving from the header's own `#if _LIBCPP_HAS_EXPERIMENTAL_PSTL`
  guard once the omission was suspicious). Second: `projected_value_t` had
  zero instantiation coverage (only declared/synopsis'd/module-exported) —
  added `static_assert`s against `std::identity`, a projection returning a
  different type (`int` → `double`), and a pointer-to-member projection,
  matching the alias's wording directly.

  Also went one step further than the prior session's scope-check
  suggested: re-examined whether *every* `ranges::` algorithm needs the
  reordering treatment, per advisor's push-back that the reordering hazard
  is about declaration order, not a blanket property of the `ranges::`
  namespace. `ranges::fold_left`/`fold_left_with_iter`
  (`__algorithm/ranges_fold.h`) have no `Proj`/`Comp` parameter at all —
  `class _Tp` already sits in the position the paper's diff targets, gaining
  only a default (`iter_value_t<_Ip>` / `range_value_t<_Rp>`), no jump
  needed. Implemented and tested alongside the rest. `ranges::fold_right`
  is not implemented in this fork at all — a separate, pre-existing gap
  from a different paper (P2322R6, `ranges::fold`) — so it's untouched and
  unmentioned beyond this note.

  New test: `libcxx/test/std/algorithms/algorithms.general/
  default_template_type_for_algorithm_values.pass.cpp`. Covers every
  touched non-range algorithm and both `erase()`/`fold_left` families with
  actual braced-init-list calls (the capability the paper is *for*, not
  just that the code compiles) inside a `static_assert`ed `constexpr
  test()` where possible; `deque`/`list`/`forward_list` (not
  constexpr-constructible in this implementation) and the PSTL
  `ExecutionPolicy` overloads (not `constexpr` at all) are split into
  separate runtime-only functions, matching the pattern the P1068R11
  session used for `uniform_int_distribution`. Full
  `libcxx/test/std/algorithms/` sweep: 302/309 passed (7 pre-existing
  unsupported, unrelated) on the narrower run, 384/391 including
  `iterators/predef.iterators/projected/` and
  `language.support/support.limits/support.limits.general/` on the wider
  one — 0 failures either way. Full container sweeps (`vector`/`deque`/
  `list`/`forwardlist`/`basic.string`): 868/877 passed (9 pre-existing
  unsupported), 0 failures. `transitive_includes.gen.py` and
  `headers_in_modulemap.sh.py` green; `module_std.gen.py` unsupported
  (same pre-existing lit-feature gap as every prior session this tracker
  has hit — the `iterator.inc`/`random.inc` module export lines added
  across this effort remain compile-unverified, not "green"). FTM
  generator run confirmed zero diff to `version`/`FeatureTestMacroTable.rst`
  (expected: `unimplemented: True` deliberately untouched).
  `Cxx2cPapers.csv` P2248R8 row flipped to `|Partial|` with a Notes-column
  breakdown of what's done vs. remaining; P3217R0 left unflipped (blocked
  on the same `ranges::` slice). Tier 6 table: P2248R8 row marked `[~]`
  (new marker — this tracker's first genuinely-partial-not-rejected Tier 6
  outcome, so `[x]`/`[!]` didn't fit; self-explained via the Notes column
  per this doc's established pattern for introducing markers).

  **Next session**: the remaining `ranges::` slice of P2248R8
  (`find`/`count`/`search_n`/`replace`/`replace_if`/`replace_copy`/
  `replace_copy_if`/`remove`/`remove_copy`/`lower_bound`/`upper_bound`/
  `equal_range`/`binary_search`/`contains`) plus P3217R0's `find_last`
  family is the natural continuation — every site needs `_Tp`/`_Type`
  moved to *after* `_Proj`/`_Comp` in libc++'s current declaration order
  (confirmed via direct grep of `ranges_find.h`/`ranges_fill.h`/
  `ranges_replace.h`/`ranges_lower_bound.h` two sessions ago), which is
  real per-site restructuring — budget accordingly, verify no internal
  code or test uses positional explicit template arguments on these
  algorithms before reordering (a silent-meaning-change risk the tenth
  session flagged), and only then flip
  `__cpp_lib_default_template_type_for_algorithm_values` and regenerate
  the 7 affected `.version.compile.pass.cpp` files. P3222R0 (blocked on
  Tier 3 mdspan work) and P3471R4 (Standard Library Hardening, still
  unassessed) remain the other two open Tier 6 rows.

- **2026-08-24 (twelfth session, same day as the eleventh)**: Completed
  the `ranges::` reordering slice of P2248R8 plus P3217R0's `find_last`
  addendum — the last piece this tracker had queued for the paper.
  Fetched both papers' raw HTML with `curl` (not `WebFetch`) again, per
  the established pattern, and extracted the exact target declaration
  order for every touched site from the `<del>`/`<ins>` diff markup
  directly rather than re-deriving it. Advisor flagged two hazards before
  editing that the prior session's scope-check hadn't fully separated:
  (1) in the binary-search family (`lower_bound`/`upper_bound`/
  `equal_range`/`binary_search`) and in `replace_if`, the value-type
  parameter can't just move to the end — it has to slot *between* `Proj`
  and the constrained `Comp`/`Pred` parameter, because `Comp`/`Pred`'s
  constraint (`indirect_strict_weak_order<const _Type*, ...>` /
  `indirect_unary_predicate<...>`) references it; appending at the end
  instead would reference an undeclared name. (2) `fill`/`fill_n`/
  `replace_copy`/`replace_copy_if` are a different edit class: their
  `output_iterator<const T&> O` constrained-parameter shorthand had to
  become a trailing `requires output_iterator<O, const T&>` clause so `O`
  could move before `T` (paper's new order) — and their defaults are
  `iter_value_t<O>` (no projection in these signatures), not
  `projected_value_t`; `replace_copy` mixes both (`T1 =
  projected_value_t<I, Proj>`, `T2 = iter_value_t<O>`) while plain
  `replace`'s second type just defaults to the first (`T2 = T1`). Touched
  16 headers: `ranges_find.h`, `ranges_count.h`, `ranges_search_n.h`,
  `ranges_replace.h`, `ranges_replace_if.h`, `ranges_replace_copy.h`,
  `ranges_replace_copy_if.h`, `ranges_fill.h`, `ranges_fill_n.h`,
  `ranges_remove.h`, `ranges_remove_copy.h`, `ranges_lower_bound.h`,
  `ranges_upper_bound.h`, `ranges_equal_range.h`, `ranges_binary_search.h`,
  `ranges_contains.h`, `ranges_find_last.h` (17 headers total). Added missing
  `<__iterator/projected.h>`/`<__iterator/iterator_traits.h>` includes to
  the five headers that gained a `projected_value_t`/`iter_value_t`
  reference but didn't already have them (`ranges_search_n.h`,
  `ranges_fill.h`, `ranges_fill_n.h`, `ranges_replace_copy.h`,
  `ranges_replace_copy_if.h`).

  Per advisor's third point, reordered every parameter list
  unconditionally (observable only via explicit template arguments, which
  a grep across `libcxx/test` and `libcxx/include` confirmed zero usage
  of for these algorithms) and guarded only the `= default` clauses behind
  `#if _LIBCPP_STD_VER >= 26`, using the same mid-declaration
  `class _Type #if ... = default #endif` trick as the non-range slice —
  including for the `output_iterator<...>` constraint move, which is now
  an unconditional trailing `requires` clause rather than duplicated
  behind the version check.

  Extended (not duplicated) the existing
  `default_template_type_for_algorithm_values.pass.cpp` with a `ranges::`
  block covering every touched algorithm via both the iterator-pair and
  range overloads with braced-init-list values, plus a case advisor asked
  for that the prior session's test coverage was missing: a projection
  whose result type differs from the range's value type
  (`ranges::find(v, {4}, [](const Point& p) { return p.y; })`, where the
  range holds `Point` but the projected/compared type is `int`) — this is
  the one thing that actually distinguishes `projected_value_t` from a
  naive `iter_value_t` default, and it was untested until now. Ran the
  full `libcxx/test/std/algorithms/` sweep (302/309, matching the prior
  session's baseline exactly, 7 pre-existing unsupported) plus the wider
  sweep including `iterators/predef.iterators/projected/` and
  `language.support/support.limits/support.limits.general/` (385/392, 7
  unsupported) and the container sweeps (`vector`/`deque`/`list`/
  `forwardlist`/`basic.string`, 566/568, 2 pre-existing unsupported) — 0
  failures across all three, confirming the reorder didn't regress any
  existing call site (per advisor's point: default arguments are lazily
  instantiated, so this is real evidence, not a tautology). Also ran
  `transitive_includes.gen.py` (125/125) and `headers_in_modulemap.sh.py`
  (1/1) clean — the five newly-added includes didn't shift the transitive
  graph in a way the generator objects to.

  Flipped `__cpp_lib_default_template_type_for_algorithm_values` to
  implemented (removed `"unimplemented": True` from
  `generate_feature_test_macro_components.py`) and regenerated —
  `version`, `FeatureTestMacroTable.rst`, and all 7 affected
  `.version.compile.pass.cpp` files (`algorithm`/`deque`/`forward_list`/
  `list`/`ranges`/`string`/`vector`) plus `version.version.compile.pass.cpp`
  (8 files total, matching the eleventh session's prediction exactly), all
  verified green via `libcxx-lit`. Before flipping, resolved the one open
  question from the prior session's advisor call: `ranges::fold_right`
  is one of the paper's `[alg.fold]` sites but isn't implemented in this
  fork at all (a separate, pre-existing gap from P2322R6, not something
  P2248R8's diff can be applied to since there's no declaration to touch).
  Decided this doesn't block `|Complete|` — every signature that actually
  exists in this fork and that the paper touches now has the default,
  which is the same shape as the existing `|Partial|` rows' caveats
  (compiler/implementation limitations on capabilities the fork doesn't
  have, not paper work left undone) except here there's nothing to
  implement at all for the missing piece, so `|Complete|` with a
  Notes-column caveat fit better than `|Partial|`. Flipped both
  `Cxx2cPapers.csv` rows: P2248R8 to `|Complete|` (Notes explain the
  fold_right carve-out) and P3217R0 to `|Complete|`. Tier 6 table: both
  checkboxes flipped to `[x]`.

  P2248R8 and P3217R0 are now fully closed out. P3222R0 (blocked on Tier 3
  mdspan work) and P3471R4 (Standard Library Hardening, still unassessed)
  remain the only open Tier 6 rows. **Next session**: pick up P3471R4
  (assess scope first — no prior session has looked at it) or revisit
  Tier 3's mdspan blocker to unblock P3222R0.

- **2026-08-24 (thirteenth session, same day as the twelfth)**: P3471R4
  (Standard Library Hardening) — audit-and-fill, not a from-scratch
  implementation, since libc++'s `_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS`-based
  hardening mode (`fast`/`extensive`/`debug`, `libcxx/include/__assert`) is
  inherited from upstream and predates this paper entirely. Fetched the raw
  paper HTML (`curl`, per the established process rule) and extracted the
  exact wording for both hardened-precondition tables (sequence containers'
  `operator[]`/`front()`/`back()`/`pop_front()`/`pop_back()`; `optional`/
  `expected`'s `operator->`/`operator*`/`error()`) plus the additional-
  functions list (`span::first`/`last`/`subspan`, `basic_string_view::
  remove_prefix`/`remove_suffix`, `span`'s dynamic-size range constructors,
  `mdspan`'s converting constructor). Audited every cell by grepping each
  implementation site directly rather than trusting the FTM generator's
  `unimplemented` flag (same discipline as the text_encoding/P1673R13
  sessions) — nearly everything was already correct: `array`, `vector`
  (incl. `vector<bool>`), `deque`, `list`, `basic_string`, `span` (both
  static- and dynamic-extent specializations, including every constructor's
  hardened precondition), `basic_string_view` (confirmed its `operator[]`
  correctly uses the *stronger* `pos < size()`, not `basic_string`'s
  `pos <= size()` — the exact distinction the paper's own Note 1 warns
  about), `bitset`, `valarray`, `optional` (both the primary template and
  the `optional<T&>` reference specialization from P2988R11), and
  `expected` (both the primary template and the `expected<void, E>`
  specialization) all had zero gaps.

  Found three real findings via advisor review before editing (first-pass
  self-review had flagged four; advisor cut one):

  1. **`inplace_vector`** (`libcxx/include/inplace_vector`) — `operator[]`/
     `front()`/`back()`/`pop_back()` had **zero** hardening in *both* the
     general (`N > 0`) and zero-capacity (`N == 0`) specializations; the
     `N == 0` specialization's accessors unconditionally dereferenced
     `data()` (always `nullptr`), an unconditional UB path with a comment
     acknowledging it ("exactly as calling front()/back()/operator[] on an
     empty vector is UB") but no `_LIBCPP_ASSERT` to make it a checked
     contract violation under hardening. Fixed: added `<__assert>` include
     and `_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS` to all four functions in the
     general specialization, and — per advisor's specific steer — used the
     same `_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(false, ...)` unconditional-
     failure shape `array<T, 0>::operator[]` already uses (not a
     `size()`-based check, since size() is always 0) for the `N == 0`
     specialization's `operator[]`/`front()`/`back()`, plus a matching
     `pop_back()` check. Confirmed `<inplace_vector>` has no
     `_LIBCPP_ENABLE_EXPERIMENTAL` gate (only `_LIBCPP_STD_VER >= 26`) per
     advisor's flag to check this after the eleventh session's PSTL-gate
     miss — no special test macro needed.
  2. **`mdspan::operator[]`** (`libcxx/include/__mdspan/mdspan.h`) — only
     the variadic-pack overload (`operator[](OtherIndexTypes...)`) had
     `_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS`; the `array<OtherIndexType,
     rank()>` and `span<OtherIndexType, rank()>` overloads were completely
     unchecked, confirmed by the pre-existing
     `assert.index_operator.pass.cpp` only ever exercising the variadic
     form. Fixed by wrapping each overload's body in an index-sequence
     lambda that checks `__mdspan_detail::__is_multidimensional_index_in`
     before calling `__acc_.access`, mirroring the variadic overload's
     check. Hit a real bug while doing this: an initial version moved
     `__acc_.access(...)` inside a lambda with implicit `auto` return-type
     deduction, which silently **strips the reference** from `access()`'s
     `reference` (`T&`) return — `auto` deduction follows template argument
     deduction rules, decaying references to prvalues — producing "non-
     const lvalue reference cannot bind to a temporary" at the *call site*
     of the lambda, not inside it, which was momentarily confusing. Fixed
     with an explicit `-> decltype(auto)` trailing return type on the
     lambda, preserving the reference. Recorded here since the same trap
     will bite anyone else moving a `reference`-returning call into an
     `auto`-deduced generic lambda in this codebase.
  3. **`forward_list::front()`/`pop_front()`** — use
     `_LIBCPP_ASSERT_NON_NULL` (a no-op in `fast` hardening mode) rather
     than `_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS` (checked in `fast`), unlike
     every other container's front()/pop_front() in this fork. Initially
     flagged as a fourth gap to fix; **advisor pushed back and this was
     correctly dropped, not fixed** — the paper's own wording
     ([structure.specifications] p3.4, §10.1) only requires that violating
     a hardened precondition be a contract violation *in a hardened
     implementation*; it says nothing about which of libc++'s three
     vendor-specific tiers (`fast`/`extensive`/`debug`) must catch it, and
     `extensive`/`debug` both already do. `NON_NULL` is also arguably the
     *more* semantically precise category here specifically: an empty
     `forward_list` means `__before_begin()->__next_ == nullptr`, so the
     failure mode really is a null-pointer dereference (a crash), not an
     attacker-influenced out-of-bounds read on a live heap pointer the way
     `vector::front()` after `clear()` is. Changing it would be unrequested
     divergence from the upstream categorization with ongoing rebase cost,
     the same reasoning this tracker already used to refuse extending
     `ExprConstant.cpp` for P1383R2. Left untouched; documented as a
     deliberate non-fix in the CSV Notes rather than silently ignored.

  New tests: `libcxx/test/libcxx/containers/sequences/inplace.vector/
  assert.pass.cpp` (new file — no prior hardening test existed for this
  container at all; covers both `N > 0` and `N == 0` specializations,
  including const-qualified overloads), and extended the existing mdspan
  `assert.index_operator.pass.cpp` with parallel `array`/`span`-overload
  blocks covering the same eight out-of-bounds cases as the pre-existing
  variadic-overload block, plus a valid-index case per overload to confirm
  no false positives. One test-writing bug caught by the compiler, not by
  review: `TEST_LIBCPP_ASSERT_FAILURE(m[std::array<int, 3>{-1, -1, -1}],
  ...)` fails to parse — braces don't protect commas from the C
  preprocessor's macro-argument splitting the way parens do, so the macro
  saw four arguments instead of two ("too many arguments provided to
  function-like macro invocation"); fixed by wrapping the whole
  subscript expression in an extra pair of parens
  (`(m[std::array<int, 3>{-1, -1, -1}])`), matching the pattern the
  pre-existing variadic-overload test already used for its own multi-arg
  `(m[-1, -1, -1])` calls — should have been obvious from precedent already
  in the file, but was missed on the first pass.

  Per advisor's specific instruction, ran every new/modified test at
  `--param hardening_mode=fast` explicitly (not just the lit default, which
  is `none` — every hardening assert test in this suite carries
  `UNSUPPORTED: libcpp-hardening-mode=none`, so a default-param run would
  have come back trivially UNSUPPORTED and proven nothing, the same trap
  the P3309R3 session hit with its `reinterpret_cast` alignment check) and
  confirmed the tests actually *ran* rather than skipped. Also ran
  `extensive` and `debug` explicitly for both new/modified test files — all
  three modes green for both. Broader regression sweeps, all at
  `hardening_mode=fast` (the discriminating mode for this paper, per
  advisor): `libcxx/test/std/containers/views/mdspan/` +
  `libcxx/test/libcxx/containers/views/mdspan/` (112 tests, 106 passed + 6
  pre-existing unsupported, 0 failures); `libcxx/test/std/containers/
  sequences/inplace.vector/` + `libcxx/test/libcxx/containers/sequences/
  inplace.vector/` (5 tests, all passed); the full `libcxx/test/std/
  containers/` + `libcxx/test/libcxx/containers/` tree, which includes
  `views/` (span, mdspan) and every sequence container (2020 tests, 1928
  passed + 92 pre-existing unsupported, 0 failures — confirms the mdspan
  and inplace_vector fixes didn't regress anything else under `containers/`
  and that `array`/`vector`/`deque`/`list`/`forward_list`/`string`/`span`/
  `bitset`/`valarray` are all still green after the audit); `libcxx/test/
  std/utilities/optional/` + `.../expected/` (176 tests, all passed, sanity
  check since these were audited but not edited). `libcxx-generate-files`
  produced zero diff (expected — no FTM added).

  **CSV status: `|Partial|`, not `|Complete|`** — per advisor: the decisive
  fact is that the paper's own §10.11 proposes `__cpp_lib_hardened_array`
  etc. as literal `20????L` placeholders ("we are not attached to any
  particular way of defining the feature-test macros"), unimplementable as
  written, and the actual LWG-adopted values can't be verified from this
  environment (no macro of this shape exists anywhere in this fork's
  generator or in upstream LLVM as of this Clang). This is the P1383R2/
  P3309R3 `|Partial|` shape (a provably-unimplementable wording section),
  not the P2248R8 `ranges::fold_right` shape (nothing to implement because
  the underlying function doesn't exist in this fork at all) — so
  `|Partial|`, with a Notes-column explanation, is the correct status
  rather than `|Complete|` with a caveat. Flipped `Cxx2cPapers.csv`'s
  P3471R4 row from blank to `|Partial|`; Tier 6 table: `[~]` (matching the
  P2248R8 precedent's introduction of that marker for a genuinely-partial,
  not-rejected outcome).

  P3471R4 is now assessed and substantively addressed — the runtime
  precondition-checking surface is complete modulo the documented
  `forward_list` non-fix; only the unverifiable FTM section remains open,
  and re-closing it requires external confirmation of the LWG-adopted
  macro names/values, not more code. It stays `[~]`, not `[x]`, for exactly
  that reason. P3222R0 (blocked on Tier 3's from-scratch mdspan/submdspan
  work) is the sole *fully*-open Tier 6 row. **Next session**: further
  P3471R4 work is blocked pending external confirmation of the adopted FTM
  wording — don't guess at it. Otherwise, pick up the Tier 3 mdspan blocker
  to unblock P3222R0, or move to a different tier (Tier 2/3's remaining
  open items) instead.

- **2026-08-24 (fourteenth session)**: User asked to work on completing
  Tier 3. Started with mdspan `submdspan`/padded layouts (P2630R4/P2642R6/
  P3355R1, the item blocking Tier 6's P3222R0) since that was the largest
  open Tier 3 item — see the dedicated sub-plan added above this session:
  **blocked**, not started, on an untracked prerequisite
  (`std::constant_wrapper`/P2781) discovered by checking the live draft
  before writing code, not just the tracked papers' own text. Advisor
  confirmed this was the right call and redirected to P1673R13 (linalg)
  as the session's actual deliverable — a real open Tier 3 row, not
  blocked, and the tracker had already pre-scoped it as "probably mostly
  done, verify the gaps" back on 2026-08-22.

  **Audit approach, per advisor's explicit sequencing**: mechanical
  name-set diff first (enumerate every function/type `libcxx/include/
  linalg` declares, diff against `[linalg.syn]` on eel.is), *then* let
  that bound the prose audit, rather than starting from prose. Extracted
  the synopsis via `grep`/regex against the eel.is-fetched text and
  diffed against the header's own declarations — clean, one apparent gap
  (`triangular_matrix_vector_2x2_product`) turned out to be a worked
  example inside a Note in `[linalg.reqs.alg]`, not a real overload the
  library must provide.

  **The real finding, caught only because advisor pushed to verify it was
  observable before mass-editing**: every algorithm in the current
  synopsis is declared with concept-constrained opaque type parameters
  (`template<in-matrix InMat, in-vector InVec, out-vector OutVec>
  void matrix_vector_product(InMat A, InVec x, OutVec y);`), but this
  fork's implementation used fully-decomposed `mdspan<ElementType,
  Extents, Layout, Accessor>` parameters checked only via `static_assert`
  in the body — meaning a wrong-rank or non-writable-output call was a
  hard compile error deep inside the function, not a SFINAE-visible
  exclusion from the overload set. Wrote a throwaway probe before editing
  anything: `static_assert(!requires{ matrix_vector_product(A, x,
  wrong_rank_y); })` — confirmed it evaluated to `false` today (the
  ill-formed call was still a valid, matched candidate) and `true` after
  the fix. Hit the same compiler quirk the P3309R3 session had already
  found and documented: a raw inline `!requires{...}` expression, when
  overload resolution excludes every candidate via a mix of constraint
  failure and arity mismatch, hard-errors in this compiler's diagnostic
  path instead of gracefully evaluating false; wrapping the check in a
  named `concept` (as that prior session's finding prescribes) fixed it
  immediately. Recorded again here since this is now the second time this
  exact shape has bitten a session — worth remembering as a standing
  quirk of this compiler, not re-discovering each time.

  Added the eleven `[linalg.helpers.concepts]` exposition-only concepts
  (`is-mdspan`, `in/out/inout-vector`, `in/out/inout-matrix`,
  `possibly-packed-out-matrix`, `in/out/inout-object`, `scalar`) as
  `__detail::__is_mdspan_v`/`__in_vector`/`__out_vector`/.../
  `__linalg_scalar`, matching the standard wording verbatim. **Chose a
  trailing-`requires`-clause retrofit over restructuring every function's
  parameter list to take the concepts' opaque type directly** — advisor's
  call, confirmed correct: reconstructing the mdspan type from each
  function's already-deduced `ElementType`/`Extents`/`Layout`/`Accessor`
  template parameters and checking it against the concept is functionally
  equivalent for every well-formed call, needed zero changes to any
  function body (the deduced names like `_MatrixExtents` stay valid and
  in scope), and avoided touching ~90 function *signatures'* parameter
  declarations — only adding one clause each. Per advisor's explicit
  instruction, did **not** delete the pre-existing
  `_LIBCPP_LINALG_REQUIRE_WRITABLE_MDSPAN`/`_LIBCPP_LINALG_REQUIRE_UNIQUE_MAPPING`
  static-assert macros or the rank-only static_asserts alongside the new
  requires-clauses — the macros give a diagnosable message
  ("linalg output mdspan must be writable") that the requires-clause
  alone would silently swallow into a generic "constraints not
  satisfied", and removing them is a separable, lower-risk cleanup for
  later, not bundled into this sweep.

  **One genuine scope decision, not a bug**: `copy`, `scale`, `add`, and
  `swap_elements` use `in-object`/`out-object`/`inout-object` in the
  synopsis, which the wording restricts to rank 1 or 2 — but this fork's
  `copy()`/`add()` (and, by the identical code shape,
  `scale()`/`swap_elements()`) deliberately and *already-testedly* accept
  rank-0 mdspans too (`libcxx/test/std/numerics/linalg/
  execution_policies.pass.cpp`'s `test_add_ranks()`, with an explicit
  comment: "add() accepts rank-0, rank-1 and rank-2 operands, matching
  copy()"). Applying the literal standard concept to these four functions
  would have silently regressed a pre-existing, intentional, already-
  tested extension — confirmed this was deliberate (not an oversight)
  before deciding to leave these four on `static_assert`-only checking
  rather than retrofit them. Recorded as a documented, permanent scope
  exclusion, not a TODO.

  Landed the retrofit across BLAS 1, 2, and 3 in six separate commits (one
  per logical batch: BLAS 1; BLAS 2 vector-product/rank-1; the
  `symmetric_matrix_product`/`hermitian_matrix_product` "xxmm" family;
  `trmm`/rank-2; rank-k/rank-2k; `trsv`/`trsm` plus the remaining
  triangle-updating `matrix_product` overloads), running the full
  `linalg/` suite (17/17 green throughout, never regressed) after every
  batch rather than once at the end — an intentional sequencing choice
  per advisor's warning that "92 edits in one commit with one test run at
  the end is the shape that makes a mid-sweep mistake expensive to find."
  A post-hoc sweep script (checking every `constexpr` declaration in the
  file for a nearby `requires`) caught one real function the batching had
  actually skipped — the 4-argument `matrix_vector_product` overload (the
  "updating" form with an addend vector) — fixed in the final commit; the
  same script's first two draft versions produced false positives from a
  crude line-window heuristic that mistook `_Divide __divide = {}`'s
  empty-braces default argument for the start of a function body, so the
  real gap wasn't obvious until the heuristic was tightened and each
  candidate was checked directly with `grep`, not trusted at face value.

  Full `linalg/` suite (17/17) and the full `numerics/` suite (940/940, 2
  pre-existing unsupported) both green after the final commit.

  **`|Partial|`, not `|Complete|`**: the live draft's `__cpp_lib_linalg`
  is `202511L`; this fork's generator/`<version>` still say `202311L`,
  and the paper or LWG issue that moved the value couldn't be identified
  from this environment (checked the 2024/2025 WG21 papers index, came
  back empty) — same "don't invent an unverifiable target" discipline as
  P3471R4's FTM macros, just discovered via the value itself rather than
  the macro name. A full member-by-member prose audit of every
  function's Preconditions/Effects/Complexity against the standard
  wording — the kind done for `text_encoding` in Tier 6 — was also not
  attempted this session (90-odd functions across three BLAS tiers is a
  different scale than that facility's five member functions); this is
  an *unverified area*, not a known defect, and is not by itself why the
  status is `|Partial|` — the FTM mismatch alone carries that.

  Post-hoc verification (advisor-prompted, same session): reconciled the
  concept-retrofit sweep mechanically — 82 `constexpr` function
  definitions in the header, 68 with a `requires` clause referencing one
  of the 11 new concepts, and the remaining 14 are exactly the
  documented unconstrained set (6 `__detail` helpers + 4 transform views
  + the 4 rank-0-scoped functions) — no missed functions. Also added
  `libcxx/test/std/numerics/linalg/helper_concepts.pass.cpp`, a
  standing test (the prior `zzprobe.pass.cpp` was thrown away after
  manual confirmation) that exercises the concepts directly rather than
  relying on existing tests only proving valid calls still compile: it
  asserts a wrong-rank output is SFINAE-excluded from
  `matrix_vector_product`, a `const`-element output is excluded via
  `out-vector`'s `is_assignable_v` half, and — the discriminating case —
  a `layout_blas_packed` output *is* accepted by
  `symmetric_matrix_rank_1_update`'s `possibly-packed-out-matrix`
  parameter (which plain `out-matrix` would have rejected, since a
  packed layout has no unique mapping). Separately dropped a spurious
  `remove_cvref_t` from `__linalg_scalar`'s `is_execution_policy_v`
  check — the standard's `scalar` concept applies it to `T` directly, so
  the fork now transcribes the wording verbatim instead of an
  unexplained variant.

  P1673R13 is now substantively audited, its one confirmed conformance
  gap is closed, and the retrofit itself has direct test coverage (not
  just non-regression).

  **Follow-up, same session: resolved the `__cpp_lib_linalg` FTM
  chain.** Rather than leaving "what moved it to `202511L`"
  unidentified, cloned `cplusplus/draft` and walked
  `git log -p`/the GitHub commits API for `source/support.tex`'s
  `cpp_lib_linalg` line. Full chain: `202311L` (P1673R13 itself,
  2023-11-16) → `202411L` (**P3222R0**, "transposed special cases for
  P2642 layouts", 2024-12-16 — already a blank row in this CSV, and
  already known from this session's earlier mdspan sub-plan to be
  blocked on P2642R6/`std::constant_wrapper`) → `202412L` (P3050R2,
  already `|Complete|`) → `202511L` (**P3371R5**, "make the rank-1,
  rank-2, rank-k, and rank-2k updates consistent with the BLAS",
  2025-11-04 at Kona — not in `Cxx2cPapers.csv` at all until now).
  Fetched P3371R5 in full and checked its four required changes
  against `libcxx/include/linalg`: (1) updating/E-taking overloads for
  every rank-1/2/k/2k function, (2) overwriting-not-accumulating
  semantics for the alpha-only overloads, and (4) alpha-only overloads
  retained plus `scalar` excluding mdspan/execution_policy were **all
  already correct** — predating this session, likely because the
  original scaffold was written against a post-P3371R5 draft even
  though the FTM literal was never updated to match. (3) was a real,
  previously-unfound gap: `hermitian_matrix_rank_1_update` and
  `hermitian_matrix_rank_k_update` used `alpha` directly instead of
  `real-if-needed(alpha)`, and all four `hermitian_matrix_rank_
  {1,2,k,2k}_update` E-taking overloads used a diagonal `E[i, i]`
  directly instead of `real-if-needed(E[i, i])` — both meaning a
  complex `alpha`, or an `E` whose diagonal isn't already exactly
  real, could leave a nonzero imaginary part on a Hermitian result's
  diagonal. Fixed in `libcxx/include/linalg` (6 call sites across 4
  functions); added `hermitian_real_if_needed.pass.cpp` with `alpha`/
  `E`-diagonal inputs that have deliberately nonzero imaginary parts
  (confirmed by hand-deriving expected outputs and cross-checking with
  a scratch binary before trusting the lit test — caught and fixed an
  unrelated bug in the *test's* own row-major flat-array indexing
  along the way, not in the header). Landed as P3371R5 `|Complete|` in
  its own new CSV row. **This does not unblock the FTM bump itself**:
  P3222R0 is earlier in the same chain and remains genuinely blocked,
  so `__cpp_lib_linalg` correctly stays `202311L` and P1673R13 stays
  `|Partial|` — but the reason is now a fully-traced, named blocker
  (P3222R0 → P2642R6 → `std::constant_wrapper`) instead of an
  unidentified gap.

  **Advisor-prompted verification, same session, before calling this
  done.** Two gaps in the above: (1) confirmed item (2)
  (overwriting-not-accumulating) by reading, not just spot-checking,
  every non-`E` overload of symmetric/hermitian rank-1/2/k/2k — all
  correct, no further changes needed. (2) `__real_if_needed` had zero
  call sites in the header outside `__blas_abs` before this session's
  6 fixes, which was itself a signal worth following further:
  `grep -n "real-if-needed" ` against the cached full `[linalg]` draft
  text (`linalg_full.txt` from the mdspan-blocker investigation)
  turned up a paragraph in `[linalg.general]` (¶4) that the
  rank-update-specific ones I'd already read are actually
  *restatements* of — the general rule applies to *any* function
  reading through a triangle-tagged, `hermitian`-named matrix
  parameter, not just the rank-update family's `E`. That covers
  `hermitian_matrix_vector_product` (both overloads) and all four
  `hermitian_matrix_product`/`[linalg.algs.blas3.xxmm]` overloads
  (whichever argument is the Hermitian factor) — each had the exact
  same `__stored ? raw : conj_if_needed(...)` ternary, missing the
  diagonal case, as the rank-update `E` parameter had. Fixed those 6
  call sites the same way (diagonal → `real_if_needed`, matching the
  general rule's `real-if-needed(m[i, i])`). Verified against a
  scratch binary before trusting the lit test (same discipline as the
  first `hermitian_real_if_needed.pass.cpp` bug), then added 3 more
  cases to that file — one per distinct index-pair shape
  (`(i, j)`/vector, `(i, k)`/left-factor, `(k, j)`/right-factor) —
  rather than one per identical overload, since the two members of
  each `replace_all` pair are byte-identical in the fixed expression
  and differ only in unrelated addend/loop-bound code. Full
  `linalg/` suite (19/19) and `numerics/` suite (942/944, 2
  pre-existing unsupported) both green after this follow-up. This is
  exactly the kind of gap a mechanical name-set diff or an SFINAE
  pass cannot find — only reading the actual wording clause and
  grepping for what's unused catches it — worth remembering next time
  a helper function in this header shows near-zero usage.

  **Next session**: either (a) attempt the prose audit this session
  skipped (budget for real per-function wording comparison, not a
  mechanical sweep — same caution this tracker has given every
  similarly-shaped task), or (b) implement `std::constant_wrapper`
  (P2781, untracked, needs its own `Cxx2cPapers.csv` row) to unblock
  both P3222R0 and the mdspan `submdspan`/padded-layouts sub-plan at
  once — they share the exact same prerequisite. P3050R2 and P3371R5
  (both already `|Complete|`) do not need re-auditing as part of
  either follow-up. Tier 3's only remaining fully-unblocked, unstarted
  work is `format`/`print`'s P3107R5/P3235R3 redesign (assessed out of
  scope in a prior session, needs its own dedicated session) and
  P2757R3 (blocked on untracked C++23 P2419R2).
