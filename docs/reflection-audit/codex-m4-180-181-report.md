# M4 design investigation: issues #180 and #181

Date: 2026-09-10

Disposition: no source fix landed. The investigation confirms that upstream PR
#261 is a real, applicable expansion-body deferral design, but it is not a
complete or safely scoped fix for both issues in this fork. #181 additionally
requires correction of tuple/range-element binding and #180's published
reproducer is not itself a `template for` body case.

## 1. Upstream PR #261

The exact upstream PR was fetched as commit `1fdd67b76362` from
`bloomberg/clang-p2996` (`refs/pull/261/head`). The PR page is
<https://github.com/bloomberg/clang-p2996/pull/261>.

Its stated design is to compute expansion size first and instantiate the body
afterwards, avoiding premature body instantiation and spurious diagnostics. The
patch:

* changes `ExpansionStmtDecl` to store `Stmt *`, because deferred processing
  can temporarily retain a non-`CXXExpansionStmt` transformed result;
* adds `FinishCXXExpansionStmt(Stmt *, UnsignedOrNone)`;
* makes `TreeTransform` transform the expansion heading/size while retaining
  the body for later substitution; and
* expands the retained body only after a concrete size is available.

The patch touches 11 files, including AST declaration storage, parsing,
serialization, Sema, template instantiation, and `TreeTransform`. Its added
tests cover a nested `template for` over annotations and non-constant expansion
size/label diagnostics. It does not add a regression for either #180 or #181.

The architecture is still recognizable in this fork: the current
`FinishCXXExpansionStmt(Heading, Body)` in `clang/lib/Sema/SemaExpand.cpp:551`
immediately stores the body and, for a known size, substitutes a combined
range-variable/body once per index (`:577-613`). The current
`TreeTransform` expansion visitors transform the body before calling that
finish function (`clang/lib/Sema/TreeTransform.h:9336-9343`, with analogous
code for iterable, destructurable, and init-list expansions). The current
`VisitExpansionStmtDecl` also assumes the stored statement is a
`CXXExpansionStmt` (`clang/lib/Sema/SemaTemplateInstantiateDecl.cpp:2177-2188`).

Therefore PR #261 is not stale in concept. It is a substantial AST/Sema/
TreeTransform port against a newer upstream base, not a cherry-pick-sized local
diagnostic fix. The fork's surrounding code is close enough that an adaptation
is plausible, but it needs its own build and expansion regression gate.

## 2. P1306R5 wording

Adopted wording and explanatory material were fetched from
<https://wg21.link/P1306R5>.

The key semantic rule is that an expansion statement specifies repeated
instantiations of its compound substatement. The wording's equivalence model
forms one block per element, each containing a fresh for-range declaration and
the compound body. Thus an iteration is not execution of one shared body AST;
it is a separate substitution/instantiation with that iteration's selected
element.

For ranges, the model first binds `constexpr_{opt} auto&& __range` and obtains
the element as `*(__begin + i)`. For tuple/destructuring expansion, it first
binds `constexpr_{opt} auto&& [__v_0, ...]` to the initializer. The selected
element is `__v_i` when the referenced element is an lvalue reference or the
initializer is an lvalue; otherwise it is `std::move(__v_i)`. This is the
value-category rule that prevents an accidental copy of a non-copyable tuple
element.

Consequences:

* A dependent `static_assert` in the body must be checked during each concrete
  body instantiation, after the iteration variable has been substituted. A
  body retained only as a single already-transformed dependent AST can miss
  that check; a body eagerly transformed in the wrong context can diagnose too
  early.
* The expansion variable and the hidden range/structured-binding variables
  must preserve reference and value category. Copying the range while creating
  the hidden binding is already wrong before body substitution begins.

## 3. Fork implementation trace

The fork uses an `ExpansionStmtDecl` as a declaration context solely to provide
template depth and lookup context. Parsing creates that context, parses the
`for` heading and body inside it, and records the resulting expansion statement
(`clang/lib/Parse/ParseStmt.cpp:257-286`). Sema then selects one of the
indeterminate, iterable, destructurable, or init-list AST nodes.

