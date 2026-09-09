# M5 diagnostic batch 8 report

Date: 2026-09-10

## P2996R13 — Reflection for C++26

Added [`m5-p2996-batch8.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch8.verify.cpp).

- 2996-21: `members_of` rejects a non-class/non-namespace reflection.
- 2996-22: `bases_of` rejects a non-class reflection.
- 2996-23: `static_data_members_of` rejects a non-class reflection.
- 2996-24: `nonstatic_data_members_of` rejects a non-class reflection.
- 2996-25: `enumerators_of` rejects a non-enumeration reflection.

Confirmed run, using the libc++ wrapper with Python's worker start method forced to `fork` due to
the known sandbox `forkserver` restriction:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch8.verify.cpp
Total Discovered Tests: 1
  Passed: 1 (100.00%)
```

No new implementation gap was found.

## Checklist totals

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 55 | 35 | 18 | 1 |

