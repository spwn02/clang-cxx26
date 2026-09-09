Pilot stopped before implementation due two design blockers:

- Pending exception handling assumes `ThrowExpr` is always `CXXThrowExpr`.
- `ThrowFn` lacks an evaluator API to construct `meta::exception` from `StringRef`/`APValue`.

Documented required redesign in [pilot report](</home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-strategy2-pilot-report.md>) and updated [REFLECTION_GAPS.md](</home/spawn/dev/toolchains/clang-p2996/docs/REFLECTION_GAPS.md>).

Committed as `0c3b32515230`. No compiler changes or tests were run; push was not attempted because pilot did not succeed.