Implemented and committed:

- #334 fixed: `MarkMemberReferenced` no longer propagates consteval-only restriction from discarded static-call objects.
- #146 fixed: parser rejects `case`/`default` labels in expansion statements.
- #200 verified already fixed.
- #150 remains deferred with precise parser/Sema/AST rationale.

Commits:

- `706251663c1e` — reflection fixes
- `b08c22c10972` — tracker and audit report

Report: [codex-m4-hard-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m4-hard-report.md)

Focused tests: 4/4 passed. Full suite was attempted but stopped due known baseline failures and nested-lit sandbox forkserver errors.

Push was blocked pending confirmation of destination: `origin` is `https://github.com/spwn02/clang-cxx26.git`.