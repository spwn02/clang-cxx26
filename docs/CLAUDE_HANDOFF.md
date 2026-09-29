# Handoff — session 2026-09-29

## Wave 7 and P1642 complete

- Wave 7: #36 constant-size SIMD range constraints; #37 constant permutation
  indices, full-width validation, and sign-safe sentinel checks. Existing P2280
  support suffices; no frontend changes.
- #79: P1642 freestanding sweep complete; feature macros now survive regeneration.
  Optional hosted facilities are allowed by the adopted compliance wording.
- Fixed two existing module defects exposed by validation: special math imports
  its stdfloat dependencies; execution system-context header is registered.
- Focused SIMD/freestanding/feature-macro gate: 133 passed.
- Full libc++ correctness gate (benchmarks excluded): 12,607 selected;
  11,489 passed, 26 expected failures, 1,089 unsupported, three preexisting failures.
  Two reproduce with pre-task headers: valarray nodiscard diagnostics and print
  nonlocking formatter assertions. The third, execution header registration,
  is repaired; all 150 affected module/registration tests passed (one unsupported).
- Remaining waves: Wave 5 #118; Wave 6 #23, #30, #148. #144 (clang-tidy
  orphan-node crash) closed 2026-09-30: root-caused to ASTReader replaying a
  cross-module implicit-member update without splicing it into the class's
  lexical decl chain (see `clang/lib/Serialization/ASTReader.cpp`,
  `finishPendingActions`, and issue #144's closing comment). Keep #116 final
  conformance review deferred until all other issues close. Waves 0–4 and
  7–9 have no open issues.
- Full gate began at 92da86fc39f4 with the final unsigned-index fix in the
  worktree, subsequently committed as ad8e794a4340; raw archive stamp retained.

## Previous handoff (historical)

# Handoff — session 2026-09-27 (continuation)

## Status: Waves 8 and 9 fully closed

Continuation of the overnight 2026-09-27 session (see git log around `cxx26-2026.09.27.1`). User directive
for this segment: "start with #65 and #78/#93, then proceed with closing wave 8 (#41, #50, if new issues
occur - file them and pursue). Complete waves 8 and 9 is our priority now." Both waves are now done:

- **Wave 9 — C++17/20 residuals + triage backlogs: 0 open, 22 closed.**
- **Wave 8 — Language-side clang/ gaps: 1 open (#147, newly filed this session), 14 closed.**

`#116` (final conformance review) remains deliberately untouched — per explicit user correction, it is only
to be worked once every other issue is closed, not before.

## Closed this session

- **#65** (C++20 LWG backlog) — 292/292 `Cxx20Issues.csv` rows Complete/Nothing To Do; zero Partial.
- **#78** (C++23 LWG backlog) — corrected 4 rows that a prior session had wrongly left Partial (LWG3798,
  LWG3530, LWG3750 — all pre-existing documentation errors, not real gaps) and fixed one genuine gap
  (LWG3515: `stacktrace`'s `operator<<` was over-templated on charT/traits; now a plain non-template
  overload per the adopted resolution). LWG3834 stays Partial, correctly blocked on #128.
- **#93** (NB rollups) — closed with a summary covering the extract audit, `<meta>` trait exceptions,
  P3179R9 bounded algorithms, and the confirmed-still-accurate `common_type`/`common_reference`/
  `underlying_type`/`invoke_result` hard-error limitation (a genuine compiler-diagnostic-model constraint,
  not a bug) plus the sender `check-types` throw-based-diagnostics infeasibility finding.
- **#41** (P2795R5 erroneous behaviour) — M3 finished: a fuller `_LIBCPP_INDETERMINATE` sweep across the
  random engines (`linear_congruential_engine`, `mersenne_twister_engine`, `subtract_with_carry_engine`,
  `philox_engine`), each site provably safe via `[rand.req.seedseq]`'s "fills the entire range" contract.
