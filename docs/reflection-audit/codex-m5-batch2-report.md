# M5 diagnostic batch 2 report

Date: 2026-09-10

Batch selection followed the checklist execution plan: the next four ordered
`Needs-New-Test` rows, P2996R13 2996-05 through 2996-08.

## Covered conditions

All four conditions are covered by
[`m5-p2996-batch2.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p2996-batch2.verify.cpp):

- 2996-05: an automatic local object cannot be represented by `reflect_object`.
- 2996-06: a non-function argument is rejected by `reflect_function`.
- 2996-07: a function obtained through a non-constant function-pointer object is not a
  suitable constant template argument for `reflect_function`.
- 2996-08: `identifier_of` and `u8identifier_of` reject a reflected type without an identifier.

The test was actually run and passed with:

```text
libcxx/utils/libcxx-lit build-libcxx -sv -j1 libcxx/test/std/experimental/reflection/m5-p2996-batch2.verify.cpp
```

Result: `Total Discovered Tests: 1; Passed: 1 (100.00%)`. Lit invoked Clang's
`-verify` mode and matched all expected errors and notes. The first in-sandbox
attempt was blocked before compilation by lit's multiprocessing fork-server
(`PermissionError: Operation not permitted`); the identical wrapper command was
then rerun with sandbox restrictions lifted and passed.

## Checklist counts

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility |
|---:|---:|---:|
| 18 | 73 | 18 |

## Commits

- `cb8f63f38f26` (`reflection: add M5 diagnostic batch 2`) — test and checklist update.
- `bd66d762689f` (`docs: report reflection M5 batch 2`) — this report.
