# Reflection Closeup Tracker

Persistent, cross-session tracking document for the **Reflection Closeup epic** — a second,
narrower follow-on to the now-closed Reflection Closure Epic (2026-09-08 through 2026-09-10,
`docs/REFLECTION.md`/`docs/REFLECTION_GAPS.md` deleted, tagged `cxx26-2026.09.10`). That epic's own
7-point completion bar was met, but 14 concrete items were deliberately deferred along the way.
This epic's only job is to close every one of them for real — genuine fixes, not documented
exceptions. Full context, the completion bar, and the model-escalation policy are in the plan at
`/home/spawn/.claude/plans/i-think-finishing-reflection-elegant-rivest.md` — **read that first**,
this document is the live tracker, not a duplicate of the plan.

**Completion bar (do not weaken this):** every item below must get an actual, full fix. If an item
resists a full fix even after a genuine escalated-model attempt, do not close it under a lesser bar
and do not leave it silently unresolved — stop and escalate the specific blocker back to the user.

**Model escalation, confirmed working 2026-09-10:**
- Terra: `codex exec -m gpt-5.6-terra -s workspace-write -C <dir> -o <report> - < prompt.txt`
  (current stable CLI, `codex-cli 0.152.0`, no special setup needed).
- Astra: `mise exec codex@0.153.0-alpha.2 -- codex exec -m gpt-6-astra -s workspace-write -C <dir>
  -o <report> - < prompt.txt` (needs the alpha CLI — installed via `mise install
  codex@0.153.0-alpha.2`, invoked via `mise exec` rather than changing the global `codex = "latest"`
  pin in `~/.config/mise/config.toml`, so other projects on this machine keep using stable).
  **Astra was only unlocked with the user's explicit permission this session** (installing an alpha
  CLI) — this is a one-time environment change already done, not something to repeat or reconsider.
- Add `-c model_reasoning_effort=xhigh` to either invocation for the hardest items — confirmed
  valid and accepted (not just `high`; `xhigh` is the real ceiling, verified directly with `gpt-6-astra`).

**Policy change, 2026-09-10 (explicit user instruction after round 1): do not use Astra.** It made
genuinely good architectural progress on item 1 but consumed ~93% of the 5-hour usage window in
roughly 15-20 minutes of work. **Use Terra at most, only if absolutely necessary, and be very
careful even then** — prefer doing work directly (Claude's own Read/Edit/Bash tools: build and
test personally) over any Codex dispatch by default now. Only reach for Terra when a task
genuinely needs Codex-scale parallel exploration or a fresh model perspective, not as the default
mode of operation this epic started with.
- **Weekly-usage resets (the user has 3) are the user's to authorize, never mine to trigger.** If a
  Codex dispatch (Terra or Astra) reports hitting a weekly usage cap, stop and ask the user before
  anything resets it. This does not apply to ordinary 5-hour-limit resets.

## Next Up

**Item 1 (escalation cluster): real progress made and documented, NOT fixed, Codex work paused.**
The user flagged mid-dispatch that the 5-hour Codex usage window had dropped to ~7% remaining.
The Astra dispatch (pid 693539) was interrupted cleanly (SIGINT, no corruption) rather than risk
it dying uncontrolled. Its on-disk diff (6 files: `Sema.h`, `SemaDecl.cpp`, `SemaDeclCXX.cpp`,
`SemaExpr.cpp`, `SemaTemplateInstantiateDecl.cpp`, `consteval-only-types.cpp`) was personally
verified with direct `ninja`/`lit` calls (no further Codex usage spent) — see the dated entry
below for the full before/after evidence. **Verdict: real, substantive progress on the actual
target bug, but not a clean fix — reverted, not committed.**

**Key finding from this round, worth preserving even though the fix didn't land:** the "5 known
SemaCXX failures" this item's description names are **not all the same bug**. Isolated via
git-stash: `PR98671.cpp` crashes (SIGABRT, a C++20 concepts/`IsAtLeastAsConstrained` assertion)
identically on the clean baseline — entirely unrelated to consteval escalation, likely deserves
its own item, and is arguably not reflection-scoped at all (flag to the user before pursuing).
`cxx2a-constexpr-dynalloc.cpp`/`cxx2b-consteval-propagate.cpp` show the exact right diagnostic
*family* (missing "call to consteval function ... is not a constant expression") but Astra's diff
made byte-for-byte zero difference to either — they're missing the diagnostic via a **different**
code path (implicit special member functions / `if constexpr` conditions, not the explicit
`ActOnCXXEnterDeclInitializer` variable-initializer path this fix targets) — this matches a note
already in the original epic's August-era history that these two were suspected to have a
different root cause. Only `builtin-is-within-lifetime.cpp` and `constant-expression-cxx11.cpp`
are the actual explicit-variable-initializer self-reference cases this fix's design addresses, and
Astra's diff **did fix both correctly**, plus `clang/test/Reflection/` stayed 20/20 including the
smuggling test (`consteval-only-types.cpp`) — the design direction (a new `CheckingConstantInitializer`
field on `ExpressionEvaluationContextRecord`, replacing the overloaded `ImmediateFunctionContext`
reuse) is sound for the narrow case. What broke: the libc++ reflection suite went from the
documented 7-failure baseline to 23 failures — 16 new regressions, listed in the dated entry below
— meaning the nested-expression-merging logic added to `HandleImmediateInvocations` for handling
sub-initializers is not yet correct for the broader space of ordinary reflection declarations.