For a known-size expansion, the current lowering is:

1. `FinishCXXExpansionStmt` combines the expansion-variable declaration and
   body into one `CompoundStmt`.
2. It creates a fresh `MultiLevelTemplateArgumentList` containing the index.
3. It calls `SubstStmt(CombinedBody, MTArgList)` once per index.
4. It stores the resulting statements in the `CXXExpansionStmt` instance list.

That is a separate `SubstStmt` call per iteration, but it is preceded by body
transformation in `TreeTransform`, and it relies on the expansion declaration
already holding a concrete `CXXExpansionStmt`. Nested/dependent expansion
contexts therefore have two interacting transformations: the outer template
instantiation transforms the body, then the expansion lowering substitutes it
again. PR #261 removes that premature body transformation and delays the
concrete expansion result until size is known.

The tuple path has an independent binding defect. In
`clang/lib/Sema/SemaExpand.cpp:245-285`,
`makeCXXDestructurableExpansionSelectExpr` creates an `auto` decomposition and
only turns it into a reference when the written expansion variable is itself a
reference. The code explicitly carries `// TODO: Add ref support`. The issue
#181 report confirms that `std::tuple<std::unique_ptr<int>, char, double,
float>` fails for all tested forms (`auto`, `auto&`, `auto&&`, and const forms)
with a deleted copy constructor. The issue is
<https://github.com/bloomberg/clang-p2996/issues/181>.

The current range helper also creates a const hidden `__range` (`QT =
Range->getType().withConst()` around `SemaExpand.cpp:150`), which is another
binding detail that must be compared against the final P1306 desugaring before
changing it. A body-deferral-only port cannot establish the required tuple
reference/value-category semantics.

The published #180 reproducer is also important to distinguish. It reflects
and substitutes `test_impl`, whose definition contains `static_assert(false)`;
the substituted function is then invoked through a reflected member pointer.
It is not literally a `static_assert` inside a `template for` body. The issue is
<https://github.com/bloomberg/clang-p2996/issues/180>. The observed behavior is
still consistent with an under-instantiated/deferred template body problem,
but attributing it solely to expansion-body lowering would require a rebuilt
minimal reproducer that includes the expansion path.

## 4. Fix decision and test scope

No implementation was attempted. Porting #261 would be a broad shared
machinery change across 11 files, and it does not address #181's independent
binding defect. Combining it with a new binding implementation would make a
two-root-cause change difficult to validate and risks the known expansion
regressions in this epic.

Existing positive expansion coverage was inventoried before making this
decision. It includes `clang/test/SemaCXX/cxx2c-expansion-stmts.cpp`, the
expansion diagnostics tests under `clang/test/Reflection/`, and the expansion
uses in `libcxx/test/std/experimental/reflection/`, including
`miscellaneous.pass.cpp`, nested reflection loops, and the lambda-capture
regression. No new `.verify.cpp`/`.pass.cpp` tests were added because there is
no candidate source change to gate; adding tests that merely encode current
failures would not validate a fix.

The correct future implementation gate should include:

* a deferred-body regression for the exact #180 substitution scenario and a
  dependent per-iteration `static_assert` case;
* #181's exact non-copyable tuple reproducer, with `auto`, `auto&`, and
  `auto&&` controls and checks for mutation/reference preservation;
* existing Clang expansion tests;
* the complete libc++ reflection directory through `libcxx-lit`, with the
  documented seven-test libc++ baseline excluded; and
* stale-binary/PCH checks and low-parallelism retries per the Ground Truth
  section before interpreting any full-suite result.

## 5. Tracker update

Rows #180 and #181 were updated to record the PR commit/design, the fork code
locations, the independent tuple-binding root cause, and why the upstream
design cannot safely be landed as a combined fix without a dedicated port and
regression gate.
