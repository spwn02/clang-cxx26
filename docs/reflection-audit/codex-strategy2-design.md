# P3560R2 strategy (2): compiler-side throwable reflection failures

Status: investigation/design only, 2026-09-09. No implementation is included in this
document's originating session.

## Scope and ground truth

The baseline and operational constraints are recorded in
[`REFLECTION_GAPS.md`](../REFLECTION_GAPS.md), especially Ground truth: use `rg` rather
than the unreliable `grep`/`find` aliases, refresh PCHs and staged libc++ headers, do
not trust stale auxiliary Clang tools, and treat 23 full-suite failures as the known
baseline. Memory pressure has already killed builds at high parallelism and even at
`-j1`; check available memory before retrying and use patient low-parallelism gates.

P3560R2 changes reflection evaluation failures from hard constant-evaluation failure
to a catchable `std::meta::exception`. The fork now has the exception class and direct
throw/catch support, and 13 public functions have library-side precondition wrappers:

```
size_of, bit_size_of, alignment_of,
has_inaccessible_nonstatic_data_members, has_inaccessible_bases,
annotations_of_with_type, template_of, template_arguments_of,
access_context::via, enumerators_of, offset_of, operator_of, subobjects_of
```

Those wrappers are strategy (1): they decide from existing public predicates before
calling `__metafunction`. The batch-4 audit states that no additional candidate is
safe under that rule. In particular, member and parameter queries were attempted and
reverted because their actual failure state is held in compiler AST/evaluation state.

## 1. Remaining functions and failure sites

The list below is the remaining public surface that must be audited for P3560 Throws
semantics. A name is included when either the adopted wording explicitly adds a
Throws condition or a companion paper added the function/condition and the fork's
implementation still reaches `DiagFn`. Functions already in the 13-item list are
omitted. The compiler helper names are the current names in
`clang/lib/AST/ExprConstantMeta.cpp`.

### Core P2996R13 names and queries

| Public function | Current failure condition | Why strategy (1) is insufficient |
|---|---|---|
| `identifier_of`, `u8identifier_of` | The reflection has no identifier, has an ambiguous/special name (operator, constructor, destructor, conversion, template-specialization cases), or the identifier cannot be represented in the requested encoding. | `has_identifier` only answers the coarse predicate. The actual spelling/encoding and consistency checks occur in `identifier_of`/`getParameterName` and AST `DeclarationName` handling. |
| `type_of` | The reflection does not have a type, or the type is not obtainable for the represented entity. | The result depends on the entity-specific AST/type path, including parameter, value, object, member, and proxy cases; no single public predicate proves the backend operation succeeds. |
| `object_of` | The reflection does not designate a static-storage object or a suitable variable/reference to one, including constant-expression/lifetime requirements. | Suitability depends on `APValue` lvalue base, storage duration, and current evaluator lifetime, not just reflection kind. |
| `constant_of` | The represented entity cannot be spliced/evaluated as a valid constant expression. | Requires evaluating the reflected entity and validating the resulting `APValue`; this is not represented by `is_value` or another public boolean. |
| `parent_of` | The reflection has no parent. | `has_parent` is a useful predicate but the backend still handles aliases, template declarations, namespaces, and special entities; using it as a wrapper would duplicate the semantic relation and can race the backend's entity normalization. |
| `dealias` / `underlying_entity_of` | The operand does not represent an entity that can be dealias'ed. | The implementation normalizes compiler-owned proxy/alias AST nodes; the applicable entity test is not equivalent to one stable public predicate for every proxy kind. |
| `is_accessible` | A member's parent class is incomplete, or a direct base relationship has an incomplete derived class; access computation can also fail while resolving the class context. | The incomplete-class checks use parent/base AST links and the supplied `access_context`; the result is deliberately not the same as the boolean accessibility answer. |
| `members_of` | The dealias'ed operand is neither a complete class type nor a namespace, or the member walk cannot be formed for the current access context. | The result is a compiler-generated range. The failure happens in `get_begin_member_decl_of`/`get_next_member_decl_of`, after AST completeness and member enumeration. |
| `bases_of` | The operand is not a complete class type, or base enumeration/access processing fails. | Same range/evaluation issue; `is_class_type` does not prove completeness or a successful AST base walk. |
| `static_data_members_of` | The underlying `members_of` operation fails, or the result cannot be filtered as static data members. | It is implemented by a compiler-backed member range followed by filtering; the failure is downstream of the public kind predicate. |
| `nonstatic_data_members_of` | The underlying `members_of` operation fails, including closure-type restrictions used by the P3560 access-query wording. | Closure restrictions, completeness, and access-context traversal are compiler facts. |
| `extract<T>` | The reflection is not a suitable object/variable/member/function/value for `T`, qualification/conversion rules fail, the value is not usable in constant evaluation, or the extracted result is not representable. | `extract` has three distinct AST/APValue paths (object/reference, member/function pointer, and value). The implementation performs conversions and constant-expression validation deep in `extract`, not through one public predicate. |
| `can_substitute`, `substitute` | The operand is not a template, an argument is not usable as a template argument, or template argument substitution/deduction/specialization fails. | `CheckTemplateArgumentList`, substitution, overload/template specialization lookup, and diagnostics live in Sema/AST. `has_template_arguments` says nothing about whether a proposed argument list substitutes. |

