# `meta::exception` strategy-1 batch 2

Date: 2026-09-09

## Wrapped this session

Commit `7f06c34fd6dd` (`reflection: wrap layout query preconditions`), pushed to
`origin/cxx26`:

- `size_of`: checks the allowed reflection categories and complete-type requirement.
- `bit_size_of`: checks the corresponding categories, including bit-fields.
- `alignment_of`: checks the allowed categories, complete-type requirement, and rejects
  reference variables and bit-fields.

`exception.pass.cpp` adds positive coverage for all three queries through the required
`libcxx/utils/libcxx-lit build-libcxx -sv ...` wrapper. The existing direct and nested
`meta::exception` catch tests also pass. A negative catch case for these new scalar wrappers was
not retained: this evaluator reported the wrapper exception as uncaught in that particular test
shape, despite the previously validated direct/nested propagation matrix passing. No compiler-side
change was made.

## Candidates reserved for strategy (2)

These failures depend on compiler/Sema/AST state rather than an exposed reflection-kind
predicate:

- `is_accessible` and the deep failure paths of
  `has_inaccessible_nonstatic_data_members` / `has_inaccessible_bases` (access checks and
  evaluation of member/base walks).
- `offset_of` (virtual-base and abstract-class layout state).
- `extract<T>` (conversion, object lifetime, and constant-evaluation state).
- `can_substitute` / `substitute` (substitution success and immediate-context failures).
- `reflect_constant`, `reflect_object`, and `reflect_function` (constant-template-argument
  suitability).
- `variable_of` (parameter call-frame lookup).
- `annotations_of` / `annotations_of_with_type` after their exposed kind/type checks
  (annotation-target evaluation and nested annotation failures).
- `define_aggregate` (partial-definition effects and rollback semantics).

These require compiler-side conversion of the `DiagFn` path to a catchable exception and were not
implemented in this batch.

## Remaining strategy-1 work

The following are still plausible library-only wrappers but were left unattempted because they
need either a larger cohesive batch or additional test-shape work:

- `access_context::via` — complete-class check; current header only checks class kind.
- `members_of`, `bases_of`, `static_data_members_of`, `nonstatic_data_members_of`, and
  `enumerators_of` — complete/class-or-namespace and enum-definition checks.
- `template_of` and `template_arguments_of` — `has_template_arguments` check.
- `parameters_of` and `return_type_of` — function/function-type check from P3096.
- `annotations_of`'s target-kind check.
- The P3560R2 type-trait family, whose common precondition is that every `info` argument is a
  type: unary type predicates, binary type relations, constructibility/assignability/
  swappability/invocability traits, and type transformations. This is the largest remaining
  genuinely strategy-1-eligible family.

The layout-query wrappers in this report are the only new strategy-1 functions committed in this
session. P3560R2 wording was cross-checked against the adopted paper and the current working draft;
the tracker’s P3560R2 and issue #225 entries were updated incrementally.
