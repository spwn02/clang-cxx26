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
- **Weekly-usage resets (the user has 3) are the user's to authorize, never mine to trigger.** If a
  Codex dispatch (Terra or Astra) reports hitting a weekly usage cap, stop and ask the user before
  anything resets it. This does not apply to ordinary 5-hour-limit resets.

## Next Up

**CU0 complete. CU1 item 1 (escalation cluster) dispatched to Astra at xhigh effort, in flight.**
Launched via `mise exec codex@0.153.0-alpha.2 -- codex exec -m gpt-6-astra -c
model_reasoning_effort=xhigh ...`, report will land at
`docs/reflection-audit/codex-closeup-item1-escalation-report.md` /
`codex-closeup-item1-escalation-final.md`, log at
`docs/reflection-audit/batch-outputs/codex-closeup-item1-escalation.log`. This is expected to run
long (genuine architecture design work, no deadline). When it returns: **personally, independently
verify any claimed fix** (build, run the exact regression gate listed in the dispatch prompt
yourself) before trusting it — do not commit on Astra's own "fixed and verified" claim alone, per
this epic's inherited validator discipline. If it reports a genuine full fix: verify, commit
(`reflection:` prefix), push, update this table's row 1 to Fixed-and-verified with evidence, move
to item 2. If it reports exhausting effort without a fix: read its writeup, decide whether the
Completion Bar's "escalate to the user" clause applies (it does, per the plan — do not close this
item under a lesser bar or move on silently), and surface it to the user precisely as instructed.

## The 14 items

| # | Item | Status | Notes |
|---|---|---|---|
| 1 | Consteval self-reference escalation cluster | Not started | `clang/lib/Sema/SemaExpr.cpp:18396`'s `HandleImmediateInvocations` has the full 3-attempt history as a source comment — read it in full before starting. Affects 5 `SemaCXX` tests. Tier 0, hardest item — use Astra from the start. |
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
