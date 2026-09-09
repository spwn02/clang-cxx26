# M5 diagnostic batch 11 report

Date: 2026-09-10

## P2996R13 — Reflection for C++26

Added [`m5-p2996-batch11.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch11.verify.cpp).

- 2996-38: `data_member_spec` rejects a non-type member specification.
- 2996-39: invalid member names are rejected.
- 2996-40: invalid width/alignment and bit-field option combinations are rejected.

Confirmed run, using the libc++ wrapper with Python's worker start method forced to `fork` due to
the known sandbox `forkserver` restriction:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch11.verify.cpp
Total Discovered Tests: 1
  Passed: 1 (100.00%)
```

The separate direct probe `data_member_spec(^^void)` was accepted, although P2996R13 requires an
object or reference member type. This is recorded as NEW-5; the passing test uses a reflected value
to cover the invalid-type diagnostic without masking the `void` gap.

## Checklist totals

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 67 | 23 | 18 | 1 |
