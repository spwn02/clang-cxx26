# M5 reflection diagnostic marathon 3

Date: 2026-09-10

## Batch 16

Added [`m5-p3491-p3560-p3795-batch16.verify.cpp`](../../libcxx/test/std/experimental/reflection/m5-p3491-p3560-p3795-batch16.verify.cpp).

- P3491R3 row 3491-04 is covered by a non-constant array-element probe for
  `define_static_array`.
- P3795R2 row 3795-04 is covered by a generated data-member annotation using a non-structural
  value; the expected `reflect_constant` and consteval-block diagnostics match.
- P3560R2 rows 3560-14 through 3560-19 were investigated with direct consteval
  `try`/`catch` probes. The library wrappers reach their `throw meta::exception(...)`, but the
  exception escapes the caller's `try`/`catch` across the consteval library call. These are not
  passing P3560 catch tests and remain open as NEW-9; no false coverage claim was made.

Validation:

```text
build-nyx/bin/clang++ -std=c++26 -fsyntax-only -freflection-latest \
  -nostdinc++ -Ibuild-libcxx/include/c++/v1 -Ilibcxx/test/support \
  libcxx/test/std/experimental/reflection/m5-p3491-p3560-p3795-batch16.verify.cpp \
  -Xclang -verify -Xclang -verify-ignore-unexpected=note
```

Passed. The preferred libc++ wrapper was also attempted with `-j1`; dependency installation
completed, but lit could not create its Python 3.14 forkserver socket in this sandbox
(`PermissionError: [Errno 1] Operation not permitted`).

## M5 status

Checklist totals are now 76 Covered, 13 Needs-New-Test, 19 Blocked-On-Unimplemented-Facility, and
1 Not-Applicable. The Needs-New-Test category is **not empty**. Remaining rows are the previously
documented NEW-1 through NEW-8 cases (including rows 2996-09, 2996-19, 2996-48, 2996-49, 3096-06,
3394-03, and 3394-05) plus P3560 rows 3560-14 through 3560-19 under NEW-9. NEW-1 through NEW-8
were not re-investigated.

M5's test-writing phase is therefore not complete. The remaining P3560 work needs exception
propagation/evaluator implementation; the other remaining rows need their already-recorded
implementation gaps fixed before passing conformance tests can be added.

## Repository delivery blocker

The working tree's `.git` subvolume is mounted read-only. `git add`/`git commit` failed with
`Unable to create .../.git/index.lock: Read-only file system`, so this batch could not be committed
or pushed, and `git log --oneline origin/cxx26..HEAD` could not verify a new commit. Source and
tracker changes remain in the working tree for the next session to stage, commit, push, and verify.
