# M5 diagnostic batch 4 report

Date: 2026-09-10

## P2996R13 resolution

The adopted wording at [meta.reflection.names](https://wg21.link/P2996R13) says that
`display_string_of(r)` returns an unspecified **non-empty** `string_view` for any reflection.
Therefore the fork's empty null-reflection fallback is a genuine implementation conformance gap,
recorded as NEW-1 in `docs/REFLECTION_GAPS.md`. It is not a required Mandates/Constant-When
diagnostic, so no fabricated `-verify` test was added.

The same wording gives `source_location_of(r)` a result for all `info` values: `source_location{}`
for listed cases and an implementation-defined value otherwise. A null reflection is not required
to diagnose, so row 2996-10 is now Not-Applicable.

## P1306R5 coverage

[`m5-p1306-batch4.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p1306-batch4.verify.cpp)
covers all five P1306 rows:

- 1306-01: non-constant iterating range initializer.
- 1306-02: invalid dereference in the generated range iteration.
- 1306-03: forbidden `static` for-range declaration specifier.
- 1306-04: identifier label enclosed by an expansion statement.
- 1306-05: invalid destructuring expansion initializer.

The test was actually run with:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p1306-batch4.verify.cpp
```

Result: `Total Discovered Tests: 1; Passed: 1 (100.00%)`; all expected diagnostics matched.

## Checklist counts

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 28 | 62 | 18 | 1 |

The checklist has 108 applicable conditions; the original 109-row inventory now excludes the one
Not-Applicable source-location row.
