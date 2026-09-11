# Item 2 / issue #237 closeout report

Date: 2026-09-11

## Outcome

Fixed and verified. The already-landed primary fix in `a9c1f8aad1d6` was not changed.

The remaining `is_type_alias(^^ct)` failure was caused by `APValue::setReflection`.
`Sema::BuildCXXReflectExpr` correctly receives `ct` as a const `TypedefType`, but constructing the
reflection calls `unwrapReflectedType`, which historically desugared every cv-qualified alias.
Because `ct` aliases the const type of a `constexpr` closure object, it became a bare `RecordType`
before `is_alias()` evaluated it. `Type::isTypedefNameType()` is only a typedef/alias-template
type-class check; it does not treat closures specially.

The fix distinguishes cv that belongs to an alias declaration's underlying type from cv applied
outside an alias. It preserves the former, including `ct`, while retaining the established
normalization for `^^const Alias == ^^const T`. The permanent regression test now requires both
`is_type_alias(^^ct)` and `has_identifier(^^ct)`.

## Commands and results

```sh
ninja -C build-nyx -j$(nproc)
```

Succeeded. Full rebuilt compiler used for all following tests.

```sh
./build-nyx/bin/clang++ -std=c++26 -freflection-latest -nostdinc++ \
  -isystem build-libcxx/include/c++/v1 -fsyntax-only \
  docs/reflection-audit/repros/nbtv-237.cpp
```

Succeeded; both original assertions now pass.

```sh
./build-nyx/bin/llvm-lit clang/test/Reflection/ -v
```

Passed: 22/22.

```sh
ninja -C build-libcxx -t clean cxx
ninja -C build-libcxx -j$(nproc) cxx
```

Succeeded; libc++ explicitly rebuilt against the new compiler.

```sh
libcxx/utils/libcxx-lit build-libcxx -sv \
  libcxx/test/std/experimental/reflection/issue-237-closure-type-alias.pass.cpp
```

Passed: 1/1.

```sh
libcxx/utils/libcxx-lit build-libcxx -s libcxx/test/std/experimental/reflection/
```

Discovered 119 tests. Reported exactly the seven documented pre-existing failures:
`attributed-function-type-queries.pass.cpp`, `entity-proxies.pass.cpp`,
`entity-proxy-member-queries.pass.cpp`, `namespace-reflection-equality-reopened.pass.cpp`,
`m5-p2996-batch13.verify.cpp`, `m5-p2996-p3096-batch12.verify.cpp`, and
`m5-p3491-batch15.verify.cpp`. No new failures.

```sh
./build-nyx/bin/llvm-lit -v clang/test/Parser clang/test/SemaCXX \
  clang/test/SemaTemplate clang/test/AST clang/test/CodeGenCXX
```

Discovered 3814 tests. Reported exactly the documented five SemaCXX baseline failures:
`PR98671.cpp`, `builtin-is-within-lifetime.cpp`, `constant-expression-cxx11.cpp`,
`cxx2a-constexpr-dynalloc.cpp`, and `cxx2b-consteval-propagate.cpp`.
