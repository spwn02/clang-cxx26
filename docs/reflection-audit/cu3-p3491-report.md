# CU3 P3491R3 static-storage report — 2026-09-11

## Result

Items 11–13 are complete and uncommitted.

- `std::define_static_object(T&&)` implements P3491R3's class/template-parameter-object path and
  its one-element-array path for non-class values.
- `std::is_string_literal` has all five specified pointer overloads. Its compiler metafunction
  identifies a string literal by the evaluated pointer's `StringLiteral` lvalue base, including a
  pointer to a literal subobject.
- `meta::reflect_constant_string` and `define_static_string` are generic input-range templates for
  all five character types. Literal ranges keep their terminating zero; other ranges receive one.
- Array promotion now requires structural, constructible, copy-constructible elements. The
  item-8 nested-array extension applies structural/copy checks recursively to leaf elements because
  an array row is not itself a structural type.

P3491R3's published feature-test value is `__cpp_lib_define_static == 202506L`; `<meta>` now
defines it and the new regression test asserts the value. `libcxx/include/version` is generated
from upstream's feature-test metadata and currently has no reflection-aware feature gate, so it was
not hand-edited or regenerated for this fork-specific experimental header.

## Regression coverage

`libcxx/test/std/experimental/reflection/p3491-static-storage.pass.cpp` checks all five character
types, literal-subobject positive and ordinary-array negative results, no double terminator,
`define_static_string`, and scalar/class `define_static_object`. The existing P3491 verify batch
now checks rejection of a copyable non-structural array element. The item-8 nested-array test was
run explicitly after the new mandate check.

## Commands and results

| Command | Result |
| --- | --- |
| `ninja -C build-nyx -j$(nproc)` | Passed (full rebuild). |
| `ninja -C build-libcxx -t clean cxx && ninja -C build-libcxx -j$(nproc) cxx` | Passed (clean rebuild, 1967 steps). |
| `libcxx/utils/libcxx-lit build-libcxx -sv libcxx/test/std/experimental/reflection/p3491-static-storage.pass.cpp` | Passed. |
| `libcxx/utils/libcxx-lit build-libcxx -sv libcxx/test/std/experimental/reflection/m5-p3491-p3560-p3795-batch16.verify.cpp` | Passed. |
| `libcxx/utils/libcxx-lit build-libcxx -sv libcxx/test/std/experimental/reflection/cwg3111-lwg4432-reflect-constant-array.pass.cpp` | Passed. |
| `./build-nyx/bin/llvm-lit clang/test/Reflection/ -v` | Passed. |
| `libcxx/utils/libcxx-lit build-libcxx -s libcxx/test/std/experimental/reflection/` | Passed; 115 discovered tests. |
| `./build-nyx/bin/llvm-lit clang/test/Parser/ clang/test/SemaCXX/ clang/test/SemaTemplate/ clang/test/AST/ clang/test/CodeGenCXX/ -v` | 3814 tests; exactly five documented pre-existing SemaCXX failures: `PR98671.cpp`, `builtin-is-within-lifetime.cpp`, `constant-expression-cxx11.cpp`, `cxx2a-constexpr-dynalloc.cpp`, `cxx2b-consteval-propagate.cpp`. |

No commit or push was performed.
