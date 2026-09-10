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

**Status as of 2026-09-11 (post item-6 verification):** Item 1 is escalated to the user, not being
worked automatically (see below). Item 2 has a partial fix landed, one sub-symptom
(`is_type_alias` identity loss) still open and deliberately parked. **Items 3, 4, 5b, and 6 are
now fixed/verified** (commits `9760450c0fe4`, `8081ce09739d`, `08999b0aea1e`, and the item-6
tracker+test commit — see their paragraphs near the end of this section and their table rows
below; item 6 needed no compiler code change, only a regression test confirming it's already
fixed). **Item 5a (issue #180) remains escalated to the user alongside item 1** — it was refuted
as an expansion-statement bug during investigation and needs the same class of Sema
instantiation-context work as item 1, not a narrow fix; see its own table row and
`docs/reflection-audit/issue-180-minimal-repro.cpp`. **Resume with item 7 (issue #150 — spliced
explicit destructor call `~[:info:]()`)**: needs a new splice-bearing destructor-name AST/Sema
representation; no prior investigation report exists yet for this one — start fresh from
`clang/lib/Parse/ParseExpr.cpp`'s `ParseUnqualifiedId(...IK_DestructorName...)` path (no splice
branch there today).

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

**Item 2 (closure-type alias, `#237`): partial fix landed (commit `a9c1f8aad1d6`), one symptom
remains open.** Worked entirely directly (no Codex usage). Root-caused and fixed the primary,
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
| 2 | Issue #237 — closure-type alias loses identity | **Partial fix landed (commit `a9c1f8aad1d6`), one symptom remains open** | Primary hard-error ("'auto' not allowed in type alias") fixed — root cause: `decltype(auto-declared-var)` retains `AutoType` sugar that leaked into `BuildReflectionSpliceType`'s reflected-operand resolution; fixed by desugaring there. Zero regressions (verified: clang/test/Reflection 20/20, libcxx reflection 7-test baseline unchanged, SemaCXX/AST/CodeGenCXX/SemaTemplate 3411 tests at the documented 5-test baseline). **Remaining**: `is_type_alias(^^ct)` still returns false even once `ct` declares successfully — traced through every layer (CXXReflectExpr construction, `VisitCXXReflectExpr` evaluation, `APValue::getReflectedType()`, the metafunction `Evaluator` callback, calling the raw `__metafn_is_alias` directly bypassing the library wrapper) and found each one correctly preserving the `TypedefType` sugar in isolation — yet the metafunction still observes a bare `RecordType`. Root cause not found; see the dated session-log entry for the full ruled-out-hypothesis list before re-investigating. |
| 3 | Issue #188 — `display_string_of(dealias(...))` not constant expr | **Fixed and verified (commit `9760450c0fe4`)** | Root cause: `dealias()`'s `desugarType()` hand-rolled sugar-strip loop was missing `DecltypeType` — an alias template's underlying type (e.g. `iterator_t<R> = decltype(ranges::begin(declval<R&>()))`) desugars one step to a `DecltypeType` whose *canonical* type is ordinary but which the loop couldn't unwrap further, leaving a still-sugared reflection whose own template-argument query resolved back to an equivalent unresolved reflection every time — a literal non-terminating recursion (confirmed via `Type*` identity tracing: same pointer recurred 380+ times), not legitimate deep nesting, eventually exhausting the constexpr call-depth budget with a generic, cause-free diagnostic. Fixed by adding `DecltypeType` to the loop's unconditional strip set (alongside pre-existing `AutoType`/`SubstTemplateTypeParmType`/`ReflectionSpliceType`). Also fixed an unrelated dead-code bug found en route in the same loop: the `UsingType` branch tested the wrong local (`TDT` instead of `UT`), so `UsingType` sugar was never actually unwrapped. Verified zero regressions: clang/test/Reflection 20/20, libcxx reflection suite at the documented 7-test pre-existing baseline (byte-for-byte same tests), SemaCXX at the documented 5-test pre-existing baseline. New regression test: `libcxx/test/std/experimental/reflection/issue-188-dealias-decltype.pass.cpp`. |
| 4 | NEW-7 — dependent splice-specifier wrongly accepted (CTAD-like position) | **Fixed and verified (commit `8081ce09739d`)** | Root cause: THREE independent, compounding bugs in `ReflectionSpliceType::getTypenameKWLoc()`'s tracking, not the diagnostic logic itself (which every prior attempt was trying to work around via unreliable heuristics instead). (1) `SemaType.cpp`'s plain (no-`typename`) `TST_type_splice` case reused the splice's own location as `TypenameKWLoc`, making implicit splices look explicit. (2) `typename [:R:]` reached via `TryAnnotateTypeOrScopeToken` goes through `ParseOptionalCXXScopeSpecifier`'s splice-rewrite, which hard-coded `SourceLocation()` instead of threading the real keyword location through — the opposite failure, discarding an explicit `typename`. (3) Even after fixing both, `DependentReflectionSpliceType::Profile` (the FoldingSet uniquing key for dependent splice types) never included `TypenameKWLoc`, so two structurally-identical dependent splices (e.g. the same depth/index template parameter in two different function templates), one with `typename` and one without, collapsed onto the same cached type node — exactly the "breaks once multiple dependent-splice templates coexist in one TU" signature all 3 prior attempts hit, via a completely different mechanism than any suspected. Fixed all three; added a `HasTypenameKW` bit to the Profile (not the raw location, to avoid over-fragmenting type identity by source position). New test: `clang/test/Reflection/new7-dependent-splice-ctad.verify.cpp`. Verified zero regressions: clang/test/Reflection 21/21, Parser+SemaCXX+SemaTemplate+AST+CodeGenCXX 3814/3814 at the documented 5-test baseline, libcxx reflection suite at the documented 7-test baseline. |
| 5a | Issue #180 — `static_assert(false)` silently ignored | **ESCALATED TO USER — not an expansion-statement bug, mis-scoped from the start** | The plan bundled this with #181 under upstream PR #261's expansion-body-deferral design, inherited from `docs/reflection-audit/codex-m4-180-181-report.md`'s investigation. That premise is refuted: the upstream repro contains no `template for` anywhere, and a minimal reproducer confirms the failure has nothing to do with expansion statements. See `docs/reflection-audit/issue-180-minimal-repro.cpp` (fails silently) and its companion `docs/reflection-audit/issue-180-control-diagnoses-correctly.cpp` (diagnoses correctly) — narrowed down from the full upstream repro to a single discriminating factor: whether the *enclosing* function calling `substitute()`+`extract()` is itself a function template. `Sema::EnsureInstantiated` (`SemaReflect.cpp:328-336`) does call `S.InstantiateFunctionDefinition(..., true, true)` for a function-template-specialization reflection, and this correctly triggers the `static_assert` diagnostic when called from an *ordinary* (non-template) consteval function — but the identical call, made while Sema's instantiation-context stack already has a frame for the *enclosing* function template's own instantiation, silently produces no diagnostic. Four candidate mechanisms considered (SFINAE-context misfire, instantiation-depth deferral, evaluator-side diagnostic suppression, an overly-eager mutual-recursion instantiation guard) — none confirmed; distinguishing them needs a targeted Sema instantiation-context trace, the same class of investigation as item 1's escalation cluster. Per the completion bar, escalating alongside item 1 rather than guessing further. **PR #261's port is NOT justified by #180 as currently understood — do not let a future session inherit that premise; if the port is ever needed, it needs its own independent justification.** |
| 5b | Issue #181 — non-copyable tuple/range element expansion binding | **Fixed and verified (commit `08999b0aea1e`)** | Three independent, compounding bugs in `SemaExpand.cpp`, found via direct instrumentation (not the guessed-at P1306R5 `std::move`-selection design from the original plan, which turned out not to be the actual mechanism at all): (1) `tryMakeCXXIterableExpansionSelectExpr`'s hidden `__range` unconditionally copy-constructed (`Range->getType().withConst()`) *before* the function had even determined whether the type is iterable, so a non-copyable range (e.g. `std::tuple<std::unique_ptr<int>, ...>`) hard-errored regardless of binding form, before ever reaching the (already-correct) destructurable path. (2) `makeCXXDestructurableExpansionSelectExpr` hardcoded `SpelledAsLValue=true` when building the hidden binding's reference type, indistinguishable from `auto&`, so `auto&&` over a genuine prvalue range failed to bind. (3) Neither path completed the range's type before a `begin()`/`end()` member lookup on it (unlike ordinary range-based for, which does), asserting in an assertions-enabled build when the type is reached only via a reference parameter. Fixing (1) by unconditionally switching to a forwarding reference (mirroring ordinary range-based for's `auto&& __range`) regressed *working* constexpr cases with a perfectly copyable range type elsewhere in the libc++ reflection suite (`expansion-lambda-capture-crash.pass.cpp`, `deduction-guide-reflection-mangling.pass.cpp`) — binding a reference to a temporary inside a manifestly-constant-evaluated context needs lifetime-extension bookkeeping a by-value copy never needed. Final fix: speculatively check copy-constructibility first (a trial `InitializationSequence`, which never commits/diagnoses unlike the real `AddInitializerToDecl` call) and only fall back to the reference when the type genuinely can't be copied — preserving by-value behavior for every case that already worked. Also had to scope the earlier `RequireCompleteType` addition to record types only (a `void`-typed range from an unrelated unresolved-overloaded-function recovery path was getting a wrong, pre-empting diagnostic) and make the reference-only lifetime-extension conditional on actually using the reference path (applying it to the by-value copy path was itself found to corrupt the constant evaluator's allocation tracking). New regression test: `libcxx/test/std/experimental/reflection/expansion-noncopyable-tuple-binding.pass.cpp`, covering all four binding forms (`auto`/`auto&`/`auto&&`/`const auto&`) plus const-source and prvalue-range variants in one translation unit (per this epic's now-repeated "isolated passes, combined breaks" lesson from items 4 and 5a). Verified zero regressions: `clang/test/Reflection/` 21/21 (including the new file), libc++ reflection+debugging+stacktrace suite at the documented 7-test baseline (123 tests total, 7 pre-existing failures, zero new), SemaCXX/Parser/AST/CodeGenCXX/SemaTemplate 3814 tests at the documented 5-test escalation-cluster baseline. Two new, unrelated findings (out of scope for this item) logged in the Next Up section above. |
| 6 | Issue #182 — `template for` + `continue` ICE | **Already fixed — verified with a regression test, no code change needed** | Upstream report: `template for` crashes at codegen (`llvm::BranchInst::BranchInst`) if its body contains `continue` (bare, or via `if constexpr`), reproducing only at `-O1`+ (`-O0` doesn't reproduce). Extensive reproduction attempts against this fork — matching the exact upstream shape, across `-O0` through `-O3`, with/without `-g`, in template and non-template enclosing functions, with `continue`/`break` combined, 1–10 iterations — could not reproduce any crash. Direct inspection of `CodeGenFunction::EmitCXXExpansionStmt` (`clang/lib/CodeGen/CGStmt.cpp`) shows correct, already-sound lowering: one `JumpDest` is pre-allocated per expansion instance up front, and each instance's continue target is simply the next instance's own `JumpDest` (or the loop's exit block for the last one) — no shared, instance-independent continuation destination for a discarded branch to dangle a reference to. Likely explanation (not confirmed, since the original bug was never reproduced to bisect against): this exact function was silently dropped by a merge during this fork's LLVM 22 sync and later restored verbatim from the pre-merge fork in commit `df8b70aeaf5e` ("restore expansion-statement CodeGen..."), which landed after the upstream report (2025-09-08) — plausibly picking up a fix (LLVM-side or otherwise) incidentally along the way. New regression test locks this in: `libcxx/test/std/experimental/reflection/expansion-continue-break-codegen.pass.cpp` (compiled at `-O2 -g`, both a bare top-of-loop `continue` and the exact `if constexpr() continue` shape from the issue title, runtime-asserted for correctness, not just "doesn't crash"). Verified: libc++ reflection+debugging+stacktrace suite at the documented 7-test baseline (124 tests, zero new failures) — no compiler code change was needed, so the broader `clang/test/Reflection`/SemaCXX baselines are unaffected and weren't re-run. |
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
