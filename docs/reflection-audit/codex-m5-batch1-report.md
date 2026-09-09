# M5 diagnostic batch 1 report

Date: 2026-09-09

Batch selection followed the checklist execution plan: the first four ordered
`Needs-New-Test` rows, P2996R13 2996-01 through 2996-04.

## Covered conditions

All four conditions are covered by
[`m5-p2996-batch1.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch1.verify.cpp):

- 2996-01: non-copy-constructible `reflect_constant` argument.
- 2996-02: explicitly supplied reference type to `reflect_constant`.
- 2996-03: non-representable pointer value passed to `reflect_constant`.
- 2996-04: function passed to `reflect_object`, violating its object-type constraint.

The test was actually run and passed with:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch1.verify.cpp
```

Result: `Total Discovered Tests: 1; Passed: 1 (100.00%)`. The run used lit's
`-verify` invocation, so each expected diagnostic annotation was matched.

The implementation and checklist update were committed as
`338d12baf78b` (`reflection: add first M5 diagnostic batch`).

## Skipped rows

None within this batch. No P1306R5/P3096R12 rows were selected because the
checklist's prescribed ordering starts with the four earlier P2996R13 rows.

## Checklist counts

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility |
|---:|---:|---:|
| 14 | 77 | 18 |

