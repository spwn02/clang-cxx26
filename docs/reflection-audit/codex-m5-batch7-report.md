# M5 diagnostic batch 7 report

Date: 2026-09-10

## P2996R13 — Reflection for C++26

Added [`m5-p2996-batch7.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch7.verify.cpp).

- 2996-16: `template_arguments_of` rejects a reflection without template arguments.
- 2996-17: `is_accessible` rejects an access context formed with a non-class `via` reflection.
- 2996-18: `has_inaccessible_nonstatic_data_members` rejects a non-class reflection.
- 2996-20: `has_inaccessible_bases` rejects a non-class reflection.

Confirmed run, using the libc++ wrapper with Python's worker start method forced to `fork` due to
the known sandbox `forkserver` restriction:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch7.verify.cpp
Total Discovered Tests: 1
  Passed: 1 (100.00%)
```

## New gap

The direct closure-type probe for 2996-19 was accepted instead of producing the required
diagnostic. It is recorded as NEW-4 in `docs/REFLECTION_GAPS.md`; no false-positive verify
coverage was added.

## Checklist totals

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 50 | 40 | 18 | 1 |

