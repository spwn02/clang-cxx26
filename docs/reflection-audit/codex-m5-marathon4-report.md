# M5 reflection diagnostic marathon 4

Date: 2026-09-10

## Batch 17

Added [`m5-p3560-batch17.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p3560-batch17.verify.cpp).

The batch covers five of the six corrected P3560R2 Needs-New-Test rows:

- 3560-14: `has_inaccessible_nonstatic_data_members(^^int, ctx)`.
- 3560-15: `has_inaccessible_bases(^^int, ctx)`.
- 3560-16: invalid `size_of(^^void)`, `bit_size_of(^^void)`, and `alignment_of(^^void)`.
- 3560-17: `template_of(^^int)`.
- 3560-19: `operator_of(^^int)`.

Each test uses a consteval `try`/`catch (const std::meta::exception&)` and checks
`exception::from()` against the wrapper that threw. The direct built-clang command with
`-Xclang -verify` passed with `expected-no-diagnostics`.

The required negative control was also performed: changing the first caught `from()` check to
`return false` caused its `static_assert` to fail. This confirms the test is exercising the
wrapper throw rather than passing trivially.

The preferred libc++ lit invocation was attempted with one worker. Dependency/header setup
completed, but lit could not start its Python 3.14 forkserver because the sandbox denied socket
creation (`PermissionError: [Errno 1] Operation not permitted`).

## 3560-18 investigation

`access_context::via(^^int)` was tested with the same catch-and-inspect shape. The wrapper reaches
its `meta::exception` throw, but constant evaluation rejects the call while destroying the
throwing `access_context` return object, reporting `lifetime has already ended`. Binding the
result to a local and using `access_context::unprivileged()` did not change this result. Row
3560-18 remains Needs-New-Test; it was not marked Covered and no new tracker gap was claimed.

## Tracker state

The checklist was updated to 81 Covered, 8 Needs-New-Test, 19 Blocked-On-Unimplemented-Facility,
and one Not-Applicable. The remaining Needs-New-Test rows are 2996-09, 2996-19, 2996-48,
2996-49, 3096-06, 3394-03, 3394-05, and 3560-18.

## Repository blocker

The working tree's `.git` subvolume became read-only. `git add` failed while creating
`.git/index.lock` with `Read-only file system`, so this batch could not be committed or pushed.
No further batch work was started. The test, checklist, tracker, and this report remain as
working-tree changes for the next session to stage, commit, push, and verify with:

```text
git log --oneline origin/cxx26..HEAD
```
