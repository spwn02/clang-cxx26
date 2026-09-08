# C++26 Reflection Closure Tracker

Persistent, cross-session tracking document for the Reflection Closure Epic — closing this fork's
gaps against (a) the C++26 reflection papers as actually adopted (not as originally drafted) and
(b) the real upstream issue tracker. Started 2026-09-08. This document supersedes
`docs/REFLECTION.md`'s informal "nearly all of static reflection is supported" claim with an actual
audit. Read this document first at the start of any reflection session; update it in place as work
completes; append a dated Session Log entry before ending a session.

This is the **single source of truth** for reflection work — mirrors `docs/CXX26_GAPS.md`'s
structure and discipline. The epic's full plan (context, completion definition, milestones M0-M7,
verification protocol) lives at
`/home/spawn/.claude/plans/i-think-finishing-reflection-elegant-rivest.md` — read that first if
you're picking this up cold.

## Next Up (updated 2026-09-08, epic start)

Working M0 (setup, essentially done) and M2 (paper audit) in parallel with M1 (upstream issue
triage, running as 5 parallel Codex batches in the background as of this writing — check
`docs/reflection-audit/batch-outputs/batch-0{0..4}-triage.md` for results, fold into the Issue
Triage table below once all 5 land). Next concrete actions once triage batches land: merge them
into the table below, then start M3 (the three known bugs) and continue M2 for the remaining 7
original papers + DRs.

## Ground truth (established during epic setup, 2026-09-08)

**Upstream tracker.** `docs/REFLECTION.md`'s stated issue tracker
(`github.com/bloomberg/clang-static-reflection`) is **dead (404)**. The real upstream is
**`github.com/bloomberg/clang-p2996`**, confirmed by git lineage: this fork's `cxx26` branch has
commit `0def1e7101e1` ("Initial release of Clang/P2996.") as a direct ancestor, and that exact SHA
exists in bloomberg/clang-p2996. As of 2026-09-08: **85 open issues, 35 open PRs**, snapshotted to
`docs/reflection-audit/upstream-issues-snapshot.json` / `upstream-prs-snapshot.json`. Confirmed via
`git fetch .../clang-p2996 p2996 --depth=1` that upstream's current `p2996` branch tip
(`bde8bcf3104e`) is **not** an ancestor of local HEAD — real divergence, not just a shallow-fetch
artifact (`git rev-list --count` resolved cleanly against local history). This fork does not
cross-reference upstream issue numbers anywhere (no numbered mapping to build from — each issue
needs an independent read).

**Build config.** `build-nyx` already has `LLVM_ENABLE_ASSERTIONS=ON` (contrary to
`CXX26_GAPS.md`'s Tier 0 note that it was off — flipped at some point during the 52-failure-triage
epic). `build-libcxx` has `LIBCXXABI_ENABLE_ASSERTIONS=ON`. **Disk is tight**: root filesystem at
99% full (3.4G free) as of epic start — `build-libcxx` alone is 20G, `build-nyx` 6.5G,
`build-libcxx-asan` 3.4G. Watch this during heavy build phases (M3+); a `ninja` failure with a
disk-space-flavored error should not be mistaken for a code bug.

## Paper-by-paper audit (M2)

Full list: P2996R13, P1306R5, P3096R12, P3293R3, P3394R4, P3491R3, P3560R2 (original 7, adopted
Sofia June 2025) + P3617R0, P3687R1 (both also Sofia) + P3795R2 (Croydon, 2026-03-26 — the meeting
that finished C++26) + P1789R3 (Kona, expansion-statement library support) + DRs CWG 3111, LWG
4432, LWG 4426, LWG 4428.

