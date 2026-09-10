Batch 17 completed:

- Added catch-and-inspect coverage for P3560 rows 3560-14–17 and 3560-19.
- Direct `clang++ -verify` passed.
- Deliberate negative control failed as expected.
- Lit was blocked by sandbox forkserver socket permissions.
- 3560-18 remains open due `access_context` return-object lifetime failure.
- Checklist updated: 81 covered, 8 remaining, 19 blocked.
- Added [codex-m5-marathon4-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m5-marathon4-report.md).

Commit/push blocked: `.git` is read-only and cannot create `.git/index.lock`.