- **#50** (P1467R9 extended floating-point types) — fully landed for the four required types
  (float16_t/bfloat16_t/float32_t/float64_t): `<charconv>` (genuine shortest-round-trip search, not the
  discarded convert-through-float approximation), `<format>` (classification + a corrected runtime test —
  the merged test initially had two wrong expected literals, both caught and fixed by cross-checking every
  assertion against plain float/double equivalents), `<complex>` (C++23 converting constructor with the
  correct `explicit(...)` rank comparison per [complex.members], full arithmetic/transcendental coverage,
  and a verified stream round-trip). See `docs/design/stdfloat_p1467.md` for the final milestone map.

## New issue filed

- **#147** — `float128_t`'s `<cmath>` support beyond abs/fabs/sqrt/classification hits a hard `static_assert`
  in the generic promotion-fallback template (not "ambiguity" as an earlier session's note claimed). Root
  cause confirmed: M2a's "no quad libm" assumption was wrong — glibc 2.26+ actually ships one
  (`hypotf128`/`atan2f128`/`sinf128`/... confirmed present via `nm -D libm.so.6`), so this is a real,
  fixable gap, not a permanent scope boundary. Bundled the never-attempted C++17 special math functions
  (`assoc_laguerre`, `riemann_zeta`, ...) for the extended types into the same issue since both are the
  same "extended-float `<cmath>` completeness" theme. Correctly did **not** reopen #50 for this — filed
  fresh per this project's standing "close issue, check dangling refs" practice.

## Verification performed this session

- `complex.number` lit suite: 98/98 at c++23 and c++26.
- `format` lit suite: 116/116 (was 115/116 before the two test-literal fixes).
- Full header sweep (`header_all_standards.gen.py`): 596/596 across c++17/20/23/26.
- `system_reserved_names.gen.py` + `transitive_includes.gen.py` guards: 279/279.
- `headers_in_modulemap.sh.py`: 1/1.
- Full `check-cxx` (`build-libcxx/libcxx/test`) run at session end — see the commit log / next session's
  first message for the result; it was still running as this doc was written.
- Module exports: confirmed no new top-level names were added by this session's changes (the `<complex>`
  edit is a new constructor overload on an existing exported class template), so `libcxx/modules/std/*.inc`
  needed no update.

## Commits this session (on top of `cxx26-2026.09.27.1`)

- Stacktrace `operator<<` de-templating (LWG3515) + CSV corrections for #78's 4 rows.
- `_LIBCPP_INDETERMINATE` sweep for #41 M3.
- `<charconv>` extended-float support (#50 M2b).
- `<format>` + `<complex>` extended-float support (#50 M2c/M3) — 2 commits (implementation, then the
  stream-round-trip test + design-note update).

## Not touched

- **#116** final conformance review — explicitly deferred until it is the last open issue on the roadmap.
- **#147** — filed but not attempted this session.

## Two-lane workflow (Claude + Codex) notes, reconfirmed working this session

- `scratchpad/codex/codex_watchdog2.sh` queue pattern held up across 4 more dispatches (130/140/150/160).
  Lesson re-confirmed the hard way: touching the live worktree with `git checkout -q -- .` right after one
  job "finishes" can race the next job the watchdog already auto-started — use the atomic
  `results/<job>.patch`/`.untracked.tar` snapshots as the merge source instead, never the live worktree.
- Every merge extracted only the hunks relevant to the current issue from a patch that also carried
  already-merged prior work, via a small python `diff --git` splitter + `git apply --include=...`, rather
  than applying the whole patch — the patches accumulate all uncommitted worktree state, not just the
  latest dispatch's delta.
- `~/.local/opt/clang-wip` (frozen snapshot compiler) is compile-only for Codex — it can't link, so any
  "verified at runtime" claim from a dispatch report needs independent re-verification with the real,
  freshly-rebuilt toolchain before being trusted. Caught two Codex-authored test bugs this way in
  `format.stdfloat.pass.cpp` (wrong `{:e}` and `{:a}` expected literals) and one report accurately
  self-flagged as compile-only (`<complex>` stream round-trip) — later independently confirmed passing at
  runtime.

## Codex worktree

`../clang-p2996-codex` — should be back at a clean, synced state now that jobs 130/140/150/160 are all
merged; verify before reusing next session.
