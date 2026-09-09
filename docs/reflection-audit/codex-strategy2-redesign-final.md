Blocked safely and reverted risky code.

- Root cause: synthesized `std::meta::exception` construction aborts via recursive inherited-constructor evaluation at `extractSubobject`.
- No implementation or test changes remain.
- Documented evidence in [codex-strategy2-redesign-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-strategy2-redesign-report.md).
- Updated tracker/design status.
- AST/Sema compile and `clang` link passed; focused test exposed the crash.
- Commit blocked because `.git` is read-only.