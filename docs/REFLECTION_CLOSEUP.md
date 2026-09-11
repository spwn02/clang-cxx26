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

**Closeout update (2026-09-11): all 14 items now have a final disposition.** Item 2 is fully
fixed and verified: its landed primary splice fix remains intact, and the former `is_type_alias`
loose end is now fixed. Items 1, 5a, and 9 remain escalated to the user; the remaining items are
fixed/verified, conclusively unreproducible, or (for #275 within item 14) escalated with evidence.
**Reflection Closeup is ready for CU5**, the final assertions-enabled `check-clang`/`check-cxx`
gate. The following superseded CU4 progress snapshot remains for historical context.

**Superseded CU4 progress snapshot:** Item 1 is escalated to the user, not being worked
automatically (see below). Item 2 has a partial fix landed, one sub-symptom (`is_type_alias`
identity loss) still open and deliberately parked. **Items 3, 4, 5b, 6, and 7 are now
fixed/verified** (commits `9760450c0fe4`, `8081ce09739d`, `08999b0aea1e`, `acd49b881802`, and
`cbba49e3c2d9` — see their paragraphs near the end of this section and their table rows below;
item 6 needed no compiler code change, only a regression test confirming it's already fixed). **Item 5a (issue #180) remains escalated to the user alongside item 1** — it was refuted
as an expansion-statement bug during investigation and needs the same class of Sema
instantiation-context work as item 1, not a narrow fix; see its own table row and
`docs/reflection-audit/issue-180-minimal-repro.cpp`. **Item 8 is now fixed and verified too**
(header-only, `libcxx/include/meta` — see its table row for the full design and the dated session-log
entry below). **Item 9 is escalated, not closed:** independent re-audit reconfirmed the missing
evaluator construction API and synthesized inherited-constructor abort; see
`docs/reflection-audit/item9-strategy2-stop-report.md`. Do not convert only a subset. **Item 10 is
now fixed and verified**: its blocker was a general P3068 constant-evaluator cleanup-registration
bug, not reflection plumbing (Terra's fix independently re-verified against a fresh full
`ninja -C build-nyx` rebuild + clean `build-libcxx` rebuild by the calling session before trusting
it — see the dated session-log entry below). **Also fixed, off the 14-item list but discovered
mid-epic**: a stale `"unimplemented": True` flag for `__cpp_lib_stacktrace` in
`generate_feature_test_macro_components.py`, left over from the earlier emergency `<stacktrace>`
port, was causing the *packaged reference-toolchain* preflight CI to fail (`std.compat.cppm`'s
generated "please update headers_not_available" guard trips as soon as `<stacktrace>` is genuinely
includable) — see the dated session-log entry below. **Items 11, 12, and 13 are now fixed and
verified** (uncommitted; see the dated session-log entry below). CU4 item 14 fixed #187/#212,
confirmed #184/#208 already fixed, and closed #253 as unreproducible; #275 is escalated after two
sequential clangd crashes. See `docs/reflection-audit/cu4-needs-reproducer-report.md`.

**Two new, unrelated findings surfaced while closing item 5b (not part of this epic's 14-item
scope, not fixed, logged here so a future session doesn't have to rediscover them):**
1. `tryMakeCXXIterableExpansionSelectExpr`'s hidden `__range` (`SemaExpand.cpp`) gathers its
   `begin()`/`end()` overload candidate set against `Range->getType()` (the *original*, non-const
   type) but the call site is later built against the const-qualified hidden variable — so a range
   type whose `begin()` is *only* overloaded non-const (no `const`-qualified overload at all, e.g.
   a bare `std::array<int,3>{...}` prvalue) fails to bind for `auto&&` with "'this' argument ...
   has type const std::array<int, 3>, but function is not marked const". Confirmed present on the
   unmodified baseline (predates this epic entirely) via direct isolation
   (`template for (auto&& elem : std::array<int,3>{1,2,3})` inside a `consteval` function). Not
   fixed — out of scope for item 5b (which is about copy-constructibility, not `begin()`
   const-overload resolution).
2. A `template for (constexpr auto ... : ...)` iterable-path loop, when the *entire enclosing
   function* is itself evaluated via `static_assert`/consteval constant-expression evaluation
   (not merely runtime code with a `constexpr` loop variable, the shape every existing passing
   test in this area uses), was observed to crash inside `TemplateDeclInstantiator::VisitVarDecl`
   / `Sema::SubstType` **exactly once**, then did not reproduce on ~4 subsequent identical
   attempts (isolated repro, the original combined test file, and a fresh lit run all passed
   cleanly afterward) — most likely a transient artifact from a concurrent session that was
   independently observed touching this same working tree during this session (stray
   `.git/index.lock` collisions from another active `git` process), not a real, reproducible
   compiler bug. Flagged here rather than silently ignored in case it recurs; if a future session
   hits this shape crashing reproducibly, this note is the starting context.

**Item 1 (escalation cluster): FIVE attempts now, all rejected empirically. Genuinely resists a
full fix — this is the completion bar's escalation case, not a "keep trying" case.** Per the plan's
own Completion Bar section: an item that resists a full fix even after real, careful effort gets
escalated back to the user rather than closed under a lesser bar or endlessly re-attempted. Attempt
5 (this session, Claude working directly, zero Codex usage) got substantially closer than any prior
attempt — see the dated entries below and the full comment at
`clang/lib/Sema/SemaExpr.cpp:18396` (search "KNOWN BUG") for the complete 5-attempt history — but
still hit a genuinely different, deeper problem than diagnostic-suppression logic: a real
heap-lifetime bug from evaluating a nested immediate invocation twice across two separate
`ExpressionEvaluationContextRecord` instances (once during the invocation's own template
instantiation, once again as an eagerly-evaluated top-level candidate in the outer variable's
record) for complex range-pipeline expressions. Confirmed via direct isolation this is NOT
diagnostic-suppression noise like every prior symptom — it's `EvaluateAsConstantExpr` genuinely
producing a different, wrong lifetime outcome on the second, redundant evaluation.

**What attempt 5 actually fixed, precisely and confirmed, before hitting the new blocker:**
1. **Corrected a wrong assumption from attempt 2** (which every subsequent attempt, including
   Astra's, built on): the "manifestly constant-evaluated" bailout in `HandleImmediateInvocations`
   is NOT universally dead code. `VarDecl::hasConstantInitialization()` genuinely depends on
   `VarDecl::getEvaluatedStmt()`, which *can* already be populated for a simple, non-dependent
   global variable's initializer by the time this function runs — confirmed empirically via
   temporary tracing. Attempt 2's specific failing test cases (self-referential locals) just
   happened to be ones where it's still unpopulated at this point; that doesn't generalize.
2. **The minimal fix that follows from correction #1**: revert ONLY the push in
   `ActOnCXXEnterDeclInitializer`/`SemaTemplateInstantiateDecl.cpp` (matching the original August
   attempt 1) and leave `HandleImmediateInvocations` completely untouched — no new fields, no
   relocated bailout, no nested-context merging. This alone correctly fixed both in-scope
   self-reference tests, kept the ordinary P2996 idiom working for simple declarations, and kept
   `clang/test/Reflection/` at 20/20 including the smuggling test.
3. **One more real bug found and fixed by direct isolation**: a `DeclRefExpr` to an
   already-invalid declaration (referencing an entity whose own initializer already failed
   elsewhere, e.g. through a using-declarator error) was getting a redundant, spurious
   `err_expr_consteval_only_type` piled on top of its real, already-correct error.
   `Expr::containsErrors()` does not catch this (the reference itself is well-formed); checking
   `cast<DeclRefExpr>(E)->getDecl()->isInvalidDecl()` does, and fixed
   `clang/test/Reflection/reflection-wording-examples.cpp` cleanly with no other regression in the
   full `clang/test/Reflection/` suite.

**The new blocker (not fixed):** with only fixes #2 and #3 applied, the full libc++ reflection
suite regresses from 32 failures down to... **still regresses**, on a *different* subset than
Astra's attempt — complex range-view-pipeline expressions like
`(members_of(...) | views::filter(...)).front()` now fail with "read of heap allocated object that
has been deleted." Root-caused precisely (see the source comment): `front()` becomes
immediate-escalating because its body takes the address of the immediate `operator*`; instantiating
`front()` happens in its own nested evaluation-context record, separate from the outer variable's
record, so `RemoveNestedImmediateInvocation`'s existing same-record deduplication never recognizes
`front()`'s call in the outer record as "the same candidate" already handled — it gets evaluated a
second, fully independent time, and that second evaluation has different (wrong) heap-lifetime
behavior than the one atomic whole-expression evaluation the ordinary constexpr-initializer check
performs (confirmed: the exact same expression works fine on the unmodified baseline). This needs
either extending nested-candidate deduplication across sibling/child records, or avoiding the
redundant eager evaluation for a candidate that will be evaluated anyway by the ordinary check —
real evaluator-architecture work, not a targeted patch.

**Decision: stop attempting this item for now, escalate to the user per the completion bar.** Five
independent attempts (August original, this epic's static-analysis pass, Astra, and two rounds this
session) have each found a genuinely different failure mode after fixing the previous one — the
problem keeps getting narrower and better-understood (this round nailed it down to one precise,
well-diagnosed evaluator bug) but a full fix has not materialized. Continuing to iterate
indefinitely without new tooling (i.e., without either a fresh evaluator-architecture design pass
or accepting the user's original "Terra if absolutely necessary" allowance) risks the same
diminishing-returns pattern. **Not resuming this item automatically — waiting for user direction**
per the plan's explicit instruction not to choose unilaterally between "keep trying" and "accept a
lesser bar" when an item resists a full fix. Moving on to item 2 in the meantime (a different,
independent item — this is not "stopping the epic," just this one item).

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

**Item 2 (closure-type alias, `#237`): initial partial-fix record (superseded by the final
2026-09-11 entry below).** Worked entirely directly (no Codex usage). Root-caused and fixed the primary,
hard-error symptom precisely: `decltype(auto-declared-var)` retains `AutoType` sugar that survived
into `Sema::BuildReflectionSpliceType`'s reflected-operand resolution, tripping the ordinary
"auto not allowed in type alias" check when reconstructing the splice as a type-alias target.
Fixed with a targeted desugar, mirroring the function's existing `UsingType`/
`SubstTemplateTypeParmType` handling. Verified zero regressions across `clang/test/Reflection/`
(20/20), the libcxx reflection 7-test baseline (unchanged), and the broader SemaCXX/AST/
CodeGenCXX/SemaTemplate suites (3411 tests, exactly the documented 5-test baseline) — **this is a
genuinely safe, non-regressing fix, unlike item 1's reverted attempts, so it was committed even
though item 2 as a whole isn't fully done yet.**

**What's still open**: the original issue's own probe also asserts `is_type_alias(^^ct)` — this
still evaluates to false even once `ct` declares successfully. Traced exhaustively without finding
the root cause:
- Confirmed via `-ast-dump` that `ct`'s own `TypeAliasDecl` is correctly formed
  (`TypeAliasDecl ... 'const (lambda at ...)'` wrapping a `ReflectionSpliceType` sugar node) —
  the alias declaration itself is fine.
- Confirmed via temporary tracing that `BuildCXXReflectExpr(SourceLocation, SourceLocation,
  QualType)` — the function that constructs the `CXXReflectExpr` for `^^ct` — receives and stores
  a correct `TypedefType` (`T=ct`), untouched, with no accidental desugaring.
- Confirmed `ReflectionEvaluator::VisitCXXReflectExpr` (`ExprConstant.cpp`) is a trivial
  passthrough (`APValue Result(E->getReflection()); return Success(Result, E);`) and
  `APValue::getReflectedType()` is a trivial `QualType::getFromOpaquePtr` round-trip — neither
  desugars anything.
- Confirmed the metafunction `Evaluator` callback (`ExprConstant.cpp`'s
  `VisitCXXMetafunctionExpr`) also just calls the same general `::Evaluate()` path — no special
  handling.
- **Ruled out the library-wrapper parameter-binding theory**: calling
  `__metafunction(detail::__metafn_is_alias, ^^ct)` directly (bypassing `is_type_alias`'s consteval
  wrapper function entirely, so there's no intermediate `info`-typed parameter copy to suspect)
  produces the exact same wrong result (`QT` observed as a bare `RecordType`, not `TypedefType`).
- Confirmed via `-ast-dump` inside the exact failing test file (not just a simpler isolated one)
  that `ct`'s declaration is still correctly formed even in the context where the metafunction
  call fails — ruling out "something later in the same TU retroactively corrupts `ct`'s cached
  type."
- Confirmed this is closure-specific, not about being inside a `consteval {}` block: an ordinary
  (non-closure) type alias declared and reflected inside the identical `consteval {}` block
  structure works correctly (`is_type_alias` returns true).

Every layer between `^^ct`'s construction and the metafunction's observation of its value has been
individually verified correct in isolation, yet the end-to-end value is wrong — meaning the loss
happens in a layer not yet identified (or in an interaction between layers each individually
correct). Given item 1's lesson about diminishing returns on a single sub-symptom, **not
continuing this specific thread further right now** — moving to item 3, will return to this with
fresh eyes (or escalate per the completion bar) rather than keep excavating linearly.

**Item 3 (`display_string_of(dealias(...))` not constant expr, `#188`): fixed and verified
(commit `9760450c0fe4`).** Worked entirely directly (no Codex usage). Root-caused precisely via
temporary `Type*`-identity and type-class tracing (added and fully reverted before commit, matching
the discipline used for items 1 and 2): `dealias()`'s `desugarType()` hand-rolled sugar-strip loop
was missing `DecltypeType`. `iterator_t<R>`'s underlying alias-template type
(`decltype(ranges::begin(declval<R&>()))`) desugars one step to a `DecltypeType` node whose own
*canonical* type is perfectly ordinary (confirmed by forcing clang's native, non-reflection type
printer to reveal it via an unrelated overload-resolution error: the true type is the shallow,
unremarkable `__wrap_iter<char *>`), but the loop had no branch matching `DecltypeType` and gave up
right there. The still-sugared reflection then fed the printer's recursive
`render_template_argument_list_of` walk, whose `template_arguments_of()` query on this specific
`DecltypeType` resolved back to an equivalent, still-unresolved reflection every single time —
confirmed via a `QualType::getAsOpaquePtr()` trace that the *exact same* `Type*` pointer recurred
across 380+ probed calls. This is a literal non-terminating recursion, not legitimate deep template
nesting (ruled out early: `-fconstexpr-steps=100000000` didn't help, since it's a call-*depth*
limit, not a step-count one; `-fconstexpr-depth=4096` crashed the compiler outright rather than
just taking longer, which was the first strong signal this wasn't ordinary deep nesting).

Fix: add `DecltypeType` to `desugarType`'s unconditional strip set, alongside the pre-existing
`AutoType`/`SubstTemplateTypeParmType`/`ReflectionSpliceType` handling — these are all structural
sugar produced while *resolving* an alias's underlying type, not a named alias the user wrote, so
(like the others already there) always stripped rather than gated behind the `UnwrapAliases` flag.
Also fixed an unrelated dead-code bug found en route in the same loop: the `UsingType` branch's
condition tested `TDT` (guaranteed null there, left over from the preceding `if`'s scope) instead
of `UT`, so `UsingType` sugar was never actually unwrapped despite the branch's evident intent —
real bug, but not the cause of this particular issue (confirmed: fixing it alone, without the
`DecltypeType` fix, left the recursion unchanged).

Considered and rejected a broader rewrite (a generic `getSingleStepDesugaredType`-to-fixed-point
loop, which would auto-handle every current and future sugar kind uniformly) as higher-risk than
warranted here without concrete evidence of other missing cases — the two confirmed, narrowly-
targeted fixes above are what's landed. Verified zero regressions: `clang/test/Reflection` 20/20,
libcxx reflection suite at the documented 7-test pre-existing baseline (identical test names,
confirmed byte-for-byte against `AGENTS.md`'s snapshot), `SemaCXX` at the documented 5-test
pre-existing baseline (item 1's escalation cluster, unaffected). New regression test:
`libcxx/test/std/experimental/reflection/issue-188-dealias-decltype.pass.cpp` (covers both the
non-termination symptom and idempotency of a repeated `dealias()` round-trip).

**Item 4 (NEW-7 — dependent splice-specifier wrongly accepted in CTAD-like position): fixed and
verified (commit `8081ce09739d`).** Worked entirely directly (no Codex usage). This is the item
with the most prior failed attempts after item 1 (3 rounds, per
`docs/reflection-audit/codex-new7-design-report.md`), and every one of them was solving the wrong
layer: each tried to *recover* "was `typename` written" via an unreliable proxy (a raw source-line
text scan; `Lexer::findPreviousToken`) rather than asking why the type system's own tracked signal
for this — `ReflectionSpliceType::getTypenameKWLoc()` — wasn't trustworthy in the first place. It
wasn't, for three independent, compounding reasons, found via direct `Type*`/location tracing
(temporary, fully reverted before this commit, matching the discipline used for items 1-3):

1. **`SemaType.cpp`'s plain (no-`typename`) `TST_type_splice` case reused the splice's own source
   location as `TypenameKWLoc`** instead of an invalid location — so every *implicit* splice looked
   exactly like one that had spelled out `typename`.
2. **The opposite failure, for the explicit case**: a `typename [:R:]` splice reached via
   `Parser::TryAnnotateTypeOrScopeToken`'s `typename`-keyword handling routes through
   `ParseOptionalCXXScopeSpecifier`'s internal splice-to-`annot_typename` rewrite (used to decide
   "is this a splice-scope-specifier or just a type"), which **hard-coded `SourceLocation()`**
   for the keyword location instead of threading through the one its caller had just consumed —
   silently discarding a genuine `typename` occurrence.
3. **The one that actually explains "works in isolation, breaks with multiple dependent-splice
   templates in one TU"** — attempt 3's exact failure signature, hit here via a completely
   different mechanism than any prior attempt suspected: even after fixing (1) and (2),
   `ASTContext::getReflectionSpliceType`'s dependent-type uniquing
   (`DependentReflectionSpliceType::Profile`, the `FoldingSet` cache key) folded in only the
   splice's operand expression and template arguments — never `TypenameKWLoc`. Two *structurally
   identical* dependent splices (e.g. the same depth-0/index-0 `info` non-type template parameter,
   used by two unrelated function templates elsewhere in the same file), one written with
   `typename` and one without, therefore produced identical folding-set keys and collapsed onto
   whichever type node happened to be built first — silently corrupting the *other* declaration's
   `TypenameKWLoc` regardless of what it actually wrote.

Fixed all three: `SemaType.cpp` passes an invalid location for the implicit case; a new
default-invalid `TypenameKWLoc` parameter on `ParseOptionalCXXScopeSpecifier` (every other call
site unaffected) threads the real location through for the explicit case;
`DependentReflectionSpliceType::Profile` gained a `HasTypenameKW` boolean in its folding-set key
(deliberately *not* the raw `SourceLocation`, to avoid over-fragmenting type identity by source
position — only the presence/absence of the keyword is semantically load-bearing for this
diagnostic). With all three fixed, `getTypenameKWLoc().isInvalid()` alone is now a fully reliable
"no explicit `typename`" signal — no text-scan or lexer heuristic needed, unlike every prior
attempt. Implemented the actual NEW-7 diagnostic (`err_dependent_splice_ctad`) in
`Sema::AddInitializerToDecl` per the original design's scope decision: dependent splice, no
explicit `typename`, copy-list-initialization only (direct-list-init, parenthesized-init, and
plain non-list copy-init are all left alone, as is any splice with explicit `typename`). New test:
`clang/test/Reflection/new7-dependent-splice-ctad.verify.cpp` (covers the forbidden form plus all
four positive controls plus a non-dependent-splice control). Verified zero regressions:
clang/test/Reflection 21/21, the broader Parser/SemaCXX/SemaTemplate/AST/CodeGenCXX suites
(3814 tests) at the documented 5-test pre-existing baseline, libcxx reflection suite at the
documented 7-test pre-existing baseline.

## The 14 items

| # | Item | Status | Notes |
|---|---|---|---|
| 1 | Consteval self-reference escalation cluster | **ESCALATED TO USER — 5 attempts, genuinely resists a full fix** | Attempt 5 (Claude, direct, no Codex) got furthest: fixed 2 real bugs (attempt 2's "dead bailout" was an overgeneralization; a `DeclRefExpr`-to-invalid-decl false positive) but hit a new, different-in-kind evaluator bug — a nested immediate invocation gets evaluated twice across separate `ExpressionEvaluationContextRecord`s with different (wrong) heap-lifetime outcomes for range-pipeline expressions. Reverted, not committed. Full history: `clang/lib/Sema/SemaExpr.cpp:18396`'s comment (5 attempts) and the dated entries below. Also confirmed: only 2 of the originally-named "5 SemaCXX tests" are this bug; `PR98671.cpp` is an unrelated pre-existing C++20 concepts crash, `cxx2a-constexpr-dynalloc.cpp`/`cxx2b-consteval-propagate.cpp` are a different consteval-escalation bug via a different code path. Per the completion bar: not resuming automatically, waiting for user direction. |
| 2 | Issue #237 — closure-type alias loses identity | **Fully fixed and verified (primary fix `a9c1f8aad1d6`; identity fix uncommitted)** | The remaining `is_type_alias(^^ct)` failure was `APValue::setReflection` normalization, not metafunction argument evaluation: its `unwrapReflectedType` stripped a `TypedefType` whenever `QualType` was cv-qualified. `ct`'s target is intrinsically const (because the closure object is `constexpr`), so this erased its alias identity before evaluation. Preserve aliases whose declared underlying type carries that cv; continue desugaring only aliases with externally applied cv, retaining the historical `^^const Alias == ^^const T` behavior. `isTypedefNameType()` itself is ordinary and has no lambda special case. Regression now checks both `is_type_alias` and `has_identifier`. Verified: full clang rebuild; Reflection 22/22; Parser/SemaCXX/SemaTemplate/AST/CodeGenCXX 3814 tests at exactly the 5-test baseline; clean libc++ rebuild; reflection 119 tests with exactly the documented 7-test baseline. |
| 3 | Issue #188 — `display_string_of(dealias(...))` not constant expr | **Fixed and verified (commit `9760450c0fe4`)** | Root cause: `dealias()`'s `desugarType()` hand-rolled sugar-strip loop was missing `DecltypeType` — an alias template's underlying type (e.g. `iterator_t<R> = decltype(ranges::begin(declval<R&>()))`) desugars one step to a `DecltypeType` whose *canonical* type is ordinary but which the loop couldn't unwrap further, leaving a still-sugared reflection whose own template-argument query resolved back to an equivalent unresolved reflection every time — a literal non-terminating recursion (confirmed via `Type*` identity tracing: same pointer recurred 380+ times), not legitimate deep nesting, eventually exhausting the constexpr call-depth budget with a generic, cause-free diagnostic. Fixed by adding `DecltypeType` to the loop's unconditional strip set (alongside pre-existing `AutoType`/`SubstTemplateTypeParmType`/`ReflectionSpliceType`). Also fixed an unrelated dead-code bug found en route in the same loop: the `UsingType` branch tested the wrong local (`TDT` instead of `UT`), so `UsingType` sugar was never actually unwrapped. Verified zero regressions: clang/test/Reflection 20/20, libcxx reflection suite at the documented 7-test pre-existing baseline (byte-for-byte same tests), SemaCXX at the documented 5-test pre-existing baseline. New regression test: `libcxx/test/std/experimental/reflection/issue-188-dealias-decltype.pass.cpp`. |
| 4 | NEW-7 — dependent splice-specifier wrongly accepted (CTAD-like position) | **Fixed and verified (commit `8081ce09739d`)** | Root cause: THREE independent, compounding bugs in `ReflectionSpliceType::getTypenameKWLoc()`'s tracking, not the diagnostic logic itself (which every prior attempt was trying to work around via unreliable heuristics instead). (1) `SemaType.cpp`'s plain (no-`typename`) `TST_type_splice` case reused the splice's own location as `TypenameKWLoc`, making implicit splices look explicit. (2) `typename [:R:]` reached via `TryAnnotateTypeOrScopeToken` goes through `ParseOptionalCXXScopeSpecifier`'s splice-rewrite, which hard-coded `SourceLocation()` instead of threading the real keyword location through — the opposite failure, discarding an explicit `typename`. (3) Even after fixing both, `DependentReflectionSpliceType::Profile` (the FoldingSet uniquing key for dependent splice types) never included `TypenameKWLoc`, so two structurally-identical dependent splices (e.g. the same depth/index template parameter in two different function templates), one with `typename` and one without, collapsed onto the same cached type node — exactly the "breaks once multiple dependent-splice templates coexist in one TU" signature all 3 prior attempts hit, via a completely different mechanism than any suspected. Fixed all three; added a `HasTypenameKW` bit to the Profile (not the raw location, to avoid over-fragmenting type identity by source position). New test: `clang/test/Reflection/new7-dependent-splice-ctad.verify.cpp`. Verified zero regressions: clang/test/Reflection 21/21, Parser+SemaCXX+SemaTemplate+AST+CodeGenCXX 3814/3814 at the documented 5-test baseline, libcxx reflection suite at the documented 7-test baseline. |
| 5a | Issue #180 — `static_assert(false)` silently ignored | **ESCALATED TO USER — not an expansion-statement bug, mis-scoped from the start** | The plan bundled this with #181 under upstream PR #261's expansion-body-deferral design, inherited from `docs/reflection-audit/codex-m4-180-181-report.md`'s investigation. That premise is refuted: the upstream repro contains no `template for` anywhere, and a minimal reproducer confirms the failure has nothing to do with expansion statements. See `docs/reflection-audit/issue-180-minimal-repro.cpp` (fails silently) and its companion `docs/reflection-audit/issue-180-control-diagnoses-correctly.cpp` (diagnoses correctly) — narrowed down from the full upstream repro to a single discriminating factor: whether the *enclosing* function calling `substitute()`+`extract()` is itself a function template. `Sema::EnsureInstantiated` (`SemaReflect.cpp:328-336`) does call `S.InstantiateFunctionDefinition(..., true, true)` for a function-template-specialization reflection, and this correctly triggers the `static_assert` diagnostic when called from an *ordinary* (non-template) consteval function — but the identical call, made while Sema's instantiation-context stack already has a frame for the *enclosing* function template's own instantiation, silently produces no diagnostic. Four candidate mechanisms considered (SFINAE-context misfire, instantiation-depth deferral, evaluator-side diagnostic suppression, an overly-eager mutual-recursion instantiation guard) — none confirmed; distinguishing them needs a targeted Sema instantiation-context trace, the same class of investigation as item 1's escalation cluster. Per the completion bar, escalating alongside item 1 rather than guessing further. **PR #261's port is NOT justified by #180 as currently understood — do not let a future session inherit that premise; if the port is ever needed, it needs its own independent justification.** |
| 5b | Issue #181 — non-copyable tuple/range element expansion binding | **Fixed and verified (commit `08999b0aea1e`)** | Three independent, compounding bugs in `SemaExpand.cpp`, found via direct instrumentation (not the guessed-at P1306R5 `std::move`-selection design from the original plan, which turned out not to be the actual mechanism at all): (1) `tryMakeCXXIterableExpansionSelectExpr`'s hidden `__range` unconditionally copy-constructed (`Range->getType().withConst()`) *before* the function had even determined whether the type is iterable, so a non-copyable range (e.g. `std::tuple<std::unique_ptr<int>, ...>`) hard-errored regardless of binding form, before ever reaching the (already-correct) destructurable path. (2) `makeCXXDestructurableExpansionSelectExpr` hardcoded `SpelledAsLValue=true` when building the hidden binding's reference type, indistinguishable from `auto&`, so `auto&&` over a genuine prvalue range failed to bind. (3) Neither path completed the range's type before a `begin()`/`end()` member lookup on it (unlike ordinary range-based for, which does), asserting in an assertions-enabled build when the type is reached only via a reference parameter. Fixing (1) by unconditionally switching to a forwarding reference (mirroring ordinary range-based for's `auto&& __range`) regressed *working* constexpr cases with a perfectly copyable range type elsewhere in the libc++ reflection suite (`expansion-lambda-capture-crash.pass.cpp`, `deduction-guide-reflection-mangling.pass.cpp`) — binding a reference to a temporary inside a manifestly-constant-evaluated context needs lifetime-extension bookkeeping a by-value copy never needed. Final fix: speculatively check copy-constructibility first (a trial `InitializationSequence`, which never commits/diagnoses unlike the real `AddInitializerToDecl` call) and only fall back to the reference when the type genuinely can't be copied — preserving by-value behavior for every case that already worked. Also had to scope the earlier `RequireCompleteType` addition to record types only (a `void`-typed range from an unrelated unresolved-overloaded-function recovery path was getting a wrong, pre-empting diagnostic) and make the reference-only lifetime-extension conditional on actually using the reference path (applying it to the by-value copy path was itself found to corrupt the constant evaluator's allocation tracking). New regression test: `libcxx/test/std/experimental/reflection/expansion-noncopyable-tuple-binding.pass.cpp`, covering all four binding forms (`auto`/`auto&`/`auto&&`/`const auto&`) plus const-source and prvalue-range variants in one translation unit (per this epic's now-repeated "isolated passes, combined breaks" lesson from items 4 and 5a). Verified zero regressions: `clang/test/Reflection/` 21/21 (including the new file), libc++ reflection+debugging+stacktrace suite at the documented 7-test baseline (123 tests total, 7 pre-existing failures, zero new), SemaCXX/Parser/AST/CodeGenCXX/SemaTemplate 3814 tests at the documented 5-test escalation-cluster baseline. Two new, unrelated findings (out of scope for this item) logged in the Next Up section above. |
| 6 | Issue #182 — `template for` + `continue` ICE | **Already fixed — verified with a regression test, no code change needed** | Upstream report: `template for` crashes at codegen (`llvm::BranchInst::BranchInst`) if its body contains `continue` (bare, or via `if constexpr`), reproducing only at `-O1`+ (`-O0` doesn't reproduce). Extensive reproduction attempts against this fork — matching the exact upstream shape, across `-O0` through `-O3`, with/without `-g`, in template and non-template enclosing functions, with `continue`/`break` combined, 1–10 iterations — could not reproduce any crash. Direct inspection of `CodeGenFunction::EmitCXXExpansionStmt` (`clang/lib/CodeGen/CGStmt.cpp`) shows correct, already-sound lowering: one `JumpDest` is pre-allocated per expansion instance up front, and each instance's continue target is simply the next instance's own `JumpDest` (or the loop's exit block for the last one) — no shared, instance-independent continuation destination for a discarded branch to dangle a reference to. Likely explanation (not confirmed, since the original bug was never reproduced to bisect against): this exact function was silently dropped by a merge during this fork's LLVM 22 sync and later restored verbatim from the pre-merge fork in commit `df8b70aeaf5e` ("restore expansion-statement CodeGen..."), which landed after the upstream report (2025-09-08) — plausibly picking up a fix (LLVM-side or otherwise) incidentally along the way. New regression test locks this in: `libcxx/test/std/experimental/reflection/expansion-continue-break-codegen.pass.cpp` (compiled at `-O2 -g`, both a bare top-of-loop `continue` and the exact `if constexpr() continue` shape from the issue title, runtime-asserted for correctness, not just "doesn't crash"). Verified: libc++ reflection+debugging+stacktrace suite at the documented 7-test baseline (124 tests, zero new failures) — no compiler code change was needed, so the broader `clang/test/Reflection`/SemaCXX baselines are unaffected and weren't re-run. |
| 7 | Issue #150 — spliced explicit destructor call `~[:info:]()` | **Fixed and verified (commit `cbba49e3c2d9`)** | The upstream report's own bare-splice repro (`value.~[:^^test:]();`) is correctly rejected per [expr.prim.splice]/2.1.2 (a bare splice-expression is ill-formed when it designates a destructor) — a maintainer comment on the issue confirmed this "works as intended" and identified the real gap: [class.dtor]p16 permits a *type-name*, *decltype-specifier*, or *computed-type-specifier* after `~`, and a splice-type-specifier (`typename[:R:]`) is a computed-type-specifier per [dcl.type.splice] — so `value.~typename[:R:]()` should work, but didn't parse at all ("expected a class name after '~' to name a destructor"). Two independent parser entry points needed the fix, both in `ParseExprCXX.cpp`: `ParseUnqualifiedId`'s destructor-name case (non-dependent object type) and `ParseCXXPseudoDestructor` (object type still dependent at parse time, e.g. inside a function template) — both already had an analogous, working `decltype`-specifier case to mirror, so splice support was added the same way in each, no new AST node needed (reuses the existing `PseudoDestructorTypeStorage`/`UnqualifiedId::setDestructorName` machinery). New `Sema::getDestructorTypeForSplice` (`SemaExprCXX.cpp`) mirrors the existing `getDestructorTypeForDecltype`'s cross-check against the statically-known object type, for the same better-diagnostic reason (a mismatched splice destructor type gets a specific "destructor type ... does not match ..." error, not a generic one). New tests: `clang/test/Reflection/issue150-spliced-destructor.cpp` (parse/Sema coverage: non-dependent, dependent/template, type-mismatch-diagnosed, and confirms the bare-splice form stays correctly rejected) and `libcxx/test/std/experimental/reflection/spliced-destructor-call.pass.cpp` (proves the destructor genuinely runs at runtime, not just parses, in both contexts). Verified zero regressions: `clang/test/Reflection/` 22/22, Parser/SemaCXX/AST/CodeGenCXX/SemaTemplate 3814 tests at the documented 5-test escalation-cluster baseline, libc++ reflection+debugging+stacktrace suite at the documented 7-test baseline (125 tests, zero new failures). |
| 8 | CWG 3111 residual — nested/multi-dimensional arrays | **Fixed and verified (commit pending)** | Root cause: a row-by-row NTTP-*reference* backing design (copy each row's own `FixedArray` object into a fresh contiguous array via `{Rows...}`) looks plausible ([temp.param]p6 permits reference NTTPs to array objects) but can never work — arrays are never copy-list-initializable from another array object in C++ at all (`int a[2][3] = {row0, row1};` is exactly as ill-formed in ordinary code). Real fix: flatten all the way down to the scalar leaf type, gather every dimension's extent along the way, and reconstruct the correctly-nested array type via a small `__nd_array_shape<ValTy, Extents...>` recursive metafunction (arbitrary rank, not limited by declarator syntax) — ordinary aggregate-init brace elision then fills the nested array correctly from one flat, row-major scalar list (`FixedNDArray`). Two secondary bugs found and fixed along the way: (1) the entry `requires`-clause's `is_constructible_v<range_value_t<R>, range_reference_t<R>>` check is unconditionally false for any array-typed `range_value_t` (arrays are never "constructible" per the trait's own specification), which would silently reject every nested case at the SFINAE boundary — added an array-aware recursive `__reflect_constant_array_row_ok_v` alternative. (2) `define_static_array`'s row-pointer extraction (`extract<const ValTy*>(array)`) doesn't work for a multi-dimensional backing object (only whole-array-by-reference extraction does); fixed by extracting the whole array by reference and decaying to a row pointer manually — the outer extent for the reference type comes directly from `R` itself (a template parameter, not a runtime read), since `is_array_v<ValTy>` can only be true when `R` is itself a genuine raw C array type. `-Wmissing-braces` on the intentional flat brace-elided initializer suppressed locally (same warning ordinary `int a[2][3]={1,2,3,4,5,6};` code triggers). New/extended test: `libcxx/test/std/experimental/reflection/cwg3111-lwg4432-reflect-constant-array.pass.cpp`'s former "nested arrays remain unsupported" section replaced with real 2D/3D coverage (`reflect_constant`, `reflect_constant_array`, `define_static_array`, a structural class-type row). Verified zero regressions: `clang/test/Reflection/` 22/22, libc++ reflection suite at the documented 7-test pre-existing baseline (114 tests, zero new failures). |
| 9 | P3560R2 strategy 2 — ~20 remaining Throws-bearing metafunctions | **Escalated — no partial fix** | Independent source audit reconfirmed both evaluator-interface blockers; synthesized-constructor pilot remains blocked by the inherited-constructor abort. See `docs/reflection-audit/item9-strategy2-stop-report.md`. |
| 10 | `3560-18` — `access_context::via` catch-and-inspect coverage | **Fixed and verified (commit pending)** | Root cause was a general P3068 constexpr-exceptions evaluator bug: `EvaluateVarDecl` allocated a local's APValue and registered its block cleanup before evaluating its initializer, but retained that cleanup when initialization threw. The declaration's APValue is therefore absent (its lifetime never began), yet try-block unwinding attempted its destruction and diagnosed `note_constexpr_destroy_out_of_lifetime`. `EvalInfo::cancelCleanup` now removes the exact pending cleanup when a local initializer fails (idempotently, because some failure paths have already unwound it). This preserves normal destructor unwinding for fully constructed locals. New compiler regression: `clang/test/SemaCXX/constexpr-p3068r6-throw.cpp`; extended `libcxx/test/std/experimental/reflection/exception.pass.cpp` proves `access_context::via(^^int)` is caught as `meta::exception` and verifies `what()`, `from()`, and `where()`. Verified clang Reflection + Parser/SemaCXX/SemaTemplate/AST/CodeGenCXX: 3836 tests, only documented 5 SemaCXX baselines; libc++ reflection: 114 tests, only documented 7 failures. Full command/output record: `docs/reflection-audit/item10-3560-18-report.md`. |
| 11 | `define_static_object` entirely missing (P3491R3) | **Fixed and verified (uncommitted)** | Implemented P3491R3's class/scalar split exactly: classes return the address of the `reflect_constant` template-parameter object; non-class objects route through a one-element `define_static_array`. Regression coverage includes both structural class and scalar objects. |
| 12 | `is_string_literal` (5 overloads) missing (P3491R3) | **Fixed and verified (uncommitted)** | Added all five `std::is_string_literal` overloads plus one minimal compiler metafunction. It evaluates the pointer and recognizes a `StringLiteral` lvalue base, so literal subobjects return true and ordinary character arrays return false. Adapted the approach from upstream PR #168 to adopted `std::meta`. |
| 13 | `reflect_constant_string` narrower than spec + `reflect_constant_array` Mandates unenforced | **Fixed and verified (uncommitted)** | Replaced fixed `char`/`char8_t` overloads with the P3491R3 range template for all five character types. Literal ranges retain their supplied terminator rather than gaining a second one. Enforced structural, constructible, and copyable array-element requirements; nested-array support preserves item 8's recursive leaf checks because array row types themselves are not structural. |
| 14 | Issues #184, #187, #208, #212, #253, #275 — needs-reproducer | **Partial: #184/#208 already fixed; #187/#212 fixed; #253 CNR; #275 escalated** | #187 fixes dependent spliced member-pointer canonicality; #212 excludes non-returning consteval blocks from NRVO checking. Permanent regressions cover #184/#187/#208/#212. #253's only attachment is an incompatible preprocessed clang-21 input. #275 reproduces two sequential clangd defects, so no partial serializer-only patch was retained. Full evidence: `docs/reflection-audit/cu4-needs-reproducer-report.md`. |

## Ground truth / where to look

See the plan file's "Ground truth / where to look" section — not duplicated here to avoid drift
between two copies. Key anchors: `clang/lib/Sema/SemaExpr.cpp:18396`, `libcxx/include/meta:1602-1645`,
`clang/include/clang/AST/MetaActions.h`, `docs/reflection-audit/*.md`, `AGENTS.md`'s "Known
pre-existing baseline failures" section.

## Session Log

### 2026-09-11 — CU4 item 14: #187/#212 fixed; #184/#208 already fixed; #253 CNR; #275 escalated

Recovered all four dead Compiler Explorer links through their shortlink API,
downloaded #253's attached module repro, and fetched #275's pinned GitLab
revision. #187 was a real assertion/nontermination: a dependent splice
qualifier is its own canonical nested-name-specifier, but
`MemberPointerType::isSugared()` called every splice sugared; canonical
member-pointer construction therefore recursed into itself. Dependent splices
now remain non-sugared. #212 reached the NRVO recalculation branch with this
fork's `ConstevalBlockDecl`; such a block has no return type, so the recalculation
now only applies to function/block contexts. New libc++ regressions cover
#184/#187/#208/#212. #253's preprocessed attachment embeds an incompatible
clang-21 `<meta>` ABI and reaches ordinary interface errors before its alleged
ICE. #275 reproduces through `clangd --check`: after its preamble serialization
assert is bypassed, a distinct `BodyIndexer` recursion over reflection/function
nodes stack-overflows. Reverted the serializer-only attempt; item remains
escalated. Full command record: `docs/reflection-audit/cu4-needs-reproducer-report.md`.

### 2026-09-11 — CU3 items 11–13 (P3491R3 static storage): fixed and verified (uncommitted)

Implemented all three separable P3491R3 gaps. `define_static_object(T&&)` follows the paper's
class/non-class effects: class values use `reflect_constant` and `extract<const U&>` to return the
template-parameter object's address; scalar values use a one-element static array. Added the five
`std::is_string_literal` overloads and a single `ExprConstantMeta.cpp` metafunction that evaluates
the pointer argument and checks whether its lvalue base is a `StringLiteral` AST node. This is the
same core approach as upstream #168, adapted to this fork's adopted `std::meta` metafunction table
rather than its old `experimental/meta` interface. `reflect_constant_string` and
`define_static_string` are now range templates supporting `char`, `wchar_t`, `char8_t`, `char16_t`,
and `char32_t`; when its range is a genuine literal, it retains the existing terminator instead of
appending another. `reflect_constant_array`/`define_static_array` now enforce P3491's structural,
constructible, and copy-constructible requirements. Item 8's nested-array extension needs a
recursive structural test through its array rows, since the compiler correctly reports raw array
types themselves non-structural even when their leaf type is structural.

New `p3491-static-storage.pass.cpp` covers all five literal character types, literal subobjects vs
ordinary arrays, one-null-terminator behavior, and scalar/class static objects. Extended
`m5-p3491-p3560-p3795-batch16.verify.cpp` proves a copyable but non-structural element is rejected.
Full report and exact commands/results: `docs/reflection-audit/cu3-p3491-report.md`. Gates: full
Clang rebuild; mandatory clean libc++ rebuild; focused P3491 and nested-array tests; full Clang
Reflection; full libc++ reflection; Parser/SemaCXX/SemaTemplate/AST/CodeGenCXX (3814 tests, exactly
the documented five SemaCXX baseline failures). No commit or push.

### 2026-09-11 — Item 10 (`3560-18`): fixed and verified (uncommitted)

Root-caused the return-object-lifetime blocker to a general constexpr-exceptions cleanup bug,
not `std::meta` exception construction or item 9's missing evaluator API. `EvaluateVarDecl`
registers a block cleanup before it calls `EvaluateInPlace`; when the initializer propagates a
P3068 exception, the local remains an absent APValue and has not begun its lifetime, but the
enclosing `BlockScopeRAII` nevertheless later tries to destroy it. Added exact, idempotent cleanup
cancellation on failed local initialization. The standard non-reflection reproducer and the new
`access_context::via` catch-and-inspect coverage both pass. Gates: rebuilt clang; rebuilt libc++
with the mandatory `-t clean cxx` flow; `constexpr-p3068r6-throw.cpp` passed; the new libc++
exception test passed; full libc++ reflection remained exactly at 7 documented baseline failures
(114 tests); clang Reflection + Parser/SemaCXX/SemaTemplate/AST/CodeGenCXX ran 3836 tests with
only the documented five SemaCXX baseline failures. Exact commands/results:
`docs/reflection-audit/item10-3560-18-report.md`. No commit or push.

### 2026-09-11 — Item 9 (P3560R2 strategy 2): escalated

Re-read both prior investigation documents in full and independently confirmed the missing `ThrowFn`/arbitrary-APValue construction API and the reference-catch `cast<CXXThrowExpr>` assumption in current source. The retained redesign report documents the separate reproduced evaluator assertion through `extractSubobject` and repeated inherited-constructor frames. No valid construction path exists without either a new evaluator primitive or layout-dependent manual fabrication, so did not land a misleading partial conversion. Full details and exact commands: `docs/reflection-audit/item9-strategy2-stop-report.md`.

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

### 2026-09-10 (later still) — Item 1 attempt 5 (Claude direct, no Codex): closest yet, still not a full fix

Following the user's explicit "do not use Astra, Terra at most if absolutely necessary, prefer
doing work directly" instruction, re-approached item 1 personally rather than dispatching Codex —
re-read `HandleImmediateInvocations`/`CheckForImmediateInvocation`/`ActOnCXXEnterDeclInitializer`
from scratch, informed by (but not copying) Astra's round-1 design.

**Correction to attempt 2's finding (load-bearing for everything since):** re-verified via
temporary `getenv`-gated tracing (added to both the current tree and, briefly, to a `git stash`'d
copy of the baseline for side-by-side comparison, removed before finishing) that
`VarDecl::hasConstantInitialization()` is NOT universally dead code at the point
`HandleImmediateInvocations` runs, as attempt 2 concluded. For a minimal isolated repro
(`decltype(^^int) r3 = ^^int;` at namespace scope), it returns `true` — the "manifestly
constant-evaluated" bailout genuinely fires and correctly skips escalation diagnosis for this
ordinary case. Attempt 2's specific test cases (self-referential locals) were cases where it's
still `false` (unpopulated) at this point; that was correct for those cases but doesn't generalize
to "always dead."

**Minimal fix derived from this correction:** revert only the conditional push in
`ActOnCXXEnterDeclInitializer` (`SemaDeclCXX.cpp`) and its mirror in
`SemaTemplateInstantiateDecl.cpp` back to an unconditional `PotentiallyEvaluated` push (exactly
matching the original August attempt 1) — and leave `HandleImmediateInvocations` completely
untouched, at its original position, with its original condition. Built and tested:
- Isolated repro (`decltype(^^int) r3 = ^^int;`): now passes cleanly (previously this exact
  push-only change, tested alone via `git stash`, confirmed the isolated repro passes — this
  step alone, before any further changes, was already a meaningful improvement over attempt 1's
  wholesale approach).
- The 5 documented `SemaCXX` tests: same split as Astra's round — 2 pass
  (`builtin-is-within-lifetime.cpp`, `constant-expression-cxx11.cpp`), 3 fail
  (`PR98671.cpp` — confirmed via `git stash` isolation to crash identically on the clean baseline,
  unrelated to this bug entirely; `cxx2a-constexpr-dynalloc.cpp`/`cxx2b-consteval-propagate.cpp` —
  confirmed via `git stash` isolation to produce byte-identical `-verify` failure output with or
  without the push-revert, meaning they're missing the right diagnostic via a different code path
  this fix doesn't touch at all).
- `clang/test/Reflection/`: 2 new failures appeared beyond the documented 20/20 baseline —
  `consteval-only-types.cpp` (the smuggling test; but only the *expected*, already-documented
  `fn1`/`fn2` diagnostic-shape change from attempt 1's original finding — the test file's own
  `-verify` annotations are stale, not a real regression) and `reflection-wording-examples.cpp`
  (a genuine new regression: `constexpr info ua = U<Base>::a;` where `U<Base>::a`'s own initializer
  is already ill-formed via an unrelated using-declarator error, now gets a redundant, spurious
  `err_expr_consteval_only_type` piled onto its already-correct error).

**Fixed the `reflection-wording-examples.cpp` regression** by adding a targeted guard in
`HandleImmediateInvocations`'s `ConstevalOnly` diagnosis loop: skip an entry if it's a
`DeclRefExpr` whose referenced declaration `isInvalidDecl()`. Traced first via temporary tracing to
confirm `Expr::containsErrors()` does NOT catch this case (the reference itself is well-formed,
only the referenced decl is invalid) before picking the right guard. Verified:
`clang/test/Reflection/` back to only the one *expected* `consteval-only-types.cpp` diagnostic-shape
difference, `reflection-wording-examples.cpp` clean.

**Ran the full libc++ reflection suite with fixes #1(minimal push-revert)+#2(invalid-decl guard)
applied: 32 failures** (worse in absolute count than Astra's 23, though on a different, more
precisely-understood subset). Root-caused one representative failure
(`member-classification.pass.cpp`'s `conversion_template = (members_of(^^T, ctx) |
std::views::filter(std::meta::is_template)).front();`) down to the exact evaluator mechanism — see
the full writeup now in `clang/lib/Sema/SemaExpr.cpp:18396`'s comment (search "Attempt 5") and the
Next Up section above. Confirmed via a minimal, isolated repro
(`(members_of(^^T, ctx) | views::filter(...)).front()`, saved conceptually here, not as a
committed file) that this exact expression works fine on the unmodified baseline and fails only
with the push-revert applied — a real regression, not a pre-existing latent bug newly exposed.

**Reverted everything** (`git checkout --` on all 3 touched files, confirmed clean via `git status`
and `git diff`, rebuilt to a verified-clean 20/20 `clang/test/Reflection/` baseline) — not safe to
commit. The full attempt-5 diff (for reference, not applied) is saved at
`docs/reflection-audit/item1-attempt5-diff-not-applied.patch`. Folded the complete history
(corrections, fixes, and the precisely-diagnosed remaining blocker) into
`clang/lib/Sema/SemaExpr.cpp:18396`'s comment as a comment-only change (verified via `git diff`
containing zero non-comment lines) so a future session — Claude or Codex, this epic or a later
one — has the full picture without re-deriving any of this.

**Per the plan's Completion Bar section, escalating item 1 to the user** rather than attempting a
6th round immediately or accepting a lesser bar. Updated the table above. Moving to item 2 next.

### 2026-09-11 — Item 8 (CWG 3111 residual, nested arrays): fixed and verified

Worked entirely directly (no Codex usage). Resumed from the epic's own limitation note at
`libcxx/include/meta:1602-1645` (flat-array fix only). First design attempt (recursing one
dimension at a time, collecting a pack of *references* to each row's own `FixedArray` object as a
reference-NTTP pack, then copying `{Rows...}` into a fresh contiguous array) compiled but crashed
empirically: `int a[2][3] = {row0, row1};` is not valid C++ at all (confirmed via a minimal,
non-reflective isolated repro — arrays are never copy-list-initializable from another array
object), so no wiring of that design could ever have worked, reference-NTTP legality
notwithstanding.

Redesigned around flattening: `reflect_constant_array` now recurses down to the ultimate scalar
leaf type via `__define_static::__flatten_reflect_constant`, gathers every dimension's extent
along the way (outer range size first, then `ValTy`'s own static extents via `extent_v`), and
reconstructs the correctly-nested array type via a new `__define_static::__nd_array_shape<ValTy,
Extents...>` (ordinary recursive class-template metafunction over the extents pack — sidesteps the
declarator syntax limitation of not being able to spell an arbitrary, pack-driven number of `[N]`
pairs). `FixedNDArray<ValTy, Shape, Vals...>` then initializes `Shape::type` from the fully flat,
row-major `Vals...` pack via ordinary aggregate-initialization brace elision (the same mechanism
`int a[2][3] = {1,2,3,4,5,6};` uses in non-reflective code) — no per-row copying needed at all.

Two secondary bugs found empirically while testing this, neither anticipated by the original
design note:
1. **The `reflect_constant_array` forward declaration's own `requires`-clause** (`is_constructible_v<range_value_t<R>, range_reference_t<R>>`) is unconditionally `false` for any array-typed
   `range_value_t` — arrays are never "constructible" per that trait's specification (there's no
   such thing as direct-initializing an array object via ordinary constructor syntax) — so it
   would have silently SFINAE-rejected every nested case at the very entry point regardless of
   what the body did. Added an array-aware alternative,
   `__reflect_constant_array_row_ok_v<T>` (recursively `is_copy_constructible_v` at the leaf,
   `is_array_v`-peeling one dimension at a time), used only when `range_value_t<R>` is itself an
   array type.
2. **`define_static_array`'s existing row-pointer extraction pattern**
   (`extract<const ValTy*>(array)`, already established/working for the flat case) does not work
   for a multi-dimensional backing object — empirically, only extracting the *whole* array by
   reference succeeds; a row-pointer target hits `extract`'s "Value"-kind exact-type-match path,
   which has no array-to-pointer decay logic at all (unlike the "Declaration"-kind path the flat
   case apparently goes through, which does). Fixed by extracting the whole array as a reference
   and decaying to a row pointer manually (`&whole[0]`). Getting the outer extent for that
   reference type hit a second, unrelated, general-C++ wall: a `constexpr` local (or an array
   bound in a type) cannot be initialized by reading an ordinary (non-`constexpr`) local or
   function parameter, even inside a `consteval` function body executing within an ambient
   manifestly-constant-evaluated context — confirmed via a minimal, non-reflective isolated repro
   before concluding it wasn't reflection-specific. Sidestepped entirely rather than worked around:
   since `is_array_v<ValTy>` can only be true when `R` is itself (modulo reference) a genuine raw C
   array type (no ordinary container can hold array-typed elements), the outer extent is already
   available directly from the template parameter `R` via `extent_v<remove_reference_t<R>, 0>` — a
   genuine compile-time constant, not something that needs computing from a runtime-shaped value at
   all.

Also hit, and suppressed, `-Wmissing-braces` on `FixedNDArray`'s intentionally flat, brace-elided
initializer (`{Vals...}`) — confirmed via a minimal isolated repro that ordinary, non-reflective
code (`int a[2][3] = {1,2,3,4,5,6};`) triggers the exact same warning under `-Wmissing-braces
-Werror`, so this is expected, not a design smell; wrapped in
`_LIBCPP_DIAGNOSTIC_PUSH`/`_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wmissing-braces")`/`_LIBCPP_DIAGNOSTIC_POP`
matching existing precedent elsewhere in this same header.

Extended the existing regression test (`cwg3111-lwg4432-reflect-constant-array.pass.cpp`'s former
"nested arrays remain unsupported" section, now real coverage): 2D and 3D `reflect_constant`/
`reflect_constant_array`, `define_static_array` on a nested array, and a 2D array of a structural
class type (not just scalars, to confirm per-leaf recursion still bottoms out correctly through the
plain non-array `reflect_constant` overload). Verified zero regressions: `clang/test/Reflection/`
22/22 (header-only change, but ran anyway per discipline), libc++ reflection suite at the
documented 7-test pre-existing baseline (114 tests total, exactly the 7 documented pre-existing
warning/verify-mismatch failures, zero new). Header-only fix, no compiler rebuild needed beyond
`ninja -C build-libcxx cxx`. Updated the table above and the Next Up pointer to item 9.

### 2026-09-11 (later) — Item 10 independently re-verified; separate stacktrace CI bug found and fixed

**Item 10 re-verification.** Dispatched Terra for item 10 (`3560-18`, `access_context::via`
catch-and-inspect coverage) after personally isolating the blocker down to a minimal, non-reflection
repro first (a `consteval` local variable whose initializer throws before completing construction
gets destroyed anyway during exception unwinding — `note_constexpr_destroy_out_of_lifetime` in
`ExprConstant.cpp`'s `HandleDestructionImpl`) — a much stronger starting point than item 9 had, so
Terra could go straight to root-causing rather than needing to rediscover the repro. Terra found the
actual bug: `EvaluateVarDecl` registers the new local's block cleanup (so its storage is addressable
during initializer evaluation) *before* running that initializer; if the initializer throws, nothing
removed the now-stale cleanup, so unwinding later tried to destroy an object whose `APValue` was left
`Absent` (lifetime never began). Fix: a new `EvalInfo::cancelCleanup(APValue&)` that removes the
matching cleanup-stack entry, called from `EvaluateVarDecl` right before propagating a failed
initializer's failure upward (idempotent — some failure paths already unwind their own cleanup
first, so a not-found case is a normal no-op, not an error).

**Did not trust the "fixed and verified" claim at face value** — independently re-ran the full
verification personally: fresh `ninja -C build-nyx -j$(nproc)` (full rebuild, not just the `clang`
target), then both my own original isolated repro and the new
`clang/test/SemaCXX/constexpr-p3068r6-throw.cpp` regression case compiled/passed directly, then the
full `clang/test/Reflection/ clang/test/Parser/ clang/test/SemaCXX/ clang/test/SemaTemplate/
clang/test/AST/ clang/test/CodeGenCXX/` suite (3836 tests) came back at exactly the documented
5-test `SemaCXX` escalation-cluster baseline, zero new failures. Then `ninja -C build-libcxx -t
clean cxx && ninja -C build-libcxx -j$(nproc) cxx` (explicit clean rebuild, not trusting the `cxx`
target's weak dependency edge on the external clang binary) followed by the new
`libcxx/test/std/experimental/reflection/exception.pass.cpp` catch-and-inspect case (asserts
`what()`, `from()`, and `where().line()` on the caught `meta::exception`), the full libc++
reflection suite (114 tests, exactly the documented 7-test baseline, zero new), and the full
`libcxx/test/std/diagnostics/stacktrace/` + `.../support.limits.general/` suites (92/92) for good
measure since the fix touches general constexpr-exception plumbing that stacktrace-adjacent tests
also exercise. All independently confirmed clean. Committed as `<see git log>`.

**Separate bug found from a user-provided CI screenshot** (GitHub Actions "Verify relocated
package" step failing on `std.compat.cppm`, from the `cxx26-preflight-2026.09.10-miracle-fixes` tag
cut during the emergency stacktrace-port phase, task #23 "Watch preflight CI run" never having been
followed up on before this). Root-caused, not dismissed as noise: the emergency `<stacktrace>` port
correctly removed `"stacktrace"` from `header_information.py`'s `headers_not_available` list (so the
header genuinely is treated as available) but never cleared the *separate*
`"unimplemented": True` flag on `__cpp_lib_stacktrace`'s own entry in
`generate_feature_test_macro_components.py` — a different data table entirely, used to generate
`libcxx/include/version`, `libcxx/modules/std.compat.cppm.in`'s "please update headers_not_available"
guards, and the FTM compile-pass test. With that flag still set, `std.compat.cppm.in` retained a
stale `#if __has_include(<stacktrace>) #error ... #endif` guard that fires the moment `<stacktrace>`
is genuinely includable — exactly the observed CI failure (this hadn't surfaced locally before
because the local dev tree's `libcxx/include/version`/`std.compat.cppm.in` were hand-edited directly
during the emergency port rather than regenerated, so they'd drifted from what the generator's data
tables would actually produce). Fixed by removing the stale flag; also corrected the same entry's
`"values"` from the paper's nominal `{"c++23": 202011}` to `{"c++26": 202011}`, since this fork's
actual `<stacktrace>` header (confirmed by reading `libcxx/include/stacktrace` directly) is gated
`_LIBCPP_STD_VER >= 26`, not 23 — using the paper's nominal version would have made the regenerated
FTM test assert the macro should be defined at `-std=c++23`, which is false for this fork. Verified
by regenerating (`ninja -C build-libcxx libcxx-generate-files`) and confirming `libcxx/include/version`
now has **zero diff** against the already-committed (hand-edited, but correct) state — the generator
and the hand-edit now agree exactly — while `std.compat.cppm.in`, `stacktrace.version.compile.pass.cpp`,
`version.version.compile.pass.cpp`, and `libcxx/docs/FeatureTestMacroTable.rst` pick up the correct,
now-generator-consistent state. Full stacktrace + support.limits.general + reflection suites (above)
confirm zero regressions from this change either. Committed separately from item 10's fix (unrelated
root causes). Cutting a fresh, properly-named preflight tag next (the prior `...miracle-fixes` name
was a project codename, not descriptive — per explicit user feedback) and watching its CI run to
closure this time (task #23).

### 2026-09-11 (final) — Item 2 (`#237`) fully fixed and verified

The parked `is_type_alias(^^ct)` sub-symptom was root-caused and fixed. The exact failing path is
not a special metafunction-argument evaluation path: `Sema::BuildCXXReflectExpr` receives `ct` as
a const `TypedefType`, then its `APValue(ReflectionKind::Type, ...)` construction calls
`APValue::setReflection`, which calls `unwrapReflectedType`. That helper treated every cv-qualified
alias as externally cv-qualified and desugared `ct` to its bare closure `RecordType` before
`is_alias()` evaluated its argument. `isTypedefNameType()` is an ordinary type-class check and has
no lambda-specific behavior.

The historical cv normalization is still needed for `^^const Alias == ^^const T`. The fix now
consults the `TypedefNameDecl` underlying type: it preserves an alias when that underlying type
itself carries the cv qualifier (the `constexpr` closure case), and desugars only when the
qualifier is external to the alias. The existing `decltype`-driven alias stripping remains
unchanged. Extended `issue-237-closure-type-alias.pass.cpp` to require both
`is_type_alias(^^ct)` and `has_identifier(^^ct)`.

Verification: full `ninja -C build-nyx -j$(nproc)` rebuild; original upstream repro compiles;
`clang/test/Reflection/` passes 22/22; Parser/SemaCXX/SemaTemplate/AST/CodeGenCXX runs 3814 tests
with exactly the documented five SemaCXX baseline failures; explicit clean `build-libcxx` cxx
rebuild succeeds; focused #237 test passes; full libc++ reflection suite discovers 119 tests and
reports exactly its documented seven pre-existing failures, with no new failures. Item 2 now has a
final fixed-and-verified disposition. All 14 Reflection Closeup items are consequently final,
ready for CU5.