### P3096R12 parameter reflection

| Public function / backend | Current failure condition | Why strategy (1) is insufficient |
|---|---|---|
| `parameters_of` (`get_ith_parameter_of`) | The operand is not a function/function type, or parameter enumeration cannot produce the requested range. | The compiler must inspect `FunctionDecl`/`FunctionProtoType`, explicit parameter count, and synthesized range endpoints. |
| `return_type_of` | The operand is not a function/function type, or its return type is undeduced/unavailable. | Current code diagnoses for non-function and undeduced cases using AST function/type state. |
| parameter forms of `identifier_of`/`u8identifier_of` | A parameter has no identifier, or its declaration spelling/encoding is unavailable or inconsistent. | `has_identifier` only supplies the precondition; the exact parameter declaration and encoding are read in the backend. |
| parameter forms of `type_of` | The reflected parameter has no obtainable declared type. | The parameter declaration is resolved through the compiler's `ParmVarDecl`, not a library predicate. |
| `variable_of` | The parameter is not a parameter, or the current evaluation is not inside the defining function/call frame needed to map it to a variable reflection. | The current implementation uses `StackLocationExpr`, `Meta.CurrentCtx()`, the function definition, and parameter index. This is explicitly evaluator state and was the reason the earlier library guard was reverted. |

`has_ellipsis_parameter`, `has_default_argument`, `is_explicit_object_parameter`, and
`is_function_parameter` are total predicates in R12 and are not strategy-(2) targets
for their non-applicable operand cases. Their current diagnostic behavior is a separate
P3096 conformance issue recorded in the tracker.

### P3394R4 annotations and P3491R3 result functions

| Public function / backend | Current failure condition | Why strategy (1) is insufficient |
|---|---|---|
| `annotations_of` (including the internal `get_ith_annotation_of`) | The operand is not an annotatable declaration/parameter, or annotation traversal/filtering cannot be evaluated. | Annotation ownership includes compiler attributes and parameter declarations; the range is constructed by AST traversal. `annotations_of_with_type` is already wrapped only for its exposed precondition. |
| `reflect_constant<T>` | The value is not structural/copy-constructible or the synthesized template-argument object is not representable. | The compiler evaluates the argument and runs `EvaluateAsConstantExpr`/template-parameter-object validation. Existing constraints do not cover all APValue representability failures. |
| `reflect_object<T&>` | The operand is not suitable as a constant template argument for `T&`. | Requires lvalue identity, storage/lifetime, and constant-evaluation state. |
| `reflect_function<T&>` | The operand is not a function or is not suitable as a constant template argument for a function reference. | Requires lvalue identity/function declaration extraction and constant-evaluation validation. |
| `data_member_spec` | Its type/options violate the adopted data-member-description rules (invalid identifier, missing/invalid width, incompatible alignment, etc.). | Option validity is partly library-visible but the final identifier and type/layout checks are compiler-side; it is not the unrecoverable `define_aggregate` operation itself. |

P3491's `define_static_string`, `define_static_array`, and the currently missing
`define_static_object` are library templates layered over constant evaluation. Their
Mandates/overload constraints need a separate wording audit; they are not current
compiler metafunction entries and should not be mixed into the first strategy-(2)
pilot. Likewise `define_aggregate` is intentionally excluded: P3560R2 explicitly
retains unrecoverable failure because partial injected definitions may already exist.

The resulting first-pass strategy-(2) set is therefore 24 public/backend targets
(counting the two identifier encodings and the three result functions separately),
with several sharing one compiler implementation path. The exact rollout should be
organized by backend path, not by mechanically changing every diagnostic call.

## 2. Recommended mechanism

### Existing paths

