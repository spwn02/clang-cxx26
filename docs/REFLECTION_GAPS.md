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

## Next Up (updated 2026-09-09, after definitive full check-clang gate)

The established validation baseline is now **23 failures**: the five original
consteval-escalation tests (`SemaCXX/PR98671.cpp`,
`SemaCXX/builtin-is-within-lifetime.cpp`, `SemaCXX/constant-expression-cxx11.cpp`,
`SemaCXX/cxx2a-constexpr-dynalloc.cpp`, and
`SemaCXX/cxx2b-consteval-propagate.cpp`) plus 18 failures from the confirmed
pre-existing ASTUnit/libclang PCH-loading bug. Do not count either cluster as a
regression from reflection changes. The last clean full-suite evidence before
the 18-test ASTUnit cluster was recorded in
`docs/reflection-audit/codex-final-gate-report.md`.

**Status as of 2026-09-09 night: M0/M1/M2/M3 done. M4 mostly done. P3293R3 and P3795R2 fully
implemented (were ~0% at epic start). P3560R2 substantially advanced (`meta::exception` class +
13 strategy-1 wrappers) but strategy 2 needs one more design-revision round before implementation
(see below) — do not re-attempt the `members_of` pilot without first fixing the two evaluator-API
gaps documented in `docs/reflection-audit/codex-strategy2-pilot-report.md`.**

Remaining M4 Confirmed-Open items now include **#150** (destructor splice `~[:info:]`) plus five
issues reclassified by the Needs-Build-To-Verify audit: **#180, #181, #188, #220, and #237**.
#169 and #346 were fixed in M4 batch 6; #221 was confirmed to be a libc++ constraint bug rather
than a reflection issue. The audit reclassified 12 as Already-Fixed and left six genuinely
unverifiable because their reports lack a usable reproducer. M1 issue and PR triage are now
complete; the 11 formerly unassessed PRs and the four cross-check PRs are dispositioned in the
table below and in [`codex-pr-triage-final-report.md`](reflection-audit/codex-pr-triage-final-report.md).

Batch-5 correction: the P2996R13 `has_c_language_linkage` item formerly listed in the paper audit
Missing list is implemented and tested by commit `6bbb1c0cfbea`.

**Concrete next actions, in rough priority order (updated 2026-09-09 night after M1 fully closed
and the NBTV audit landed — items 1/2 below are DONE, kept struck through for history):**
0. ~~Re-audit the 27 Needs-Build-To-Verify issues from M1~~ — **done**, see
   `codex-nbtv-audit-report.md` (12 Already-Fixed, 9 Confirmed-Open, 6 still unverifiable).
   ~~Finish PR triage~~ — **done**, see `codex-pr-triage-final-report.md`, M1 fully complete.
1. Work the remaining 5 Confirmed-Open issues from the NBTV audit: **#180, #181, #188, #220,
   and #237** (plus #150, tracked separately below). #169 and #346 were fixed in M4 batch 6;
   #221 is not a reflection bug. The remaining issues need architectural expansion, evaluator,
   or type-reconstruction work.
2. M5 (diagnostic/ill-formed-program test suite) — not started, genuinely large; the M2 paper
   audits already enumerate most Throws/Mandates conditions per paper, which is the concrete
   starting checklist per the plan. Scope it with a design pass before diving into writing tests,
   same as was done for P3560R2/P3293R3 before implementing.
3. #150 (destructor splice) — still deferred, needs a new parser/Sema destructor-name-via-splice
   representation, per the M4-hard session's notes.
4. **P3560R2 strategy 2 — DO NOT re-attempt without fresh design work first.** Two rounds of
   careful investigation (pilot: 2 API gaps; redesign: a real evaluator `SIGABRT` in
   `extractSubobject`/`HandleConstructorCall`/`VisitCXXInheritedCtorInitExpr` when evaluating a
   compiler-synthesized `CXXConstructExpr` through an inherited constructor) show this needs
   either a different construction strategy (avoid the inherited-constructor evaluation path
   entirely — investigate whether `std::meta::exception` can be built via a non-inherited
   constructor path, or via direct APValue field population instead of a synthesized
   `CXXConstructExpr`) or a genuine evaluator fix for that crash. Treat with the same caution as
   the M3 escalation-cluster bug below — this is not a quick follow-up.
5. M3 escalation-cluster bug — worth another look given today's accumulated splice/scope-lookup
   expertise, but real regression risk (prior attempt broke 9 libc++ tests) — treat with the same
   caution as strategy 2.

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

**Recurring gotcha: PCH/tool-binary staleness causes some false-alarm failures, distinct from OOM.**
Only `ninja -C build-nyx clang` gets rebuilt after most source edits (it's the fast, targeted
command used throughout this epic) — but `c-index-test`, `clang-extdef-mapping`,
`clang-scan-deps`, and other test-suite tool binaries do NOT get rebuilt alongside it, and go
stale relative to `clang`'s PCH/serialization format. Symptom: a validation run shows ~15-20 new
"unable to load precompiled file" failures concentrated in `ClangScanDeps/*`, `Index/Core/*-pch*`,
`Interpreter/*pch*`, `Tooling/pch.cpp`, `Analysis/func-mapping-test.cpp` — check tool binary mtimes
(`ls -la build-nyx/bin/{clang-22,c-index-test,clang-extdef-mapping,clang-scan-deps}`) before
concluding these are real regressions; if `clang-22` is newer than the others, rebuild the stale
ones (`ninja -C build-nyx c-index-test clang-extdef-mapping clang-scan-deps clang-import-test`,
same -j discipline) and re-verify. This is the same root cause as the earlier 308-failure scare.

This staleness warning does **not** explain every such failure. The separate baseline bug audited
in `docs/reflection-audit/codex-pch-bug-report.md` makes `ASTUnit::LoadFromASTFile` reject a PCH
in `readASTFileControlBlock` even when the same PCH loads through the compiler's `-include-pch`
path and all consumer binaries are freshly rebuilt. It is pre-existing and should be tracked as
one repeated ASTUnit/libclang consumer failure, not attributed to stale PCHs or reflection.

