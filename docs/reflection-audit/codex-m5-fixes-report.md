# M5 Reflection Fixes Report

Date: 2026-09-10

## NEW-4 — 2996-19 — Fixed-and-verified

Changed `clang/lib/AST/ExprConstantMeta.cpp:400-404,1004,5008-5028` to expose an internal
closure-type query backed by `CXXRecordDecl::isLambda()`. `libcxx/include/meta:2452-2457` now
rejects closure types through the existing `meta::exception` wrapper path.

Test: `libcxx/test/std/experimental/reflection/new-4-closure-inaccessible.verify.cpp`, with a
closure negative control and valid ordinary-class positive control.

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only -Xclang -verify -Xclang -verify-ignore-unexpected=note libcxx/test/std/experimental/reflection/new-4-closure-inaccessible.verify.cpp
```

Output: exit 0, no output. Status: Fixed-and-verified. Commit was blocked by the read-only
`.git/index.lock` filesystem.

## NEW-6 — 2996-48 — Fixed-and-verified

Changed `clang/lib/Sema/SemaReflect.cpp:1839-1843` to reject a reflected constructor in the
splice-type path using the existing `err_unexpected_reflection_kind_in_splice` diagnostic.
Destructor handling was not changed.

Test: `libcxx/test/std/experimental/reflection/new-6-constructor-splice.verify.cpp`, using a
constructor reflection obtained from `members_of`, plus a valid type splice.

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only -Xclang -verify libcxx/test/std/experimental/reflection/new-6-constructor-splice.verify.cpp
```

Output: exit 0, no output. Status: Fixed-and-verified. Commit was blocked by the read-only
`.git/index.lock` filesystem.

## NEW-7 — 2996-49 — Deferred-with-reason

Traced the dependent form through `clang/lib/Sema/SemaReflect.cpp:1683-1690`, where it becomes a
`DependentReflectionSpliceType`, and through later declaration initializer/deduction handling.
No narrow rejection hook was found that distinguishes the forbidden CTAD-like
`[:R:] value = {1}` case from valid dependent direct-list initialization. A broad rejection at
splice-type construction would risk valid dependent declarations. No implementation or test was
added. Status: Deferred-with-reason.

Verification command (reproducer confirmation):

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only /tmp/reflection-new7.cpp
```

Output: exit 0, no output for the known accepted reproducer.

## NEW-2 — 3394-03 — Fixed-and-verified

Changed `clang/lib/Sema/SemaDeclAttr.cpp:2296-2301` to reject C++26 annotations attached to
`EmptyDecl` using the standard invalid-declaration attribute diagnostic.

Test: `libcxx/test/std/experimental/reflection/new-2-empty-declaration.verify.cpp`, with
`[[=1]];` negative control and a valid annotated variable.

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fannotation-attributes -fsyntax-only -Xclang -verify libcxx/test/std/experimental/reflection/new-2-empty-declaration.verify.cpp
```

Output: exit 0, no output. Status: Fixed-and-verified. Commit was blocked by the read-only
`.git/index.lock` filesystem.

## NEW-3 — 3394-05 — Fixed-and-verified

Changed `libcxx/include/meta:2287-2289` so `annotations_of_with_type` requires the filter to be
both a type reflection and a complete type, matching P3394R4's Constant When requirement.

Test: `libcxx/test/std/experimental/reflection/new-3-annotations-with-type.verify.cpp`, with
`^^void` negative control and complete `^^int` positive control.

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only -Xclang -verify -Xclang -verify-ignore-unexpected=note libcxx/test/std/experimental/reflection/new-3-annotations-with-type.verify.cpp
```

Output: exit 0, no output after refreshing staged libc++ headers. Status: Fixed-and-verified.
Commit was blocked by the read-only `.git/index.lock` filesystem.

## NEW-8 — 3096-06 — No fix justified

P3096R12 does not impose a parameter-only domain on all three named functions. It specifies
`has_identifier` for entities generally and makes `identifier_of`/`u8identifier_of` conditional on
`has_identifier(r)`; parameter reflections receive additional naming rules. Existing accepted
type-reflection results are therefore conforming. `type_of(^^S)` already rejects as independently
verified. No implementation or test change was made; tracker/checklist note records the corrected
interpretation.

## Full-gate status

The required libc++ wrapper gate was launched with `-j1` and escalation:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 build-nyx/tools/clang/test
```

The sandboxed attempt failed before tests because Python’s forkserver socket was denied. The
escalated run completed its full discovery and reported 48,517 total tests, 25,231 passed,
23 failed, and 23,263 skipped; the 23 failures matched the documented baseline clusters. The
corresponding libc++ wrapper gate was launched as:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test
```

It discovered 11,832 tests but the environment marked 11,826 unsupported/skipped and ran six;
all six passed before the long-running gate was interrupted. No new libc++ failure was observed,
but this is not a complete executed check-cxx population because of the configuration’s skip set.