`CXXMetafunctionExpr::DiagnoseFn` is currently:

```cpp
std::function<PartialDiagnostic &(SourceLocation, unsigned)>;
```

`VisitCXXMetafunctionExpr` in `clang/lib/AST/ExprConstant.cpp` collects those
diagnostics, invokes the implementation, and calls `Error(E)` when the implementation
returns failure. `ExprConstant.cpp` already has the required P3068 propagation model:
`EvalInfo::PendingExceptionInfo` stores an `APValue`, its `QualType`, the throw
expression, and call-stack notes; `VisitCXXThrowExpr` evaluates the operand, stores
that record, and returns `false`; `CXXTryStmt` moves the record into a matching handler
or restores it for propagation.

### Options

* **(a) Boolean/enum parameter.** Add `ThrowMode` to `Metafunction::evaluate` and
  `CXXMetafunctionExpr::ImplFn`, then make every failure site choose between
  `Diagnoser(...)` and a throw helper. This is explicit, but it spreads policy checks
  through every metafunction and makes it easy to miss one of the many early returns.
  A bare bool is especially poor because it cannot carry the exception message,
  originating reflection, or source location.

* **(b) Replace `DiagFn` with an adapter that throws.** The current callback cannot do
  this safely. It returns a mutable `PartialDiagnostic&`, while the callback has no
  `EvalInfo` access and no way to return a real `meta::exception` APValue. Returning a
  dummy diagnostic and setting a side flag would still require a second callback to
  create `PendingExceptionInfo`; it also risks code continuing to append `<<` arguments
  after the failure has been signalled.

* **(c) A dual failure sink (recommended).** Keep ordinary diagnostic construction,
  but add a second callback with explicit failure semantics:

```cpp
using ThrowFn = std::function<bool(SourceLocation, StringRef Message,
                                   APValue From)>;
using ImplFn = std::function<bool(APValue &, EvaluateFn, DiagnoseFn, ThrowFn,
                                  bool AllowInjection, QualType, SourceRange,
                                  ArrayRef<Expr *>, Decl *)>;
```

  Add a small `Fail(...)` helper in `ExprConstantMeta.cpp` that receives the current
  `ThrowFn`, message, and the metafunction reflection (`^^members_of`, etc.). In normal
  non-throwing evaluation it calls `Diagnoser` exactly as today. In the P3560 path the
  `ThrowFn` callback, created in `VisitCXXMetafunctionExpr`, constructs the
  `meta::exception` value and installs `Info.PendingException`; it returns `false` so
  the metafunction returns failure immediately. Every call site must use `Fail`, rather
  than directly calling `Diagnoser`, for a function covered by P3560.

The callback should not be implemented by pretending that a diagnostic is a throw.
It needs a dedicated evaluator helper, tentatively
`BuildMetaExceptionAndPend(EvalInfo &, SourceLocation, StringRef, APValue From)`.
That helper should:

1. Find/cache the canonical `std::meta::exception` record declaration in the AST
   (the same translation unit that provided `<meta>`).
2. Build the exception object's APValue using the exception constructor's constant
   evaluation path, so its `optional<string> what_`, `u8string u8what_`, `info from_`,
   and `source_location where_` have the exact library layout. Do not hand-code field
   offsets; the record layout and APValue aggregate APIs must be used.
3. Store the resulting APValue and `meta::exception` `QualType` in
   `PendingExceptionInfo`, capturing call-stack notes exactly as `VisitCXXThrowExpr`
   does. The throw site should be a synthetic evaluator-owned expression or a stable
   metafunction source expression whose range is `E->getSourceRange()`; it must not be
   a dangling temporary AST node.
4. Ensure the pending exception is visible to all evaluator callers before they turn a
   nested `false` into an ordinary side-effect/evaluation failure. This is already the
   rule used around `Info.PendingException` in comma/logical-expression evaluation.

The first implementation may instead expose a narrowly scoped `EvalInfo` method to
`ExprConstantMeta.cpp` through a private evaluator interface if constructing the
exception APValue requires more context. Do not make `EvalInfo` globally public merely
to pass a flag.

### Minimal source footprint

Expected implementation files, in order:

* `clang/include/clang/AST/ExprCXX.h`: add `ThrowFn` and the extra `ImplFn` parameter.
* `clang/include/clang/AST/Metafunction.h` and
  `clang/lib/AST/ExprConstantMeta.cpp`: thread the callback and centralize `Fail`;
  convert only the selected P3560 failure sites, preserving diagnostic mode.
