# Waves 5/6 handoff for Claude

Updated 2026-09-29. Continue from this file and `docs/WAVES56.md`; do not
restart implementation or trust a prior session's completion claim without
checking the current diff and test archives.

## Objective and current state

The task is to finish waves 5 and 6 and close #118, #23, #30, and #148 only
after implementation, review, validation, commits, and publication. Issue #116
is deliberately deferred. Preserve ABI, source provenance, and licenses.

The working tree is at `a7ca98e7709d5618281751bd9c099906ff6a74e3` with 118
changed/untracked paths; the aggregate tracked diff is 98 files, 3,163
insertions and 744 deletions. The feature implementation is integrated in the
shared checkout but is uncommitted and unpushed. `git diff --check` passed.
Review the whole current diff before staging: do not assume every changed path
is safe to publish as one commit. The workspace currently includes changes for
all four wave issues and some tracker/report edits.

GitHub issue snapshots fetched from `spwn02/clang-cxx26` on 2026-09-29 show
#23 (milestone 7), #30 (milestone 7), #118 (milestone 6), and #148 (milestone
7) all still open. The seven empty completed milestones for Waves 0–4, 8, and
9 were already closed earlier, as recorded in `docs/WAVES56.md`. No milestone
tool was available to independently list milestone records in this session.
Do not close milestones 6/7 or the four open issues while their criteria and
publication remain incomplete.

## Implemented scope

- **#23 freestanding**: changes cover all seven tracked papers (P2198R7,
  P2338R4, P2013R5, P2407R5, P2937R0, P2833R2, P2976R1). See
  `docs/waves56-freestanding-report.md` for declaration audit and tests.
  The Luna worker's structured result is
  `build-waves56-state/freestanding-result.json`; it is a proposal report,
  not a root sign-off.
- **#118 constexpr math**: external MPFR 4.2.2 discovery and documentation,
  APFloat/MPFR integer-significand transfer, adaptive rounding, evaluator
  support for P1383 functions, and wrapper annotations are present. P0533's
  feature macro intentionally stays disabled because its whole declaration
  surface is not proven. See `docs/waves56-math-report.md`.
- **#148 exception handles**: evaluator-owned identity and lifetime hooks,
  constexpr-only libc++ paths, runtime ABI preservation, and optional-ref cast
  behavior are present. See `docs/waves56-exceptions-report.md`.
- **#30 observable checkpoints**: Clang builtin, LLVM intrinsic and optimizer
  treatment, final lowering removal, freestanding utility declaration/macro,
  and I/O/contract insertion paths are present. See
  `docs/waves56-checkpoints-report.md`.

These reports describe implementation and validation, not issue completion.
Inspect source changes and official adopted wording before accepting them.

## Validation already completed

Baselines were archived before integration:

- Clang: `.git/waves56/archive/results/check-clang-20260928T232136Z-c4bce44feeeb-waves56-baseline.json`
  — 44,706 PASS, 25 XFAIL, 5,171 UNSUPPORTED, one known failure
  (`SemaCXX/cxx2b-consteval-propagate.cpp`).
- libc++: `.git/waves56/archive/results/check-cxx-20260928T232624Z-d2b30e2af33f-waves56-baseline.json`
  — 11,490 PASS, 26 XFAIL, 1,089 UNSUPPORTED, two known failures
  (`valarray/nodiscard.verify.cpp`, `print/bounded_writes.pass.cpp`),
  benchmarks excluded.

Final archived candidates:

- Clang: `/tmp/waves56-candidate/results/check-clang-20260929T134820Z-a7ca98e7709d-post-math-fixes.json`
  — 44,707 PASS, 25 XFAIL, 5,171 UNSUPPORTED, four failures. Diff against
  baseline found three additions, all `utils/update_cc_test_checks/*` helper
  failures from Python 3.14 forkserver socket denials in the sandbox; the
  remaining failure is the known baseline test. The full gate ran before the
  final removal of an experimental constexpr `div` evaluator path. Focused
  tests after that removal passed, but rerun/archive full `check-clang` if
  the available session budget permits before calling the gate final.
- libc++: `/tmp/waves56-candidate/results/check-cxx-20260929T153254Z-a7ca98e7709d-post-clean-clang-rebuild.json`
  — 11,497 PASS, 26 XFAIL, 1,089 UNSUPPORTED, five failures. Diff found only
  three added environment failures: GDB ptrace denial and two Unix socket
  bind denials in filesystem tests. No feature regression remained. Benchmarks
  were excluded.

