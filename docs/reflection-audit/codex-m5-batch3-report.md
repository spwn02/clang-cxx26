# M5 diagnostic batch 3 report

Date: 2026-09-10

Batch selection followed the checklist execution plan: the next ordered P2996R13 rows after
batch 2, 2996-09 through 2996-15.

## Covered conditions

Rows 2996-11 through 2996-15 are covered by
[`m5-p2996-batch3.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp):

- 2996-11: `type_of` rejects a null reflection.
- 2996-12: `parent_of` rejects a null reflection.
- 2996-13: `object_of` rejects a null reflection.
- 2996-14: `constant_of` rejects a null reflection.
- 2996-15: `template_of` rejects a reflection without template arguments.

The test was actually run and passed with:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp
```

Result: `Total Discovered Tests: 1; Passed: 1 (100.00%)`. Lit invoked Clang's `-verify` mode and
matched all expected errors and notes. The first in-sandbox attempt was blocked before compilation
by lit's multiprocessing fork-server (`PermissionError: Operation not permitted`); the identical
wrapper command was rerun with sandbox restrictions lifted and passed.

## Still open

Rows 2996-09 (`display_string_of`) and 2996-10 (`source_location_of`) remain `Needs-New-Test`.
Probing null reflections showed the current implementation intentionally supplies fallback/empty
results rather than emitting a diagnostic, so no negative `-verify` test was added or claimed.

## Checklist counts

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility |
|---:|---:|---:|
| 23 | 68 | 18 |

## Commit

The test and checklist update are committed as `reflection: add M5 diagnostic batch 3`.
