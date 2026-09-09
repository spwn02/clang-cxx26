Implemented and pushed M5 batch 1.

- Covered P2996R13 rows 2996-01 through 2996-04.
- Added [m5-p2996-batch1.verify.cpp](/home/spawn/dev/toolchains/clang-p2996/libcxx/test/std/experimental/reflection/m5-p2996-batch1.verify.cpp).
- Verified with libc++ lit: 1/1 passed, including all `-verify` diagnostics.
- Updated checklist counts: 14 covered, 77 needs-test, 18 blocked.
- Added [batch report](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m5-batch1-report.md).
- Commits:
  - `338d12baf78b` reflection test/checklist
  - `e120c48af042` report
- Pushed to `origin/cxx26`; `git log origin/cxx26..HEAD` is empty.