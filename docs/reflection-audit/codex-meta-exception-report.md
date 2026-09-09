# P3560R2 `meta::exception` report

Date: 2026-09-09

Implemented `std::meta::exception` in `libcxx/include/meta` with the P3560R2 synopsis:

- UTF-8 and ordinary-encoding consteval constructors;
- `what`, `u8what`, `from`, and `where` accessors;
- defaulted copy/move construction and assignment;
- inheritance from `std::exception`.

The C++26-facing `std::exception` base destructor is constexpr, which is required to form the
derived consteval-only object. The pre-C++26 ABI keeps its existing out-of-line destructor.

`exception.pass.cpp` tests both constructors, all accessors, and direct throw/catch in a consteval
function. The focused libc++ test passes.

Strategy (1) began for the three LWG 4428 functions. Their exposed type/class checks now throw
`meta::exception` in the header before compiler metafunction dispatch:

`has_inaccessible_nonstatic_data_members`, `has_inaccessible_bases`, and
`annotations_of_with_type`.

Future work should continue strategy (1) across the remaining Throws-bearing functions whose
preconditions can be checked from existing predicates. Then strategy (2) must add a compiler-side
throw mode for `DiagFn` failures that arise deep in Sema/AST logic, especially access checks and
substitution. The three wrapper paths need a compiler regression test once nested consteval
exception propagation is supported; this evaluator currently reports an uncaught exception when a
called consteval function throws, even inside the caller's try/catch.

Strategy (2) still looks like the correct long-term plan. The implementation review found no
library-only way to recover failures discovered only after compiler dispatch, and `DiagFn` is
architecturally a diagnostic callback rather than a throwable value. Strategy (1) is useful as a
low-risk first tranche, but it cannot replace a compiler-side throw mode for the full API.
