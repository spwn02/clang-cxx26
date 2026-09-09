Pre-existing baseline bug confirmed; not caused by today’s reflection commits.

- Failure occurs in `ASTUnit::readASTFileControlBlock`, before `ASTReader::ReadAST`.
- Underlying error is discarded; ASTUnit emits only generic `unable to load precompiled file`.
- Fresh consumer-tool rebuild and `LIBCLANG_DISABLE_PCH_VALIDATION=1` do not change result.
- Approximately 18 failures are one repeated ASTUnit/libclang PCH-consumer issue, not staleness.

Documented and committed:

[Audit report]( /home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-pch-bug-report.md )

[Tracker update]( /home/spawn/dev/toolchains/clang-p2996/docs/REFLECTION_GAPS.md )

Commit: `b4b3da303400`