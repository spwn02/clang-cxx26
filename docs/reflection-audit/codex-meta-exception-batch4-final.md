Implemented and pushed batch 4.

- Added wrappers: `offset_of`, `operator_of`, `subobjects_of`.
- Total strategy-(1) wrappers: 13.
- Focused `libcxx-lit` test passed 1/1 after each change.
- Updated [REFLECTION_GAPS.md](/home/spawn/dev/toolchains/clang-p2996/docs/REFLECTION_GAPS.md).
- Added [batch-4 report](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-meta-exception-batch4-report.md).
- Strategy (1) surface is exhausted; strategy (2) compiler-side `DiagFn` rewiring is next. This matches [P3560R2](https://isocpp.org/files/papers/P3560R2.html) and the adopted [P2996R13](https://isocpp.org/files/papers/P2996R13.html) constraints.

Commits pushed to `origin/cxx26`:

`28c22ede23c3`, `83327c39ce9a`, `4a67ac99161f`

Pre-existing untracked file remains untouched: `docs/reflection-audit/codex-m4-hard-final.md`.