**2026-09-09, later same day: repeated OOM kills even at `-j1`** during an attempt to rebuild
those stale tools — `free -h` showed only 2.5G free with Chrome (dozens of tabs/processes) and
Discord/Electron actively consuming memory; the user is visibly using the machine heavily right
now. **When even `-j1` gets OOM-killed, stop retrying immediately and defer the build to a later
heartbeat** rather than hammering it — repeated retries under real memory pressure risk degrading
the user's actual foreground session, which matters more than this epic's throughput. Check
`free -h` and `ps --sort=-rss` first on the next attempt; proceed once there's real headroom.

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
| P3795R2 (Croydon cleanup) | **Complete — pieces 1–4 complete 2026-09-09** | Added scope-identification and tuple metafunctions, verified parameter annotations, and added `data_member_options.annotations` with constant/object validation, compiler ABI threading, generated-member `CXX26AnnotationAttr` attachment, and `annotations_of` coverage. Focused tests pass. |
| P3293R3 "Splicing a Base Class Subobject" | **Implemented — `subobjects_of` and non-virtual direct base-subobject splice completed 2026-09-09** | `BuildReflectionSpliceExpr` now admits non-virtual `ReflectionKind::BaseSpecifier` splices, and `SemaExprMember` emits the ordinary checked derived-to-base conversion with its exact base path. `libcxx/test/std/experimental/reflection/base-splice.verify.cpp` covers read/write access plus virtual-base, array-element, and non-base diagnostics; `subobjects-of.pass.cpp` covers ordering. |
| P3394R4 "Annotations for Reflection" | **Substantially implemented, one confirmed drift** | Core mechanism (parsing `[[=constant-expression]]`, storage, `is_annotation`/`annotations_of`, parameter-annotation extension from P3096, module serialization round-trip) is solid and well-tested. **Confirmed gap, corroborating issue #185 from the M1 triage**: `annotations_of_with_type(info, info)` doesn't exist under that name — implemented instead as an `annotations_of(info, info)` overload with equivalent filter logic but a different name/signature than the final adopted wording; stale pre-R4 API (`annotation_of_type<T>`, `annotate`) still present alongside it. Two items flagged Unverified-without-a-build: the empty-declaration annotation restriction, and the exact direction of the "order preserved" guarantee. |
| P3491R3 "define_static_{string,object,array}" | **Substantially implemented, 3 confirmed gaps** | Final adopted synopsis already absorbed P3617R0's `reflect_constant_array`/`reflect_constant_string` split — confirms the earlier delegation finding *is* the paper's own intent, not a deviation. Gaps: (1) **`define_static_object` is entirely missing** — zero occurrences anywhere, despite `docs/REFLECTION.md:65` claiming all three of `define_static_{string,object,array}` are supported (that line is inaccurate). (2) `is_string_literal` (5 overloads) missing entirely, no compiler query backing it either. (3) `reflect_constant_string` is narrower than spec — fork has 2 fixed non-template overloads (`char`/`char8_t` only) vs. the paper's one generic template covering `wchar_t`/`char16_t`/`char32_t` too; also doesn't implement the "already a string literal → skip double-terminating" carve-out (likely masked in practice but unproven equivalent). `reflect_constant_array`'s Mandates (structural-type, `copy_constructible`) aren't enforced, only `is_constructible_v`. |
| P3560R2 "Error Handling in Reflection" | **Partial — `meta::exception` and thirteen library-side checks implemented 2026-09-09; compiler-side and remaining Throws rewiring open.** | `libcxx/include/meta` now defines the synopsis shape, consteval constructors/accessors, defaulted special members, and `std::exception` base; `exception.pass.cpp` covers both constructors, all accessors, direct consteval try/catch, and positive layout/member/template-query/offset/operator/subobjects coverage. Strategy (1) wraps `has_inaccessible_nonstatic_data_members`, `has_inaccessible_bases`, `annotations_of_with_type`, `size_of`, `bit_size_of`, `alignment_of`, `template_of`, `template_arguments_of`, `access_context::via`, `enumerators_of`, `offset_of`, `operator_of`, and `subobjects_of` before compiler dispatch. The remaining Throws-bearing functions still use `DiagFn`; strategy (2) remains needed for failures not checkable from exposed predicates, including `members_of`/`bases_of`/static and non-static member queries, `parameters_of`/`return_type_of`/`variable_of`, identifier/type/object/constant/parent/dealias queries, `is_accessible`, `extract`, substitution, annotation traversal, `reflect_constant`/`reflect_object`/`reflect_function`, and `data_member_spec`. `define_aggregate` remains intentionally unrecoverable per P3560R2. Concrete design and pilot: [`docs/reflection-audit/codex-strategy2-design.md`](reflection-audit/codex-strategy2-design.md). Nested consteval propagation was re-tested on 2026-09-09 through two user calls and a `std::meta::exception` wrapper; it passes after refreshing staged libc++ headers. See `docs/reflection-audit/codex-nested-throw-report.md`. **Pilot blocked 2026-09-09 before code changes:** the pending-exception reference-catch path assumes every throw site is a `CXXThrowExpr`, and the callback has no evaluator API for constexpr construction of an exception from arbitrary `StringRef`/`APValue` inputs. See [`docs/reflection-audit/codex-strategy2-pilot-report.md`](reflection-audit/codex-strategy2-pilot-report.md) for exact required design changes. |

