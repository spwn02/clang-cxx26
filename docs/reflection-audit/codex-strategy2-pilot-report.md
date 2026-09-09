# P3560R2 strategy (2) pilot report

Date: 2026-09-09

## Result

The `members_of` pilot was not implemented. Investigation found two concrete
obstacles in the recommended design as currently written. No compiler or test
files were changed, so no build or full `check-clang` gate was run.

The recommended pilot remains `members_of`, and the recommended dual failure
sink remains the right general direction. Strategy (2) is not fundamentally
invalid, but the design document needs a small evaluator-interface revision
before implementation can safely begin.

## Obstacle 1: the pending-exception throw-site contract

`EvalInfo::PendingExceptionInfo` stores `const Expr *ThrowExpr`. The existing
constant-evaluation catch path in `clang/lib/AST/ExprConstant.cpp` treats this
as a `CXXThrowExpr` when the handler catches by reference:

```cpp
const Expr *ObjKey = cast<CXXThrowExpr>(Exc.ThrowExpr)->getSubExpr();
```

A compiler-side metafunction failure has no source `throw` expression. The
design proposes either a synthetic evaluator-owned expression or a stable
metafunction expression. Neither satisfies that cast: a synthetic expression
would need a real thrown-object subexpression, while using the
`CXXMetafunctionExpr` itself would assert/crash on the required
`catch (const std::meta::exception&)` test. This directly prevents the
design's own end-to-end pilot from working with the current propagation code.

The fix should make pending exceptions carry a separate stable object key (or
otherwise teach the reference-catch path to bind the stored APValue without
assuming `CXXThrowExpr`). The diagnostic expression/range should remain a
separate field. A plain cast change is insufficient because the reference
binding needs the exception object's static type and stable temporary identity.

## Obstacle 2: constructing the exception APValue

The proposed `ThrowFn(SourceLocation, StringRef, APValue From)` callback can
signal failure, but the callback boundary exposes no evaluator operation that
constructs an arbitrary `std::meta::exception` object from those APValues.
`EvalInfo`, `Evaluate`, and `PendingExceptionInfo` are private implementation
details in `ExprConstant.cpp`; `ExprConstantMeta.cpp` cannot use them directly.

Constructing a `CXXConstructExpr` in the callback is not an existing API-level
solution. It requires semantically converted AST arguments for the selected
`std::meta::exception` constructor, including a string-view argument and a
reflection argument synthesized from an APValue. The current metafunction
callback has neither a Sema conversion path nor an APValue-to-expression
adapter. Hand-populating `APValue` fields would violate the design's explicit
requirement to use the constexpr constructor and record layout, and would be
fragile for `optional<string>`, `u8string`, and `source_location` layout.

The fix should add a narrow evaluator-private operation, as the design allows,
that constructs the exception through the existing constexpr constructor path
and returns/installs the resulting APValue. That operation must also provide a
stable object key for obstacle 1 and locate the canonical `std::meta::exception`
record from the translation unit that supplied `<meta>`.

## Recommended next design revision

Extend the private evaluator interface with a semantic operation along these
lines:

```cpp
bool BuildAndPendMetaException(const CXXMetafunctionExpr &,
                               SourceLocation, StringRef, APValue From);
```

Have `PendingExceptionInfo` store both the diagnostic/throw location and a
stable exception-object key (or an evaluator-owned equivalent), and update
reference-catch binding to use that key. Then thread the dual `ThrowFn` sink
through `Metafunction` and convert only the invalid-reflection branch of
`get_begin_member_decl_of` first. Re-run the mandated catch, nested propagation,
uncaught-diagnostic, valid-member, and ordinary-diagnostic tests before any
additional rollout.

## Validation

- Read `REFLECTION_GAPS.md` Ground Truth and the complete strategy design.
- Confirmed the pilot is `members_of` and the required invalid operand is
  `members_of(^^int, access_context::unchecked())`.
- Confirmed the existing reference-catch cast at
  `clang/lib/AST/ExprConstant.cpp:6649` (line may move after edits).
- Confirmed no source changes were made and the worktree remained clean before
  this report was added.
- Full `check-clang` was intentionally not run because no implementation was
  produced to validate and the documented memory pressure makes an unrelated
  baseline run unsafe here.
