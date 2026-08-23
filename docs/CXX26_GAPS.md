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
| [!] | P2900R14 | Contracts | **Deferred** — see Scope section above |

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
    — **not started, confirmed from-scratch.** `libcxx/include/__mdspan/`
    has only `layout_left.h`/`layout_right.h`/`layout_stride.h`/
    `extents.h`/`aligned_accessor.h`/`mdspan.h`; no `submdspan` free
    function, `submdspan_mapping`, or `layout_left_padded`/
    `layout_right_padded` anywhere in the tree. Matches the FTM: the
    generator's `__cpp_lib_submdspan` entry is `unimplemented: True` with
    the C++26 padded-layout value line literally commented out. This is a
    real from-scratch implementation project, not a conformance pass —
    scope it as its own session.
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
| [ ] | P1673R13 | BLAS-based linear algebra interface | Assessed 2026-08-22, not a from-scratch project — 3150-line implementation with 17 pre-existing passing tests; CSV status is likely stale, not accurate. Needs a real audit against the paper's wording before flipping, not assumed — see mdspan/linalg block note |
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
| [x] | P0493R5 | Atomic min/max | Complete 2026-08-23 — `fetch_max`/`fetch_min` added to `atomic<Integral>`, `atomic<Floating-point>`, `atomic_ref<Integral>`, `atomic_ref<Floating-point>`, plus the `atomic_fetch_max(_explicit)`/`atomic_fetch_min(_explicit)` free functions. Integral routes through the pre-existing `__c11_atomic_fetch_max/min`/`__atomic_fetch_max/min` compiler builtins (single `atomicrmw`, no CAS loop); floating-point follows `fmaximum_num`/`fminimum_num` NaN semantics via the same CAS-loop fallback pattern `fetch_add`/`fetch_sub` already used for the fp80-long-double case |
| [x] | P2835R7 | `atomic_ref` object address exposure | Complete 2026-08-23 — `constexpr T* address() const noexcept` added to `__atomic_ref_base<T>` (inherited by every specialization). Per the paper itself the return type is `T*`, not the `COPYCV(T,void)*` shown on eel.is — that form comes from a later merge (likely P3323R1-adjacent wording polish), not this paper; don't "fix" this back if a future session compares against the live draft |
| [ ] | P3323R1 | cv-qualified types in `atomic`/`atomic_ref` | Assessed 2026-08-23, not attempted — see session log. Two real blockers, not scope-timidity: (1) eel.is is post-P3309R3-merge, so its `atomic_ref` synopsis has an extra converting constructor this paper does not add (confirmed against the paper's own wording diff) and marks everything `constexpr` — can't be copied directly, every signature needs hand reconciliation against the paper text; (2) the conformant signatures use `value_type` (`remove_cv_t<T>`) for store/load/exchange/compare_exchange parameters, but `__atomic_ref_base`'s internal `__compare_exchange`/`__clear_padding` helpers are typed on `T*` itself — for `atomic_ref<volatile T>` this is a real type mismatch, not just a `requires`-clause gap, so a conformant implementation needs those internals re-threaded first. What *is* already confirmed safe: specialization selection still works unchanged (`std::integral<const int>` is satisfied, so `atomic_ref<const int>` already picks the integral specialization), and `atomic_ref<T* const>` naturally falls through to the primary (non-pointer) template rather than needing special-casing, since `T* const` doesn't match the `atomic_ref<_Tp*>` partial-specialization pattern |
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

| Status | Paper | Feature |
|---|---|---|
| [ ] | P2592R3 | Hashing support for `std::chrono` value classes |
| [ ] | P1885R12 | `text_encoding` naming |
| [ ] | P2862R1 | `text_encoding::name()` should never return null |
| [x] | P2641R4 | Checking if a `union` alternative is active |
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
  pre-existing, unrelated reflection-module (`std.cppm`/P2996) failure
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