Additional evidence:

- Focused compiler tests for checkpoints, exceptions and math passed; math
  focused set was 12/12. The exact-math test was corrected so `remquo` checks
  only the standard-guaranteed low three quotient bits. Experimental constexpr
  `div` support was removed because it is not a P0533 constexpr operation.
- Focused libc++ feature suite passed 133/133. Additional targeted module,
  C++23 math reference, header lint, modulemap and transitive-include checks
  passed 6/6.
- After the final AST change, libc++ was explicitly cleaned and rebuilt:
  `CCACHE_DISABLE=1 ninja -C build-libcxx -j$(nproc) -t clean cxx`, then
  `CCACHE_DISABLE=1 ninja -C build-libcxx -j$(nproc) cxx` (2011/2011 actions).
- Freestanding worker matrix reports ten positive checks, 18 expected
  negatives, configured allocator positive, 16 generated macro checks,
  C++17 execution policy boundaries, and hosted/freestanding module parses.
- `testdiff.py` exits 1 when it reports newly failing tests. The three
  additions in each suite were independently identified as sandbox-only.

Python 3.14 lit workaround, if needed in this sandbox:

```sh
python3 -c 'import multiprocessing,runpy,sys; multiprocessing.set_start_method("fork"); sys.argv=["build-nyx/bin/llvm-lit", *sys.argv[1:]]; runpy.run_path("build-nyx/bin/llvm-lit",run_name="__main__")' <lit args>
```

For libcxx-lit use `PYTHONPATH=/tmp/waves56-python`. Always use `-j$(nproc)`
for Ninja and set `CCACHE_DISABLE=1`. After every AST/Sema edit, clean and
rebuild libc++ `cxx` as above before trusting libc++ tests.

## Tracker and reports

Read `docs/WAVES56.md`, `docs/CXX26_GAPS.md`, and the four
`docs/waves56-*-report.md` files. The reports and tracker have a 2026-09-29
gate addendum; earlier historical entries are intentionally retained. Current
issues remain open. The issue #23 GitHub description has stale pre-implementation
checkboxes and #118/#148 descriptions predate this implementation; update issue
text only after reviewing final evidence. Do not use a tracker checkbox as a
substitute for the issue's full acceptance criteria.

## Continuation infrastructure is unavailable

The documented supervisor exists at `cxx26/dev/wave-supervisor.py`, with state
at `build-waves56-state/state.json`. On the latest check, state had
`stay_alive: true` but PID 1610517 was absent; `systemctl --user status
cxx26-waves56` failed because the user bus was inaccessible. Only the
freestanding task was recorded and it was already `reported_complete`; no
other workers were queued. Its saved failure says the usage limit resets at
6:12 AM, with no reliable date interpretation. Therefore no overnight wake-up
is running or verified. Do not claim that it will resume. A user/system
service or active session must be restored before any persistent continuation
can be relied on. The issue and tests can still be continued interactively.

## Next work

1. Re-read `AGENTS.md`, `docs/WAVES56.md`, all feature reports, and the actual
   full diff. Check current build outputs and the archived xUnit records.
2. Re-derive each issue's current adopted wording and acceptance criteria.
   Math deserves special scrutiny: issue #118 is P1383, not merely P0533;
   test supported target formats, rounding edges and rejected range/domain
   cases. For #30, scrutinize whether the intrinsic semantics truly preserve
   the defined observable prefix through every optimization pipeline. For
   #148, validate exactly-once destruction and escape diagnostics. For #23,
   verify generated configs, modules, language modes, and disabled
   exceptions/threads.
3. Resolve any real failures/bugs, add tests, and rerun affected gates. Rerun
   full Clang after the final AST edit. Preserve baseline/environment
   distinctions explicitly.
4. Split and commit coherent validated source changes with tracker updates;
   push only validated commits. Git metadata was read-only in prior attempts,
   so check whether repository commit/push is available and report an explicit
   blocker if not. Do not stage unrelated edits.
5. Only after all gates, review, commits and publication are complete, update
   GitHub issues/milestones and cut the annotated prerelease tag. Keep #116
   deferred. End with a clean tree and an accurate summary.