**Next action, once Codex usage recovers:** resume with Terra (cheaper) rather than Astra first,
handing it this exact finding (the design is right for the target case, the regression is
specifically in the `HandleImmediateInvocations` nested-context-merging addition — see the diff
excerpt in the dated entry) rather than starting over. **Do not dispatch any more Codex work until
usage has recovered** — this is a 5-hour rolling limit, not the weekly cap the user must
personally authorize resetting; it should recover on its own with time, no action needed from
either side.

**Non-Codex investigation done while paused (2026-09-10, no Codex usage spent):**
- Read `PR98671.cpp` and `SemaConcept.cpp:2563-2581` directly: the crash is
  `Sema::IsAtLeastAsConstrained`'s `#ifndef NDEBUG` assertion (only visible on this
  assertions-enabled build) firing when `SetEligibleMethods`/`ComputeSpecialMemberFunctionsEligiblity`
  passes an instantiated `FunctionDecl` where a non-instantiated one is expected, while computing
  special-member eligibility for `S2<int>`'s two constrained constructor templates. Confirmed via
  the test file's own comment ("Ensure that no assertion is raised...") that this is a regression
  test for a known upstream issue (`PR98671`) that has re-broken in some form — entirely C++20
  concepts/special-member machinery, zero reflection content. **Not pursuing a fix — out of this
  epic's 14-item scope; flagging to the user as a possible separate item rather than scope-creeping
  it in unilaterally.**
