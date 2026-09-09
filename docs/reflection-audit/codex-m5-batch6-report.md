# M5 diagnostic batch 6 report

Date: 2026-09-10

## P3617R0 — `reflect_constant_{array,string}`

Added [`m5-p3617-p3687-batch6.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p3617-p3687-batch6.verify.cpp).

- 3617-02: string-literal input is checked to omit the source null character and receive one
  new terminator.
- 3617-03: a structural, copy-constructible array element type is lifted from a constant range.
- 3617-04: reflected string/array types have the required element extent and can be extracted.

Confirmed run:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p3617-p3687-batch6.verify.cpp
Total Discovered Tests: 1
  Passed: 1 (100.00%)
```

The wrapper's normal Python 3.14 worker setup hit the known sandbox `forkserver` permission
failure; the same libc++ lit invocation was rerun with the worker start method forced to `fork`.
The final run is the confirmed result above.

## P3687R1 — Final adjustments

The same verify test covers:

- 3687-01: removed unparenthesized splice template arguments are rejected.
- 3687-02: reflection of a declaration replacing a using-declarator is rejected.
- 3687-03: lookup through two base paths produces the required ambiguous-member diagnostic.

All expected diagnostics matched in the confirmed 1/1 libc++ lit run above.

## P3491R3 status check

No test was fabricated for the remaining P3491 rows. `define_static_object` and all
`is_string_literal` overloads are still absent. `reflect_constant_string` remains limited to
`char` and `char8_t`, so the five-character-type requirement is still blocked. The existing
array implementation is exercised by the P3617 test, but the checklist retains P3491 rows
3491-03, 3491-04, and 3491-08 as pending diagnostic/conformance coverage rather than claiming
that shared positive coverage closes their complete adopted-wording obligations.

## New gaps

None found. Existing NEW-1, NEW-2, and NEW-3 were not re-investigated.

## Checklist and commits

Checklist totals after this batch:

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 46 | 44 | 18 | 1 |

Commit `b207887cc90f` (`reflection: cover P3617 and P3687 M5 conditions`) contains the test,
checklist updates, and the tracker session-log entry. It was pushed to `origin/cxx26`; after the
push, `git log --oneline origin/cxx26..HEAD` was empty.