* `clang/lib/AST/ExprConstant.cpp`: create the callback in
  `VisitCXXMetafunctionExpr`, implement pending-exception construction, and leave
  existing `VisitCXXThrowExpr`/catch propagation as the authority for unwinding.
* A small AST/Sema lookup helper only if the exception record cannot be found cleanly
  from `ASTContext`; avoid changing general Sema diagnostic code in the pilot.
* `libcxx/include/meta` only if the APValue construction reveals a missing constexpr
  constructor/layout property; the class itself is already present.
* A focused libc++ reflection test, preferably a new
  `meta-exception-strategy2.pass.cpp` or an extension of `exception.pass.cpp`.

No changes to ordinary `DiagFn` consumers outside reflection should be necessary.

## 3. Risk assessment

This is materially larger than strategy (1), but its blast radius is still narrower
than the M3 consteval self-reference escalation bug. Strategy (2) is entered only from
`CXXMetafunctionExpr`; the default callback remains diagnostic-only, and non-throwing
metafunction callers continue to receive the same `PartialDiagnostic` path. The main
shared risk is evaluator control flow: a pending exception must propagate through every
intermediate `return false` and must be cleared only by a matching constant-evaluation
handler. A missed check can turn a catchable reflection error into either a swallowed
failure or a spurious “not a constant expression” diagnostic.

The M3 bug changes context-wide immediate-invocation behavior in `SemaDeclCXX.cpp` and
`SemaExpr.cpp`; its previous attempted fix regressed nine libc++ tests because it affects
ordinary consteval/constexpr code. Strategy (2) does not alter those context gates or
ordinary Sema immediate-invocation processing. Its regression surface is therefore more
contained, but it still requires focused tests for nested calls, catch-all/catching the
exact type, uncaught exceptions, and ordinary hard-diagnostic callers.

The highest risks are:

* constructing an APValue with the exact `meta::exception` type/layout;
* preserving source-location/from-reflection metadata through nested evaluation;
* accidentally converting evaluator argument failures into reflection exceptions (only
  failures owned by the target metafunction should use `ThrowFn`);
* side effects in `define_enum`/`data_member_spec`/injection paths, where throwing after
  mutation would need rollback. `define_aggregate` must remain hard-failing, and the
  pilot should avoid all injection functions.

## 4. Pilot and phasing

### Pilot: `members_of`

Implement only the invalid-reflection branch of `members_of` first. It is a good proof
point because the wording is simple, the current failure is deep in the compiler
(`get_begin_member_decl_of` / `get_next_member_decl_of`), and it has no injection or
mutation. Use `members_of(^^int, access_context::unchecked())` as the failure case.

The end-to-end test should be a consteval function that catches the exception:

```cpp
consteval bool catches_members_error() {
  try {
    (void)std::meta::members_of(^^int,
                                std::meta::access_context::unchecked());
    return false;
  } catch (const std::meta::exception& e) {
    return e.from() == ^^std::meta::members_of &&
           e.u8what() == u8"invalid reflection operand";
  }
}
static_assert(catches_members_error());
```

Also include a nested caller (`outer()` calls `inner()`), an uncaught negative
`static_assert`/diagnostic check, and one valid `members_of(^^CompleteClass, ctx)` case.
The pilot is complete only when the exception is caught by user code, metadata survives
the nested call, the uncaught case reports through `CheckUncaughtException`, and the
ordinary non-throwing diagnostic test remains unchanged.

### Rollout order after the pilot

1. Convert the shared member-range backend: `bases_of`, `static_data_members_of`,
   `nonstatic_data_members_of`, then `annotations_of` and parameter range helpers.
2. Convert pure query failures: `type_of`, `parent_of`, `dealias`, `is_accessible`,
   `identifier_of`/`u8identifier_of`, and `return_type_of`.
3. Convert AST/APValue validation: `object_of`, `constant_of`, `extract`, and the
   `reflect_*` result functions.
4. Convert substitution only after Sema's nested diagnostic suppression and pending
   exception behavior has dedicated tests.
5. Keep `define_aggregate` outside the mechanism; separately decide whether the adopted
   P3491/data-member-description wording warrants a transactional design.

Each phase gets its own focused test and commit. After compiler changes, rebuild
`build-nyx` first; if libc++ tests are used, explicitly clean/rebuild `build-libcxx`
`cxx` so staged headers and the library are not stale. Use low parallelism under the
documented memory conditions, and compare full-suite results against the 23-failure
baseline rather than an assumed five-failure baseline.