| Paper | Disposition | Evidence |
|---|---|---|
| P3617R0 `reflect_constant_{array,string}` | **Likely-Already-Implemented** | `libcxx/include/meta:2610-2665` — `reflect_constant_array`/`reflect_constant_string` both exist with the paper's exact split; `define_static_array` already delegates to `reflect_constant_array` matching the paper's refactored `_Effects_` clause almost verbatim. **Not yet verified**: exact Mandates (structural-type / constructible_from / copy_constructible constraints) and the template-parameter-object non-uniqueness note — needs a line-by-line check against the fetched wording before marking `Complete`. |
| P3687R1 Poll 1 (remove template-arg splicing) | **Already-Conformant** | `clang/lib/Parse/ParseTemplate.cpp:1345-1357` explicitly rejects an unparenthesized splice as a template argument (`err_splice_template_argument`), with a comment citing "CXX26: ... to carve out syntactic space for future splice template arguments" — this fork was already deliberately conservative here, matching the paper's removal. No non-conformance found. |
| P3687R1 Poll 2a (entity proxy) | **Substantially implemented, flag-gated** | `ReflectionKind::EntityProxy` is a real AST-level reflection kind with broad handling across `clang/lib/AST/ExprConstantMeta.cpp` (member queries, layout, names, etc. all have `EntityProxy` cases). `underlying_entity_of`/`proxied_entity_of`/`is_entity_proxy` all exist in both `libcxx/include/meta` and the compiler backend (`clang/lib/AST/ExprConstantMeta.cpp:137,143,382`), wired into the metafunction dispatch table. Old name `dealias` kept as a deprecated alias — exactly the paper's rename pattern. Gated behind `LangOpts.EntityProxyReflection` (real flag, default off, `LangOptions.def:289`). **Not yet verified**: whether `docs/REFLECTION.md`'s flag list needs updating to mention this (it currently doesn't name an entity-proxy flag at all — docs are stale here), and whether the `_DEALIAS_` exposition-only threading through `members_of`/`bases_of`/`size_of`/etc. matches the paper exactly (spot-checked a few call sites, not exhaustive). |
| P3687R1 Poll 2b (using-decl reflection ill-formed by default) | **Already-Conformant** | `clang/lib/AST/ExprConstantMeta.cpp:1480` — `UsingShadowDecl` is rejected as reflectable (`return false`) unless `EntityProxyReflection` is explicitly on. Default behavior matches the paper's "ill-formed" requirement; the flag-gated alternative is the newer proxy semantics from Poll 2a, not a conformance violation of the default. |
| P3795R2 (Croydon cleanup) | **Confirmed-Open, real gap** | Grepped `libcxx/include/meta`, `clang/lib/Sema/SemaReflect.cpp`, `clang/lib/AST/ExprConstantMeta.cpp`, `clang/include/clang/AST/Metafunction.h` for `current_function`/`current_class`/`current_namespace`/`is_applicable_type`/`is_nothrow_applicable_type`/`apply_result` — **zero matches, none exist**. `data_member_options` (`libcxx/include/meta:2331-2358`) has no `annotations` field (contrast: the sibling `enumerator_options` struct *does* already have one — exactly the kind of cross-paper asymmetry this cleanup paper exists to fix). Parameter-annotation extension and the error-handling front-matter clarification not yet checked. **This is the single largest confirmed paper-level gap found so far** — matches the pre-epic prediction that pre-March-2026 work would be built against stale wording. |
| P2996R13, P1306R5, P3096R12, P3293R3, P3394R4, P3491R3, P3560R2 | **Not yet audited this epic** | `docs/REFLECTION.md` claims these are supported; only spot-checked incidentally so far (e.g. `dealias`/`data_member_options` above). Full clause-by-clause audit still pending — see M2 in the plan. |
| CWG 3111 (array-type template parameter objects) | **Not yet checked** | Resolved as a C++26 DR at Kona 2025-11-07; affects [meta.define.static]-family reflection queries for array-type template parameter objects. Cross-check against `reflect_constant_array`'s implementation above once its own audit is complete — likely the same code path. |
| LWG 4432 (element init for `reflect_constant_array`) | **Not yet checked** | Resolved Kona 2025-11-04/08. Clarifies copy- vs. direct-initialization semantics for array elements — check `reflect_constant_array`'s actual element-init strategy in `ExprConstantMeta.cpp` against this. |
| LWG 4426 (`reflect_constant_string` literal detection) | **Not yet checked** | Live-checked 2026-09-08: status is now **C++26** (moved from Tentatively Ready). Wording changes "trailing null terminator" → "trailing u+0000 null character" and adds "is a reference to" for precision, in [meta.reflection.array]. Small wording-only fix — check whether the fork's `is_string_literal`-style detection already matches the *intent* even if built against the old imprecise wording. |
| LWG 4428 (metafunctions shouldn't be defined via constant subexpressions) | **Not yet checked** | Live-checked 2026-09-08: status is now **C++26**. Rewords Throws clauses for `has_inaccessible_nonstatic_data_members()`/`has_inaccessible_bases()`/`annotations_of_with_type()` in [meta.reflection.access.queries]/[meta.reflection.annotation] to talk about exceptions instead of "is a constant subexpression" (accuracy fix now that reflection reports errors via `meta::exception`, not non-constant-expression fallback). Check the fork's actual throw conditions in `SemaReflect.cpp`/`ExprConstantMeta.cpp` for these three functions against the corrected wording. |
| P1789R3 (expansion-statement library support) | **Not yet checked** | Adopted Kona; adds structured-binding-style support to `integer_sequence` for `template for`. Check `libcxx/include/meta` / `<utility>` for `integer_sequence` iterator/range support. |

## Known bugs (M3 — not yet fixed)

1. **`clang/test/Reflection/splice-namespaces.cpp` crash.** `UNREACHABLE` in
   `NestedNameSpecifier.h:83` ("invalid prefix for namespace"). Root cause:
   `NestedNameSpecifier::MakeNamespacePtrKind` doesn't handle a `Kind::Splice`/
   `Kind::SpliceWithTemplate` prefix (i.e. `[:some_ns:]::inner::x`). Fix sketch already in
   `docs/CXX26_GAPS.md` (~line 399-412): a new `NamespaceWithSplice` `StoredKind`. Not yet
   implemented.
2. **`clang/test/Reflection/splice-exprs.cpp` regression.** Missing `expected-error` ("not derived
   from") at line 23; 15/16 tests in the file pass. Not yet determined whether the diagnostic
   check regressed or the test's expectation is stale — read the actual wording being tested
   before touching either.
3. **Consteval self-reference escalation cluster.** `SemaExpr.cpp`
   (`HandleImmediateInvocations`/`Rec.ConstevalOnly`), introduced by the LLVM 22 merge. Affects
   `libcxx/test/std/experimental/reflection/reflection-ex-parsing-command-line-options-2.sh.cpp`
   among others. **A prior fix (`6b5f636e6ba1`) regressed 9 libc++ reflection tests and was
   reverted (`87bcf7d13116`) — any new attempt must be verified against exactly those 9 tests
   before being trusted, and must not repeat a blanket revert if trouble recurs.**

## Upstream issue triage (M1)

*Pending — 5 parallel Codex batches launched 2026-09-08 covering all 85 open issues, writing to
`docs/reflection-audit/batch-outputs/batch-0{0..4}-triage.md`. Merge results here once all land.*

## Upstream PR triage (M1)

*Pending — not yet started; 35 open PRs snapshotted to
`docs/reflection-audit/upstream-prs-snapshot.json`.*

## Session Log

**2026-09-08 — Epic start (M0 + partial M2).** Established ground truth: real upstream tracker is
`bloomberg/clang-p2996` not the dead URL in `REFLECTION.md`; confirmed real divergence from
upstream's current tip; snapshotted 85 issues + 35 PRs; confirmed `LLVM_ENABLE_ASSERTIONS=ON`
already on in `build-nyx`; flagged tight disk (99% full, 3.4G free). Launched 5 parallel read-only
Codex triage batches for M1 (results pending). Did a direct wording-fetch + source cross-check for
the 3 late papers (P3617R0, P3687R1, P3795R2): P3617R0 and P3687R1 both look substantially/already
implemented (details above) — a materially better starting position than the pre-epic assumption
that all 3 were likely stale gaps. **P3795R2 is a confirmed real, unimplemented gap** — zero of its
three new scope-identification functions, its three tuple metafunctions, or its
`data_member_options.annotations` field exist anywhere in the tree. Live-checked LWG 4426/4428:
both resolved to C++26 status at Kona (revision R126), wording fetched. Remaining M2 work: the
original 7 papers (not yet audited this epic beyond incidental spot-checks), CWG 3111, LWG 4432,
P1789R3.
