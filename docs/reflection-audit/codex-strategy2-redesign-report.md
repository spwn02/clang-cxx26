# P3560R2 strategy (2) plumbing redesign report

Date: 2026-09-09

The evaluator plumbing was not retained. A narrowly scoped implementation attempt was made
and reverted after the focused pilot exposed a genuine evaluator crash.

The attempted design made `CXXMetafunctionExpr::DiagnoseFn` carry an optional `ThrowFn`. The callback received
the source location, message, reflection APValue, and Sema-backed `MetaActions`. Sema looks
up `std::meta::exception`, selects its `string_view` constructor, and builds a normal
`CXXConstructExpr` with the message and reflection arguments. Constant evaluation evaluates
that expression, preserving the real record layout and constexpr constructor semantics,
then records it as `EvalInfo::PendingException` with captured call-stack notes.

The attempted pending-exception change distinguished the diagnostic throw expression from the stable temporary
object key. User `throw` expressions keep using their operand as the key; evaluator-generated
exceptions use the owning metafunction expression. Reference catches therefore no longer
assume every pending exception originated at a `CXXThrowExpr`.

The sole pilot call site was the invalid-type branch of `members_of`; no other metafunction
failure path was changed. The temporary focused test added direct and one-level nested catches and checked
`what()`, `from()`, and `where()` while retaining the existing ordinary `throw` test.

Validation status: affected AST and Sema translation units compiled, and `clang` linked. The
focused test first failed to install an exception, then the constructor-selection adjustment
reached evaluation but caused `SIGABRT`. `coredumpctl info` identifies the assertion at
`extractSubobject` called from `handleLValueToRValueConversion`, with repeated
`HandleConstructorCall`/`VisitCXXInheritedCtorInitExpr` frames. This proves the synthesized
constructor expression is not currently a safe evaluator entry point. The implementation and
temporary test were reverted; no commit or push was made.
