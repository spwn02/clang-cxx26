Investigated all 8 issues.

Fixed:
- #169: prevented templated-lambda capture-analysis crash.
- #346: removed false reflected-reference warning.

Deferred as genuinely open: #180, #181, #188, #220, #237.

Not a reflection bug:
- #221: libc++ `expected` constraint-recursion failure.

Final M4 Confirmed-Open backlog: 6 issues — #150, #180, #181, #188, #220, #237.

Report: [codex-m4-batch6-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m4-batch6-report.md)

Commit/push and full check-clang gate were blocked: `.git` is read-only, and lit’s Python `forkserver` is forbidden. Changes remain uncommitted in the worktree.