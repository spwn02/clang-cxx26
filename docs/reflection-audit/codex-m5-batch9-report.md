# M5 diagnostic batch 9 report

Date: 2026-09-10

## P2996R13 — Reflection for C++26

Added [`m5-p2996-batch9.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch9.verify.cpp).

- 2996-26: `offset_of` rejects a non-member/non-base reflection.
- 2996-27/-28: `size_of` rejects an invalid reflection and an incomplete type.
- 2996-29/-30: `alignment_of` rejects an invalid reflection and an incomplete type.
- 2996-31/-32: `bit_size_of` rejects an invalid reflection and an incomplete type.

Confirmed run, using the libc++ wrapper with Python's worker start method forced to `fork` due to
the known sandbox `forkserver` restriction:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch9.verify.cpp
Total Discovered Tests: 1
  Passed: 1 (100.00%)
```

No new implementation gap was found.

## Checklist totals

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 62 | 28 | 18 | 1 |

