# P3560R2 `meta::exception` strategy-(1) batch 4

Implemented and tested batch 4 on 2026-09-09.

## Completed

Added library-side `meta::exception` precondition checks for:

- `offset_of`
- `operator_of`
- `subobjects_of`

Each check runs before `__metafunction`. The checks use only existing public
reflection predicates: `is_nonstatic_data_member`, `is_base`,
`is_operator_function`, `is_operator_function_template`, and `is_class_type`.
Positive coverage was added to `exception.pass.cpp`; `libcxx-lit` passed 1/1
after each change.

## Cumulative status

Thirteen unique functions are now wrapped across all strategy-(1) sessions:

`size_of`, `bit_size_of`, `alignment_of`,
`has_inaccessible_nonstatic_data_members`, `has_inaccessible_bases`,
`annotations_of_with_type`, `template_of`, `template_arguments_of`,
`access_context::via`, `enumerators_of`, `offset_of`, `operator_of`, and
`subobjects_of`.

The earlier member-query and parameter-query guards were reverted and are not
included in this count; their preconditions require compiler-side state.

## Remaining candidates

No additional candidate is cleanly eligible under strategy (1)'s strict rule.
`source_location_of` and `has_identifier` have no corresponding failing
precondition. `identifier_of` and `u8identifier_of` need more than
`has_identifier` for the specified ambiguous-name/encoding cases. `type_of`,
`object_of`, `constant_of`, `extract`, and `is_accessible` depend on reflected
entity details, lifetime, compatibility, or constant-evaluation state that is
not represented by one existing library predicate. `define_static_string`,
`define_static_array`, `define_static_object`, `reflect_constant`,
`reflect_object`, and `reflect_function` have template Mandates or validity
conditions whose current constraints/overload structure cannot be safely
converted to `meta::exception` with a header-only predicate check.

## Next step

Strategy (2) remains the right next step: compiler-side `DiagFn` rewiring for
failures requiring Sema/AST-internal state, especially member and parameter
queries, access checks, substitution/extraction failures, and annotation or
constant-evaluation failures. Strategy (1) has now covered most/all of the
surface that can be handled safely without compiler changes. The epic can move
to strategy (2), or mark P3560R2 complete as far as strategy (1) permits.
