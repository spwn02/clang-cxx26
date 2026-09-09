Completed and pushed M5 batch 3.

- Covered P2996R13 rows 2996-11 through 2996-15.
- Verified lit pass: 1/1, all diagnostics matched.
- Rows 2996-09/10 remain open; implementation returns fallback/empty results.
- Counts: 23 covered, 68 needs-test, 18 blocked.
- Commit: `ca622745eb12`
- Push verified: `git log --oneline origin/cxx26..HEAD`

Files: [batch 3 test](/home/spawn/dev/toolchains/clang-p2996/libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp), [report](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m5-batch3-report.md).