- Re-read the relevant lines of Astra's (reverted, not recoverable via git — only visible in the
  log) diff for the exact type of the new field: `VarDecl *CheckingConstantInitializer = nullptr;`
  on `ExpressionEvaluationContextRecord` (not a bare `bool` — rules out the simplest "two unrelated
  declarations look identical" hypothesis). The merge condition in `HandleImmediateInvocations` was
  `Rec.CheckingConstantInitializer == SemaRef.parentEvaluationContext().CheckingConstantInitializer`.
  The log also showed at least one path where a *child* context's field is set by inheriting the
  *parent's* value (`ExprEvalContexts.back().CheckingConstantInitializer = Prev.CheckingConstantInitializer;`)
  rather than always being freshly assigned per new top-level declaration — worth the next
  dispatch's first debugging step being: instrument exactly which `VarDecl*` value each of the 16
  regressed tests' declarations sees at the merge-condition check, since a stale/reused value
  (e.g. from `ExprEvalContexts`' underlying vector reusing a popped slot's memory before the new
  push fully overwrites this specific field) would explain corruption bleeding between unrelated
  sibling `constexpr auto X = ...;` declarations in the same file — exactly the pattern in the 16
  regressed tests (all have many sequential top-level reflection declarations in one namespace;
  the 2 tests that stayed fixed have only isolated single declarations). This is a lead for the
  next dispatch to verify empirically, not a confirmed root cause — I did not rebuild/test this
  hypothesis myself (the diff isn't recoverable without re-generating it, and doing so would risk
  another build cycle without the ability to verify against real Codex-assisted debugging anyway).

## The 14 items

| # | Item | Status | Notes |
|---|---|---|---|
| 1 | Consteval self-reference escalation cluster | 4th attempt in progress, reverted, usage-paused | Astra's round-1 design (`CheckingConstantInitializer` field) correctly fixed the 2 tests actually in scope; introduced 16 new libcxx regressions in `HandleImmediateInvocations`'s nested-context-merging logic. Reverted, not committed. See the dated 2026-09-10 entry below for full evidence and the exact regression list. Also discovered: only 2 of the originally-named "5 SemaCXX tests" are this bug; `PR98671.cpp` is an unrelated pre-existing concepts crash, `cxx2a-constexpr-dynalloc.cpp`/`cxx2b-consteval-propagate.cpp` are a different consteval-escalation bug (different code path). Resume with Terra once Codex usage recovers. |
| 2 | Issue #237 — closure-type alias loses identity | Not started | `using ct = typename[:cr:];` fails for closure types. Start from `SemaReflect.cpp`'s splice-type handling. |
| 3 | Issue #188 — `display_string_of(dealias(...))` not constant expr | Not started | Bottoms out in `pretty_printer::print` → `reflect_invoke(^^tprint, ...)` constant evaluation for canonicalized template-specialization types. |
| 4 | NEW-7 — dependent splice-specifier wrongly accepted (CTAD-like position) | Not started | `docs/reflection-audit/codex-new7-design-report.md` — 3 prior attempts, 3 different bugs. Needs to understand `AddInitializerToDecl` timing for dependent splice-typed declarators. |
| 5 | Issues #180/#181 — expansion-statement body deferral + non-copyable tuple binding | Not started | `docs/reflection-audit/codex-m4-180-181-report.md` — PR #261's design applicable, ~11-file port. #181 has an independent binding defect (`SemaExpand.cpp:245-285`, `:150`). |
| 6 | Issue #182 — `template for` + `continue` ICE | Not started | CodeGen needs instance-discard awareness in expansion control-flow lowering. |
| 7 | Issue #150 — spliced explicit destructor call `~[:info:]()` | Not started | Needs a new splice-bearing destructor-name AST/Sema representation. |
| 8 | CWG 3111 residual — nested/multi-dimensional arrays | Not started | `libcxx/include/meta:1602-1645` has the flat-array fix + limitation note. `FixedArray<ValTy, Vals...>` can't hold an array-typed pack element — needs a real backing representation, not just a rejection. |
| 9 | P3560R2 strategy 2 — ~20 remaining Throws-bearing metafunctions | Not started | `docs/reflection-audit/codex-strategy2-design.md` + `codex-strategy2-pilot-report.md` — two real blockers found (no evaluator API for arbitrary-input exception construction; a genuine evaluator SIGABRT in the inherited-constructor path). Read both before touching this. |
| 10 | `3560-18` — `access_context::via` catch-and-inspect coverage | Not started | Blocked on a return-object lifetime workaround in constant evaluation. Smaller/separate from #9. |
| 11 | `define_static_object` entirely missing (P3491R3) | Not started | Zero occurrences anywhere in the tree. |
| 12 | `is_string_literal` (5 overloads) missing (P3491R3) | Not started | Issue #168 has a ready-made upstream implementation targeting the legacy `experimental/meta` API — needs adaptation, not verbatim port. |
| 13 | `reflect_constant_string` narrower than spec + `reflect_constant_array` Mandates unenforced | Not started | Fork has 2 fixed overloads (`char`/`char8_t`) vs. paper's generic template (+ `wchar_t`/`char16_t`/`char32_t`); missing "already a string literal" carve-out; `copy_constructible`/structural-type Mandates not checked. |
| 14 | Issues #184, #187, #208, #212, #253, #275 — needs-reproducer | Not started | All previously blocked on a dead Godbolt link. Re-check the live upstream issue threads for accumulated detail before giving up on each. |

## Ground truth / where to look

See the plan file's "Ground truth / where to look" section — not duplicated here to avoid drift
between two copies. Key anchors: `clang/lib/Sema/SemaExpr.cpp:18396`, `libcxx/include/meta:1602-1645`,
`clang/include/clang/AST/MetaActions.h`, `docs/reflection-audit/*.md`, `AGENTS.md`'s "Known
pre-existing baseline failures" section.

## Session Log

### 2026-09-10 — CU0 setup
Created this tracker. Confirmed Terra (`gpt-5.6-terra`, stable CLI) and Astra (`gpt-6-astra`,
needed the alpha CLI `0.153.0-alpha.2`, installed with the user's explicit permission via `mise
install codex@0.153.0-alpha.2` and invoked via `mise exec` rather than changing the global pin) both
resolve and respond. Seeded the 14-item table from the approved plan. Setting up the recurring cron
heartbeat next, then starting CU1 with item 1 (escalation cluster) using Astra directly.

### 2026-09-10 (later) — Item 1 first Astra round: real progress, not landed, usage-constrained

Dispatched Astra at `xhigh` effort with the full 3-attempt history, exact regression gate, and a
design lead (give the C++23 constexpr/constinit-initializer case its own
`ExpressionEvaluationContext` state instead of reusing `ImmediateFunctionContext`). It converged on
a design along those lines: a new `CheckingConstantInitializer` bit on
`ExpressionEvaluationContextRecord`, with `ActOnCXXEnterDeclInitializer` (`SemaDeclCXX.cpp`) always
pushing `PotentiallyEvaluated` now and setting this new flag instead of conditionally picking
`ImmediateFunctionContext`; `CheckForImmediateInvocation` (`SemaExpr.cpp`) skips its escalation-
marking when the flag is set; `HandleImmediateInvocations` merges a nested sub-context's candidates
up into its parent when both share the same `CheckingConstantInitializer` value (a new block right
before the existing "manifestly constant-evaluated" bailout) rather than processing them in place.
Mirrored in `SemaTemplateInstantiateDecl.cpp`'s template-instantiation variable-initializer path.

**The user flagged the 5-hour Codex usage window had dropped to ~7% remaining mid-dispatch.**
Interrupted the process with `SIGINT` (pid 693539) — exited cleanly, log ends with "turn
interrupted", no partial/corrupted file state; `git status` showed a coherent 6-file diff, not a
half-written mess. Verified the result personally from there using only direct `ninja`/`lit` calls
(zero additional Codex usage spent):

- Rebuilt `clang` with the diff applied — succeeded cleanly.
- The 5 documented `SemaCXX` tests: 2 passed (`builtin-is-within-lifetime.cpp`,
  `constant-expression-cxx11.cpp` — genuinely fixed, verified the diagnostic is now present and
  correct), 3 still failed (`PR98671.cpp`, `cxx2a-constexpr-dynalloc.cpp`,
  `cxx2b-consteval-propagate.cpp`).
- **Isolation via `git stash`**: re-ran the same 5 tests against the unmodified baseline.
  `PR98671.cpp` crashes (SIGABRT) identically with or without the diff —
  `clang/lib/Sema/SemaConcept.cpp:2579`'s `IsAtLeastAsConstrained` assertion
  (`"use non-instantiated function declaration for constraints partial ordering"`), triggered while
  instantiating `S2<int>` and computing special-member eligibility. **Pre-existing, unrelated to
  consteval escalation or reflection at all** — a pure C++20 concepts/constraints bug. Worth its
  own item, but likely out of this reflection epic's scope; flag to the user before pursuing.
  `cxx2a-constexpr-dynalloc.cpp` and `cxx2b-consteval-propagate.cpp` produce **byte-identical**
  `-verify` failure output with or without the diff — the diff changed nothing for either. Both are
  missing the same diagnostic *family* ("call to consteval function ... is not a constant
  expression") but via implicit-special-member-function-definition and `if constexpr`-condition
  paths, not the explicit `constexpr`/`constinit` variable-initializer path this fix's design
  targets — matches a suspicion already on record from the original epic's August history that
  these two don't share the escalation cluster's exact root cause. **So of the 5, only 2 are
  actually in this fix's scope, and both of those 2 now pass correctly.**
- `clang/test/Reflection/`: 20/20, including `consteval-only-types.cpp` (the smuggling test) —
  this is the exact test that broke the third prior attempt, and it's clean here.
- `libcxx/test/std/experimental/reflection/` (full subtree, via the real `libcxx-lit` wrapper):
  **23 failures, up from the documented 7-test baseline** — a real regression. The 7 baseline
  tests are all still present in the 23 (not fixed, not worsened), but 16 new failures appeared:
  `define-aggregate.verify.cpp`, `description-of-template-kinds.verify.cpp`,
  `m5-p2996-batch1.verify.cpp`, `m5-p2996-batch2.verify.cpp`, `m5-p2996-batch3.verify.cpp`,
  `m5-p2996-batch7.verify.cpp`, `m5-p2996-batch8.verify.cpp`, `m5-p2996-batch9.verify.cpp`,
  `m5-p2996-batch10.verify.cpp`, `m5-p2996-batch11.verify.cpp`,
  `m5-p3491-p3560-p3795-batch16.verify.cpp`, `m5-p3795-batch14.verify.cpp`,
  `new-3-annotations-with-type.verify.cpp`, `new-4-closure-inaccessible.verify.cpp`,
  `substitute.verify.cpp`, `to-and-from-values.verify.cpp`. Not yet root-caused — the regression is
  most plausibly in `HandleImmediateInvocations`'s new nested-context-merging block (the design's
  riskiest, least-tested new piece — it was mid-way through Astra's own verification pass, testing
  ~20 declaration shapes with temporary trace instrumentation, when it was interrupted; it had not
  yet reached the libc++ suite).

**Reverted the diff entirely** (`git checkout --` on all 6 touched files, confirmed clean, rebuilt
to restore a known-good tree) — not safe to commit given the completion bar. Nothing committed or
pushed this round. **Codex usage-constrained: pausing further Codex dispatches on this or any item
until the 5-hour window recovers naturally** (this is the rolling 5-hour limit, not the weekly cap
the user must personally authorize — no action needed, just time). When resuming: hand the next
session (Terra first, cheaper, since the design direction is already validated for the target
case) this exact diff shape and the 16-test regression list as a starting point rather than a blank
slate — the fix isn't wrong, it's incomplete specifically around nested-context merging.
