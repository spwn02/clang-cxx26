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

## Next Up (updated 2026-09-08, after M1 issue triage landed)

M0 done. M1's issue triage is done (all 85 issues dispositioned — 29 Confirmed-Open, 27
Needs-Build-To-Verify, 19 Already-Fixed, 8 Out-of-Scope, 2 Not-Applicable); PR triage (35 open PRs)
still pending. M2's paper audit is running for the remaining 7 original papers in 4 parallel
background agents as of this writing (P2996R13 alone; P1306R5+P3096R12; P3293R3+P3394R4;
P3491R3+P3560R2) — fold results into the Paper-by-paper audit table above once they land. Next
concrete actions: merge those, then start M3 (the three known bugs — the mangling cluster from M1
found above is a natural M4 opener once M3's individually-gated bugs are done), then PR triage,
then M4's Confirmed-Open backlog (start with the mangling cluster: #286/#290/#298/#300/#312, one
root cause in `ItaniumMangle.cpp`'s `ReflectionKind::Template` case, five issues closed together).

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

**Memory is also tight**: a `ninja -C build-nyx clang -j22` (full `nproc`) rebuild after editing
`ItaniumMangle.cpp` was OOM-killed 2026-09-09 (30G RAM total, only ~5G free at the time, 12G swap
already in use on this shared machine). **Use `-j4` (or lower) for `clang`/`check-clang` rebuilds
after touching a large/heavily-templated file**, not full `nproc` — a killed build can look like a
hang or an unrelated failure if you don't check `dmesg`/the task notification's kill reason first.

**Root cause identified 2026-09-09: this is the user's actual personal desktop, not a dedicated
build box.** `ps`/`pgrep` during a second OOM kill (this time on `ninja check-clang` itself, not a
compile) showed Discord (Vesktop), Steam, Chrome, and 1Password all running concurrently, eating
~16G of the 30G total before any build work starts. **`ninja check-clang`'s default `llvm-lit`
invocation also has no explicit `-j` and defaults to `nproc` (22) parallel test-worker processes,
each spawning its own `clang` subprocess — just as memory-hungry as a 22-way compile.** Invoke
`llvm-lit` directly with an explicit low `-j` (e.g. `build-nyx/bin/llvm-lit -j4 -sv
build-nyx/tools/clang/test`) instead of going through the opaque `ninja check-clang` wrapper, for
both builds AND test runs, for the rest of this epic. Re-check `free -h` before any full-suite run
if it's been a while since the last one — available memory on a shared desktop fluctuates with
whatever else the user is doing.

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
| P3293R3 "Splicing a Base Class Subobject" | **Missing, ~0% implemented** | `obj.[:base:]` is explicitly rejected: `SemaReflect.cpp:1890`, `Sema::BuildReflectionSpliceExpr` diagnoses `ReflectionKind::BaseSpecifier` with `err_unexpected_reflection_kind_in_splice` (grouped with categorically-disallowed splice targets). `subobjects_of(info, access_context)` doesn't exist anywhere in `clang/` or `libcxx/` (grep-confirmed zero hits). No test files reference this paper at all. The prerequisite building blocks (`bases_of()`, `nonstatic_data_members_of()`) are already implemented and tested, so `subobjects_of` itself would be a straightforward compose-and-wire task — but the splice-to-base grammar/semantics is real new Sema/Parse work. **Second confirmed real paper-level gap, larger than expected.** |
| P3394R4 "Annotations for Reflection" | **Substantially implemented, one confirmed drift** | Core mechanism (parsing `[[=constant-expression]]`, storage, `is_annotation`/`annotations_of`, parameter-annotation extension from P3096, module serialization round-trip) is solid and well-tested. **Confirmed gap, corroborating issue #185 from the M1 triage**: `annotations_of_with_type(info, info)` doesn't exist under that name — implemented instead as an `annotations_of(info, info)` overload with equivalent filter logic but a different name/signature than the final adopted wording; stale pre-R4 API (`annotation_of_type<T>`, `annotate`) still present alongside it. Two items flagged Unverified-without-a-build: the empty-declaration annotation restriction, and the exact direction of the "order preserved" guarantee. |
| P3491R3 "define_static_{string,object,array}" | **Substantially implemented, 3 confirmed gaps** | Final adopted synopsis already absorbed P3617R0's `reflect_constant_array`/`reflect_constant_string` split — confirms the earlier delegation finding *is* the paper's own intent, not a deviation. Gaps: (1) **`define_static_object` is entirely missing** — zero occurrences anywhere, despite `docs/REFLECTION.md:65` claiming all three of `define_static_{string,object,array}` are supported (that line is inaccurate). (2) `is_string_literal` (5 overloads) missing entirely, no compiler query backing it either. (3) `reflect_constant_string` is narrower than spec — fork has 2 fixed non-template overloads (`char`/`char8_t` only) vs. the paper's one generic template covering `wchar_t`/`char16_t`/`char32_t` too; also doesn't implement the "already a string literal → skip double-terminating" carve-out (likely masked in practice but unproven equivalent). `reflect_constant_array`'s Mandates (structural-type, `copy_constructible`) aren't enforced, only `is_constructible_v`. |
| P3560R2 "Error Handling in Reflection" | **~0% implemented at the library level — full architectural gap** | `class std::meta::exception` doesn't exist anywhere (same root as issue #225). **This is the single biggest, most structurally important gap found in this epic so far**: every metafunction that P3560R2 specifies should throw `meta::exception` on failure instead currently uses this fork's pre-P3068 `DiagFn`-callback hard-diagnostic model (`Metafunction::DiagnoseFn` in `ExprConstantMeta.cpp`) — a "Constant When" evaluation-failure pattern, not a catchable exception. No throws-flag exists anywhere in `clang/include/clang/AST/Metafunction.h`. The P3068R6 compiler prerequisite (throw-in-consteval) is confirmed done (`ExprConstant.cpp`, per `CXX26_GAPS.md`), so the primitive exists — the reflection library was simply never rewired to use it. LWG 4428's wording fix is moot until this exists (same root cause). No `.verify.cpp` test anywhere exercises throw/catch reflection-error behavior — all still check classic hard-diagnostic `expected-error` patterns. **Recommended next major work item once M3's three known bugs are closed**: implement `meta::exception` as a real class, then rewire the `DiagFn` paths metafunction-by-metafunction — well-scoped, single root cause, unblocks P3560R2 + LWG 4428 + closes issue #225 all at once. |
| P2996R13 "Reflection for C++26" (core) | **Substantially implemented, real small gaps + 2 concrete bugs** | Grammar (reflect-operator, all 3 splicer forms) matches. **Missing**: `has_c_language_linkage`, `has_parent`, `type_order` (upstream libc++ itself tracks this as open, LWG4305), `is_virtual_base_of_type`, `is_trivially_relocatable_type`/`is_replaceable_type`/`is_nothrow_relocatable_type` (underlying `<type_traits>` facilities exist, just no `std::meta` wrapper), and `reference_constructs_from_temporary`/`reference_converts_from_temporary` are present but **commented out** with a `TODO(CXX26)` and had the wrong arity even before being disabled. **Signature drift**: `type_underlying_type`→`underlying_type` rename, `member_offset::total_bits()` missing `const` (real usability bug, not cosmetic), `reflect_constant(T)` by-value vs. paper's by-const-ref, `extract<T>` over-excludes rvalue-references, `data_member_options::bit_width`→`width` rename, `access_context::via(info)` doesn't accept the null reflection the paper explicitly permits (untested either way). **Two concrete, verified-by-direct-read bugs, no build needed**: `symbol_of`/`u8symbol_of` table has `"^"` (not `"^="`) at the `op_caret_equals` slot — this is issue #319 from the M1 triage, independently reconfirmed here with the exact table detail; and `op_co_await` is spelled `"coawait"` instead of the paper's `"co_await"` in the same tables. Framing correction: P2996R13 has **no `Throws:` clauses** at all (unlike P3560R2) — failure is `Mandates:`/`Constant When:`, and the fork's `throw`-inside-`consteval` + `requires`-clause pattern soundly encodes both without needing `meta::exception`. `reflect_invoke`/`subobjects_of`/`define_static_*` are correctly out of P2996R13's own current scope (moved to companion papers or removed pre-R13) — not fork gaps. The ~90-entry `[meta.reflection.traits]` family and most boolean predicates are presence-confirmed but not individually behavior-verified — flagged Implemented-Behavior-Unverified as a group, not itemized. |
| P1306R5 "Expansion Statements" | **Fully implemented — `docs/REFLECTION.md`'s own caveat is stale, not a real gap** | All 3 categories (enumerating/iterating/destructuring) implemented and tested, including the iterating (range-based) form the docs claim isn't supported (`docs/REFLECTION.md:48`: "expansions over constexpr ranges are not supported" — this line dates to commit `e130488` 2024-09-17, *before* iterating expansion was added in `e39580dc8a5c` 2025-03-03; `clang/test/SemaCXX/cxx2c-expansion-stmts.cpp` exercises it extensively including a `constexpr`-declared range, and is not among the documented pre-existing failures). Grammar, control-flow-limiting (no labels), `break`/`continue` semantics, empty-expansion (N=0) handling all match. Two items Implemented-Behavior-Unverified (for-range-declaration decl-specifier restriction, "S1 encloses S2" scoping) — no isolated negative test, but nothing observed contradicts them either. **Doc fix needed** (low-risk, text-only): update `docs/REFLECTION.md:48,63` to drop the stale caveat and mention the iterating category. **One real, already-known, separately-tracked bug remains**: the consteval self-reference escalation cluster (M3 bug #3 above) affects `template for` range-init specifically in one test — not a P1306 gap, a compiler bug already documented and deliberately deferred. |
| P3096R12 "Function Parameter Reflection" | **Substantially implemented, 2 concrete Returns-clause deviations found — FIXED 2026-09-09** | All 7 new metafunctions plus the 4 extended pre-existing ones (`identifier_of`/`u8identifier_of`/`type_of`/`has_identifier` for parameters) present and mostly matching, including two previously-buggy-now-fixed items (`variable_of`'s call-frame lookup, a Parameter-vs-Declaration equality hashing bug — both fixed 2026-09-07 per `CXX26_GAPS.md`). **Two real deviations**: `has_ellipsis_parameter(info r)` and `has_default_argument(info r)` are specified as **total functions** ("Otherwise, false" — no `Constant When`, and R12 explicitly *removed* `has_default_argument`'s Constant When per a LEWG poll) but this fork's implementation (`ExprConstantMeta.cpp:6452-6530`) still diagnoses/fails evaluation for any non-applicable reflection kind instead of returning `false`. Low-risk, well-scoped fix: relax both to return `false` for kinds where they currently diagnose, matching the two already-correct sibling total-functions `is_explicit_object_parameter`/`is_function_parameter` right next to them in the same file. Mandates/Constant-When enforcement elsewhere is systematically Implemented-Behavior-Unverified — matches `docs/REFLECTION.md`'s own admission that ill-formed-program diagnostics are largely untested (this is exactly what M5 exists to close). |
| CWG 3111 (array-type template parameter objects) | **Not yet checked** | Resolved as a C++26 DR at Kona 2025-11-07; affects [meta.define.static]-family reflection queries for array-type template parameter objects. Cross-check against `reflect_constant_array`'s implementation above once its own audit is complete — likely the same code path. |
| LWG 4432 (element init for `reflect_constant_array`) | **Not yet checked** | Resolved Kona 2025-11-04/08. Clarifies copy- vs. direct-initialization semantics for array elements — check `reflect_constant_array`'s actual element-init strategy in `ExprConstantMeta.cpp` against this. |
| LWG 4426 (`reflect_constant_string` literal detection) | **Not yet checked** | Live-checked 2026-09-08: status is now **C++26** (moved from Tentatively Ready). Wording changes "trailing null terminator" → "trailing u+0000 null character" and adds "is a reference to" for precision, in [meta.reflection.array]. Small wording-only fix — check whether the fork's `is_string_literal`-style detection already matches the *intent* even if built against the old imprecise wording. |
| LWG 4428 (metafunctions shouldn't be defined via constant subexpressions) | **Not yet checked** | Live-checked 2026-09-08: status is now **C++26**. Rewords Throws clauses for `has_inaccessible_nonstatic_data_members()`/`has_inaccessible_bases()`/`annotations_of_with_type()` in [meta.reflection.access.queries]/[meta.reflection.annotation] to talk about exceptions instead of "is a constant subexpression" (accuracy fix now that reflection reports errors via `meta::exception`, not non-constant-expression fallback). Check the fork's actual throw conditions in `SemaReflect.cpp`/`ExprConstantMeta.cpp` for these three functions against the corrected wording. |
| P1789R3 (expansion-statement library support) | **Not yet checked** | Adopted Kona; adds structured-binding-style support to `integer_sequence` for `template for`. Check `libcxx/include/meta` / `<utility>` for `integer_sequence` iterator/range support. |

## Known bugs (M3)

1. **`clang/test/Reflection/splice-namespaces.cpp` crash — ALREADY FIXED, stale doc entry.**
   `docs/CXX26_GAPS.md`'s note (from the 2026-09-04 Contracts Hardening epic) predates a lot of
   subsequent `NestedNameSpecifier` work. Checked 2026-09-08: `MakeNamespacePtrKind`
   (`clang/include/clang/AST/NestedNameSpecifier.h:68-86`) already handles `Kind::Splice`/
   `Kind::SpliceWithTemplate` prefixes via the `NamespaceWithNamespace` `StoredKind` (added at some
   point after the doc note was written, alongside a sibling `NamespaceWithGlobal`). Verified via
   `llvm-lit clang/test/Reflection/splice-namespaces.cpp` — **PASS**. No action needed; this entry
   stays here only so a cold session doesn't re-investigate it.
2. **`clang/test/Reflection/splice-exprs.cpp` — FIXED 2026-09-08.** The missing `expected-error`
   ("not derived from") at line 23 was a **stale test expectation, not a compiler regression**:
   `c.[:^^C::i:]` (splicing an anonymous-union member through a base expression) is *supposed* to
   succeed since commit `f33742c88aa3` ("fix member-splice access through an intermediate
   anonymous struct") deliberately made this well-formed, matching ordinary C++'s `c.i` semantics
   — confirmed independently by the M1 triage's Already-Fixed disposition on issue #265. Verified
   `c.[:^^C::i:]` now compiles cleanly with `-fsyntax-only` (exit 0). Updated the test to assert
   success instead of the old rejection; `llvm-lit` confirms **PASS** (16/16 in
   `clang/test/Reflection/` now). **Full `check-clang` gate run and verified clean of new
   regressions**: 5 failures found (`PR98671.cpp`, `builtin-is-within-lifetime.cpp`,
   `constant-expression-cxx11.cpp`, `cxx2a-constexpr-dynalloc.cpp`, `cxx2b-consteval-propagate.cpp`),
   all confirmed pre-existing and unrelated — the compiler binary (`build-nyx/bin/clang-22`)
   predates every change made this session (only `libcxx/include/meta`, two doc files, and this one
   test file were touched, and none overlap the 5 failing test paths). `PR98671.cpp` is the
   already-documented pre-existing concepts-partial-ordering bug (`CXX26_GAPS.md` Tier 0). The other
   4 all match the consteval self-reference escalation cluster (M3 bug #3 above, deliberately not
   fixed this session) — `builtin-is-within-lifetime.cpp`/`constant-expression-cxx11.cpp` are named
   explicitly in the original bug-diagnosis commit (`6b5f636e6ba1`)'s message as the tests this exact
   bug affects; `cxx2a-constexpr-dynalloc.cpp`/`cxx2b-consteval-propagate.cpp` are new observations
   (not previously enumerated anywhere in this repo's docs) but match the same consteval/
   immediate-function-context bug family by symptom and naming — worth folding into bug #3's
   "affected tests" list whenever that bug gets its dedicated fix session.
3. **Consteval self-reference escalation cluster — investigated 2026-09-08, deliberately NOT
   fixed this session, precise mechanism now documented.** Root mechanism: `SemaDeclCXX.cpp`'s
   `ActOnCXXEnterDeclInitializer` pushes `ExpressionEvaluationContext::ImmediateFunctionContext`
   for every C++23+ `constexpr`/`constinit` variable's initializer (`:19105-19116`). This one push
   feeds two different consumers that need opposite behavior from it:
   - `CheckForImmediateInvocation` (`SemaExpr.cpp:18144-18149`) bails out and never registers a
     call as an `ImmediateInvocationCandidate` whenever `isImmediateFunctionContext()` is true —
     correct in general (an immediate invocation failing inside another immediate-function-context
     should *escalate*, not diagnose locally), but this is what suppresses the diagnostic a
     self-referential `constexpr` variable's consteval call genuinely needs.
   - `HandleImmediateInvocations` (`SemaExpr.cpp:18397-18402`) *also* bails out entirely — skips
     its whole candidate-processing loop, including the `Rec.ConstevalOnly` bucket — whenever
     `Rec.isImmediateFunctionContext()` is true on the record being popped. This is what makes the
     ordinary P2996 idiom `constexpr auto R = <consteval-fn-returning-info>();` work: the ordinary
     VarDecl-level constant-evaluation (a separate mechanism from this one) still runs and
     succeeds, and this bail-out just prevents a *redundant* secondary diagnosis pass from also
     firing on a `ConstevalOnly`-typed subexpression that already evaluated fine.
   Both consumers key off the exact same `isImmediateFunctionContext()`/`Rec.isImmediateFunctionContext()`
   signal, gated purely by *whether the push happened*, not by *whether the contained call actually
   succeeded* — so there is no way to satisfy "diagnose the genuinely-failing self-reference case"
   and "don't diagnose the genuinely-succeeding reflection idiom" by touching the push/gate alone.
   **Confirms the prior revert's diagnosis exactly** (`87bcf7d13116`'s message) and adds the two
   precise line ranges. A correct fix needs a per-candidate success/failure signal threaded through
   to the point of diagnosis (e.g. distinguish in `Rec.ConstevalOnly`/`ImmediateInvocationCandidates`
   whether the specific candidate already evaluated successfully elsewhere, not a context-wide
   flag) — nontrivial, matches the prior session's own "left as documented future work" call.
   **Deliberately not attempted this session**: the existing 9-test regression risk is real and
   well-documented (`docs/LLVM22_SYNC.md`), there's no quick, low-risk version of this fix, and
   this epic has substantially higher-value, lower-risk, well-scoped work queued (the 5-issue
   mangling cluster with ready upstream PRs, `meta::exception` for P3560R2, `subobjects_of` for
   P3293R3). Revisit with a dedicated session once the backlog above is smaller. Affects
   `libcxx/test/std/experimental/reflection/reflection-ex-parsing-command-line-options-2.sh.cpp`
   among others (not yet re-enumerated this session — the original 9-test list is in
   `docs/LLVM22_SYNC.md`, re-check it's still current before reusing it as the regression gate).

## Upstream issue triage (M1)

All 85 open `bloomberg/clang-p2996` issues triaged 2026-09-08 by 5 parallel Codex batches
(read-only source cross-reference, no builds). Full per-batch reasoning kept at
`docs/reflection-audit/batch-outputs/batch-0{0..4}-triage.md` (git-tracked). **Counts: 29
Confirmed-Open, 27 Needs-Build-To-Verify, 19 Already-Fixed, 8 Out-of-Scope, 2 Not-Applicable.**

**Cross-links noticed between batches / to the paper audit above:**
- **Reflection-NTTP mangling cluster — 4 of 5 issues FIXED 2026-09-09** (ported from upstream PR
  #316, the cumulative version superseding #287/#299/#301): #286 (overload discriminator via
  ODRHash), #298 (Template-kind deduction-guide mangling), #300 (specialization-context fix —
  `AddFunctionDecl` no-ops there, folds in the pattern's function type + ref-qualifier), #312
  (Declaration-kind deduction-guide-specialization mangling) all closed in
  `clang/lib/AST/ItaniumMangle.cpp`'s `mangleReflection`. Verified via `check-clang` (identical
  5-test pre-existing baseline, zero new failures) plus 4 ported regression tests, all passing.
  #290 (entity-proxy mangling crash) remains open — separate PR (#291), not part of this cluster's
  root cause, port next.
- **Expansion-statement control-flow/robustness is the other major cluster**: #146 (expansion
  generates raw `case` labels), #150 (spliced destructor call), #181 (non-copyable range), #182
  (`template for` + `continue` ICE), #326/#327 (ICE on unresolved-overload / uninstantiated
  ranges). Several are in `clang/lib/Sema/SemaExpand.cpp` and `clang/lib/CodeGen/CGStmt.cpp`.
- **Issue #225 ("`meta::exception` unimplemented") is the same gap as P3560R2's core
  requirement** in the paper audit above — not a separate item, the same fix closes both.
- **Issue #185 ("Annotation API changed in R1") independently confirms the P3394R4 audit is not
  yet done this epic** — the batch found `<meta>` still has the old `annotations_of(info, info)`/
  `annotation_of_type`/`annotate` API surface and is missing `annotations_of_with_type` entirely,
  which is exactly the kind of drift the paper audit (still pending for P3394R4) needs to nail
  down precisely.
- **Entity-proxy query gaps (#290) directly extend the P3687R1 audit finding above**: the feature
  is real and flag-gated (`EntityProxyReflection`), but several query handlers
  (`is_constructor`/`is_destructor`/`is_special_member_function` in `ExprConstantMeta.cpp`) still
  `llvm_unreachable`/mishandle `ReflectionKind::EntityProxy`, and the Itanium mangler doesn't
  discriminate proxy-mangled names either. So P3687R1's disposition above ("substantially
  implemented") needs a caveat: implemented for the paper's core semantics, but with real
  follow-on query/mangling gaps the triage surfaced independently.

Full merged table (issue # | title | disposition | evidence | confidence) — split by batch range
for readability, same as the source files:

**105-184:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 105 | `experimental/meta` missing | Not-Applicable | Fork exposes current `<meta>`, not the obsolete path; build/install guidance issue, already closed upstream. | High |
| 120 | Repeated first argument on Windows | Confirmed-Open | Itanium mangling fixed (`698fc39db256`); MS mangling still `llvm_unreachable` at `MicrosoftMangle.cpp:2171-2172`. | High |
| 146 | Expansion generates `case` labels | Confirmed-Open | `ParseStmt.cpp:2250-2253` accepts `case` in expansion body with no control-flow check; `CGStmt.cpp:1575-1621` emits unconditionally. | High |
| 150 | Spliced explicit destructor call | Confirmed-Open | `ParseReflect.cpp:49-58` permits ordinary destructor names; splice-as-destructor-name path (`~[:...:]`) absent at `:243-289`. | High |
| 151 | ICE in member-wise swap | Already-Fixed | Fixes `f0a3e5e612db`/`0f70ed5eb99e`; `CGStmt.cpp:1605-1618` scoping; `miscellaneous.pass.cpp:126-145` close coverage (no dedicated regression test though). | Medium |
| 154 | `underlying_type` ICE for non-enum | Needs-Build-To-Verify | `libcxx/include/meta:1995-1999`; repro: `std::meta::underlying_type(^^int)`. | Medium |
| 169 | Crash in templated lambda | Needs-Build-To-Verify | `TreeTransform.h:9352-9419`; repro in batch file. | Medium |
| 173 | Templated `named_tuple` | Out-of-Scope | Feature request; `define_aggregate` (`libcxx/include/meta:2503-2515`) already covers the underlying need. | High |
| 175 | Namespace comparison | Already-Fixed | `2245a73e94f5`; `namespace-reflection-equality-reopened.pass.cpp:70-87`. | High |
| 176 | `members_of` + empty namespace redeclaration | Already-Fixed | `ExprConstantMeta.cpp:1535-1545`; same test file `:41-61`, `:70-84`. | Medium |
| 177 | Enum NTTP loses enumerator identity | Out-of-Scope | Intentional: reflected value vs. enumerator declaration distinction, `ExprConstantMeta.cpp:4192-4208`; `entity-classification.pass.cpp:61-82` requires this. | High |
| 178 | `template for` over `integer_sequence` | Needs-Build-To-Verify | `ParseStmt.cpp:1980-1992`; repro in batch file. | Medium |
| 180 | `static_assert(false)` ignored | Needs-Build-To-Verify | No fork-specific handling found; repro needs `substitute`+`if constexpr` build test. | Medium |
| 181 | Non-copyable tuple in `template for` | Needs-Build-To-Verify | `ParseStmt.cpp:1980-1992` vs. `libcxx/include/meta:2638-2655`; repro in batch file. | Medium |
| 182 | `template for` + `continue` ICE | Confirmed-Open | `CGStmt.cpp:1599-1617` doesn't account for `if constexpr`-discarded expansion instances; no regression test. | High |
| 183 | ICE with imported reflection function | Already-Fixed | `module-imports.sh.cpp:1-17`; serialization fix `090152727f3f` (broader than original scenario). | Medium |
| 184 | Spurious consteval-only diagnostic | Needs-Build-To-Verify | `SemaReflect.cpp:948-974`, `TreeTransform.h:9368-9419`; no test for the specific nested-lambda escalation case. | Medium |

**185-225:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 185 | Annotation API changed in R1 | Confirmed-Open | Obsolete `annotations_of(info,info)`/`annotation_of_type`/`annotate` still present; `annotations_of_with_type` absent (`libcxx/include/meta:358-364`, `2085-2128`). See P3394R4 cross-link above. | High |
| 187 | Compilation never ends | Needs-Build-To-Verify | No reproducer in snapshot, Godbolt-link only. | Low |
| 188 | `display_string_of(dealias(...))` not constant expr | Needs-Build-To-Verify | `libcxx/include/meta:3303-3308`, `2960-2990`; no regression test for this combination. | Medium |
| 189 | "Upstream to LLVM" | Out-of-Scope | Distribution/adoption request, not a defect. | High |
| 200 | `parent_of` wrong for class-template aliases | Confirmed-Open | `ExprConstantMeta.cpp:3055-3060` doesn't preserve alias layer; `related-reflections.pass.cpp:104-121` doesn't cover this exact case. | High |
| 203 | Unbalanced diagnostic parentheses | Needs-Build-To-Verify | `SemaExpand.cpp:82-121`; repro in batch file. | Medium |
| 204 | ICE: `template for` over overload set | Needs-Build-To-Verify | `SemaExpand.cpp:82-121`, no dedicated test; repro in batch file. | Medium |
| 205 | ICE: templated lambda + `define_static_array` | Already-Fixed | Fix `f72d85e5a0fd`; `SemaExpand.cpp:148-173`. | High |
| 208 | CRTP constexpr degradation | Needs-Build-To-Verify | Godbolt-link only, insufficient detail to localize. | Low |
| 210 | Capturing lambda inside expansion statement | Already-Fixed | Fixes `f0a3e5e612db`, `0f70ed5eb99e`; `SemaExpand.cpp:148-173`. | High |
| 211 | Static enum member wrong `type_of` | Needs-Build-To-Verify | `ExprConstantMeta.cpp:2982-2994` looks correct but unverified for this exact case. | Medium |
| 212 | ICE in `if constexpr` optional extraction | Needs-Build-To-Verify | Insufficient repro detail in snapshot. | Low |
| 215 | Empty `reflect_constant_array` result | Already-Fixed | Fix `5dafd8cc4a45`; `libcxx/include/meta:2598-2616`; `static-arrays.pass.cpp:64-70`. | High |
| 220 | `display_string_of(type_of(undeduced))` hangs | Needs-Build-To-Verify | `ExprConstantMeta.cpp:6565-6594` lacks explicit undeduced-return guard. | Medium |
| 221 | `expected<bool,int>` in `vector` fails | Needs-Build-To-Verify | `__expected/expected.h:1164-1173`; depends on constraint normalization/CTAD, needs build. | Medium |
| 222 | `define_aggregate` nested incomplete type in template arg | Needs-Build-To-Verify | `ExprConstantMeta.cpp:6092-6119`, `SemaReflect.cpp:638-710`; different path than the reported case, no matching test. | Medium |
| 225 | `meta::exception` unimplemented | Confirmed-Open | No declaration anywhere in `<meta>`; P3068 compiler prerequisite (throw-in-constexpr) already implemented (`ExprConstant.cpp:881-893`,`6635-6689`) but the library type itself is missing. **Same gap as P3560R2's core requirement.** | High |

**230-273:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 230 | Reflecting `std::int32_t` | Out-of-Scope | Deliberate: using-shadow-decl rejection unless entity-proxy reflection on (`SemaReflect.cpp:1044-1050`,`1314-1336`); unresolved WG21 semantics. | High |
| 232 | `template for` + `display_string_of` | Needs-Build-To-Verify | `libcxx/include/meta:2960-2970`; no test for this nested `type_of(field)` case. | Medium |
| 234 | Protected base member reflection | Needs-Build-To-Verify | `SemaReflect.cpp:214-250` looks like it should permit this; unverified without build. | Medium |
| 235 | Ambiguous constructor reflection | Out-of-Scope | Unresolved design question — multiple ctors, no WG21 resolution to implement against (`SemaReflect.cpp:1398-1430`). | High |
| 237 | Alias of closure type loses identity | Needs-Build-To-Verify | `libcxx/include/meta:3090-3120`; ambiguous whether intentional. | Low |
| 239 | Closure `operator()` reported overloaded | Needs-Build-To-Verify | `SemaReflect.cpp:1412-1430`; generic-lambda operator template case unverified. | Medium |
| 245 | Protected member as reflected template arg | Not-Applicable | `access_context` model (`libcxx/include/meta:1053-1090`) intentionally preserves access-context effects. | High |
| 246 | Order-dependent `define_static_array` | Needs-Build-To-Verify | `libcxx/include/meta:2642-2660`; likely already fixed by `01836b3333d5` consteval-only caching fix, needs build to confirm. | Medium |
| 252 | VS Code `__has_feature(reflection)` | Out-of-Scope | Editor/IntelliSense issue, not Clang. | High |
| 253 | ICE during recursive reflection in modules | Needs-Build-To-Verify | Module/annotation serialization work exists (`f63157a8d87c`) but no exact repro match. | Medium |
| 254 | `define_static_string` hits constexpr step limit | Confirmed-Open | `libcxx/include/meta:2655-2660` has no chunking/step-limit management for long strings. | High |
| 256 | `__is_consteval_only` + defaulted special members | Already-Fixed | `Decl.cpp:5426-5475` fixed by `01836b3333d5`; `p3603-consteval-only.pass.cpp` covers it. | High |
| 259 | Windows build errors | Out-of-Scope | Upstream platform/build-system issue, unrelated to this fork's two-tree build architecture. | High |
| 262 | Annotation crash with modules | Already-Fixed | `f63157a8d87c` added `CXX26AnnotationAttr` serialization; `annotation-module-serialization.sh.cpp`. | High |
| 264 | Constant evaluation of arrow splices | Needs-Build-To-Verify | `SemaReflect.cpp:1730-1745`; provenance question needs build to verify. | Medium |
| 265 | Anonymous union members unavailable for splicing | Already-Fixed | `f33742c88aa3`, `2ea0a79fe7bb`; `anon-union.pass.cpp:18-31`. | High |
| 273 | Dependent function-template reflection seen as overload set | Already-Fixed | `41fa327e7d63`; `SemaReflect.cpp:998-1030`. | High |

**275-309:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 275 | clangd crashes while code compiles | Needs-Build-To-Verify | Fork changed substantially since reported clangd build; no local repro. | Low |
| 276 | Splice type aliases treated as identical | Fixed | PR #277 ported; dependent splice types now canonicalize by operand and template arguments, with distinct/same-alias regression coverage. | Medium |
| 280 | `has_parent` missing | Confirmed-Open | `parent_of` exists, `has_parent` doesn't (`libcxx/include/meta:56-60`,`934-942`). Missing library feature. | High |
| 281 | ICE from `annotations_of(^^member)` | Already-Fixed | `SemaReflect.cpp:1371-1377`; `p3394-annotations.pass.cpp:89-110`,`131-142`. | Medium |
| 286 | Same-named function-template reflections collide | Confirmed-Open | `ItaniumMangle.cpp:4932-4937` mangles template name only, no overload discriminator. **Mangling cluster — see cross-link above.** | High |
| 288 | Reentrant constant evaluation UAF | Fixed | PR #289 ported; immediate-invocation and cleanup paths reacquire evaluation-context records after reentrant operations, with a 64-level reflection instantiation regression test. | High |
| 290 | Entity-proxy queries + mangling crash | Fixed | PR #291 ported; proxy member predicates now return false and proxy NTTP mangling targets the underlying declaration; `entity-proxy-member-queries.pass.cpp`. | High |
| 292 | `AttributedType` blinds function queries | Already-Fixed | Fix `1dee6d809821`; `ExprConstantMeta.cpp:1622-1629`; `attributed-function-type-queries.pass.cpp`. | High |
| 294 | `can_substitute`/`substitute` crash on invalid formed types | Fixed | PR #295 ported; substitution APIs now return failure and suppress diagnostics for invalid formed types, with function/alias/variable/member-template regression coverage. | High |
| 296 | `members_of` eagerly instantiates member bodies | Already-Fixed | Fix `7220baffd57e`; `members-of-lazily-ill-formed-bodies.pass.cpp:18-28,75-89`. | High |
| 298 | Deduction-guide reflection mangler ICE | Confirmed-Open | `ItaniumMangle.cpp:4932-4937` unconditional `mangleTemplateName`; deduction guides unsupported elsewhere in same mangler. **Mangling cluster.** | High |
| 300 | Same-headed member-template reflections collide | Confirmed-Open | No ODR/type/ref-qualifier discriminator in template-reflection mangling; `ODRHash.cpp:670-690` returns early for class-template-specialization members. **Mangling cluster.** | High |
| 302 | Reopened namespace reflections compare unequal | Already-Fixed | `2245a73e94f5`; `APValue.cpp:575-584`; `namespace-reflection-equality-reopened.pass.cpp:19-31,70-95`. | High |
| 303 | Reopened namespace walk truncates members | Fixed | PR #306 ported; out-of-line class-member definitions are excluded from namespace enumeration and traversal returns to their lexical namespace, with reopened-namespace coverage. | High |
| 304 | `is_complete_type` fails through aliases | Already-Fixed | Fix `7f0f89cc7e75`; `ExprConstantMeta.cpp:4865-4880`; `is-complete-type-alias-sugar.pass.cpp`. | High |
| 308 | Tracking issue for #286-#304 | Out-of-Scope | Aggregate meta-issue; entries triaged individually above. | High |
| 309 | Dependent splice ICE during `auto` NTTP deduction | Fixed | PR #310 ported; null-model dependent splices are classified by their own value kind, with libc++ requires-expression regression coverage. | High |

**311-350:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 311 | Builtin-template diagnostic ICE | Fixed | PR #315 ported; `DescriptionOf` handles builtin and template-template-parameter reflections with a safe fallback, with a builtin-template diagnostic regression test. | High |
| 312 | Deduction-guide specialization mangling ICE | Confirmed-Open | `ItaniumMangle.cpp:4898-4917`, `1444`, `1738` — `llvm_unreachable("Can't mangle a deduction guide name!")`. **Mangling cluster.** | High |
| 313 | `members_of` truncates after linkage specifier | Already-Fixed | `ExprConstantMeta.cpp:1551-1560` already descends into `LinkageSpecDecl`. | High |
| 314 | LP64 NEON vector mangling ICE | Confirmed-Open | `ItaniumMangle.cpp:3941-3959` — `long` on LP64 uncovered. Reflection-independent but real. | High |
| 319 | `op_caret_equals` symbol typo | Confirmed-Open | Both operator tables still use `"^"` at the `op_caret_equals` slot (`libcxx/include/meta:906-925`). | High |
| 321 | 32K+ template packs miscompile | Confirmed-Open | `SubstNonTypeTemplateParmPackExpr::NumArguments` still a 15-bit bitfield (`ExprCXX.h:4761-4778`). | High |
| 322 | Parameter-name result depends on instantiation | Already-Fixed | Fix `fad02ea72cc7`; `ExprConstantMeta.cpp:1130-1148`; `param-name-consistency-instantiation.pass.cpp`. | High |
| 326 | Expansion statement ICE during instantiation | Confirmed-Open | `TreeTransform.h:9544-9573` → `SemaExpand.cpp:439-460` doesn't reject unresolved-overload ranges. | High |
| 327 | Expansion statement ICE on unresolved overload range | Confirmed-Open | `SemaExpand.cpp:190-219` reaches ADL candidate construction on an unresolved range. | High |
| 329 | Constant evaluation crash through PCH | Confirmed-Open | `ASTWriterStmt.cpp:498-515` serializes `CXXMetafunctionExpr` args as ordinary statements; evaluator-side state not covered; no PCH/module regression test. | Medium |
| 331 | `reflect_object` rejects explicit defaulted copy ctor | Needs-Build-To-Verify | `ExprConstantMeta.cpp:3170-3207`; no explicit exception found but needs build. | Medium |
| 332 | `reflect_constant` rejects pointer to mixed consteval-only type | Needs-Build-To-Verify | `ExprConstantMeta.cpp:3227-3264`, `ExprConstant.cpp:2405-2410`; needs build. | Medium |
| 333 | Splice operand convertible to `meta::info` rejected | Already-Fixed | `SemaReflect.cpp:1575-1593` already handles `DefaultLvalueConversion` + implicit conversion. | High |
| 334 | Static member call inherits consteval-only object restriction | Confirmed-Open | `ExprConstant.cpp:2405-2410` doesn't distinguish unevaluated object expression. | Medium |
| 342 | `^^derived::operator()` rejects using-declaration | Confirmed-Open | `SemaReflect.cpp:1042-1050,1320-1339` unconditional rejection, no operator-function-id distinction. | High |
| 346 | Spurious warning for reflected reference type | Needs-Build-To-Verify | `DiagnosticParseKinds.td:1828-1829`, `ParseReflect.cpp:149`; needs build to confirm type-info availability at warn site. | Medium |
| 350 | `->[:member:]` assertion with lvalue pointer | Confirmed-Open | `SemaExprMember.cpp:1233-1257,1330-1335` — no lvalue-to-rvalue conversion before `IsArrow` build. | High |

## Upstream PR triage (M1)

**Major finding (2026-09-08): most of the 35 open upstream PRs are ready-made fixes for issues
this epic just triaged as Confirmed-Open.** Titles alone map cleanly onto the mangling cluster and
several other confirmed issues — this changes M4 from "write fixes from scratch" to "assess and
port these diffs" for a large fraction of the backlog. **Do not re-derive a fix from scratch before
checking this list first.**

| PR # | Title | Maps to issue(s) |
|---:|---|---|
| 353 | Reflect the introduced function when an id-expression names a using-declarator | #342 |
| 352 | Convert the base of a member splice before building the member expression | #350 |
| 347 | Remove warning if `&` is not directly in code | #346 (likely) |
| 345 | Allow reflections of values designating immediate functions | — (check against #184/#334 consteval-only cluster) |
| 340 | Add `std::meta::has_c_language_linkage` | — (new facility, check against P2996R13 synopsis) |
| 330 | [Clang][P2996] Implement PCH serialization for `ExplDependentCallExpr` | #329 (related) |
| 328 | fix crash on expansion statement over an overload set | #327, possibly #326 |
| 323 | Fix silent miscompile of template argument packs with 2^15+ elements | #321 |
| 320 | Fix `symbol_of`/`u8symbol_of` table entry for `op_caret_equals` | #319 |
| 318 | Handle LP64 long-element NEON vectors in the Itanium mangler | #314 |
| 317 | Don't truncate member enumeration at linkage-spec typedef tags | #313 (already-fixed here — check if this fork's fix differs/is equivalent) |
| 316 | Mangle deduction-guide specialization reflections | #312 |
| 315 | Describe builtin templates instead of crashing | #311 — Fixed; ported and verified in commit |
| 310 | Don't crash classifying a dependent splice expression | #309 — Fixed; ported and verified in commit |
| 306 | Keep namespace member walks clear of out-of-line class-member definitions | #303 — Fixed; ported and verified in commit |
| 301 | Discriminate same-headed member-template reflections of a specialization in NTTP mangling | #300 |
| 299 | Mangle deduction-guide reflections instead of hitting unreachable | #298 |
| 295 | Report substitution failure instead of crashing when substitution forms an invalid type | #294 — Fixed; ported and verified in commit |
| 291 | Handle entity-proxy reflections in member metafunctions and NTTP mangling | #290 — Fixed; ported and verified in commit |
| 289 | Fix use-after-free: `ExprEvalContexts` reallocates under held record references during reentrant consteval evaluation | #288 — Fixed; ported and verified in commit |
| 287 | Discriminate same-named function-template reflections in NTTP mangling | #286 |
| 279 | Fix reflect unresolved lookup | — (check against #239/#204 overload-related issues) |
| 277 | Fix canonicalization of dependent splice types | #276 — Fixed; ported and verified in commit |
| 261 | Defer expansion statement body instantiation | — (check against #180/expansion cluster) |
| 249 | Merging upstream 91cdd350 [clang] Improve nested name specifier AST representation | — (general upstream sync, check relevance) |
| 244 | Fix `Sema::BuildCXXReflectExpr` wrongly thinks a template is overloaded (e.g. reflecting a lambda's `operator()`) | — (check against #239) |
| 227,195,170 | [P3816] Implement `std::consteval_hash<std::meta::info>` (3 iterations) | **New paper not in this epic's list — P3816, need to identify and add to the paper audit** |
| 207 | Examples from P2996 - Reflection for C++26 | — (test/example content, low priority) |
| 168 | Adding string literal manipulation | — (check scope) |
| 166 | fix `is_reflection_type` | — (check scope) |
| 163 | [P3074] Implementing part of trivial unions | **Another paper reference — P3074, cross-check against the language-side gaps list in `docs/CXX26_GAPS.md`** |
| 135 | ast dump for splice specifier and reflection splice type | tooling, low priority |
| 124 | Custom annotation requires type to be `equality_comparable` | — (check against annotation/P3394R4 work) |

**Action items surfaced by this list — both resolved 2026-09-08:**
- **P3816R3 "Hashing meta::info"** (`consteval_hash<std::meta::info>`) is **Out-of-Scope for this
  epic** — confirmed via live wording fetch: still a proposal under SG7 discussion, not adopted
  into the C++26 working draft (implementers still debating hash-stability semantics across TUs).
  The 3 competing upstream PRs (#227/#195/#170) are experimental attempts at a not-yet-standardized
  facility. Not required for "genuinely complete C++26 reflection"; could be revisited later as an
  experimental opt-in (matching this fork's existing pattern for P3381/P3385), but that's a
  post-epic nice-to-have, not a completion blocker.
- **P3074R7 "Trivial unions"** is **already tracked in `docs/CXX26_GAPS.md`'s language-side gaps
  table** (currently `[ ]` unstarted) — it's a general language paper, not reflection-specific,
  despite PR #163 living in the reflection repo. Not duplicated here; if PR #163's diff is useful,
  route the actual fix work through `CXX26_GAPS.md`, not this tracker.
- For every PR mapped to a Confirmed-Open issue above: fetch the diff (`gh pr diff -R
  bloomberg/clang-p2996 <N>`), check whether it applies cleanly or needs adaptation to this fork's
  divergent surrounding code, and prefer porting/adapting it over writing an independent fix in M4.

**Portability spot-check (2026-09-08): PR #287 confirmed directly portable.** Diff saved to
`docs/reflection-audit/pr-diffs/pr-287.diff`. Small, surgical change to
`CXXNameMangler::mangleReflection`'s `ReflectionKind::Template` case (append an `ODRHash`-based
discriminator so overloaded function templates reflected as NTTPs don't collide/silently fold in
CodeGen) plus a well-commented regression test. This fork's `ItaniumMangle.cpp:4932-4938` matches
the diff's "before" context byte-for-byte. High confidence the other 4 mangling-cluster PRs (#291,
#299, #301, #316) are similarly portable — diffs fetched to `docs/reflection-audit/pr-diffs/`,
full application deferred to M4 (each needs its own build-gated commit).
- For every PR mapped to a Confirmed-Open issue above: fetch the diff (`gh pr diff -R
  bloomberg/clang-p2996 <N>`), check whether it applies cleanly or needs adaptation to this fork's
  divergent surrounding code, and prefer porting/adapting it over writing an independent fix in M4.

**Portability spot-check (2026-09-08): PR #287 confirmed directly portable.** Diff saved to
`docs/reflection-audit/pr-diffs/pr-287.diff`. Small, surgical change to
`CXXNameMangler::mangleReflection`'s `ReflectionKind::Template` case (append an `ODRHash`-based
discriminator so overloaded function templates reflected as NTTPs don't collide/silently fold in
CodeGen) plus a well-commented regression test. This fork's `ItaniumMangle.cpp:4932-4938` matches
the diff's "before" context byte-for-byte. High confidence the other 4 mangling-cluster PRs (#291,
#299, #301, #316) are similarly portable — diffs fetched to `docs/reflection-audit/pr-diffs/`,
full application deferred to M4 (each needs its own build-gated commit).

## Session Log

**2026-09-09 — M4 PR batch, PR #289.** Ported upstream PR #289 for issue #288. Immediate
invocation handling and cleanup now reacquire `ExprEvalContexts` records after reentrant
evaluation, preventing dangling references after `SmallVector` growth. Added the 64-level
`consteval-reentrant-instantiation.pass.cpp` regression test; it passed. Built `clang` with
`-j2`, rebuilt stale auxiliary tools, and ran the capped direct-lit Clang gate: 49,819 discovered,
44,614 passed, exactly the five documented baseline failures.

**2026-09-09 — M4 PR batch, PR #306.** Ported upstream PR #306 for issue #303. Namespace member
walks now reject out-of-line class-member definitions as namespace entities and switch back to the
lexical namespace when traversing from such a definition. Added
`namespace-members-out-of-line-defs.pass.cpp`; it passed. Built `clang` with `-j2`, rebuilt stale
auxiliary tools, and ran the capped direct-lit Clang gate: 49,819 discovered, 44,614 passed,
exactly the five documented baseline failures.

**2026-09-09 — M4 PR batch, PR #315.** Ported upstream PR #315 for issue #311. `DescriptionOf`
now describes builtin and template-template-parameter reflections and falls back safely for
unanticipated template kinds instead of reaching `llvm_unreachable`. Added
`description-of-template-kinds.verify.cpp`; it passed. Built `clang` with `-j2`, rebuilt stale
auxiliary tools, and ran the capped direct-lit Clang gate: 49,819 discovered, 44,614 passed,
exactly the five documented baseline failures.

**2026-09-09 — M4 PR batch, PR #291.** Ported upstream PR #291 for issue #290. Entity-proxy
member predicates now classify using-shadow proxies as false, and proxy reflection NTTP mangling
encodes the target declaration while retaining the proxy discriminator. Added
`entity-proxy-member-queries.pass.cpp`; the libc++ wrapper test passed. The initial full gate
exposed stale revision-stamped `clang-scan-deps`, `c-index-test`, `clang-repl`, `clang-check`, and
`clang-extdef-mapping` binaries; rebuilding those tools removed all unrelated failures. Final
capped direct-lit gate: 49,818 discovered, 44,613 passed, exactly the five documented baseline
failures. Commit and push follow.

**2026-09-09 — M4 PR batch, PR #277.** Ported upstream PR #277 for issue #276. Dependent
reflection splice types now canonicalize using their splice operand and template arguments, while
nondependent splice types canonicalize to the underlying type. Added
`dependent-splice-overloads.cpp`. Built `clang` with `-j2`, passed the focused test, and ran the
capped direct-lit Clang gate: 49,819 discovered, 44,614 passed, exactly the five documented
baseline failures.

**2026-09-09 — M4 PR batch, PR #310.** Ported upstream PR #310 for issue #309. Dependent
`CXXSpliceExpr` nodes with no model are now classified from their own value kind instead of being
dereferenced. Added `auto-nttp-dependent-splice-requires.pass.cpp`; the libc++ wrapper test
passed. Built `clang` with `-j2`; after rebuilding stale auxiliary tools, the capped direct-lit
Clang gate reported 49,819 discovered, 44,614 passed, exactly the five documented baseline
failures.

**2026-09-09 — M4 PR batch, PR #295.** Ported upstream PR #295 for issue #294. Metafunction
substitution now propagates null specialization results instead of asserting, suppresses Sema
diagnostics for `can_substitute`, and reports the normalized substitution-failed diagnostic for
`substitute`. Added invalid-type-formation coverage for function, alias, variable, and member
templates; both focused libc++ tests passed. Built `clang` with `-j2`, rebuilt stale auxiliary
tools, and ran the capped direct-lit Clang gate: 49,819 discovered, 44,614 passed, exactly the
five documented baseline failures. The fork-specific verify test also records its duplicate
underlying invalid-reference diagnostic.

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
