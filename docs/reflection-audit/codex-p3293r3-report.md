Piece 1 complete and committed.

- Implemented header-only `std::meta::subobjects_of`.
- Added order/equivalence regression test.
- Updated tracker and report.
- Focused test passed: 1/1.
- Piece 2 remains documented but untouched.

Commit: `3cc388c9425e`

[Report](</home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-p3293r3-report.md>)

Piece 2 implementation (working tree, 2026-09-09):

- `BuildReflectionSpliceExpr` now accepts non-virtual `ReflectionKind::BaseSpecifier` values and
  creates an lvalue splice model carrying the reflected base type.
- The splice-member path in `SemaExprMember.cpp` recognizes the reflected base and produces the
  normal checked `CK_DerivedToBase` implicit cast, including the compiler-computed `CXXCastPath`
  and source cv-qualification/value category.
- Virtual base relationships and array-element operands are rejected explicitly. Other reflection
  kinds continue through the existing `err_unexpected_reflection_kind_in_splice` path.
- Added `libcxx/test/std/experimental/reflection/base-splice.verify.cpp`, covering base member
  read/write, virtual-base rejection, array-element rejection, and non-base rejection.
- Focused `libcxx-lit` test passed (1/1). `ninja -C build-nyx -j1 clang` passed. Full Clang gate
-  completed after refreshing stale test-consumer binaries: 44,612 passed, 25 expected failures,
  5,171 unsupported, and 8 failures. Five are the established baseline failures (`PR98671.cpp`,
  `builtin-is-within-lifetime.cpp`, `constant-expression-cxx11.cpp`,
  `cxx2a-constexpr-dynalloc.cpp`, and `cxx2b-consteval-propagate.cpp`); three were stale
  `clang-repl`/`clang-check` PCH consumers. Rebuilt those tools and reran them plus all 16 Clang
  reflection tests: 21/21 passed.
