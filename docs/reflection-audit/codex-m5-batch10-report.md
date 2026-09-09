# M5 diagnostic batch 10 report

Date: 2026-09-10

## P2996R13 — Reflection for C++26

Added [`m5-p2996-batch10.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch10.verify.cpp).

- 2996-33/-34/-35: `extract<T>` rejects invalid reflected type, value, and pointer conversions.
- 2996-36: `can_substitute` rejects a non-template reflection.
- 2996-37: `substitute` rejects a non-template reflection.

Confirmed run, using the libc++ wrapper with Python's worker start method forced to `fork` due to
the known sandbox `forkserver` restriction:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch10.verify.cpp
Total Discovered Tests: 1
  Passed: 1 (100.00%)
```

No new implementation gap was found.

## Checklist totals

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 64 | 26 | 18 | 1 |
