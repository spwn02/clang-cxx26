Audit complete. No compiler bug reproduced.

- Nested direct, multi-level, ordinary-caller, and `std::meta::exception` wrapper cases pass.
- Root cause of prior failure: stale staged libc++ headers.
- Updated tracker and report: [codex-nested-throw-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-nested-throw-report.md)
- Commit: `558a2230d34a` (`reflection: audit nested consteval throws`)
- No compiler change or full gate required.