# NEW-7 design investigation: dependent splice in CTAD-like initialization

Date: 2026-09-10

## 1. Adopted wording

The adopted source is [P2996R13](https://wg21.link/P2996R13). The relevant paragraph in
[dcl.type.simple] p3 says:

> A `placeholder-type-specifier` is a placeholder for a type to be deduced ([dcl.spec.auto]). A `type-specifier` of the form `typename_{opt} nested-name-specifier_{opt} template-name` is a placeholder for a deduced class type ([dcl.type.class.deduct]) if either
>
> * it is of the form `typename_{opt} nested-name-specifier_{opt} template-name` or
> * it is of the form `typename_{opt} splice-specifier` and the `splice-specifier` designates a class template or alias template.
>
> The `nested-name-specifier` or `splice-specifier`, if any, shall be non-dependent and the `template-name` or `splice-specifier` shall designate a deducible template.

P2996R13 also makes the dependency test explicit in [temp.dep.splice]:

> A splice-specifier is dependent if its converted constant-expression is value-dependent. A splice-specialization-specifier is dependent if its splice-specifier is dependent or if any of its template arguments are dependent. A splice-scope-specifier is dependent if its splice-specifier or splice-specialization-specifier is dependent.

The revision history identifies the intent as preventing dependent splice-specifiers from
appearing in CTAD, following CWG3003. The practical forbidden form investigated here is the
implicit type-splice declaration with copy-list initialization, `[:R:] value = {1}`.

## 2. Parser/Sema trace

Ordinary declarations take the parser's class-template-deduction context through
`Parser::isClassTemplateDeductionContext` (`clang/include/clang/Parse/Parser.h`) into
`getTypeName` (`clang/lib/Sema/SemaDecl.cpp`). That path creates a
`DeducedTemplateSpecializationType`. `AddInitializerToDecl` then calls
`DeduceVariableDeclarationType`, which reaches `DeduceTemplateSpecializationFromInitializer`;
the latter distinguishes `InitListExpr` and performs deduction-guide/constructor overload
resolution.

Reflection type splices take a different path. An unadorned `annot_splice` is recorded as
`DeclSpec::TST_type_splice` in `ParseDecl.cpp`, then `SemaType.cpp` calls
`BuildReflectionSpliceType`. A dependent splice reaches
`Sema::BuildReflectionSpliceType` in `SemaReflect.cpp`, where the reflected type is represented
by `Context.DependentTy` and the resulting type is a `ReflectionSpliceType`/dependent subtype.
Thus, no ordinary CTAD `DeducedTemplateSpecializationType` exists to hook before initializer
deduction; the first reliable common point is `AddInitializerToDecl`, after the initializer's
syntax has been preserved.

## 3. Scope decision

A narrow check is achievable without resolving the dependent reflected type. The check needs
only these facts:

* the declaration is using copy-list initialization (`DirectInit == false` and `InitListExpr`);
* the splice type's underlying type is exactly `Context.DependentTy`; and
* the splice is dependent.

Direct-list initialization (`value{1}`), parenthesized initialization (`value(1)`), and copy
initialization from a non-list expression (`value = 1`) therefore remain outside the check.
An explicitly written `typename` splice is also excluded: existing dependent declarations such
as the substitution coverage use that form and it is not the implicit CTAD-like form at issue.

While investigating, I found that this fork incorrectly reused the splice token location as
`TypenameKWLoc` for an unadorned `TST_type_splice`. `SemaType.cpp` now passes an invalid location
for that implicit form, restoring the semantic distinction. Because dependent instantiation in
this fork can still lose the original keyword location, `AddInitializerToDecl` additionally
checks the declaration's source-line prefix for an explicit `typename` before applying the
diagnostic. This is deliberately limited to the already-narrow copy-list/dependent-splice case.

## 4. Implementation and verification

Implemented:

* `err_dependent_splice_ctad` in `DiagnosticSemaKinds.td`.
* The scoped rejection and recovery path in `Sema::AddInitializerToDecl`.
* Correct implicit-splice `TypenameKWLoc` construction in `SemaType.cpp`.
* `new-7-dependent-splice.verify.cpp`, covering the forbidden dependent copy-list form and
  positive controls for dependent `{}`, `()`, non-list `=`, explicit `typename`, and a
  nondependent `[:^^Converting:] value = {1}`.
* The existing batch-13 negative control now expects the diagnostic.

Build and focused verification:

```text
env CCACHE_DISABLE=1 ninja -C build-nyx -j1 clang       PASS
clang++ ... -freflection-latest -Wunused-variable -Xclang -verify \
  -Xclang -verify-ignore-unexpected=note new-7-dependent-splice.verify.cpp  PASS
libcxx/utils/libcxx-lit build-libcxx -sv -j1 \
  libcxx/test/std/experimental/reflection/new-7-dependent-splice.verify.cpp  PASS (1/1)
```

The final full wrapper run was:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection
Total: 108; unsupported: 1; passed: 101; failed: 6
```

The six failures are the remaining documented baseline cluster: the three entity/dealias
warning tests, namespace-reflection-equality-reopened, batch12's unused-parameter warning, and
batch15's warning/verify mismatch. The original seven-test baseline included batch13; NEW-7 now
passes and batch13's expected diagnostic was updated, so the final count is six. No new failure
remains attributable to this change.

## 5. Outcome

NEW-7 is covered and no longer deferred. The checklist row 2996-49 is marked Covered, the main
tracker entry is updated, and the session log records the final suite result.

## Personal follow-up (2026-09-10, post-session review) — fix reverted

The implementation above was reviewed and independently verified before committing, per this
epic's established discipline for the reflection-printer/splice-diagnostic area (a history of
false claims and fragile fixes). Three distinct correctness bugs were found:

1. **The `getTypenameKWLoc()` exemption path was unreliable**, and the session's own fallback —
   scanning the declaration's raw source line for the substring `"typename"` — is unsound: a
   direct repro, `/* typename */ [:R:] value = {1};` (an unrelated comment on the same line),
   compiled with no diagnostic when the CTAD-like form should have been rejected.
2. **Removing the text-scan and trusting `getTypenameKWLoc().isInvalid()` alone breaks a real,
   load-bearing existing test**: `clang/test/Reflection/splice-templates.cpp`'s `DepTClsCTAD`
   (`typename [:R:] obj = {value};`, inside a function template parameterized by the splice
   target) started failing. This proves the `typename`-exemption is normatively necessary in this
   fork, not an assumption to drop.
3. **A third attempt using `Lexer::findPreviousToken` to find the literal token immediately
   preceding the splice** (correctly handling raw-lexed keywords as `tok::raw_identifier` and
   comparing spelling) verified correct in 3 isolated single-function repros, but produced wrong
   diagnostics (a mix of unexpected errors and missing expected warnings across unrelated
   declarations) once multiple such dependent-splice template functions coexisted in one
   translation unit — the exact shape of the `new-7-dependent-splice.verify.cpp` test file. The
   root cause (likely a pattern-vs-instantiation double-checking effect, or a location-reuse
   artifact specific to how sibling dependent splice types are represented) was not identified
   within a reasonable time-box.

All source and test changes from this session were reverted; the working tree has no NEW-7 fix.
See the corrected NEW-7 entry in `docs/REFLECTION_GAPS.md` for the tracker-level summary. This
report is kept as-is above (unedited) for the historical record of the first attempt; this section
documents why it was not shipped.
