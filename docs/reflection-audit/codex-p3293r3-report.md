# P3293R3 implementation report

## Piece 1: `subobjects_of`

Implemented `std::meta::subobjects_of(info, access_context)` in
`libcxx/include/meta`. It returns `bases_of` followed by
`nonstatic_data_members_of`, preserving both source orders and using the
existing access filtering of those prerequisite queries.

Added `subobjects-of.pass.cpp`, verifying result size, exact equality with
manual concatenation, and element order for a class with two bases and two
nonstatic data members.

## Piece 2: base-subobject splicing

Not started. `Sema::BuildReflectionSpliceExpr` still rejects
`ReflectionKind::BaseSpecifier` with `err_unexpected_reflection_kind_in_splice`
in `clang/lib/Sema/SemaReflect.cpp`. A continuation must implement
`obj.[:base_reflection:]` using ordinary base-to-derived conversion semantics,
then add positive read/write tests and negative coverage for non-base
reflections, virtual bases, and array elements.

## Verification

`libcxx/utils/libcxx-lit build-libcxx -j1 -sv
libcxx/test/std/experimental/reflection/subobjects-of.pass.cpp` passed.

No Clang sources changed, so the full check-clang gate was not applicable to
piece 1.
