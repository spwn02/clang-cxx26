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
| [~] | P2944R3 | `reference_wrapper` comparisons | Partial — blocked on `optional`/`tuple` equality changes from P2165R4; check if P2988R11 work unblocked this |
| [~] | P1383R2 | `constexpr` for `<cmath>`/`<cstdlib>` | `<complex>` done; scalar math functions remain |
| [x] | P3168R2 | `std::optional` range support | Done 2026-08-20 — implementation was already complete via P2988R11; added missing test coverage |

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
| [~] | P2300R10 | `std::execution` (sender/receiver) | In progress 2026-08-20 — see dedicated sub-plan below. Scope confirmed to collapse with P3325R5/P3396R1 into one effort (merged draft wording); do not attempt as one commit. |
| [~] | P3325R5 | Execution environment utility | Folded into the P2300R10 sub-plan below (its content is `[exec.envs]`/`prop`+`env`, M1) — tracked/flipped together with P2300R10, not separately |
| [~] | P3396R1 | `std::execution` wording fixes | No separable content — already merged into current draft wording used by the sub-plan below; flips to Complete alongside P2300R10 at M6, not separately implemented |
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
- [~] **M5** — Remaining sender adaptors: `upon_error`/`upon_stopped` **done
  2026-08-21**, `let_value`/`let_error`/`let_stopped` **done 2026-08-21**,
  `starts_on`/`continues_on`/`on`/`schedule_from`, `when_all`/
  `when_all_with_variant`, `into_variant`, `stopped_as_optional`/
  `stopped_as_error`, `write_env`, `unstoppable`, `bulk`/`bulk_chunked`/
  `bulk_unchunked`. (`associate`/`spawn`/`spawn_future` are part of the
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
- [ ] **M6** — Coroutine integration: `as_awaitable`,
  `with_awaitable_senders`. Flip `__cpp_lib_senders` `unimplemented: False`
  and all three CSV rows to `|Complete|` **only here** — it's a single
  all-or-nothing `202406` value; flipping it at any earlier milestone
  would make conforming user code detect a feature surface that isn't
  fully there yet.

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
set to `|In Progress|` as of this sub-plan (2026-08-20); do not touch
`__cpp_lib_senders`'s `unimplemented` flag until M6.

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
