# M5 diagnostic batch 5 report

Date: 2026-09-10

## P3096R12 — Function Parameter Reflection

Added [`m5-p3096-batch5.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p3096-batch5.verify.cpp).

- 3096-01: invalid `parameters_of` domain is rejected by constant-evaluation diagnostics.
- 3096-02: invalid `return_type_of` domain is rejected by constant-evaluation diagnostics.
- 3096-03: invalid `variable_of` domain is rejected by constant-evaluation diagnostics.
- 3096-04: `has_ellipsis_parameter` returns false for a non-function reflection.
- 3096-05: `has_default_argument` returns false for a non-parameter reflection.
- 3096-07: existing `clang/test/Reflection/lift-operator.cpp` covers the requires-expression
  local-parameter rejection.
- 3096-06 remains Needs-New-Test. Ordinary declaration queries are valid outside parameter
  reflections, so no unsupported rejection was fabricated.

Confirmed runs:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p3096-batch5.verify.cpp
Total Discovered Tests: 1; Passed: 1 (100.00%)

./build-nyx/bin/llvm-lit -j1 clang/test/Reflection/lift-operator.cpp -v
Total Discovered Tests: 1; Passed: 1 (100.00%)
```

Commit: `a09c5efbac2b` (`reflection: cover P3096 diagnostic conditions`), pushed to
`origin/cxx26`; `git log --oneline origin/cxx26..HEAD` was empty after the push.

## P3394R4 — Annotations for Reflection

Added [`m5-p3394-batch5.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p3394-batch5.verify.cpp).

- 3394-01: non-structural annotation operand is rejected.
- 3394-02: existing `annotations-regression.verify.cpp` coverage retained.
- 3394-04: mixed annotation/ordinary attribute specifier is rejected.
- 3394-06: repeated annotation order is checked; existing accumulation coverage retained.
- 3394-03 exposed NEW-2: `[[=1]];` is accepted although empty-declaration annotations are
  prohibited by P3394R4.
- 3394-05 exposed NEW-3: invalid `annotations_of_with_type` item/type probes are accepted and
  produce empty results rather than the required diagnostic.

NEW-2 and NEW-3 were documented in `docs/REFLECTION_GAPS.md` and the checklist. No passing
`-verify` test was fabricated for either implementation gap.

Confirmed run:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p3394-batch5.verify.cpp
Total Discovered Tests: 1; Passed: 1 (100.00%)
```

Commit: `b1cfe48f305d` (`reflection: cover P3394 annotation diagnostics`), pushed to
`origin/cxx26`; `git log --oneline origin/cxx26..HEAD` was empty after the push.

## Checklist status

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 37 | 53 | 18 | 1 |

New gaps this session: NEW-2 and NEW-3. Existing NEW-1 was not re-investigated.
