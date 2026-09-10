# CWG 3111 / LWG 4432 audit

Date: 2026-09-10

## Verdict

The proposed change does not pass verification and needs reconsideration. The
flat-array path becomes reachable after changing `reflect_constant` to take
`const T&`, but the implementation cannot currently represent the requested
nested-array case through its existing substituted-global-array mechanism.
The working tree has been restored to the prior session's intended diff; no
experimental workaround remains.

## Builds and test counts

`ninja -C build-nyx clang -j$(nproc)` initially stopped because ccache could
not write its read-only cache. The same build succeeded with `CCACHE_DISABLE=1`.

| Gate | Before | After intended diff | Result |
|---|---:|---:|---|
| New `cwg3111-lwg4432-reflect-constant-array.pass.cpp` | N/A (new) | 0 passed, 1 failed | failed |
| `libcxx/test/std/experimental/reflection/` | 108 discovered: 100 passed, 7 failed, 1 unsupported | 109 discovered: 99 passed, 9 failed, 1 unsupported | 2 additional failures |
| `static-arrays.pass.cpp` | 1/1 passed | 1/1 passed | no regression |
| `clang/test/Reflection/` | 20/20 passed (documented baseline) | 20/20 passed | no regression |

The two additional libc++ failures are the new test and
`m5-p2996-batch1.verify.cpp`. The latter is an existing direct caller: changing
the parameter from by-value to reference suppresses the expected deleted-copy
diagnostic for `NonCopyable`. A temporary local-copy experiment restored the
copy attempt but moved/duplicated diagnostics and caused broader regressions;
it was reverted.

The seven other subtree failures are exactly the documented pre-existing
baseline: `attributed-function-type-queries.pass.cpp`, `entity-proxies.pass.cpp`,
`entity-proxy-member-queries.pass.cpp`, `namespace-reflection-equality-reopened.pass.cpp`,
`m5-p2996-batch13.verify.cpp`, `m5-p2996-p3096-batch12.verify.cpp`, and
`m5-p3491-batch15.verify.cpp`.

## Mechanical fixes attempted

1. `libcxx/include/meta:1622`: admitted array types in the
   `reflect_constant` constraint, because `is_structural_type(^^T)` is false
   for array types and otherwise prevents the new branch from being selected.
2. `libcxx/include/meta:1611` and `:2834`: admitted array-valued range elements
   to `reflect_constant_array`, allowing nested arrays to reach recursive
   reflection.
3. The new test's extraction checks were temporarily changed to the fork's
   pointer extraction convention. These changes were reverted because they did
   not solve nested arrays and are not part of the retained diff.

Items 1 and 2 exposed the fundamental failure below and were reverted. The
retained working-tree diff is again only the prior session's change.

## Design blocker

With nested input `const int[2][2]`, recursive reflection reaches
`reflect_constant_array` with `ValTy = int[2]`. Its backing template is:

```cpp
template <typename ValTy, ValTy... Vals>
inline constexpr ValTy FixedArray[sizeof...(Vals)] = {Vals...};
```

For an array `ValTy`, the non-type template parameter pack is adjusted to a
pointer parameter. Substitution then fails with:

```text
value of type 'const int[2]' is not implicitly convertible to 'int *'
```

Therefore the current mechanism can handle scalar/ordinary element values but
cannot preserve nested array element type. Relaxing the range constraint only
makes this failure visible; changing extraction syntax or adding casts cannot
fix it. A follow-up needs a representation/design that can materialize nested
array objects (or compiler support for the required array template-parameter
object semantics). The array branch must not be claimed complete until that is
resolved.

## Caller audit

Searched `libcxx/include`, `libcxx/test`, and `clang/test` for plain
`reflect_constant` calls across 36 files, excluding `reflect_constant_array`
and `reflect_constant_string`. Existing callers otherwise remained within the
documented seven-failure baseline. `m5-p2996-batch1.verify.cpp` is the concrete
signature-behavior regression described above.

No commit or push was performed.

## Follow-up verification — 2026-09-10

The targeted fixes were rebuilt with `CCACHE_DISABLE=1`; clang was already
up-to-date (`ninja -C build-nyx clang -j$(nproc)` reported no work). The new
array test and `m5-p2996-batch1.verify.cpp` each pass 1/1 when run alone
through the libc++ wrapper. The latter again reports the `2996-01` deleted
copy constructor diagnostic at the caller, with no unexpected diagnostics.

The final reflection-subtree result is:

| Gate | Before intended diff | Prior intended diff | Follow-up final diff |
|---|---:|---:|---:|
| Reflection subtree | 108: 100 passed, 7 failed, 1 unsupported | 109: 99 passed, 9 failed, 1 unsupported | 109: 101 passed, 7 failed, 1 unsupported |
| New array test | N/A | 0 passed, 1 failed | 1 passed, 0 failed |
| `m5-p2996-batch1.verify.cpp` | 1 passed | 0 passed, 1 failed | 1 passed, 0 failed |
| `static-arrays.pass.cpp` | 1/1 | 1/1 | 1/1 |
| `clang/test/Reflection/` | 20/20 | 20/20 | 20/20 |

The seven subtree failures are exactly the documented baseline:
`attributed-function-type-queries.pass.cpp`, `entity-proxies.pass.cpp`,
`entity-proxy-member-queries.pass.cpp`,
`namespace-reflection-equality-reopened.pass.cpp`,
`m5-p2996-batch13.verify.cpp`, `m5-p2996-p3096-batch12.verify.cpp`, and
`m5-p3491-batch15.verify.cpp`. The new test is therefore a net discovered test
with zero new subtree failures. The first full-subtree attempt briefly showed
eight failures because an intermediate overload split made flat-array calls
ambiguous; that was corrected before the final run.

Copy-constructibility Mandate regression: resolved. Keeping non-array
`reflect_constant` by-value preserves the original caller-site diagnostic;
materializing a local or temporary copy inside the `const T&` overload instead
created extra header/constant-expression diagnostics and was not retained.

Nested-array hard-failure: not fully resolved by the final overload design.
The flat-array overload is more specialized and nested arrays no longer reach
`reflect_constant_array`'s `FixedArray` substitution, but a direct probe with
`int[2][2]` still selects the legacy by-value overload after array-to-pointer
decay and compiles as a pointer reflection. Thus the intended clean SFINAE
rejection for nested arrays is not achieved. Preventing that decay while
retaining the exact by-value Mandate diagnostic requires a deeper overload/API
design; no non-SFINAE poison-pill workaround was added.

No commit or push was performed.