**Implementation approach (scoped 2026-09-09, read-only prep, no code written yet):**
`CXXMetafunctionExpr::DiagnoseFn` (`clang/include/clang/AST/ExprCXX.h:5605`) is
`std::function<PartialDiagnostic&(SourceLocation, ...)>` — a hard-diagnostic-emission callback,
architecturally distinct from a throwable C++ value. Two viable strategies, not mutually exclusive:
1. **Library-only, no compiler change, for functions whose failure condition is checkable from
   already-exposed predicates** (e.g. `has_default_argument`/`has_ellipsis_parameter`-style
   Mandates that just check a reflection's `ReflectionKind`): wrap the header function to check the
   precondition itself and `throw meta::exception(...)` before ever calling `__metafunction(...)`,
   leaving the compiler-side `DiagFn` path untouched as a backstop for truly-unreachable cases.
   Start here — lower risk, no `ExprConstantMeta.cpp`/`Metafunction.h` changes, and covers a
   meaningful fraction of the ~30 `Throws:`-bearing functions found in the P2996R13/P3096R12/etc.
   audits above.
2. **Compiler-side, for failures only detectable deep in Sema/AST logic** (substitution failures,
   access checks, most of `[meta.reflection.access.queries]`/`[meta.reflection.member.queries]`):
   needs a real "throw-instead-of-diagnose" mode threaded through `Metafunction`/`DiagFn`, using the
   already-working P3068 throw-in-consteval machinery (`ExprConstant.cpp`) to actually raise a
   `meta::exception` object catchable by user `try`/`catch`, not just fail evaluation with a
   compiler error. Bigger, touches shared dispatch infrastructure — do after (1) proves the
   `meta::exception` class shape is right, and scope it as its own dedicated session, not squeezed
   into a mixed batch.
**First concrete step for either strategy**: implement the `class std::meta::exception` type itself
in `libcxx/include/meta` per the P3560R2 synopsis already fetched during the M2 audit (constructors
taking `u8string_view`/`string_view` + `info from` + `source_location where = source_location::current()`,
accessors `what()`/`u8what()`/`from()`/`where()`) — self-contained, buildable/testable independent
of any `DiagFn` rewiring, and unblocks starting strategy (1) on the first candidate function
immediately after.
| P2996R13 "Reflection for C++26" (core) | **Substantially implemented, real small gaps + 2 concrete bugs** | Grammar (reflect-operator, all 3 splicer forms) matches. **Missing**: `has_c_language_linkage`, `has_parent`, `type_order` (upstream libc++ itself tracks this as open, LWG4305), `is_virtual_base_of_type`, `is_trivially_relocatable_type`/`is_replaceable_type`/`is_nothrow_relocatable_type` (underlying `<type_traits>` facilities exist, just no `std::meta` wrapper), and `reference_constructs_from_temporary`/`reference_converts_from_temporary` are present but **commented out** with a `TODO(CXX26)` and had the wrong arity even before being disabled. **Signature drift**: `type_underlying_type`→`underlying_type` rename, `member_offset::total_bits()` missing `const` (real usability bug, not cosmetic), `reflect_constant(T)` by-value vs. paper's by-const-ref, `extract<T>` over-excludes rvalue-references, `data_member_options::bit_width`→`width` rename, `access_context::via(info)` doesn't accept the null reflection the paper explicitly permits (untested either way). **Two concrete, verified-by-direct-read bugs, no build needed**: `symbol_of`/`u8symbol_of` table has `"^"` (not `"^="`) at the `op_caret_equals` slot — this is issue #319 from the M1 triage, independently reconfirmed here with the exact table detail; and `op_co_await` is spelled `"coawait"` instead of the paper's `"co_await"` in the same tables. Framing correction: P2996R13 has **no `Throws:` clauses** at all (unlike P3560R2) — failure is `Mandates:`/`Constant When:`, and the fork's `throw`-inside-`consteval` + `requires`-clause pattern soundly encodes both without needing `meta::exception`. `reflect_invoke`/`subobjects_of`/`define_static_*` are correctly out of P2996R13's own current scope (moved to companion papers or removed pre-R13) — not fork gaps. The ~90-entry `[meta.reflection.traits]` family and most boolean predicates are presence-confirmed but not individually behavior-verified — flagged Implemented-Behavior-Unverified as a group, not itemized. |
| P1306R5 "Expansion Statements" | **Fully implemented — `docs/REFLECTION.md`'s own caveat is stale, not a real gap** | All 3 categories (enumerating/iterating/destructuring) implemented and tested, including the iterating (range-based) form the docs claim isn't supported (`docs/REFLECTION.md:48`: "expansions over constexpr ranges are not supported" — this line dates to commit `e130488` 2024-09-17, *before* iterating expansion was added in `e39580dc8a5c` 2025-03-03; `clang/test/SemaCXX/cxx2c-expansion-stmts.cpp` exercises it extensively including a `constexpr`-declared range, and is not among the documented pre-existing failures). Grammar, control-flow-limiting (no labels), `break`/`continue` semantics, empty-expansion (N=0) handling all match. Two items Implemented-Behavior-Unverified (for-range-declaration decl-specifier restriction, "S1 encloses S2" scoping) — no isolated negative test, but nothing observed contradicts them either. **Doc fix needed** (low-risk, text-only): update `docs/REFLECTION.md:48,63` to drop the stale caveat and mention the iterating category. **One real, already-known, separately-tracked bug remains**: the consteval self-reference escalation cluster (M3 bug #3 above) affects `template for` range-init specifically in one test — not a P1306 gap, a compiler bug already documented and deliberately deferred. |
| P3096R12 "Function Parameter Reflection" | **Substantially implemented, 2 concrete Returns-clause deviations found — FIXED 2026-09-09** | All 7 new metafunctions plus the 4 extended pre-existing ones (`identifier_of`/`u8identifier_of`/`type_of`/`has_identifier` for parameters) present and mostly matching, including two previously-buggy-now-fixed items (`variable_of`'s call-frame lookup, a Parameter-vs-Declaration equality hashing bug — both fixed 2026-09-07 per `CXX26_GAPS.md`). **Two real deviations**: `has_ellipsis_parameter(info r)` and `has_default_argument(info r)` are specified as **total functions** ("Otherwise, false" — no `Constant When`, and R12 explicitly *removed* `has_default_argument`'s Constant When per a LEWG poll) but this fork's implementation (`ExprConstantMeta.cpp:6452-6530`) still diagnoses/fails evaluation for any non-applicable reflection kind instead of returning `false`. Low-risk, well-scoped fix: relax both to return `false` for kinds where they currently diagnose, matching the two already-correct sibling total-functions `is_explicit_object_parameter`/`is_function_parameter` right next to them in the same file. Mandates/Constant-When enforcement elsewhere is systematically Implemented-Behavior-Unverified — matches `docs/REFLECTION.md`'s own admission that ill-formed-program diagnostics are largely untested (this is exactly what M5 exists to close). |
| CWG 3111 (array-type template parameter objects) | **Not yet checked** | Resolved as a C++26 DR at Kona 2025-11-07; affects [meta.define.static]-family reflection queries for array-type template parameter objects. Cross-check against `reflect_constant_array`'s implementation above once its own audit is complete — likely the same code path. |
| LWG 4432 (element init for `reflect_constant_array`) | **Not yet checked** | Resolved Kona 2025-11-04/08. Clarifies copy- vs. direct-initialization semantics for array elements — check `reflect_constant_array`'s actual element-init strategy in `ExprConstantMeta.cpp` against this. |
| LWG 4426 (`reflect_constant_string` literal detection) | **Not yet checked** | Live-checked 2026-09-08: status is now **C++26** (moved from Tentatively Ready). Wording changes "trailing null terminator" → "trailing u+0000 null character" and adds "is a reference to" for precision, in [meta.reflection.array]. Small wording-only fix — check whether the fork's `is_string_literal`-style detection already matches the *intent* even if built against the old imprecise wording. |
| LWG 4428 (metafunctions shouldn't be defined via constant subexpressions) | **Partial — library-side checks added 2026-09-09** | The three named functions now throw `meta::exception` for exposed invalid type/class preconditions in `libcxx/include/meta`; deep access and annotation failures still follow compiler-side `DiagFn` and require strategy (2).
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

4. **ASTUnit/libclang PCH control-block load failure — pre-existing baseline, investigated
   2026-09-09.** A PCH emitted by current `clang -cc1` loads successfully through the compiler's
   `-include-pch` path but is rejected by `c-index-test -module-file` and the corresponding
   libclang/ASTUnit path. Fresh auxiliary-tool rebuilds and disabled ASTReader validation do not
   change the result. Temporary instrumentation showed the failure is in
   `readASTFileControlBlock`, before `ASTReader::ReadAST`; the helper discards the underlying
   error and ASTUnit emits only `unable to load precompiled file`. No PCH consumer or serialization
   code changed during today's reflection commits, so this is not an epic regression. The roughly
   18 Index/Core, ClangScanDeps, Interpreter, Tooling, and Analysis failures are one repeated root
   cause. Full details: `docs/reflection-audit/codex-pch-bug-report.md`.

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
| 120 | Repeated first argument on Windows | Confirmed-Open → Skipped 2026-09-09 | Upstream discussion points to PR #243 (`b725cb40f042`) as a placeholder Microsoft mangler implementation, but GitHub API access was unavailable this session and no local port exists. Linux cannot verify MSVC ABI output. A real fix should add a Microsoft equivalent of the Itanium `mangleReflection` path, call it from `mangleTemplateArgValue` instead of the `APValue::Reflection` `llvm_unreachable`, and add Windows/ClangCL regression coverage for distinct reflection NTTPs and expansion results. Not verified here. | High |
| 146 | Expansion generates `case` labels | Confirmed-Open → Fixed 2026-09-09 | Parser diagnostics reject `case` and `default` labels while parsing an expansion body, before the raw-label CodeGen path can be reached. `expansion-case-labels.cpp` covers both forms. | High |
| 150 | Spliced explicit destructor call | Confirmed-Open → Skipped 2026-09-09 | Re-traced `ParseExpr.cpp`'s `~` path: `ParseUnqualifiedId(...IK_DestructorName...)` accepts ordinary type/name tokens but has no splice branch. Supporting `~[:info:]()` needs a splice-bearing destructor-name AST/Sema representation and destructor lookup/call formation; the existing member-splice path cannot preserve those semantics. | High |
| 151 | ICE in member-wise swap | Already-Fixed | Fixes `f0a3e5e612db`/`0f70ed5eb99e`; `CGStmt.cpp:1605-1618` scoping; `miscellaneous.pass.cpp:126-145` close coverage (no dedicated regression test though). | Medium |
| 154 | `underlying_type` ICE for non-enum | Already-Fixed | Direct `-freflection-latest -fsyntax-only` probe diagnoses the invalid `underlying_type_t<int>` substitution cleanly; no ICE or process failure at current HEAD. | High |
| 169 | Crash in templated lambda | Confirmed-Open → Fixed 2026-09-09 | Exact reproducer aborted in `CheckIfAnyEnclosingLambdasMustCaptureAnyPotentialCaptures` when an expansion statement left the lambda scope stack out of sync with `CurContext`. The potential-capture walk now returns for that invalid context. **Independently re-verified** (the first attempt was uncommitted due to a sandbox `.git`-read-only block and had no gate run) — built clang, ran the test directly (compile+execute, exit 0) since full-suite lit runs were OOM-killed repeatedly; also confirmed `clang/test/Reflection` 20/20 clean. Test lives at `libcxx/test/std/experimental/reflection/expansion-lambda-capture-crash.pass.cpp` (needs real `<meta>`/`define_static_array`/`members_of`, not available in the bare `clang/test/Reflection` environment — the original test file was misplaced there and has been moved). | High |
| 173 | Templated `named_tuple` | Out-of-Scope | Feature request; `define_aggregate` (`libcxx/include/meta:2503-2515`) already covers the underlying need. | High |
| 175 | Namespace comparison | Already-Fixed | `2245a73e94f5`; `namespace-reflection-equality-reopened.pass.cpp:70-87`. | High |
| 176 | `members_of` + empty namespace redeclaration | Already-Fixed | `ExprConstantMeta.cpp:1535-1545`; same test file `:41-61`, `:70-84`. | Medium |
| 177 | Enum NTTP loses enumerator identity | Out-of-Scope | Intentional: reflected value vs. enumerator declaration distinction, `ExprConstantMeta.cpp:4192-4208`; `entity-classification.pass.cpp:61-82` requires this. | High |
| 178 | `template for` over `integer_sequence` | Already-Fixed | Direct `-freflection-latest -fsyntax-only` probe over `std::integer_sequence<int,1,5>` succeeds at current HEAD. | High |
| 180 | `static_assert(false)` ignored | Confirmed-Open — deferred 2026-09-09 | Exact reproducer still compiles successfully, instantiates `test_impl<int>`, and ignores the dependent `static_assert(false)` that should make substitution ill-formed. This needs the broader expansion-body deferral design associated with PR #261; no safe local diagnostic tweak identified. | High |
| 181 | Non-copyable tuple in `template for` | Confirmed-Open — deferred 2026-09-09 | Exact range-for probe still attempts to copy `const tuple<unique_ptr<...>>` and diagnoses its deleted copy constructor. Correct range/reference preservation is coupled to the #180/#261 expansion work; no safe local fix identified. | High |
| 182 | `template for` + `continue` ICE | Confirmed-Open → Skipped 2026-09-09 | CodeGen currently creates one continuation destination per expansion instance before emitting discarded `if constexpr` bodies; fixing this needs instance-discard awareness in expansion control-flow lowering and a regression gate for break/continue nesting. No safe local patch was identified; deferred with the M3 consteval escalation cluster. | High |
| 183 | ICE with imported reflection function | Already-Fixed | `module-imports.sh.cpp:1-17`; serialization fix `090152727f3f` (broader than original scenario). | Medium |
| 184 | Spurious consteval-only diagnostic | Needs-Build-To-Verify | No exact source reproducer was available in the issue body beyond a Godbolt link; retain until the nested-lambda/consteval-only NTTP example is rebuilt. | Low |

**185-225:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 185 | Annotation API changed in R1 | Confirmed-Open → Fixed 2026-09-09, commit `625ed6cec16a` | Added adopted `annotations_of_with_type(info, info)` as a compatibility-preserving forwarding wrapper over the existing filtered implementation, with focused coverage. Legacy APIs remain available for existing fork tests and callers. | High |
| 187 | Compilation never ends | Needs-Build-To-Verify | No reproducer in snapshot, Godbolt-link only. | Low |
| 188 | `display_string_of(dealias(...))` not constant expr | Confirmed-Open — deferred 2026-09-09 | Rebuilt the issue's sequence of `ranges::max_element` type displays. Current HEAD rejects `dealias`/double-`dealias` cases for template-heavy iterator types as not constant expressions. Requires dedicated evaluator/printer investigation; no narrow safe fix identified. | High |
| 189 | "Upstream to LLVM" | Out-of-Scope | Distribution/adoption request, not a defect. | High |
| 200 | `parent_of` wrong for class-template aliases | Confirmed-Open → Already-Fixed 2026-09-09 | Direct probe `static_assert(parent_of(^^T::A) == ^^T)` passes. Existing `findTypeDecl` preserves the top-level `UsingType` alias before template-specialization fallback; the stale deferral note incorrectly described the current checkout. | High |
| 203 | Unbalanced diagnostic parentheses | Already-Fixed | Direct malformed-range probe now emits a balanced diagnostic (`cannot expand over a function 'void ()'; did you mean to call it with no arguments?`) with no unbalanced-parenthesis output. | High |
| 204 | ICE: `template for` over overload set | Already-Fixed | Exact overloaded-function-range probe now produces ordinary overload-resolution diagnostics and no crash/abort at current HEAD. | High |
| 205 | ICE: templated lambda + `define_static_array` | Already-Fixed | Fix `f72d85e5a0fd`; `SemaExpand.cpp:148-173`. | High |
| 208 | CRTP constexpr degradation | Needs-Build-To-Verify | Godbolt-link only, insufficient detail to localize. | Low |
| 210 | Capturing lambda inside expansion statement | Already-Fixed | Fixes `f0a3e5e612db`, `0f70ed5eb99e`; `SemaExpand.cpp:148-173`. | High |
| 211 | Static enum member wrong `type_of` | Already-Fixed | Direct probe for `static E S::value` confirms `type_of(^^S::value) == ^^E`. | High |
| 212 | ICE in `if constexpr` optional extraction | Needs-Build-To-Verify | Insufficient repro detail in snapshot. | Low |
| 215 | Empty `reflect_constant_array` result | Already-Fixed | Fix `5dafd8cc4a45`; `libcxx/include/meta:2598-2616`; `static-arrays.pass.cpp:64-70`. | High |
| 220 | `display_string_of(type_of(undeduced))` hangs | Confirmed-Open — deferred 2026-09-09 | Exact issue probe with `auto operator()();` still exceeds a 20-second timeout without producing the expected diagnostic. Likely infinite recursion around undeduced return-type handling; needs a dedicated traced evaluator fix. | High |
| 221 | `expected<bool,int>` in `vector` fails | Confirmed-Open → Not a reflection bug 2026-09-09 | Exact `std::vector{value(), value()}` probe fails in libc++ `__expected/expected.h:1167` with self-dependent constraint satisfaction while copying the expected elements. The source contains no reflection; leave outside the M4 reflection backlog. | High |
| 222 | `define_aggregate` nested incomplete type in template arg | Already-Fixed | Exact non-templated-inner-type/structural-template-argument probe compiles successfully at current HEAD; no `'inside_S' cannot be defined in a parameter type` diagnostic. | High |
| 225 | `meta::exception` unimplemented | Confirmed-Open → Partial 2026-09-09 | `<meta>` now declares and defines the P3560R2 class, with direct consteval construction/accessor/throw-catch regression coverage. Remaining issue scope is the unrewired Throws-bearing metafunctions; nested consteval exception propagation passes after staged-header refresh. | High |

**230-273:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 230 | Reflecting `std::int32_t` | Out-of-Scope | Deliberate: using-shadow-decl rejection unless entity-proxy reflection on (`SemaReflect.cpp:1044-1050`,`1314-1336`); unresolved WG21 semantics. | High |
| 232 | `template for` + `display_string_of` | Already-Fixed | Direct probe over a member of type `std::vector<int>` (exercising pretty-printer template-argument rendering inside `template for`) succeeds at current HEAD. | High |
| 234 | Protected base member reflection | Already-Fixed | Direct derived-context probe with `static_assert(is_protected(^^A::protected_virtual_function))` succeeds at current HEAD. | High |
| 235 | Ambiguous constructor reflection | Out-of-Scope | Unresolved design question — multiple ctors, no WG21 resolution to implement against (`SemaReflect.cpp:1398-1430`). | High |
| 237 | Alias of closure type loses identity | Confirmed-Open — deferred 2026-09-09 | The issue's `using ct = typename[:cr:]` closure-alias probe still diagnoses `'auto' not allowed in type alias`; reconstructing an invented closure type through a splice needs deeper alias/type work. No safe local fix identified. | High |
| 239 | Closure `operator()` reported overloaded | Confirmed-Open → Fixed in this batch | `BuildCXXReflectExpr` now preserves a unique `TemplateDecl` when invented-`auto` deduction fails, covering deduced-`this` closure call operators; ported from PR #244 and validated by the full Clang gate with no reflection failures. | High |
| 245 | Protected member as reflected template arg | Not-Applicable | `access_context` model (`libcxx/include/meta:1053-1090`) intentionally preserves access-context effects. | High |
| 246 | Order-dependent `define_static_array` | Already-Fixed | Both the two-declaration probe and the variant with `arr_0` removed compile successfully; no order dependence at current HEAD. | High |
| 252 | VS Code `__has_feature(reflection)` | Out-of-Scope | Editor/IntelliSense issue, not Clang. | High |
| 253 | ICE during recursive reflection in modules | Needs-Build-To-Verify | Module/annotation serialization work exists (`f63157a8d87c`) but no exact repro match. | Medium |
| 254 | `define_static_string` hits constexpr step limit | Confirmed-Open → Skipped 2026-09-09 | The failure is the evaluator's finite constexpr operation budget, not an incorrect result or missing semantic branch. Chunking the copy loop could reduce per-expression work but cannot guarantee acceptance under an implementation-selected step limit and would alter the established implementation strategy; callers can raise `-fconstexpr-steps`. | High |
| 256 | `__is_consteval_only` + defaulted special members | Already-Fixed | `Decl.cpp:5426-5475` fixed by `01836b3333d5`; `p3603-consteval-only.pass.cpp` covers it. | High |
| 259 | Windows build errors | Out-of-Scope | Upstream platform/build-system issue, unrelated to this fork's two-tree build architecture. | High |
| 262 | Annotation crash with modules | Already-Fixed | `f63157a8d87c` added `CXX26AnnotationAttr` serialization; `annotation-module-serialization.sh.cpp`. | High |
| 264 | Constant evaluation of arrow splices | Already-Fixed | Computed `nonstatic_data_members_of` reflection used in `(&value)->[:field:]` during constant evaluation succeeds at current HEAD. | High |
| 265 | Anonymous union members unavailable for splicing | Already-Fixed | `f33742c88aa3`, `2ea0a79fe7bb`; `anon-union.pass.cpp:18-31`. | High |
| 273 | Dependent function-template reflection seen as overload set | Already-Fixed | `41fa327e7d63`; `SemaReflect.cpp:998-1030`. | High |

**275-309:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 275 | clangd crashes while code compiles | Needs-Build-To-Verify | Fork changed substantially since reported clangd build; no local repro. | Low |
| 276 | Splice type aliases treated as identical | Fixed | PR #277 ported; dependent splice types now canonicalize by operand and template arguments, with distinct/same-alias regression coverage. | Medium |
| 280 | `has_parent` missing | Confirmed-Open → Fixed 2026-09-09, commit `c21e31f8eb5e` | Added `std::meta::has_parent(info)` and compiler dispatch, returning false for reflections without an associated parent; `has-parent.pass.cpp` covers type, declaration, namespace, null-parent, and value cases. | High |
| 281 | ICE from `annotations_of(^^member)` | Already-Fixed | `SemaReflect.cpp:1371-1377`; `p3394-annotations.pass.cpp:89-110`,`131-142`. | Medium |
| 286 | Same-named function-template reflections collide | Confirmed-Open → Fixed 2026-09-09, commit `2262084ae5e1` | Mangling cluster fix adds overload discrimination via ODRHash. | High |
| 288 | Reentrant constant evaluation UAF | Fixed | PR #289 ported; immediate-invocation and cleanup paths reacquire evaluation-context records after reentrant operations, with a 64-level reflection instantiation regression test. | High |
| 290 | Entity-proxy queries + mangling crash | Fixed | PR #291 ported; proxy member predicates now return false and proxy NTTP mangling targets the underlying declaration; `entity-proxy-member-queries.pass.cpp`. | High |
| 292 | `AttributedType` blinds function queries | Already-Fixed | Fix `1dee6d809821`; `ExprConstantMeta.cpp:1622-1629`; `attributed-function-type-queries.pass.cpp`. | High |
| 294 | `can_substitute`/`substitute` crash on invalid formed types | Fixed | PR #295 ported; substitution APIs now return failure and suppress diagnostics for invalid formed types, with function/alias/variable/member-template regression coverage. | High |
| 296 | `members_of` eagerly instantiates member bodies | Already-Fixed | Fix `7220baffd57e`; `members-of-lazily-ill-formed-bodies.pass.cpp:18-28,75-89`. | High |
| 298 | Deduction-guide reflection mangler ICE | Confirmed-Open → Fixed 2026-09-09, commit `2262084ae5e1` | Mangling cluster fix handles Template-kind deduction-guide reflections. | High |
| 300 | Same-headed member-template reflections collide | Confirmed-Open → Fixed 2026-09-09, commit `2262084ae5e1` | Mangling cluster fix adds specialization-context type and ref-qualifier discrimination. | High |
| 302 | Reopened namespace reflections compare unequal | Already-Fixed | `2245a73e94f5`; `APValue.cpp:575-584`; `namespace-reflection-equality-reopened.pass.cpp:19-31,70-95`. | High |
| 303 | Reopened namespace walk truncates members | Fixed | PR #306 ported; out-of-line class-member definitions are excluded from namespace enumeration and traversal returns to their lexical namespace, with reopened-namespace coverage. | High |
| 304 | `is_complete_type` fails through aliases | Already-Fixed | Fix `7f0f89cc7e75`; `ExprConstantMeta.cpp:4865-4880`; `is-complete-type-alias-sugar.pass.cpp`. | High |
| 308 | Tracking issue for #286-#304 | Out-of-Scope | Aggregate meta-issue; entries triaged individually above. | High |
| 309 | Dependent splice ICE during `auto` NTTP deduction | Fixed | PR #310 ported; null-model dependent splices are classified by their own value kind, with libc++ requires-expression regression coverage. | High |

**311-350:**
| # | Title | Disposition | Evidence | Conf. |
|---:|---|---|---|---|
| 311 | Builtin-template diagnostic ICE | Fixed | PR #315 ported; `DescriptionOf` handles builtin and template-template-parameter reflections with a safe fallback, with a builtin-template diagnostic regression test. | High |
| 312 | Deduction-guide specialization mangling ICE | Confirmed-Open → Fixed 2026-09-09, commit `2262084ae5e1` | Mangling cluster fix handles declaration-kind deduction-guide-specialization reflections. | High |
| 313 | `members_of` truncates after linkage specifier | Already-Fixed | `ExprConstantMeta.cpp:1551-1560` already descends into `LinkageSpecDecl`. | High |
| 314 | LP64 NEON vector mangling ICE | Fixed | PR #318 ported; LP64 `long`/`unsigned long` NEON elements use the 64-bit ABI spellings, with focused AArch64 mangling coverage. | High |
| 319 | `op_caret_equals` symbol typo | Confirmed-Open → Fixed 2026-09-08, commit `357aff58a79b` | Both operator tables now use `"^="` at the `op_caret_equals` slot, with regression coverage. | High |
| 321 | 32K+ template packs miscompile | Fixed | PR #323 ported; template pack counts and substitution indices are full-width (with bounded packed storage where required), and a 50K-element `define_static_string` regression passes. | High |
| 322 | Parameter-name result depends on instantiation | Already-Fixed | Fix `fad02ea72cc7`; `ExprConstantMeta.cpp:1130-1148`; `param-name-consistency-instantiation.pass.cpp`. | High |
| 326 | Expansion statement ICE during instantiation | Confirmed-Open → Fixed 2026-09-09, commit `c1d0c5075bbb` | The shared `BuildCXXExpansionSelectExpr` validation is called from `TransformCXXIndeterminateExpansionSelectExpr`, so the function-range rejection covers instantiation as well as parse time; regression coverage includes dependent function templates. | High |
| 327 | Expansion statement ICE on unresolved overload range | Fixed | PR #328 ported; function and function-pointer ranges are diagnosed before expansion/ADL candidate construction, with overload-range regression coverage. | High |
| 329 | Constant evaluation crash through PCH | Confirmed-Open → Skipped 2026-09-09 | This is an AST serialization/evaluator-state redesign: `CXXMetafunctionExpr` callbacks and reflection evaluation context must round-trip through PCH/module serialization, with evaluator-owned state reconstructed on deserialization. A local statement-serialization tweak would risk stale callbacks/UAFs; requires a dedicated PCH/module design and reproducer gate. | Medium |
| 331 | `reflect_object` rejects explicit defaulted copy ctor | Already-Fixed | Exact issue probe with `A(const A&) = default` compiles successfully at current HEAD. | High |
| 332 | `reflect_constant` rejects pointer to mixed consteval-only type | Already-Fixed | Exact issue probe with both `meta::info` member and non-static `consteval` member function compiles successfully at current HEAD. | High |
| 333 | Splice operand convertible to `meta::info` rejected | Already-Fixed | `SemaReflect.cpp:1575-1593` already handles `DefaultLvalueConversion` + implicit conversion. | High |
| 334 | Static member call inherits consteval-only object restriction | Confirmed-Open → Fixed 2026-09-09 | `MarkMemberReferenced` now removes a direct consteval-only object reference from the immediate-context set for non-arrow static member calls; side effects remain normally analyzed. `static-member-consteval-only.cpp` covers the reported runtime call. | Medium |
| 342 | `^^derived::operator()` rejects using-declaration | Fixed | PR #353 ported; reflection-name syntax remains rejected, while id-expressions naming introduced operators/templates resolve to the target declaration, including dependent cases. | High |
| 346 | Spurious warning for reflected reference type | Confirmed-Open → Fixed 2026-09-09 | Exact probe emitted `-Wreflexing-parse` for `^^decltype(std::move(1))`, despite no source `&`/`&&` token. **Independent re-verification caught a real bug in the first attempt's fix**: it checked `LastTypeToken.is(tok::ampamp)` only, which suppressed the warning entirely for the single-`&` case too, regressing `clang/test/Reflection/lift-operator.cpp`'s `'&' binds to reflection operand` expectations (2 previously-passing checks started failing) — fixed by checking `.isOneOf(tok::amp, tok::ampamp)` instead. Verified: `clang/test/Reflection` back to 20/20 (including `lift-operator.cpp`), the `std::move(1)` repro passes cleanly via direct `clang++` invocation and `libcxx-lit`. Test lives at `libcxx/test/std/experimental/reflection/no-spurious-reflect-reference-warning.pass.cpp` (needs `<utility>`/`std::move`, not available in bare `clang/test/Reflection`). **Two new, separate, genuinely pre-existing bugs found while constructing a minimal repro** (confirmed via `git stash` against unmodified code, unrelated to this fix): `^^decltype(<ordinary-function-call>)` (non-template) fails to parse at all (`type name requires a specifier or qualifier` / spurious Blocks-syntax errors — likely a tentative-parse disambiguation gap for call-expressions inside `decltype` as a reflect-operand), and `^^SomeTypeAliasName` (a plain identifier naming a type alias, with no lookahead token in `{[`, `(`, `*`, `&`, `&&`, cv-qualifiers`}` after it) is misrouted down the unqualified-id/entity path instead of being recognized as a type, with the same downstream parse corruption. Neither is in the M1 issue list — **not yet triaged as new issues, flagged here for a future session**. | High |
| 350 | `->[:member:]` assertion with lvalue pointer | Fixed | PR #352 ported; splice member bases undergo the ordinary member-access conversions, covering lvalue pointers and array decay. | High |

## Upstream PR triage (M1)

**Major finding (2026-09-08): most of the 35 open upstream PRs are ready-made fixes for issues
this epic just triaged as Confirmed-Open.** Titles alone map cleanly onto the mangling cluster and
several other confirmed issues — this changes M4 from "write fixes from scratch" to "assess and
port these diffs" for a large fraction of the backlog. **Do not re-derive a fix from scratch before
checking this list first.**

| PR # | Title | Maps to issue(s) |
|---:|---|---|
| 353 | Reflect the introduced function when an id-expression names a using-declarator | #342 — Fixed; ported and verified in commit |
| 352 | Convert the base of a member splice before building the member expression | #350 — Fixed; ported and verified in commit |
| 347 | Remove warning if `&` is not directly in code | #346 (likely) |
| 345 | Allow reflections of values designating immediate functions | **#184/#334 consteval-only cluster — #334 Already-Fixed; #184 Needs-Build-To-Verify. Ready-made upstream fix exists; port in M4 only after a buildable reproducer/gate.** |
| 340 | Add `std::meta::has_c_language_linkage` | **Fixed in this batch; compiler/library implementation and focused coverage added.** |
| 330 | [Clang][P2996] Implement PCH serialization for `ExplDependentCallExpr` | #329 (related) |
| 328 | fix crash on expansion statement over an overload set | #327, possibly #326 — Fixed; ported and verified in commit |
| 323 | Fix silent miscompile of template argument packs with 2^15+ elements | #321 — Fixed; ported and verified in commit |
| 320 | Fix `symbol_of`/`u8symbol_of` table entry for `op_caret_equals` | #319 |
| 318 | Handle LP64 long-element NEON vectors in the Itanium mangler | #314 — Fixed; ported and verified in commit |
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
| 279 | Fix reflect unresolved lookup | **#273 (dependent function-template reflection) and #276 (dependent splice-type canonicalization) — both Already-Fixed here by independent ports. The PR is a ready-made upstream equivalent/stacked fix; no port needed unless regression coverage is revisited.** |
| 277 | Fix canonicalization of dependent splice types | #276 — Fixed; ported and verified in commit |
| 261 | Defer expansion statement body instantiation | **Expansion cluster, principally #180 and related #181 — Confirmed-Open. Ready-made upstream AST/Sema/TreeTransform fix exists; port/adapt in M4 with the expansion regression gate.** |
| 249 | Merging upstream 91cdd350 [clang] Improve nested name specifier AST representation | **General LLVM synchronization (11 files), not a reflection-specific defect or C++26 paper facility. Current fork has independently reconciled the relevant AST representation; out of scope for this closure epic.** |
| 244 | Fix `Sema::BuildCXXReflectExpr` wrongly thinks a template is overloaded (e.g. reflecting a lambda's `operator()`) | **Fixed in this batch; closes #239.** |
| 227,195,170 | [P3816] Implement `std::consteval_hash<std::meta::info>` (3 iterations) | **Out-of-Scope confirmed: P3816 remains a separate, non-adopted proposal, not required C++26 reflection. These are experimental competing implementations; no issue or paper-gap action here.** |
| 207 | Examples from P2996 - Reflection for C++26 | **Low-priority/out-of-scope for conformance closure: Docker/build scaffolding and examples only; no compiler or standard-library fix.** |
| 168 | Adding string literal manipulation | **P3491R3 facility (`is_string_literal` and string-literal helpers), in scope and currently missing per the paper audit. Ready-made upstream implementation exists; future M4 paper-gap port, with wording/API review first.** |
| 166 | fix `is_reflection_type` | **Fixed in this batch; alias/dealias correction plus alias regression coverage added.** |
| 163 | [P3074] Implementing part of trivial unions | **Out-of-scope for this tracker, confirmed: P3074 is language-side work already tracked in `docs/CXX26_GAPS.md`; route any port there.** |
| 135 | ast dump for splice specifier and reflection splice type | **Low-priority tooling/AST-dump coverage, not C++26 reflection conformance; defer outside closure critical path.** |
| 124 | Custom annotation requires type to be `equality_comparable` | **P3394R4 annotation Mandates correction. Ready-made constraint exists, but PR changes legacy `experimental/meta`; adapt against current adopted `meta` API in future M4/M5 rather than port verbatim.** |

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

## M5 diagnostic checklist

M5 scoping is complete. The paper-by-paper inventory of every identified Constraints/Mandates/
Throws/Constant-When ill-formed condition, existing coverage, implementation blockers, row counts,
and recommended facility-first execution plan is in
[`reflection-audit/M5-diagnostic-checklist.md`](reflection-audit/M5-diagnostic-checklist.md).
The inventory records **109 conditions: 23 covered, 68 needing new tests, and 18 blocked on an
unimplemented facility or P3560R2 exception plumbing**. Future M5 sessions must update checklist
rows as tests land and append their results to this tracker’s Session Log.

## Session Log

**2026-09-09 — M5 diagnostic scoping.** Read Ground Truth, the original M5 definition, the full
M2 paper audit, the existing reflection verify/pass tests, and the adopted wording for all ten
papers. Added `reflection-audit/M5-diagnostic-checklist.md` with 109 condition rows, coverage and
implementation classifications, exact existing-test pointers, totals, and a facility-first plan.
No test files or implementation sources were changed.

**2026-09-09 — P3560R2 strategy-(2) design investigation.** Read Ground truth, the P3560R2
row, the five paper-audit rows, the 13-function strategy-(1) history/report, P3068 pending
exception machinery, and the metafunction dispatch. Added
`docs/reflection-audit/codex-strategy2-design.md`: enumerated remaining compiler-side targets,
rejected a raw `DiagFn` adapter, recommended a dual failure sink with evaluator-owned pending
exception construction, assessed risk against the M3 escalation cluster, and specified a
`members_of` throw/catch pilot. No source-code changes were made.

**2026-09-09 — P3795R2 piece 1.** Added the three scope-identification metafunctions. The
compiler reuses the existing `StackLocationExpr` call-frame lookup used by
`access_context::current()`, while `<meta>` enforces the paper's function/class contracts with
`meta::exception` and walks to the nearest namespace. Added `p3795-scope.pass.cpp`; focused
libc++ lit passed 1/1 and the Clang reflection suite passed 18/18. A low-memory full Clang lit
attempt was interrupted after 24 tests; it reported only the five documented baseline failures
and no new failure. Tuple metafunctions, generated-member annotations, and the parameter-
annotation audit remain.

**2026-09-09 — P3795R2 piece 2.** Added the three tuple-related metafunctions as library
implementations using the existing tuple queries and invocability traits. Focused libc++
test `p3795-tuple.pass.cpp` passes. No compiler changes were needed.

**2026-09-09 — P3795R2 piece 4.** Independently reran the existing
`p3394-parameter-annotations.pass.cpp` test through `libcxx-lit`; it passed 1/1. Parameter
annotations therefore require no new implementation in this fork. Piece 3 remains open.

**2026-09-09 — P3560R2 step 1 and strategy (1) start.** Added `std::meta::exception` to
`libcxx/include/meta`, including both consteval constructors, all four accessors, defaulted special
members, and direct consteval try/catch coverage. Made the C++26-facing `std::exception` base
destructor constexpr so the derived consteval-only type can be formed, while preserving the
pre-C++26 ABI destructor. Added library-side `meta::exception` checks to the three LWG 4428
functions. A later nested-call audit found those paths catch correctly after refreshing staged
libc++ headers; the earlier contrary result was a stale-header false positive. Remaining Throws
rewiring and compiler-side DiagFn work are deferred.

**2026-09-09 — nested throw audit.** Re-tested direct, one-level, two-level, ordinary-caller,
and `std::meta::exception` wrapper cases. All pass after refreshing staged libc++ headers; the
earlier failure was a stale-header false positive, not an `ExprConstant.cpp` propagation or RAII
bug. Findings: `docs/reflection-audit/codex-nested-throw-report.md`.

**2026-09-09 — P3560R2 strategy (1), batch 2.** Added library-side precondition throws to
`size_of`, `bit_size_of`, and `alignment_of`, using existing reflection-kind predicates and
`is_complete_type`; added positive layout-query coverage to `exception.pass.cpp`. A negative
catch test for these newly wrapped scalar queries was not retained because this build's evaluator
reported the wrapper exception as uncaught in that test shape, while the existing direct and
nested `meta::exception` propagation tests remain passing. Access-context, member-query, template,
parameter, extraction, substitution, and annotation-target failures requiring additional compiler
state or non-trivial result handling remain for later strategy (1)/(2) work.

**2026-09-09 — P3560R2 strategy (1), batch 3.** Attempted library-side `meta::exception`
precondition throws for the member-query group. `members_of`, `bases_of`,
`static_data_members_of`, and `nonstatic_data_members_of` remain unchanged after the full
reflection sweep showed that their existing proxy/bit-field/definition behavior requires deeper
compiler state; these are reserved for strategy (2).

**2026-09-09 — P3560R2 strategy (1), batch 3 continued.** Attempted guards for `parameters_of`
and `return_type_of`; existing builtin-template diagnostic behavior requires compiler-side state,
so these remain reserved for strategy (2).

**2026-09-09 — P3560R2 strategy (1), batch 3 continued.** Added a library-side `meta::exception`
precondition throw to `enumerators_of`, using `is_enumerable_type`; added positive enum-query
coverage to `exception.pass.cpp`. The focused libc++ lit test passed.

**2026-09-09 — P3560R2 strategy (1), batch 3 continued.** Added a library-side `meta::exception`
precondition throw to `access_context::via`, using `is_class_type`; added positive coverage to
`exception.pass.cpp`. The focused libc++ lit test passed.

**2026-09-09 — P3560R2 strategy (1), batch 3 final gate.** Retained only the four additional
safe wrappers from this session: `template_of`, `template_arguments_of`, `access_context::via`,
and `enumerators_of`. Member and parameter query guards were reverted after full-sweep failures
showed that their preconditions require compiler state. Corrected layout-query predicate ordering
and compatibility for zero-width bit-fields, aligned reference variables, and data-member specs.
The libc++ reflection sweep passed 75/76 tests, with one unsupported and no failures.

**2026-09-09 — P3560R2 strategy (1), batch 3 continued.** Added library-side `meta::exception`
precondition throws to `template_of` and `template_arguments_of`, using
`has_template_arguments`. Added positive template-query coverage to `exception.pass.cpp`; the
focused libc++ lit test passed.

**2026-09-09 — P3560R2 strategy (1), batch 4 start.** Added a library-side `meta::exception`
precondition throw to `offset_of`, using the exposed `is_nonstatic_data_member` and `is_base`
predicates. Positive offset-query coverage passed through `libcxx-lit`. This brings the total
strategy-(1) wrappers to eleven; operator and class-type query candidates remain under review.

**2026-09-09 — P3560R2 strategy (1), batch 4 continued.** Added a library-side `meta::exception`
precondition throw to `operator_of`, using the exposed operator-function predicates. Positive
operator-query coverage passed through `libcxx-lit`; the total strategy-(1) wrapper count is now
twelve.

**2026-09-09 — P3560R2 strategy (1), batch 4 final.** Added a library-side `meta::exception`
precondition throw to `subobjects_of`, using `is_class_type` before delegating to base/member
queries. Positive subobject-query coverage passed through `libcxx-lit`; thirteen wrappers are
now implemented across all sessions. Remaining named candidates either have no failing
precondition (`source_location_of`, `has_identifier`), require ambiguity/encoding or
constant-evaluation state (`identifier_of`, `u8identifier_of`, `type_of`, `object_of`,
`constant_of`, `extract`, `is_accessible`), or are constrained templates whose Mandates cannot
be converted safely using only the current library predicates (`define_static_*`,
`reflect_constant`, `reflect_object`, `reflect_function`).

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

**2026-09-09 — M4 PR batch, PR #318.** Ported upstream PR #318 for issue #314. Itanium NEON
mangling now maps LP64 `long` and `unsigned long` elements to the 64-bit ABI names for both
polynomial and ordinary vectors. The focused AArch64 mangling test passed. Built `clang` with
`-j2`, rebuilt stale auxiliary tools, and ran the capped direct-lit Clang gate: 49,819 discovered,
44,614 passed, exactly the five documented baseline failures.

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

**2026-09-09 — M4 PR batch, PR #323.** Ported upstream PR #323 for issue #321. Template
substitution pack counts and non-type pack indices no longer truncate at 15 bits; the remaining
packed type index is widened to 26 bits with overflow assertions. Added the 32K+/50K-element
`define_static_string` regression; it passed through the libc++ wrapper. Built `clang` with `-j2`,
rebuilt stale auxiliary tools, and ran the capped direct-lit Clang gate: 49,819 discovered,
44,614 passed, exactly the five documented baseline failures.

**2026-09-09 — M4 PR batch, PR #328.** Ported upstream PR #328 for issue #327. Expansion
statement ranges with function or function-pointer type are now rejected before iterable
expansion and ADL candidate construction; placeholder ranges are checked first. Added
`expansion-statements-overload-range.cpp`; it passed after adapting the expected overload-recovery
diagnostic to this fork. Built `clang` with `-j2`, rebuilt stale auxiliary tools, and ran the capped
direct-lit Clang gate: 49,820 discovered, 44,615 passed, exactly the five documented baseline
failures.

**2026-09-09 — M4 PR batch, PR #352.** Ported upstream PR #352 for issue #350. Member splice
bases now use `PerformMemberExprBaseConversion`, matching ordinary `->` access for lvalue pointer
conversion and array-to-pointer decay. Extended `splice-exprs.cpp` with pointer, array, consteval,
and dependent-splice cases; it passed. Built `clang` with `-j2`, rebuilt stale auxiliary tools, and
ran the capped direct-lit Clang gate: 49,820 discovered, 44,615 passed, exactly the five documented
baseline failures.

**2026-09-09 — M4 PR batch, PR #353.** Ported upstream PR #353 for issue #342. Reflection-name
syntax continues to reject using-declarators, while id-expression forms now reflect the introduced
target declaration; this distinction also works for operators, templates, and dependent bases.
Added the using-declarator wording regression to `reflection-wording-examples.cpp`; it passed.
Built `clang` with `-j2`, rebuilt stale auxiliary tools, and ran the capped direct-lit Clang gate:
49,820 discovered, 44,615 passed, exactly the five documented baseline failures.

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
**2026-09-09 — M4 batch 2.** Reconciled stale fixed rows for #286/#298/#300/#312/#319, confirmed
#326 was covered by existing function-range validation, added `has_parent` (#280), and added the
adopted `annotations_of_with_type` API (#185). Documented precise skips for #120/#146/#150/#182/
#200/#254/#329/#334. See `docs/reflection-audit/codex-m4-batch2-report.md` for details.
**2026-09-09 — definitive final gate.** Cleaned all Clang test `Output` directories, rebuilt
current-HEAD Clang and stale test consumers, and ran the complete `clang/test` suite. Found only
the five established baseline failures listed above; no reflection-session regression. See
`docs/reflection-audit/codex-final-gate-report.md`.

**2026-09-09 — P3293R3 piece 1.** Added header-only `subobjects_of`, composing accessible bases
before accessible nonstatic data members. Added a compile-time regression test covering order and
equivalence to manual concatenation. P3293R3 base-subobject splice syntax remains deferred to
piece 2; see the following entry.
**2026-09-09 — P3293R3 piece 2.** Implemented `obj.[:base:]` for direct non-virtual base
relationships. The splice builds a checked `CK_DerivedToBase` cast using the reflected base's
path, preserving cv-qualification and lvalue/xvalue category. Virtual bases and array elements
are rejected with dedicated diagnostics; non-base reflections retain the existing rejection.
Focused libc++ verification passed. Full Clang gate is in progress; record its final baseline
result below before committing. Definitive full-tree run after cleaning generated outputs: 44,612
passes, 25 expected failures, 5,171 unsupported tests, and 8 failures. Five are the established
baseline tests (`PR98671.cpp`, `builtin-is-within-lifetime.cpp`, `constant-expression-cxx11.cpp`,
`cxx2a-constexpr-dynalloc.cpp`, and `cxx2b-consteval-propagate.cpp`); the other three were stale
`clang-repl`/`clang-check` PCH consumers and passed after those binaries were rebuilt. Final
targeted rerun of those three plus all 16 Clang reflection tests passed 21/21.

**2026-09-09 — P3795R2 piece 3.** Added `data_member_options.annotations`, ABI threading,
constant validation, generated-member annotation attributes, and focused coverage. Commit
`47e5bba54f7e`; focused libc++ test passed 1/1. A serial current-HEAD full gate was started with
the Python `fork` workaround required by this sandbox and reproduced the five established
baseline failures first, but was interrupted after negligible progress because the 23,495-test
`-j1` sweep was taking tens of seconds per test. No reflection failure appeared before stopping;
the current-HEAD definitive gate remains outstanding. Combined evidence is in
`docs/reflection-audit/codex-p3795r2-piece3-and-gate-report.md`.

**2026-09-09 — ASTUnit/libclang PCH audit.** Confirmed reported PCH failure is a pre-existing
consumer-path bug, not reflection or stale binaries. The compiler's `-include-pch` accepts the
same file; `c-index-test -module-file` fails during `readASTFileControlBlock`, whose underlying
error is discarded before ASTUnit emits its generic diagnostic. Corrected this tracker’s
staleness warning and added the bug as Known Bug 4. Full details are in
`docs/reflection-audit/codex-pch-bug-report.md`.

**2026-09-09 — M4 hard-issue reattempt.** Re-traced deferred issues #150, #200, #334, and
#146 using the P3293 base-splice and P3795 scope-lookup implementations as precedent. #150
remains deferred: `~[:info:]` needs a splice-bearing destructor-name representation and new
parser/Sema formation. #200 is already correct in this checkout; `parent_of(^^T::A) == ^^T`
passes because `findTypeDecl` preserves the top-level alias layer. Fixed #334 by preventing a
discarded direct object reference from a non-arrow static member call from entering Sema's
consteval-only set. Fixed #146 with parser diagnostics for `case` and `default` labels inside
expansion statements; parser rejection makes the existing raw-label CodeGen path unreachable.
Focused reflection tests passed 4/4. The validation baseline for this session is 23 known
failures (five consteval-escalation tests plus 18 ASTUnit/libclang PCH consumer failures), not
five. Full audit and effort estimates: `docs/reflection-audit/codex-m4-hard-report.md`.

**Verification note (2026-09-09, later): `check-clang :: Reflection` confirmed clean (20/20)**
directly. `check-clang :: SemaCXX` (the suite most relevant to #334's `MarkMemberReferenced`
change, 1407 tests) could **not** be run to completion — repeated OOM kills within the first
5-25 tests across 3 separate attempts, all at `-j1`, on this session's memory-constrained shared
desktop. **Partial evidence gathered across those 3 attempts shows zero unexpected failures** —
only the two already-known baseline tests (`cxx2a-constexpr-dynalloc.cpp`,
`cxx2b-consteval-propagate.cpp`) appeared, each time. Combined with Codex's own focused-test pass
(4/4) at commit time, this is treated as reasonably-verified but **not exhaustively gate-confirmed
for the full SemaCXX suite** — flag this explicitly if #334's fix is ever suspected of causing a
downstream issue; a genuine uninterrupted `SemaCXX` run is still owed.

**2026-09-09 — M4 batch 6, issue #169.** Reproduced the assertion in
`CheckIfAnyEnclosingLambdasMustCaptureAnyPotentialCaptures` with the exact NBTV source. The
expansion-statement parser can leave `CurrentLSI` on the function-scope stack after its call
operator is no longer `CurContext`; the existing debug assertion fired before the function's
capture walk could decline the invalid context. Added a guard for that state and a no-crash
regression test. Direct rebuilt-compiler probe passes after rebuilding Clang; lit is unavailable in
this sandbox because its Python worker setup requires a forbidden `forkserver` operation.

**2026-09-09 — M4 batch 6, issue #346.** Reproduced the spurious `-Wreflexing-parse` warning for
`^^decltype(std::move(1))`. The warning path used the semantic reference type and the following
token, which incorrectly treated a reference nested inside `decltype` as binding to `^^`. It now
checks the raw token at the parsed type-id's end location before warning. The exact probe is clean,
while the direct ambiguous `^^int&& != ^^int` case still warns.

**2026-09-09 — P3560R2 strategy (2) pilot investigation.** Confirmed the design's pilot is
`members_of`, but stopped before source changes after finding two implementation blockers: the
existing pending-exception reference-catch path casts every throw site to `CXXThrowExpr`, while
the proposed metafunction/synthetic throw site is not one; and the proposed `ThrowFn` has no
evaluator API for constexpr construction of `meta::exception` from a `StringRef` and `APValue`.
Recorded required private evaluator-interface changes in
`docs/reflection-audit/codex-strategy2-pilot-report.md`; committed in the documentation commit.

**2026-09-09 — P3560R2 strategy (2) redesign attempt.** A narrowly scoped implementation
was reverted after the focused pilot reached the synthesized `std::meta::exception` constructor
and aborted in `extractSubobject` through recursive inherited-constructor evaluation. The
core pending-object-key and callback design remains unimplemented; see
`codex-strategy2-redesign-report.md` for the exact coredump evidence. No risky change remains
in shared exception evaluation.

**2026-09-09 — Needs-Build-To-Verify audit.** Rebuilt and ran 21 of the 27 M1 issue probes against
current HEAD, reclassifying 12 as Already-Fixed and nine as Confirmed-Open; six remain
Needs-Build-To-Verify because their reports provide only an inaccessible Godbolt case, an
unavailable attachment, or an external clangd project. Exact open cases were added to the M4
backlog. Clang Reflection passed 20/20 and the two focused libc++ reflection tests passed 2/2.

**2026-09-09 — M1 upstream PR triage complete.** Fetched and reviewed descriptions and diffs for
the 11 formerly unassessed PRs (#340, #345, #279, #261, #249, #244, #207, #168, #166, #135, and
#124), and re-confirmed the existing out-of-scope dispositions for P3816 (#227/#195/#170) and
P3074 (#163). The table now records definitive issue mappings, ready-made fixes for future M4
ports, paper-gap items, and low-priority/out-of-scope content. No implementation changes were
made in this triage session.

**2026-09-09 — M4 batch 5.** Ported PR #166 (`is_reflection_type` alias/dealias correction), PR
#340 (`has_c_language_linkage`), and PR #244 (unique closure operator-template reflection),
closing issue #239. Focused libc++ tests passed 2/2 and Clang Reflection passed 20/20. The full
Clang gate reported the documented 23 baseline failures plus three Python 3.14 forkserver helper
test failures; no reflection failure appeared. Commit `6bbb1c0cfbea` was pushed to
`origin/cxx26`. The remaining Confirmed-Open backlog is 9 items; full details are in
`docs/reflection-audit/codex-m4-batch5-report.md`.

**2026-09-10 — M5 batch 3.** Added and ran
`m5-p2996-batch3.verify.cpp`, covering P2996R13 rows 2996-11 through 2996-15. The focused
libc++ lit test passed 1/1 with all `-verify` diagnostics matched. Rows 2996-09 and 2996-10
remain open because the current display-string/source-location implementation returns fallback
or empty results for null reflections instead of diagnosing. Checklist totals are now 23 covered,
68 needing new tests, and 18 blocked. See
`docs/reflection-audit/codex-m5-batch3-report.md`.
