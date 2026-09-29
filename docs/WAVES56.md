# Waves 5 and 6 closure

Active since 2026-09-29. Issues #23, #118, #148, #30 remain open until
implementation, review, full correctness gates, and publication finish.
Issue #116 remains deferred. Preserve ABI, upstream authorship and licenses.
GitHub milestone 6 is "Wave 5 — Atomics" (only #118 remains open);
milestone 7 is "Wave 6 — Long tail" (#23, #30 and #148 remain open).
Verified against GitHub on 2026-09-29; #116 belongs to neither milestone.

## Required work

- #23: finish P2198R7, P2338R4, P2013R5, P2407R5, P2937R0,
  P2833R2 and P2976R1. Audit adopted declarations/deletion rules,
  optional global allocation and detection, generator-owned macros,
  exception/thread-disabled configurations and module exports.
- #118: discover installed MPFR 4.2.2 through CMake, document dependency
  licensing separately, implement outstanding P1383 operations with exact
  APFloat/MPFR conversion and target rounding. Test all supported formats,
  exceptional values, large arguments, boundary rounding and rejected
  domain/range errors. Preserve runtime calls; do not advertise incomplete
  P0533/P1383 support.
- #148: stable evaluator exception identity and shared lifetime; compiler
  operations for capture/retain/release/rethrow, constexpr library paths
  preserving runtime ABI and symbols. Retain `optional<const E&>` cast
  interface, reuse handler matching, destroy exactly once, reject escapes.
- #30: freestanding `<utility>` checkpoint and generated macro, compiler
  builtin and LLVM intrinsic preserving the defined observable prefix even
  against later UB. Remove only at final machine-code lowering. Cover I/O
  and contract boundaries, optimizer inference, LTO, instrumentation and
  debug information. A memory barrier alone does not satisfy P1494R5.

## Coordination and gates

Only gpt-6-luna workers at low reasoning effort. Sparse isolated worktrees
hold proposals; orchestrator owns review, shared builds, integration,
focused tests, commits and publication. No worker may modify shared builds.
Archive current full Clang/libc++ baselines before integration. Exclude
benchmarks; investigate every introduced failure/ICE and pursue discovered
bugs to resolution, even after filing them. After AST/Sema changes rebuild
Clang and explicitly clean/rebuild libc++. All Ninja commands use `-j$(nproc)`.
Commit coherent validated steps with tracker updates. Push validated
milestones, close issues/waves, publish annotated prerelease tag, finish
with a clean working tree.

## Continuation

`cxx26/dev/wave-supervisor.py` runs as a host systemd user service. Its
state, session IDs, prompts, attempts, retry times and JSONL logs live in
`build-waves56-state/`; isolated sparse worktrees live in
`build-waves56-worktrees/`. Baseline archives remain in `.git/waves56/archive/`.
A file lock excludes duplicate runners;
systemd's control-group cleanup kills prior workers before restarting.
Only an explicit usage-limit error schedules retry at the reported reset
time (five hours when absent). Other execution failures stop for inspection.
Successful worker turns require integration review, never issue closure.
Workers must produce a fresh structured result; incomplete work
with further progress available resumes its saved session after one minute.
Missing/stale results or an explicit blocker require inspection. Reported
completion records evidence and remains subject to review.
Use `systemctl --user status cxx26-waves56` and inspect `state.json`/logs.
Stop through `systemctl --user stop cxx26-waves56`; a `STOP` file also stops
dispatch between turns. User service survives interactive session closure;
reboot/logout persistence is not claimed (user lingering is disabled).
The supervisor stays alive while idle and accepts newly queued worker tasks.
Root orchestrator owns integration and Git publication; a worker's inability
to stage protected Git metadata does not block source implementation.

## Session log

- 2026-09-29: Clean starting SHA `fc70a4e11032`. Installed MPFR reports
  4.2.2; Release build trees, Clang assertions enabled. Validated Luna/low
  access and host service launch. Added durable serial worker supervisor
  with recovery/limit tests. Infrastructure and worker launch validation
  underway; no feature or issue is claimed complete.
- 2026-09-29: Launched detached `cxx26-waves56-baseline` (Clang followed
  by libc++ correctness gates) and `cxx26-waves56` (four isolated proposals,
  then integration). Corrected mutually exclusive CLI launch flags and
  added safe handling when a worker exits before consuming stdin. Five
  supervisor tests pass, covering recovery, duplicate exclusion, usage-limit
  session preservation and required integration evidence. First live worker
  saved session `01a0ea53-a7f5-7702-958a-8772cd302c52`; implementation
  and baseline validation are running, not complete.
- 2026-09-29: Verified live supervisor restart resumed the same saved Luna
  session and sparse worktree. Added directory fsync for durable state and
  recognition of local clock/date reset messages, with focused coverage.
  No new frontend/library implementation has been integrated yet.
- 2026-09-29: Current archived full Clang baseline: 44,706 PASS, 25 XFAIL,
  5,171 UNSUPPORTED and one FAIL (`SemaCXX/cxx2b-consteval-propagate.cpp`).
  This known failure is part of the previously recorded immediate-invocation
  cluster; the older five-failure count is not the current gate baseline.
  Archive stamp `check-clang-20260928T232136Z-c4bce44feeeb-waves56-baseline`.
  Full libc++ correctness baseline started with benchmarks excluded.
- 2026-09-29: Six supervisor tests pass; missing executable/worktree now
  records infrastructure failure and stops instead of restart-looping.
  Initial libc++ failures match the preceding gate's known valarray
  nodiscard and print bounded-write failures; the full run remains active.
- 2026-09-29: Both baselines finished: libc++ 11,490 PASS, 26 XFAIL,
  1,089 UNSUPPORTED, two known failures and 134 excluded benchmarks.
  Initial worker queue stopped with incomplete proposals and an unwritable
  integration-result path under Git metadata. Moved state/worktrees into
  writable ignored build directories, resumed freestanding session, and
  retained integration/commits at the root. Luna/low math, exceptions and
  checkpoints implementation resumes in parallel; root validates variant
  independently. No issue or wave closure is claimed.
- 2026-09-29: Integrated and validated P2407R5 variant boundary: eight
  deleted get overloads, surviving constexpr get_if/visitation/emplacement,
  regenerated feature macro. All 56 focused hosted/freestanding/macro tests
  pass. Math and checkpoint compiler core build is running; review corrected
  checkpoint terminator ordering, willreturn suppression and MPFR conversion
  double rounding before validation. Other feature work remains active.
